#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemAmount.h"
#include "ARSGenerator.h"
#include "ARSSettingsData.h"
#include "ARSPreviewActor.generated.h"

class AFGBuildableRailroadTrack;
class AFGHologram;
class UFGBlueprintDescriptor;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class USplineComponent;

UCLASS(NotPlaceable)
class AUTORAILSUPPORTS_API AARSPreviewActor : public AActor
{
	GENERATED_BODY()
public:
	AARSPreviewActor();

	void UpdateFromHologram(AFGHologram* RailHologram, const FARSSettingsData& SettingsData, bool bAllowUpdateWhileLocked = false, bool bCollectAnchors = false);

	void UpdateFromTrack(AFGBuildableRailroadTrack* Track, const FARSSettingsData& SettingsData, bool bCollectAnchors = false);

	void UpdateFromSplinePoints(const TArray<FARSSplinePoint>& Points, const FARSSettingsData& SettingsData, uint8 Stencil);

	static uint8 GetHologramStencil(AFGHologram* RailHologram);

	void ClearPreview();

	// material cost of everything the current preview would stamp, for the build-gun cost panel
	const TArray<FItemAmount>& GetPlannedCost() const { return mPlannedCost; }

	struct FARSStampAnchor
	{
		int32 PartIndex = 0;
		FTransform Transform;
	};
	bool GetClosestStampTransform(int32 PartIndex, const FVector& From, FTransform& OutTransform) const;

private:
	void RebuildFromSpline(USplineComponent* Spline, const FARSSettingsData& SettingsData, uint8 Stencil, bool bCollectAnchors);

	UInstancedStaticMeshComponent* GetOrCreateISM(UStaticMesh* Mesh);

	TArray<FItemAmount> mPlannedCost;

	TArray<FARSStampAnchor> mStampAnchors;

	UPROPERTY()
	TObjectPtr<USceneComponent> mRoot;

	UPROPERTY()
	TObjectPtr<USplineComponent> mReplicatedSpline;

	UPROPERTY()
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> mMeshToISM;

	uint8 mDepthStencil;
};
