#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "ARSModConfigStruct.generated.h"

USTRUCT(BlueprintType)
struct FARSModConfigStruct {
    GENERATED_BODY()
public:
    // ctrl position step (meters)
    UPROPERTY(BlueprintReadWrite)
    float PositionStepTiny { 0.01f };

    // alt position step (meters)
    UPROPERTY(BlueprintReadWrite)
    float PositionStepSmall { 0.1f };

    // shift position step (meters)
    UPROPERTY(BlueprintReadWrite)
    float PositionStepLarge { 4.0f };

    // ctrl rotation step (degrees)
    UPROPERTY(BlueprintReadWrite)
    float RotationStepTiny { 0.1f };

    // alt rotation step (degrees)
    UPROPERTY(BlueprintReadWrite)
    float RotationStepSmall { 1.0f };

    // shift rotation step (degrees)
    UPROPERTY(BlueprintReadWrite)
    float RotationStepLarge { 45.0f };

    UPROPERTY(BlueprintReadWrite)
    float InfoPanelBackgroundOpacity { 0.0f };

    static FARSModConfigStruct GetActiveConfig(UObject* WorldContext) {
        FARSModConfigStruct ConfigStruct{};
        FConfigId ConfigId{"AutoRailSupports", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FARSModConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};
