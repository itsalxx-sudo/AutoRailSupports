#pragma once
#include "CoreMinimal.h"
#include "ARSSettingsData.h"

class AFGBlueprintProxy;
class AFGBuildableRailroadTrack;
class AFGCharacterPlayer;
class UFGBlueprintDescriptor;
class UWorld;

constexpr float ARSDimensionUnitMeters = 0.8f;

struct FARSPillarSlot
{
	UFGBlueprintDescriptor* Descriptor = nullptr;
	int32 EveryN = 4;
	int32 SpacingOffset = 0;
	int32 Priority = 0;
	bool bPreventOverlap = true;
	bool bAlignToRail = false;
	EARSSideMode SideMode = EARSSideMode::Center;
	bool bAutoRoll = false;
	FRotator RotationOffset = FRotator::ZeroRotator;
	float ZOffset = 0.0f;
	float SideOffset = 0.0f;
	float ForwardOffset = 0.0f;
	FBox LocalBounds = FBox(ForceInit);
};

struct FARSSettings
{
	UFGBlueprintDescriptor* UnderRailDescriptor = nullptr;

	TArray<FARSPillarSlot> PillarSlots;

	FBox UnderRailLocalBounds = FBox(ForceInit);

	float UnderRailLength = 800.0f;
	bool bCoverEndGap = false;
	bool bCombineIntoOneBlueprint = false;
	FRotator UnderRailRotationOffset = FRotator::ZeroRotator;
	float UnderRailZOffset = 0.0f;
	float UnderRailSideOffset = 0.0f;
	float UnderRailForwardOffset = 0.0f;

	bool bDoubleRails = false;
	// signed
	float TrackSeparation = 600.0f;
	EARSSideMode UnderRailSideMode = EARSSideMode::BothMirrored;
	bool bUnderRailAutoRoll = false;
	bool bSplitUnderRailSides = false;
	FARSUnderRailSide PartnerUnderRail;
};

struct FARSStampJob
{
	TWeakObjectPtr<UFGBlueprintDescriptor> Descriptor;
	FTransform Transform;
	int32 PartIndex = 0;
	// sits on the second track rather than the rail being placed
	bool bPartnerSide = false;
	TWeakObjectPtr<AFGCharacterPlayer> Payer;
	FBox LocalBounds = FBox(ForceInit);
	uint64 BatchId = 0;
	bool bCombineIntoOneBlueprint = false;
};

struct FARSPreviewPiece
{
	TWeakObjectPtr<class UStaticMesh> Mesh;
	FTransform RelativeToOrigin = FTransform::Identity;
};

struct FARSChainSegment
{
	AFGBuildableRailroadTrack* Track = nullptr;
	bool bReversed = false;
	float StartDistance = 0.0f;
	float Length = 0.0f;
};

using FARSPathSampler = TFunction<void(float Distance, FVector& OutLocation, FVector& OutDirection)>;

struct FARSPathCurve
{
	FARSPathSampler Sampler;
	float Length = 0.0f;
};

class AUTORAILSUPPORTS_API FARSGenerator
{
public:
	static TArray<TArray<FARSChainSegment>> BuildChains(const TArray<AFGBuildableRailroadTrack*>& Tracks);

	static void PlanChain(const TArray<FARSChainSegment>& Chain, const FARSSettings& Settings,
	                      AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs);

	static void PlanPath(const FARSPathSampler& Sampler, float TotalLength, const FARSSettings& Settings,
	                     AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs);

	static void PlanDoublePath(const FARSPathCurve& Source, const FARSPathCurve& Mirror, const FARSSettings& Settings,
	                           AFGCharacterPlayer* Payer, uint64 BatchId, TArray<FARSStampJob>& OutJobs);

	static FARSPathCurve MakeSplineCurve(const class USplineComponent* Spline);

	static bool ExecuteStampJob(UWorld* World, const FARSStampJob& Job, AFGBlueprintProxy*& InOutSharedProxy, FText& OutFailReason);

	static bool MeasureBlueprintLocalBounds(UWorld* World, UFGBlueprintDescriptor* Descriptor, FBox& OutLocalBounds,
	                                        TArray<FARSPreviewPiece>* OutPieces = nullptr);

	static bool MeasureBlueprintInHologramWorld(UWorld* World, UFGBlueprintDescriptor* Descriptor, FBox& OutLocalBounds,
	                                            TArray<FARSPreviewPiece>* OutPieces = nullptr);

	static void MergeItemAmounts(TArray<struct FItemAmount>& InOutTotal, const TArray<struct FItemAmount>& Extra);

private:
	static void SampleChain(const TArray<FARSChainSegment>& Chain, float Distance, FVector& OutLocation, FVector& OutDirection);

	static bool CanAfford(UWorld* World, AFGCharacterPlayer* Payer, const TArray<struct FItemAmount>& Cost);
	static void ChargeCost(UWorld* World, AFGCharacterPlayer* Payer, const TArray<struct FItemAmount>& Cost);
};
