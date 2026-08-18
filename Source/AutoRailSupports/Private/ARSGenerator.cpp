#include "ARSGenerator.h"

#include "ARSCurve.h"
#include "ARSDebug.h"
#include "Components/SplineComponent.h"
#include "FGBlueprintProxy.h"
#include "FGBlueprintSubsystem.h"
#include "FGCentralStorageSubsystem.h"
#include "FGCharacterPlayer.h"
#include "FGFactoryBlueprintTypes.h"
#include "FGGameState.h"
#include "FGInventoryComponent.h"
#include "FGInventoryLibrary.h"
#include "FGPlayerState.h"
#include "FGRailroadTrackConnectionComponent.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "AbstractInstanceInterface.h"
#include "AbstractInstanceManager.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FGDynamicStruct.h"
#include "FGLightweightBuildableSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"


namespace
{
	// Hard caps
	constexpr int32 MaxSegmentsPerChain = 512;
	constexpr int32 MaxStampsPerChain = 5000;

	bool FindBatchNeighbor(AFGBuildableRailroadTrack* Track, int32 EndIndex,
	                       const TSet<AFGBuildableRailroadTrack*>& Batch,
	                       const TSet<AFGBuildableRailroadTrack*>& Excluded,
	                       AFGBuildableRailroadTrack*& OutNeighbor, int32& OutEnteredEnd)
	{
		UFGRailroadTrackConnectionComponent* Connection = Track->GetConnection(EndIndex);
		if (!Connection)
		{
			return false;
		}
		for (UFGRailroadTrackConnectionComponent* Other : Connection->GetConnections())
		{
			if (!Other)
			{
				continue;
			}
			AFGBuildableRailroadTrack* OtherTrack = Other->GetTrack();
			if (!OtherTrack || OtherTrack == Track || !Batch.Contains(OtherTrack) || Excluded.Contains(OtherTrack))
			{
				continue;
			}
			OutNeighbor = OtherTrack;
			OutEnteredEnd = (Other == OtherTrack->GetConnection(0)) ? 0 : 1;
			return true;
		}
		return false;
	}

	//STATION GEOMETRY
	// shared by the single and double rail paths
	struct FARSStationBasis
	{
		FVector Location = FVector::ZeroVector;
		FVector Direction = FVector::ForwardVector;
		FVector FlatDirection = FVector::ForwardVector;
		FVector RightDirection = FVector::RightVector;
		FQuat SplineRotation = FQuat::Identity;
	};

	FARSStationBasis MakeBasis(const FVector& Location, const FVector& RawDirection)
	{
		FARSStationBasis Basis;
		Basis.Location = Location;
		const FVector Direction = RawDirection.GetSafeNormal();
		Basis.Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction;
		Basis.FlatDirection = FVector(Basis.Direction.X, Basis.Direction.Y, 0.0f).GetSafeNormal();
		if (Basis.FlatDirection.IsNearlyZero())
		{
			Basis.FlatDirection = FVector::ForwardVector;
		}
		Basis.RightDirection = FVector::CrossProduct(FVector::UpVector, Basis.FlatDirection);
		Basis.SplineRotation = FRotationMatrix::MakeFromXZ(Basis.Direction, FVector::UpVector).ToQuat(); // X along track, Z near world-up
		return Basis;
	}

	FARSStationBasis SampleBasis(const FARSPathSampler& Sampler, float Distance)
	{
		FVector Location = FVector::ZeroVector;
		FVector Direction = FVector::ForwardVector;
		Sampler(Distance, Location, Direction);
		return MakeBasis(Location, Direction);
	}

	// shift up by the content's top height so the top face lands below the rail
	FVector PivotedLocation(const FVector& InLocation, const FQuat& Heading, const FQuat& Rotation, const FBox& LocalBounds)
	{
		if (!LocalBounds.IsValid)
		{
			return InLocation;
		}
		const FVector Center = LocalBounds.GetCenter();
		const FVector Drop(0.0f, 0.0f, Center.Z - LocalBounds.Max.Z);
		return InLocation + Heading.RotateVector(Drop) - Rotation.RotateVector(Center);
	}

	bool SlotStampsAt(const FARSPillarSlot& Slot, int32 Index)
	{
		return Index >= Slot.SpacingOffset && ((Index - Slot.SpacingOffset) % Slot.EveryN) == 0;
	}

	// stations sit at tile centers
	struct FARSStationPlan
	{
		int32 Count = 0;
		float Step = 0.0f;
		float Remainder = 0.0f;
		float TotalLength = 0.0f;
		bool bTrailingTileOverhangs = false;

		float DistanceAt(int32 Index) const { return (Index + 0.5f) * Step; }
	};

	FARSStationPlan PlanStations(float TotalLength, float Step)
	{
		FARSStationPlan Plan;
		Plan.TotalLength = TotalLength;
		Plan.Step = Step;
		const int32 FullTileCount = FMath::FloorToInt(TotalLength / Step);
		Plan.Remainder = TotalLength - FullTileCount * Step;
		Plan.bTrailingTileOverhangs = Plan.Remainder > 0.5f * Step;
		Plan.Count = FMath::Min(Plan.bTrailingTileOverhangs ? FullTileCount + 1 : FullTileCount, MaxStampsPerChain);
		return Plan;
	}

	bool WantsEndGapStamp(const FARSStationPlan& Plan, const FARSSettings& Settings, bool bCoverEndGap)
	{
		return bCoverEndGap && Settings.UnderRailDescriptor
			&& !Plan.bTrailingTileOverhangs && Plan.Remainder > 1.0f && Plan.Count < MaxStampsPerChain;
	}

	struct FARSStampParams
	{
		UFGBlueprintDescriptor* Descriptor = nullptr;
		FBox LocalBounds = FBox(ForceInit);
		int32 PartIndex = 0;
		FRotator RotationOffset = FRotator::ZeroRotator;
		float ZOffset = 0.0f;
		float SideOffset = 0.0f;
		float ForwardOffset = 0.0f;
		bool bAlignToRail = true;
		bool bMirrored = false;
		float OutwardSign = 1.0f;
		float RollDegrees = 0.0f;
		bool bPartnerSide = false;
	};

	void EmitStamp(const FARSStationBasis& Basis, const FARSStampParams& Params, const FARSSettings& Settings,
	               AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs)
	{
		const FQuat LevelHeading = Params.bAlignToRail
			? Basis.SplineRotation
			: FRotator(0.0f, Basis.FlatDirection.Rotation().Yaw, 0.0f).Quaternion();
		const FQuat Heading = FMath::IsNearlyZero(Params.RollDegrees)
			? LevelHeading
			: LevelHeading * FRotator(0.0f, 0.0f, Params.RollDegrees).Quaternion();
		const FQuat Mirror = Params.bMirrored ? FRotator(0.0f, 180.0f, 0.0f).Quaternion() : FQuat::Identity;
		const FQuat Rotation = Heading * Params.RotationOffset.Quaternion() * Mirror;
		const FVector StampLocation = Basis.Location
			+ FVector(0.0f, 0.0f, Params.ZOffset)
			+ Basis.RightDirection * Params.SideOffset * Params.OutwardSign
			+ Basis.Direction * Params.ForwardOffset;

		FARSStampJob& Job = OutJobs.AddDefaulted_GetRef();
		Job.Descriptor = Params.Descriptor;
		Job.Transform = FTransform(Rotation, PivotedLocation(StampLocation, Heading, Rotation, Params.LocalBounds));
		Job.PartIndex = Params.PartIndex;
		Job.bPartnerSide = Params.bPartnerSide;
		Job.LocalBounds = Params.LocalBounds;
		Job.Payer = Payer;
		Job.BatchId = BatchId;
		Job.bCombineIntoOneBlueprint = Settings.bCombineIntoOneBlueprint;
	}

	struct FARSUnderRailValues
	{
		FARSStampParams Params;
		float Step = 800.0f;
		bool bCoverEndGap = false;
	};

	FARSUnderRailValues UnderRailValuesFor(const FARSSettings& Settings, bool bPartner)
	{
		FARSUnderRailValues Values;
		Values.Params.Descriptor = Settings.UnderRailDescriptor;
		Values.Params.LocalBounds = Settings.UnderRailLocalBounds;
		Values.Params.bPartnerSide = bPartner;

		if (bPartner && Settings.bSplitUnderRailSides)
		{
			const FARSUnderRailSide& Side = Settings.PartnerUnderRail;
			Values.Params.RotationOffset = Side.RotationOffset;
			Values.Params.ZOffset = Side.ZOffset;
			Values.Params.SideOffset = Side.SideOffset;
			Values.Params.ForwardOffset = Side.ForwardOffset;
			Values.Step = FMath::Max(Side.Length, 50.0f);
			Values.bCoverEndGap = Side.bCoverEndGap;
			return Values;
		}
		Values.Params.RotationOffset = Settings.UnderRailRotationOffset;
		Values.Params.ZOffset = Settings.UnderRailZOffset;
		Values.Params.SideOffset = Settings.UnderRailSideOffset;
		Values.Params.ForwardOffset = Settings.UnderRailForwardOffset;
		Values.Step = FMath::Max(Settings.UnderRailLength, 50.0f);
		Values.bCoverEndGap = Settings.bCoverEndGap;
		return Values;
	}

	FARSStampParams PillarParams(const FARSPillarSlot& Slot)
	{
		FARSStampParams Params;
		Params.Descriptor = Slot.Descriptor;
		Params.LocalBounds = Slot.LocalBounds;
		Params.PartIndex = Slot.Priority + 1;
		Params.RotationOffset = Slot.RotationOffset;
		Params.ZOffset = Slot.ZOffset;
		Params.SideOffset = Slot.SideOffset;
		Params.ForwardOffset = Slot.ForwardOffset;
		Params.bAlignToRail = Slot.bAlignToRail;
		return Params;
	}

	// the highest-priority slot always stamps, lower ones only with prevention off
	int32 TopFiringPriority(const FARSSettings& Settings, int32 Index)
	{
		int32 TopPriority = -1;
		for (const FARSPillarSlot& PillarSlot : Settings.PillarSlots)
		{
			if (SlotStampsAt(PillarSlot, Index))
			{
				TopPriority = FMath::Max(TopPriority, PillarSlot.Priority);
			}
		}
		return TopPriority;
	}

	bool SlotIsSuppressed(const FARSPillarSlot& Slot, int32 TopPriority)
	{
		return Slot.bPreventOverlap && Slot.Priority < TopPriority;
	}
}

TArray<TArray<FARSChainSegment>> FARSGenerator::BuildChains(const TArray<AFGBuildableRailroadTrack*>& Tracks)
{
	TArray<TArray<FARSChainSegment>> Chains;

	TSet<AFGBuildableRailroadTrack*> Batch;
	for (AFGBuildableRailroadTrack* Track : Tracks)
	{
		if (Track)
		{
			Batch.Add(Track);
		}
	}

	TSet<AFGBuildableRailroadTrack*> Visited;
	for (AFGBuildableRailroadTrack* Seed : Tracks)
	{
		if (!Seed || Visited.Contains(Seed))
		{
			continue;
		}

		// walk back from the seed to find where the chain starts
		AFGBuildableRailroadTrack* Start = Seed;
		bool bStartReversed = false;
		{
			TSet<AFGBuildableRailroadTrack*> BackVisited;
			BackVisited.Add(Seed);
			AFGBuildableRailroadTrack* Current = Seed;
			bool bCurrentReversed = false;
			for (int32 Guard = 0; Guard < MaxSegmentsPerChain; Guard++)
			{
				const int32 NearEnd = bCurrentReversed ? 1 : 0;
				AFGBuildableRailroadTrack* Previous = nullptr;
				int32 EnteredEnd = INDEX_NONE;
				if (!FindBatchNeighbor(Current, NearEnd, Batch, BackVisited, Previous, EnteredEnd))
				{
					break;
				}
				bCurrentReversed = (EnteredEnd == 0);
				Current = Previous;
				BackVisited.Add(Current);
			}
			Start = Current;
			bStartReversed = bCurrentReversed;
		}

		// walk forwards from the start
		TArray<FARSChainSegment> Chain;
		AFGBuildableRailroadTrack* Current = Start;
		bool bCurrentReversed = bStartReversed;
		float AccumulatedDistance = 0.0f;
		while (Current && Chain.Num() < MaxSegmentsPerChain)
		{
			Visited.Add(Current);

			FARSChainSegment Segment;
			Segment.Track = Current;
			Segment.bReversed = bCurrentReversed;
			Segment.StartDistance = AccumulatedDistance;
			Segment.Length = Current->GetLength();
			AccumulatedDistance += Segment.Length;
			Chain.Add(Segment);

			const int32 FarEnd = bCurrentReversed ? 0 : 1;
			AFGBuildableRailroadTrack* Next = nullptr;
			int32 EnteredEnd = INDEX_NONE;
			if (!FindBatchNeighbor(Current, FarEnd, Batch, Visited, Next, EnteredEnd))
			{
				break;
			}
			bCurrentReversed = (EnteredEnd == 1);
			Current = Next;
		}

		if (Chain.Num() > 0)
		{
			Chains.Add(MoveTemp(Chain));
		}
	}
	return Chains;
}

void FARSGenerator::SampleChain(const TArray<FARSChainSegment>& Chain, float Distance, FVector& OutLocation, FVector& OutDirection)
{
	const FARSChainSegment* Segment = &Chain.Last();
	for (const FARSChainSegment& Candidate : Chain)
	{
		if (Distance < Candidate.StartDistance + Candidate.Length)
		{
			Segment = &Candidate;
			break;
		}
	}

	float LocalDistance = FMath::Clamp(Distance - Segment->StartDistance, 0.0f, Segment->Length);
	if (Segment->bReversed)
	{
		LocalDistance = Segment->Length - LocalDistance;
	}
	Segment->Track->GetWorldLocationAndDirectionAtDistance(LocalDistance, OutLocation, OutDirection);
	if (Segment->bReversed)
	{
		OutDirection = -OutDirection;
	}
}

FARSPathCurve FARSGenerator::MakeSplineCurve(const USplineComponent* Spline)
{
	FARSPathCurve Curve;
	if (!Spline)
	{
		return Curve;
	}
	Curve.Length = Spline->GetSplineLength();
	Curve.Sampler = [Spline](float Distance, FVector& OutLocation, FVector& OutDirection)
	{
		OutLocation = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		OutDirection = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	};
	return Curve;
}

void FARSGenerator::PlanChain(const TArray<FARSChainSegment>& Chain, const FARSSettings& Settings,
                              AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs)
{
	if (Chain.Num() == 0)
	{
		return;
	}
	const float TotalLength = Chain.Last().StartDistance + Chain.Last().Length;
	FARSPathSampler SourceSampler = [&Chain](float Distance, FVector& OutLocation, FVector& OutDirection)
	{
		SampleChain(Chain, Distance, OutLocation, OutDirection);
	};

	if (Settings.bDoubleRails)
	{
		TArray<FARSCurvePoint> Points;
		TArray<FARSCurvePoint> Offset;
		FARSCurve::ReadChainPoints(Chain, Points);
		FARSCurve::OffsetPoints(Points, Settings.TrackSeparation, Offset);
		if (USplineComponent* MirrorSpline = FARSCurve::MakeScratchSpline(Offset))
		{
			PlanDoublePath({ SourceSampler, TotalLength }, MakeSplineCurve(MirrorSpline),
				Settings, Payer, BatchId, OutJobs);
			return;
		}
	}
	PlanPath(SourceSampler, TotalLength, Settings, Payer, BatchId, OutJobs);
}

void FARSGenerator::PlanPath(const FARSPathSampler& Sampler, float TotalLength, const FARSSettings& Settings,
                             AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs)
{
	ARS_SCOPE_PERF(PlanPath);
	if (TotalLength <= 0.0f || (!Settings.UnderRailDescriptor && Settings.PillarSlots.Num() == 0))
	{
		return;
	}

	const FARSUnderRailValues UnderRail = UnderRailValuesFor(Settings, /*bPartner*/ false);
	const FARSStationPlan Plan = PlanStations(TotalLength, UnderRail.Step);

	for (int32 Index = 0; Index < Plan.Count; Index++)
	{
		const int32 TopPriority = TopFiringPriority(Settings, Index);
		if (!Settings.UnderRailDescriptor && TopPriority < 0)
		{
			continue; // Nothing to place here, so don't pay for the sample
		}

		const FARSStationBasis Basis = SampleBasis(Sampler, Plan.DistanceAt(Index));
		if (Settings.UnderRailDescriptor)
		{
			EmitStamp(Basis, UnderRail.Params, Settings, Payer, BatchId, OutJobs);
		}
		for (const FARSPillarSlot& PillarSlot : Settings.PillarSlots)
		{
			if (SlotStampsAt(PillarSlot, Index) && !SlotIsSuppressed(PillarSlot, TopPriority))
			{
				EmitStamp(Basis, PillarParams(PillarSlot), Settings, Payer, BatchId, OutJobs);
			}
		}
	}

	if (WantsEndGapStamp(Plan, Settings, UnderRail.bCoverEndGap))
	{
		EmitStamp(SampleBasis(Sampler, TotalLength), UnderRail.Params, Settings, Payer, BatchId, OutJobs);
	}
}

void FARSGenerator::PlanDoublePath(const FARSPathCurve& Source, const FARSPathCurve& Mirror, const FARSSettings& Settings,
                                   AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs)
{
	ARS_SCOPE_PERF(PlanPath);
	if (Source.Length <= 0.0f || Mirror.Length <= 0.0f
		|| (!Settings.UnderRailDescriptor && Settings.PillarSlots.Num() == 0))
	{
		return;
	}

	const float Step = FMath::Max(Settings.UnderRailLength, 50.0f);
	const float Separation = Settings.TrackSeparation;
	// positive separation on the right
	const bool bSourceIsLeft = Separation >= 0.0f;

	struct FARSTrackSide
	{
		const FARSPathCurve* Curve = nullptr;
		bool bIsLeft = false;
		bool bIsPartner = false;
	};
	const FARSTrackSide Sides[2] = { { &Source, bSourceIsLeft, false }, { &Mirror, !bSourceIsLeft, true } };

	auto SamplePair = [&Source, &Mirror](float Fraction, FVector& OutSourceLocation, FVector& OutSourceDirection,
	                                     FVector& OutMirrorLocation, FVector& OutMirrorDirection)
	{
		Source.Sampler(Fraction * Source.Length, OutSourceLocation, OutSourceDirection);
		Mirror.Sampler(Fraction * Mirror.Length, OutMirrorLocation, OutMirrorDirection);
	};

	auto RollAt = [&](float Fraction) -> float
	{
		FVector SourceLocation, SourceDirection, MirrorLocation, MirrorDirection;
		SamplePair(Fraction, SourceLocation, SourceDirection, MirrorLocation, MirrorDirection);
		const float LeftZ = bSourceIsLeft ? SourceLocation.Z : MirrorLocation.Z;
		const float RightZ = bSourceIsLeft ? MirrorLocation.Z : SourceLocation.Z;
		return -FMath::RadiansToDegrees(FMath::Atan2(RightZ - LeftZ, FMath::Abs(Separation)));
	};

	auto AnchorBasis = [&](float Fraction) -> FARSStationBasis
	{
		FVector SourceLocation, SourceDirection, MirrorLocation, MirrorDirection;
		SamplePair(Fraction, SourceLocation, SourceDirection, MirrorLocation, MirrorDirection);
		return MakeBasis(0.5f * (SourceLocation + MirrorLocation),
			SourceDirection.GetSafeNormal() + MirrorDirection.GetSafeNormal());
	};

	auto ForEachSide = [&Sides](EARSSideMode Mode, TFunctionRef<void(const FARSTrackSide&, bool bMirrored)> Body)
	{
		for (const FARSTrackSide& Side : Sides)
		{
			switch (Mode)
			{
			case EARSSideMode::BothMirrored:   Body(Side, Side.bIsPartner); break;
			case EARSSideMode::BothSameFacing: Body(Side, false); break;
			case EARSSideMode::LeftOnly:  if (Side.bIsLeft)  { Body(Side, false); } break;
			case EARSSideMode::RightOnly: if (!Side.bIsLeft) { Body(Side, false); } break;
			default: break; // center never lands on a rail
			}
		}
	};

	auto OutwardSignFor = [](const FARSTrackSide& Side) { return Side.bIsLeft ? -1.0f : 1.0f; };

	TArray<float> Distances;
	auto CollectDistances = [&](float CurveLength, const FARSUnderRailValues& Values)
	{
		Distances.Reset();
		const FARSStationPlan Plan = PlanStations(CurveLength, Values.Step);
		for (int32 Index = 0; Index < Plan.Count; Index++)
		{
			Distances.Add(Plan.DistanceAt(Index));
		}
		if (WantsEndGapStamp(Plan, Settings, Values.bCoverEndGap))
		{
			Distances.Add(CurveLength);
		}
	};

	if (Settings.UnderRailDescriptor)
	{
		if (Settings.UnderRailSideMode == EARSSideMode::Center)
		{
			FARSUnderRailValues Values = UnderRailValuesFor(Settings, /*bPartner*/ false);
			CollectDistances(Source.Length, Values);
			for (const float Distance : Distances)
			{
				const float Fraction = Distance / Source.Length;
				Values.Params.RollDegrees = Settings.bUnderRailAutoRoll ? RollAt(Fraction) : 0.0f;
				EmitStamp(AnchorBasis(Fraction), Values.Params, Settings, Payer, BatchId, OutJobs);
			}
		}
		else
		{
			ForEachSide(Settings.UnderRailSideMode, [&](const FARSTrackSide& Side, bool bMirrored)
			{
				FARSUnderRailValues Values = UnderRailValuesFor(Settings, Side.bIsPartner);
				Values.Params.bMirrored = bMirrored;
				Values.Params.OutwardSign = OutwardSignFor(Side);
				CollectDistances(Side.Curve->Length, Values);
				for (const float Distance : Distances)
				{
					EmitStamp(SampleBasis(Side.Curve->Sampler, Distance), Values.Params, Settings, Payer, BatchId, OutJobs);
				}
			});
		}
	}

	const FARSStationPlan SourcePlan = PlanStations(Source.Length, Step);
	for (int32 Index = 0; Index < SourcePlan.Count; Index++)
	{
		const int32 TopPriority = TopFiringPriority(Settings, Index);
		if (TopPriority < 0)
		{
			continue;
		}
		const float Fraction = SourcePlan.DistanceAt(Index) / Source.Length;

		for (const FARSPillarSlot& PillarSlot : Settings.PillarSlots)
		{
			if (!SlotStampsAt(PillarSlot, Index) || SlotIsSuppressed(PillarSlot, TopPriority))
			{
				continue;
			}
			FARSStampParams Params = PillarParams(PillarSlot);

			if (PillarSlot.SideMode == EARSSideMode::Center)
			{
				Params.RollDegrees = PillarSlot.bAutoRoll ? RollAt(Fraction) : 0.0f;
				EmitStamp(AnchorBasis(Fraction), Params, Settings, Payer, BatchId, OutJobs);
				continue;
			}
			ForEachSide(PillarSlot.SideMode, [&](const FARSTrackSide& Side, bool bMirrored)
			{
				Params.bMirrored = bMirrored;
				Params.OutwardSign = OutwardSignFor(Side);
				Params.bPartnerSide = Side.bIsPartner;
				EmitStamp(SampleBasis(Side.Curve->Sampler, Fraction * Side.Curve->Length),
					Params, Settings, Payer, BatchId, OutJobs);
			});
		}
	}
}

bool FARSGenerator::MeasureBlueprintLocalBounds(UWorld* World, UFGBlueprintDescriptor* Descriptor, FBox& OutLocalBounds,
                                                TArray<FARSPreviewPiece>* OutPieces)
{
	ARS_SCOPE_PERF(Measure);
	OutLocalBounds = FBox(ForceInit);
	if (OutPieces)
	{
		OutPieces->Reset();
	}
	if (!World || !Descriptor)
	{
		return false;
	}
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(World);
	if (!BlueprintSubsystem)
	{
		return false;
	}

	// spawn in the main world far below the map and measure
	const FVector ProbeLocation(0.0f, 0.0f, -250000.0f);
	const FTransform ProbeTransform(FQuat::Identity, ProbeLocation);

	AFGBlueprintProxy* Proxy = World->SpawnActorDeferred<AFGBlueprintProxy>(
		AFGBlueprintProxy::StaticClass(), ProbeTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Proxy)
	{
		return false;
	}
	Proxy->mBlueprintName = Descriptor->GetItemNameFromInstance();
	Proxy->mLocalBounds = FBox(FVector(-100.0f), FVector(100.0f));
	Proxy->FinishSpawning(ProbeTransform);

	TArray<AFGBuildable*> Spawned;
	BlueprintSubsystem->LoadStoredBlueprint(Descriptor, ProbeTransform, Spawned, /*useBlueprintWorld*/ false, nullptr, nullptr, Proxy);

	FBox WorldBounds(ForceInit);

	TSet<AFGBuildable*> RealBuildables;
	for (AFGBuildable* Buildable : Spawned)
	{
		if (IsValid(Buildable))
		{
			RealBuildables.Add(Buildable);
		}
	}
	for (AFGBuildable* Buildable : Proxy->GetBuildables())
	{
		if (IsValid(Buildable))
		{
			RealBuildables.Add(Buildable);
		}
	}
	for (AFGBuildable* Buildable : RealBuildables)
	{
		const FBox ActorBounds = Buildable->GetComponentsBoundingBox(/*bNonColliding*/ true);
		if (ActorBounds.IsValid && !ActorBounds.GetExtent().IsNearlyZero())
		{
			WorldBounds += ActorBounds;
		}

		if (OutPieces)
		{
			TArray<UStaticMeshComponent*> MeshComponents;
			Buildable->GetComponents<UStaticMeshComponent>(MeshComponents);
			for (UStaticMeshComponent* MeshComponent : MeshComponents)
			{
				if (MeshComponent && MeshComponent->GetStaticMesh() && MeshComponent->IsVisible())
				{
					FARSPreviewPiece& Piece = OutPieces->AddDefaulted_GetRef();
					Piece.Mesh = MeshComponent->GetStaticMesh();
					Piece.RelativeToOrigin = MeshComponent->GetComponentTransform().GetRelativeTransform(ProbeTransform);
				}
			}
		}
	}

	AFGLightweightBuildableSubsystem* LightweightSubsystem = AFGLightweightBuildableSubsystem::Get(World);
	const TArray<FBuildableClassLightweightIndices> LightweightEntries = Proxy->GetLightweightClassAndIndices();
	if (LightweightSubsystem)
	{
		for (const FBuildableClassLightweightIndices& Entry : LightweightEntries)
		{
			for (const int32 InstanceIndex : Entry.Indices)
			{
				const FRuntimeBuildableInstanceData* RuntimeData = LightweightSubsystem->GetRuntimeDataForBuildableClassAndIndex(Entry.BuildableClass, InstanceIndex);
				if (RuntimeData && RuntimeData->BoundingBox.IsValid)
				{
					WorldBounds += RuntimeData->BoundingBox.TransformBy(RuntimeData->Transform);
				}
				else if (RuntimeData)
				{
					// not computed yet, so approximate from the class's instance meshes
					if (const AFGBuildable* BuildableCDO = Entry.BuildableClass ? Entry.BuildableClass->GetDefaultObject<AFGBuildable>() : nullptr)
					{
						for (const FInstanceData& InstanceData : IAbstractInstanceInterface::Execute_GetActorLightweightInstanceData(BuildableCDO))
						{
							if (InstanceData.StaticMesh)
							{
								WorldBounds += InstanceData.StaticMesh->GetBoundingBox().TransformBy(InstanceData.RelativeTransform * RuntimeData->Transform);
							}
						}
					}
				}

				if (OutPieces && RuntimeData)
				{
					for (const FInstanceOwnerHandlePtr& Handle : RuntimeData->Handles)
					{
						if (Handle.IsValid() && Handle->IsInstanced())
						{
							const ULightweightHierarchicalInstancedStaticMeshComponent* InstanceComponent = Handle->GetInstanceComponent();
							UStaticMesh* Mesh = InstanceComponent ? InstanceComponent->GetStaticMesh() : nullptr;
							if (Mesh)
							{
								FARSPreviewPiece& Piece = OutPieces->AddDefaulted_GetRef();
								Piece.Mesh = Mesh;
								Piece.RelativeToOrigin = Handle->GetWorldTransform().GetRelativeTransform(ProbeTransform);
							}
						}
					}
				}
			}
		}
	}

	if (LightweightSubsystem)
	{
		for (const FBuildableClassLightweightIndices& Entry : LightweightEntries)
		{
			for (const int32 InstanceIndex : Entry.Indices)
			{
				LightweightSubsystem->RemoveByInstanceIndex(Entry.BuildableClass, InstanceIndex);
			}
		}
	}
	for (AFGBuildable* Buildable : RealBuildables)
	{
		Buildable->Destroy();
	}
	if (IsValid(Proxy))
	{
		Proxy->Destroy();
	}

	if (!WorldBounds.IsValid)
	{
		UE_LOG(LogARSGenerator, Warning, TEXT("Could not measure content bounds for blueprint '%s'"), *Descriptor->GetBlueprintNameAsString());
		return false;
	}

	OutLocalBounds = WorldBounds.ShiftBy(-ProbeLocation);
	return true;
}

bool FARSGenerator::MeasureBlueprintInHologramWorld(UWorld* World, UFGBlueprintDescriptor* Descriptor, FBox& OutLocalBounds,
                                                   TArray<FARSPreviewPiece>* OutPieces)
{
	ARS_SCOPE_PERF(Measure);
	OutLocalBounds = FBox(ForceInit);
	if (OutPieces)
	{
		OutPieces->Reset();
	}
	if (!World || !Descriptor)
	{
		return false;
	}
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(World);
	if (!BlueprintSubsystem)
	{
		return false;
	}

	// the world the game keeps for building blueprint holograms
	TArray<AFGBuildable*> Buildables;
	BlueprintSubsystem->LoadStoredBlueprint(Descriptor, FTransform::Identity, Buildables, /*useBlueprintWorld*/ true);

	FBox LocalBounds(ForceInit);
	auto AddPiece = [&LocalBounds, OutPieces](UStaticMesh* Mesh, const FTransform& Transform)
	{
		if (!Mesh)
		{
			return;
		}
		LocalBounds += Mesh->GetBoundingBox().TransformBy(Transform);
		if (OutPieces)
		{
			FARSPreviewPiece& Piece = OutPieces->AddDefaulted_GetRef();
			Piece.Mesh = Mesh;
			Piece.RelativeToOrigin = Transform;
		}
	};

	for (AFGBuildable* Buildable : Buildables)
	{
		if (!IsValid(Buildable))
		{
			continue;
		}
		const FTransform ActorTransform = Buildable->GetActorTransform();

		TArray<UStaticMeshComponent*> MeshComponents;
		Buildable->GetComponents<UStaticMeshComponent>(MeshComponents);
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (!MeshComponent || !MeshComponent->GetStaticMesh())
			{
				continue;
			}
			if (UFGColoredInstanceMeshProxy* Proxy = Cast<UFGColoredInstanceMeshProxy>(MeshComponent))
			{
				Proxy->SetInstanced(false);
			}
			if (MeshComponent->bHiddenInGame)
			{
				continue;
			}
			AddPiece(MeshComponent->GetStaticMesh(), MeshComponent->GetComponentTransform());
		}

		const AFGBuildable* BuildableCDO = Buildable->GetClass()->GetDefaultObject<AFGBuildable>();
		if (!BuildableCDO)
		{
			continue;
		}

		TArray<FInstanceData> InstanceDatas;
		BuildableCDO->CreateLightweightBuildableInstanceData(Buildable->GetLightweightTypeSpecificData(), InstanceDatas);

		if (InstanceDatas.Num() == 0 && BuildableCDO->Implements<UAbstractInstanceInterface>())
		{
			InstanceDatas = IAbstractInstanceInterface::Execute_GetActorLightweightInstanceData(BuildableCDO);
		}

		for (const FInstanceData& InstanceData : InstanceDatas)
		{
			AddPiece(InstanceData.StaticMesh, InstanceData.RelativeTransform * ActorTransform);
		}
	}

	for (AFGBuildable* Buildable : Buildables)
	{
		if (IsValid(Buildable))
		{
			Buildable->Destroy();
		}
	}

	if (!LocalBounds.IsValid)
	{
		UE_LOG(LogARSGenerator, Warning,
			TEXT("Could not measure content bounds for blueprint '%s' in the hologram world (%d buildables)"),
			*Descriptor->GetBlueprintNameAsString(), Buildables.Num());
		return false;
	}
	OutLocalBounds = LocalBounds;
	return true;
}

bool FARSGenerator::CanAfford(UWorld* World, AFGCharacterPlayer* Payer, const TArray<FItemAmount>& Cost)
{
	UFGInventoryComponent* Inventory = Payer->GetInventory();
	if (!Inventory)
	{
		return false;
	}
	AFGCentralStorageSubsystem* CentralStorage = AFGCentralStorageSubsystem::Get(World);
	for (const FItemAmount& Entry : Cost)
	{
		int32 Available = Inventory->GetNumItems(Entry.ItemClass);
		if (CentralStorage)
		{
			Available += CentralStorage->GetNumItemsFromCentralStorage(Entry.ItemClass);
		}
		if (Available < Entry.Amount)
		{
			return false;
		}
	}
	return true;
}

void FARSGenerator::ChargeCost(UWorld* World, AFGCharacterPlayer* Payer, const TArray<FItemAmount>& Cost)
{
	UFGInventoryComponent* Inventory = Payer->GetInventory();
	AFGCentralStorageSubsystem* CentralStorage = AFGCentralStorageSubsystem::Get(World);
	bool bInventoryFirst = true;
	if (const AFGPlayerState* PlayerState = Payer->GetPlayerState<AFGPlayerState>())
	{
		bInventoryFirst = PlayerState->GetTakeFromInventoryBeforeCentralStorage();
	}
	for (const FItemAmount& Entry : Cost)
	{
		UFGInventoryLibrary::GrabItemsFromInventoryAndCentralStorage(Inventory, CentralStorage, bInventoryFirst, Entry.ItemClass, Entry.Amount);
	}
}

void FARSGenerator::MergeItemAmounts(TArray<FItemAmount>& InOutTotal, const TArray<FItemAmount>& Extra)
{
	for (const FItemAmount& Entry : Extra)
	{
		if (FItemAmount* Existing = InOutTotal.FindByPredicate(
			[&Entry](const FItemAmount& Total) { return Total.ItemClass == Entry.ItemClass; }))
		{
			Existing->Amount += Entry.Amount;
		}
		else
		{
			InOutTotal.Add(Entry);
		}
	}
}

bool FARSGenerator::ExecuteStampJob(UWorld* World, const FARSStampJob& Job, AFGBlueprintProxy*& InOutSharedProxy, FText& OutFailReason)
{
	ARS_SCOPE_PERF(ExecuteStampJob);
	UFGBlueprintDescriptor* Descriptor = Job.Descriptor.Get();
	if (!World || !Descriptor)
	{
		return false;
	}
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(World);
	if (!BlueprintSubsystem)
	{
		return false;
	}

	AFGCharacterPlayer* Payer = Job.Payer.Get();
	const AFGGameState* GameState = World->GetGameState<AFGGameState>();
	bool bNoCost = GameState && GameState->GetCheatNoCost();
	if (!bNoCost && Payer && Payer->GetInventory())
	{
		bNoCost = Payer->GetInventory()->GetNoBuildCost();
	}

	TArray<FItemAmount> Cost;
	Descriptor->GetBlueprintCost(Cost);

	if (!bNoCost)
	{
		if (!Payer)
		{
			OutFailReason = NSLOCTEXT("AutoRailSupports", "NoPayer", "could not determine the building player, supports were skipped.");
			return false;
		}
		if (!CanAfford(World, Payer, Cost))
		{
			OutFailReason = FText::Format(
				NSLOCTEXT("AutoRailSupports", "CantAfford", "not enough materials for blueprint '{0}', remaining blueprint parts were skipped for this build."),
				Descriptor->GetItemNameFromInstance());
			return false;
		}
	}

	AFGBlueprintProxy* Proxy = nullptr;
	if (Job.bCombineIntoOneBlueprint && InOutSharedProxy)
	{
		Proxy = InOutSharedProxy;
	}
	else
	{
		Proxy = World->SpawnActorDeferred<AFGBlueprintProxy>(
			AFGBlueprintProxy::StaticClass(), Job.Transform, nullptr, Payer,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Proxy)
		{
			return false;
		}
		Proxy->mBlueprintName = Job.bCombineIntoOneBlueprint
			? NSLOCTEXT("AutoRailSupports", "CombinedBlueprintName", "Auto Rail Supports")
			: Descriptor->GetItemNameFromInstance();
		FBox LocalBounds = Job.LocalBounds;
		if (!LocalBounds.IsValid)
		{
			const FVector PlaceholderHalfExtent = FVector(Descriptor->GetDimensionsOnInstance()) * (ARSDimensionUnitMeters * 100.0f * 0.5f);
			LocalBounds = FBox(-PlaceholderHalfExtent, PlaceholderHalfExtent);
		}
		Proxy->mLocalBounds = LocalBounds;
		if (UBoxComponent* BoundingBox = Proxy->GetBoundingBox())
		{
			BoundingBox->SetRelativeLocation(LocalBounds.GetCenter());
			BoundingBox->SetBoxExtent(LocalBounds.GetExtent());
		}
		Proxy->FinishSpawning(Job.Transform);
		if (Job.bCombineIntoOneBlueprint)
		{
			InOutSharedProxy = Proxy;
		}
	}

	TArray<AFGBuildable*> SpawnedBuildables;
	BlueprintSubsystem->LoadStoredBlueprint(Descriptor, Job.Transform, SpawnedBuildables,
		/*useBlueprintWorld*/ false, /*designer*/ nullptr, Payer, Proxy);

	if (SpawnedBuildables.Num() == 0 && !Proxy->GetLightweightClassAndIndices().Num())
	{
		UE_LOG(LogARSGenerator, Warning, TEXT("LoadStoredBlueprint spawned nothing for '%s'"), *Descriptor->GetBlueprintNameAsString());
	}

	if (!bNoCost)
	{
		ChargeCost(World, Payer, Cost);
	}
	return true;
}
