#include "ARSPreviewActor.h"

#include "ARSCurve.h"
#include "ARSDebug.h"
#include "ARSSubsystem.h"
#include "FGBlueprintSubsystem.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "Hologram/FGHologram.h"

AARSPreviewActor::AARSPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	mRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(mRoot);
	mDepthStencil = static_cast<uint8>(EOutlineColor::OC_HOLOGRAM);
}

void AARSPreviewActor::ClearPreview()
{
	for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : mMeshToISM)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearInstances();
		}
	}
	mPlannedCost.Reset();
	mStampAnchors.Reset();
}

UInstancedStaticMeshComponent* AARSPreviewActor::GetOrCreateISM(UStaticMesh* Mesh)
{
	if (!Mesh)
	{
		return nullptr;
	}
	TObjectPtr<UInstancedStaticMeshComponent>& Slot = mMeshToISM.FindOrAdd(Mesh);
	if (!Slot)
	{
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
		ISM->SetMobility(EComponentMobility::Movable);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCastShadow(false);
		ISM->SetStaticMesh(Mesh);
		ISM->SetRenderCustomDepth(true);
		ISM->SetCustomDepthStencilValue(mDepthStencil);
		ISM->RegisterComponent();
		ISM->AttachToComponent(mRoot, FAttachmentTransformRules::KeepWorldTransform);
		AddInstanceComponent(ISM);
		Slot = ISM;
	}
	return Slot;
}

void AARSPreviewActor::UpdateFromHologram(AFGHologram* RailHologram, const FARSSettingsData& SettingsData, bool bAllowUpdateWhileLocked, bool bCollectAnchors)
{
	ARS_SCOPE_PERF(PreviewUpdate);
	if (RailHologram && RailHologram->IsHologramLocked() && !bAllowUpdateWhileLocked)
	{
		return;
	}
	if (!RailHologram)
	{
		ClearPreview();
		return;
	}

	RebuildFromSpline(RailHologram->FindComponentByClass<USplineComponent>(), SettingsData,
		GetHologramStencil(RailHologram), bCollectAnchors);
}

uint8 AARSPreviewActor::GetHologramStencil(AFGHologram* RailHologram)
{
	if (RailHologram)
	{
		TArray<UMeshComponent*> HologramMeshes;
		RailHologram->GetComponents<UMeshComponent>(HologramMeshes);
		for (const UMeshComponent* MeshComp : HologramMeshes)
		{
			if (MeshComp && MeshComp->bRenderCustomDepth && MeshComp->CustomDepthStencilValue > 0)
			{
				return static_cast<uint8>(MeshComp->CustomDepthStencilValue);
			}
		}
	}
	return static_cast<uint8>(EOutlineColor::OC_HOLOGRAM);
}

void AARSPreviewActor::UpdateFromSplinePoints(const TArray<FARSSplinePoint>& Points, const FARSSettingsData& SettingsData, uint8 Stencil)
{
	ARS_SCOPE_PERF(PreviewUpdate);
	if (Points.Num() < 2)
	{
		ClearPreview();
		return;
	}
	if (!mReplicatedSpline)
	{
		mReplicatedSpline = NewObject<USplineComponent>(this);
		mReplicatedSpline->SetMobility(EComponentMobility::Movable);
		mReplicatedSpline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		mReplicatedSpline->RegisterComponent();
		mReplicatedSpline->AttachToComponent(mRoot, FAttachmentTransformRules::KeepWorldTransform);
		AddInstanceComponent(mReplicatedSpline);
	}

	mReplicatedSpline->ClearSplinePoints(false);
	for (int32 i = 0; i < Points.Num(); i++)
	{
		const FARSSplinePoint& Point = Points[i];
		mReplicatedSpline->AddSplinePoint(Point.Location, ESplineCoordinateSpace::World, false);
		mReplicatedSpline->SetTangentsAtSplinePoint(i, Point.ArriveTangent, Point.LeaveTangent,
			ESplineCoordinateSpace::World, false);
	}
	mReplicatedSpline->UpdateSpline();

	RebuildFromSpline(mReplicatedSpline, SettingsData, Stencil, /*bCollectAnchors*/ false);
}

void AARSPreviewActor::UpdateFromTrack(AFGBuildableRailroadTrack* Track, const FARSSettingsData& SettingsData, bool bCollectAnchors)
{
	ARS_SCOPE_PERF(PreviewUpdate);
	USplineComponent* Spline = Track ? Track->FindComponentByClass<USplineComponent>() : nullptr;
	RebuildFromSpline(Spline, SettingsData, static_cast<uint8>(EOutlineColor::OC_HOLOGRAM), bCollectAnchors);
}

bool AARSPreviewActor::GetClosestStampTransform(int32 PartIndex, const FVector& From, FTransform& OutTransform,
                                                bool* OutPartnerSide) const
{
	float BestDistSq = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const FARSStampAnchor& Anchor : mStampAnchors)
	{
		if (Anchor.PartIndex != PartIndex)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Anchor.Transform.GetLocation(), From);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			OutTransform = Anchor.Transform;
			if (OutPartnerSide)
			{
				*OutPartnerSide = Anchor.bPartnerSide;
			}
			bFound = true;
		}
	}
	return bFound;
}

void AARSPreviewActor::RebuildFromSpline(USplineComponent* Spline, const FARSSettingsData& SettingsData, uint8 Stencil, bool bCollectAnchors)
{
	ClearPreview(); // also drops the previous stamp anchors
	if (!Spline)
	{
		return;
	}
	const float TotalLength = Spline->GetSplineLength();
	if (TotalLength <= 0.0f)
	{
		return;
	}
	if (Stencil != mDepthStencil)
	{
		mDepthStencil = Stencil;
	}

	AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld());
	if (!Subsystem)
	{
		return;
	}
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(GetWorld());
	const FARSSettings Settings = AARSSubsystem::ResolveSettingsFrom(SettingsData, BlueprintSubsystem,
		[Subsystem](UFGBlueprintDescriptor* Descriptor, FBox& OutBounds)
		{
			OutBounds = Subsystem->GetMeasuredBlueprint(Descriptor).LocalBounds;
		},
		nullptr);

	if (!Settings.UnderRailDescriptor && Settings.PillarSlots.Num() == 0)
	{
		return;
	}

	TArray<FARSStampJob> Jobs;
	const FARSPathCurve SourceCurve = FARSGenerator::MakeSplineCurve(Spline);
	if (Settings.bDoubleRails)
	{
		TArray<FARSCurvePoint> Points;
		TArray<FARSCurvePoint> Offset;
		FARSCurve::ReadPoints(Spline, Points);
		FARSCurve::OffsetPoints(Points, Settings.TrackSeparation, Offset);
		if (USplineComponent* MirrorSpline = FARSCurve::MakeScratchSpline(Offset))
		{
			FARSGenerator::PlanDoublePath(SourceCurve, FARSGenerator::MakeSplineCurve(MirrorSpline),
				Settings, nullptr, 0, Jobs);
		}
	}
	else
	{
		FARSGenerator::PlanPath(SourceCurve.Sampler, TotalLength, Settings, nullptr, 0, Jobs);
	}

	mPlannedCost.Reset();
	TMap<TWeakObjectPtr<UFGBlueprintDescriptor>, TArray<FItemAmount>> CostByDescriptor;

	{
		ARS_SCOPE_PERF(PreviewFill);
		for (const FARSStampJob& Job : Jobs)
		{
			UFGBlueprintDescriptor* Descriptor = Job.Descriptor.Get();
			if (!Descriptor)
			{
				continue;
			}
			const TArray<FItemAmount>* DescriptorCost = CostByDescriptor.Find(Descriptor);
			if (!DescriptorCost)
			{
				TArray<FItemAmount>& NewCost = CostByDescriptor.Add(Descriptor);
				Descriptor->GetBlueprintCost(NewCost);
				DescriptorCost = &NewCost;
			}
			FARSGenerator::MergeItemAmounts(mPlannedCost, *DescriptorCost);

			if (bCollectAnchors)
			{
				FTransform Anchor = Job.Transform;
				if (Job.LocalBounds.IsValid)
				{
					Anchor.SetLocation(Job.Transform.TransformPosition(Job.LocalBounds.GetCenter()));
				}
				mStampAnchors.Add({ Job.PartIndex, Job.bPartnerSide, Anchor });
			}

			const AARSSubsystem::FARSMeasuredBlueprint& Measured = Subsystem->GetMeasuredBlueprint(Descriptor);
			for (const FARSPreviewPiece& Piece : Measured.Pieces)
			{
				UStaticMesh* Mesh = Piece.Mesh.Get();
				if (!Mesh)
				{
					continue;
				}
				if (UInstancedStaticMeshComponent* ISM = GetOrCreateISM(Mesh))
				{
					ISM->AddInstance(Piece.RelativeToOrigin * Job.Transform, /*bWorldSpace*/ true);
				}
			}
		}
	}

	for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : mMeshToISM)
	{
		if (Pair.Value && Pair.Value->CustomDepthStencilValue != mDepthStencil)
		{
			Pair.Value->SetCustomDepthStencilValue(mDepthStencil);
		}
	}
}
