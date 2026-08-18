#include "ARSGenerator.h"

#include "ARSDebug.h"
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
#include "Buildables/FGBuildableBlueprintDesigner.h"
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

void FARSGenerator::PlanChain(const TArray<FARSChainSegment>& Chain, const FARSSettings& Settings,
                              AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs)
{
	if (Chain.Num() == 0)
	{
		return;
	}
	const float TotalLength = Chain.Last().StartDistance + Chain.Last().Length;
	const int32 FirstJob = OutJobs.Num();
	PlanPath(
		[&Chain](float Distance, FVector& OutLocation, FVector& OutDirection)
		{
			SampleChain(Chain, Distance, OutLocation, OutDirection);
		},
		TotalLength, Settings, Payer, BatchId, OutJobs);

	AFGBuildableBlueprintDesigner* Designer = Chain[0].Track ? Chain[0].Track->GetBlueprintDesigner() : nullptr;
	if (!Designer)
	{
		return;
	}
	for (int32 i = FirstJob; i < OutJobs.Num(); i++)
	{
		OutJobs[i].Designer = Designer;
	}
}

void FARSGenerator::PlanPath(const FARSPathSampler& Sampler, float TotalLength, const FARSSettings& Settings,
                             AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs)
{
	ARS_SCOPE_PERF(PlanPath);
	if (TotalLength <= 0.0f || (!Settings.UnderRailDescriptor && Settings.PillarSlots.Num() == 0))
	{
		return;
	}

	const float Step = FMath::Max(Settings.UnderRailLength, 50.0f);

	// shift up by the content's top height so the top face lands below the rail
	auto PivotedLocation = [](const FVector& InLocation, const FQuat& Heading, const FQuat& Rotation, const FBox& LocalBounds) -> FVector
	{
		if (!LocalBounds.IsValid)
		{
			return InLocation;
		}
		const FVector Center = LocalBounds.GetCenter();
		const FVector Top(0.0f, 0.0f, LocalBounds.Max.Z);
		// pivot about the box center, the gizmo parks here
		return InLocation + Heading.RotateVector(Center - Top) - Rotation.RotateVector(Center);
	};

	auto ComputeBasis = [&Sampler](float Distance, FVector& OutLocation, FVector& OutDirection, FVector& OutFlatDirection, FVector& OutRightDirection, FQuat& OutSplineRotation)
	{
		FVector Direction = FVector::ForwardVector;
		Sampler(Distance, OutLocation, Direction);
		Direction = Direction.GetSafeNormal();
		OutDirection = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction;
		OutFlatDirection = Direction;
		OutFlatDirection.Z = 0.0f;
		OutFlatDirection = OutFlatDirection.GetSafeNormal();
		if (OutFlatDirection.IsNearlyZero())
		{
			OutFlatDirection = FVector::ForwardVector;
		}
		OutRightDirection = FVector::CrossProduct(FVector::UpVector, OutFlatDirection);
		OutSplineRotation = FRotationMatrix::MakeFromXZ(OutDirection, FVector::UpVector).ToQuat(); // X along track, Z near world-up
	};

	auto SlotStampsAt = [](const FARSPillarSlot& Slot, int32 Index) -> bool
	{
		return Index >= Slot.SpacingOffset && ((Index - Slot.SpacingOffset) % Slot.EveryN) == 0;
	};

	auto StampUnderRailAt = [&](float Distance)
	{
		if (!Settings.UnderRailDescriptor)
		{
			return;
		}
		FVector Location, Direction, FlatDirection, RightDirection;
		FQuat SplineRotation;
		ComputeBasis(Distance, Location, Direction, FlatDirection, RightDirection, SplineRotation);
		// user rotation goes on top of the spline rotation
		const FQuat Rotation = SplineRotation * Settings.UnderRailRotationOffset.Quaternion();
		const FVector StampLocation = Location
			+ FVector(0.0f, 0.0f, Settings.UnderRailZOffset)
			+ RightDirection * Settings.UnderRailSideOffset
			+ Direction * Settings.UnderRailForwardOffset;
		FARSStampJob& Job = OutJobs.AddDefaulted_GetRef();
		Job.Descriptor = Settings.UnderRailDescriptor;
		Job.Transform = FTransform(Rotation, PivotedLocation(StampLocation, SplineRotation, Rotation, Settings.UnderRailLocalBounds));
		Job.LocalBounds = Settings.UnderRailLocalBounds;
		Job.Payer = Payer;
		Job.BatchId = BatchId;
		Job.bCombineIntoOneBlueprint = Settings.bCombineIntoOneBlueprint;
	};

	const int32 FullTileCount = FMath::FloorToInt(TotalLength / Step);
	const float Remainder = TotalLength - FullTileCount * Step;
	const bool bTrailingTileOverhangs = Remainder > 0.5f * Step;
	int32 StampCount = FMath::Min(bTrailingTileOverhangs ? FullTileCount + 1 : FullTileCount, MaxStampsPerChain);

	for (int32 Index = 0; Index < StampCount; Index++)
	{
		const float Distance = (Index + 0.5f) * Step;

		StampUnderRailAt(Distance);

		// pillars: the highest-priority slot always stamps, lower ones only with prevention off
		int32 TopPriority = -1;
		for (const FARSPillarSlot& PillarSlot : Settings.PillarSlots)
		{
			if (SlotStampsAt(PillarSlot, Index))
			{
				TopPriority = FMath::Max(TopPriority, PillarSlot.Priority);
			}
		}
		if (TopPriority >= 0)
		{
			FVector Location, Direction, FlatDirection, RightDirection;
			FQuat SplineRotation;
			ComputeBasis(Distance, Location, Direction, FlatDirection, RightDirection, SplineRotation);

			for (const FARSPillarSlot& PillarSlot : Settings.PillarSlots)
			{
				if (!SlotStampsAt(PillarSlot, Index))
				{
					continue;
				}
				if (PillarSlot.bPreventOverlap && PillarSlot.Priority < TopPriority)
				{
					continue;
				}

				const FQuat HeadingRotation = PillarSlot.bAlignToRail
					? SplineRotation
					: FRotator(0.0f, FlatDirection.Rotation().Yaw, 0.0f).Quaternion();
				const FQuat Rotation = HeadingRotation * PillarSlot.RotationOffset.Quaternion();
				const FVector StampLocation = Location
					+ FVector(0.0f, 0.0f, PillarSlot.ZOffset)
					+ RightDirection * PillarSlot.SideOffset
					+ Direction * PillarSlot.ForwardOffset;

				FARSStampJob& Job = OutJobs.AddDefaulted_GetRef();
				Job.Descriptor = PillarSlot.Descriptor;
				Job.Transform = FTransform(Rotation, PivotedLocation(StampLocation, HeadingRotation, Rotation, PillarSlot.LocalBounds));
				Job.PartIndex = PillarSlot.Priority + 1;
				Job.LocalBounds = PillarSlot.LocalBounds;
				Job.Payer = Payer;
				Job.BatchId = BatchId;
				Job.bCombineIntoOneBlueprint = Settings.bCombineIntoOneBlueprint;
			}
		}
	}

	if (Settings.bCoverEndGap && Settings.UnderRailDescriptor && !bTrailingTileOverhangs && Remainder > 1.0f && StampCount < MaxStampsPerChain)
	{
		StampUnderRailAt(TotalLength);
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

	AFGBuildableBlueprintDesigner* Designer = Job.Designer.Get();

	AFGBlueprintProxy* Proxy = nullptr;
	if (Job.bCombineIntoOneBlueprint && InOutSharedProxy)
	{
		Proxy = InOutSharedProxy;
	}
	else if (!Designer)
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
		/*useBlueprintWorld*/ false, Designer, Payer, Proxy);

	if (Designer)
	{
		for (AFGBuildable* Buildable : SpawnedBuildables)
		{
			Designer->OnBuildableConstructedInsideDesigner(Buildable);
		}
	}

	if (SpawnedBuildables.Num() == 0 && (!Proxy || !Proxy->GetLightweightClassAndIndices().Num()))
	{
		UE_LOG(LogARSGenerator, Warning, TEXT("LoadStoredBlueprint spawned nothing for '%s'"), *Descriptor->GetBlueprintNameAsString());
	}

	if (!bNoCost)
	{
		ChargeCost(World, Payer, Cost);
	}
	return true;
}
