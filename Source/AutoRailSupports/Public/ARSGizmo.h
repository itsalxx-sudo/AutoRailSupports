#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ARSGizmo.generated.h"

class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FARSGizmoCircle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ARS Gizmo") FName Name;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ARS Gizmo") TObjectPtr<UStaticMesh> Mesh = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ARS Gizmo") FRotator MeshRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ARS Gizmo") float MeshScale = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ARS Gizmo") FLinearColor Color = FLinearColor::White;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ARS Gizmo") int32 AxisIndex = 0;

	// runtime only
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Component = nullptr;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material = nullptr;
	float Progress = 0.0f;
};

UCLASS(Blueprintable)
class AUTORAILSUPPORTS_API UARSGizmo : public USceneComponent
{
	GENERATED_BODY()
public:
	UARSGizmo();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void InitCircles();

	void DestroyCircles();

	void SetActiveAxis(int32 Axis) { mActiveAxis = Axis; }

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ARS Gizmo") TArray<FARSGizmoCircle> mCircles;

	int32 mActiveAxis = 0;
};
