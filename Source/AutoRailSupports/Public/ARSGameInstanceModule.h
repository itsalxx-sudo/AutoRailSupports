#pragma once
#include "CoreMinimal.h"
#include "Module/GameInstanceModule.h"
#include "ARSGameInstanceModule.generated.h"

UCLASS()
class AUTORAILSUPPORTS_API UARSGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()
public:
	UARSGameInstanceModule();

	// adds the mod config to ModConfigurations before SML reads the array
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
