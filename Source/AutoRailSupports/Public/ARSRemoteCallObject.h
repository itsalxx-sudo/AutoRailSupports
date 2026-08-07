#pragma once
#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "ARSSettingsData.h"
#include "ARSRemoteCallObject.generated.h"

class AFGBuildableRailroadTrack;

// client<->server bridge
UCLASS(NotBlueprintable)
class AUTORAILSUPPORTS_API UARSRemoteCallObject : public UFGRemoteCallObject
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void Server_SetSettings(FARSSettingsData NewSettings);

	UFUNCTION(Server, Reliable)
	void Server_SetEnabled(bool bNewEnabled);

	UFUNCTION(Server, Reliable)
	void Server_RequestSettings();

	UFUNCTION(Client, Reliable)
	void Client_Settings(FARSSettingsData Settings);

	UFUNCTION(Server, Reliable)
	void Server_ApplyAimedRail(AFGBuildableRailroadTrack* Track);

	UFUNCTION(Server, Reliable)
	void Server_SetPreviewSource(AActor* Source);

	UFUNCTION(Server, Unreliable)
	void Server_SetPreviewSpline(const TArray<FARSSplinePoint>& Points, uint8 Stencil);

	UFUNCTION(Server, Reliable)
	void Server_RequestBlueprintSize(const FString& BlueprintName);

	UFUNCTION(Client, Reliable)
	void Client_BlueprintSize(const FString& BlueprintName, FVector SizeMeters);

	UFUNCTION(Client, Reliable)
	void Client_OpenUI();

	UFUNCTION(Client, Reliable)
	void Client_Notify(const FString& Message);

private:
	FString GetSenderKey() const;

	UPROPERTY(Replicated)
	int32 mDummyReplicatedField = 0;
};
