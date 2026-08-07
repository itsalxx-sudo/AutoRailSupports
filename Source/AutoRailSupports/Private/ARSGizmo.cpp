#include "ARSGizmo.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UARSGizmo::UARSGizmo()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UARSGizmo::InitCircles()
{
	for (FARSGizmoCircle& Circle : mCircles)
	{
		if (!Circle.Mesh || Circle.Component)
		{
			continue; // no mesh authored, or already built
		}
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, Circle.Name.IsNone() ? NAME_None : Circle.Name);
		Comp->SetCastShadow(false);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetRenderCustomDepth(true);
		Comp->SetCustomDepthStencilValue(1);
		Comp->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_255);
		Comp->SetStaticMesh(Circle.Mesh);
		Comp->SetRelativeRotation(Circle.MeshRotation);
		Comp->SetRelativeScale3D(FVector(Circle.MeshScale));

		if (UMaterialInterface* BaseMaterial = Circle.Mesh->GetMaterial(0))
		{
			Circle.Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (Circle.Material)
			{
				Circle.Material->SetVectorParameterValue(TEXT("baseColor"), Circle.Color);
				Circle.Material->SetScalarParameterValue(TEXT("progress"), 0.0f);
				Comp->SetMaterial(0, Circle.Material);
			}
		}

		Comp->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
		Comp->RegisterComponent();
		Circle.Component = Comp;
	}
}

void UARSGizmo::DestroyCircles()
{
	for (FARSGizmoCircle& Circle : mCircles)
	{
		if (Circle.Component)
		{
			Circle.Component->UnregisterComponent();
			Circle.Component->DestroyComponent();
		}
		Circle.Component = nullptr;
		Circle.Material = nullptr;
		Circle.Progress = 0.0f;
	}
}

void UARSGizmo::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	DestroyCircles();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UARSGizmo::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	constexpr float Speed = 20.0f;
	for (FARSGizmoCircle& Circle : mCircles)
	{
		const float Target = (Circle.AxisIndex == mActiveAxis) ? 1.0f : 0.0f;
		Circle.Progress = FMath::FInterpTo(Circle.Progress, Target, DeltaTime, Speed);
		if (Circle.Material)
		{
			Circle.Material->SetScalarParameterValue(TEXT("progress"), Circle.Progress);
		}
	}
}
