#include "ARSRemoteCallObject.h"

#include "ARSClientManager.h"
#include "ARSConfigWidget.h"
#include "ARSDebug.h"
#include "ARSModConfigStruct.h"
#include "ARSSubsystem.h"
#include "FGBlueprintSubsystem.h"
#include "FGChatManager.h"
#include "FGFactoryBlueprintTypes.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "Net/UnrealNetwork.h"

void UARSRemoteCallObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UARSRemoteCallObject, mDummyReplicatedField);
}

FString UARSRemoteCallObject::GetSenderKey() const
{
	const AFGPlayerController* PlayerController = Cast<AFGPlayerController>(GetOuter());
	return AARSSubsystem::GetPlayerKey(PlayerController ? PlayerController->PlayerState : nullptr);
}

void UARSRemoteCallObject::Server_SetSettings_Implementation(FARSSettingsData NewSettings)
{
	if (AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld()))
	{
		const FString UserId = GetSenderKey();
		Subsystem->ApplySettingsFor(UserId, NewSettings);
		Client_Settings(Subsystem->GetSettingsFor(UserId));
	}
}

void UARSRemoteCallObject::Server_SetEnabled_Implementation(bool bNewEnabled)
{
	if (AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld()))
	{
		const FString UserId = GetSenderKey();
		const bool bEnabled = Subsystem->SetEnabledSettingFor(UserId, bNewEnabled);
		Client_Settings(Subsystem->GetSettingsFor(UserId));
		Client_Notify(FString::Printf(TEXT("Auto Rail Supports is now %s."), bEnabled ? TEXT("ON") : TEXT("OFF")), /*bConfirmation*/ true);
	}
}

void UARSRemoteCallObject::Server_RequestSettings_Implementation()
{
	if (AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld()))
	{
		Client_Settings(Subsystem->GetSettingsFor(GetSenderKey()));
	}
}

void UARSRemoteCallObject::Client_Settings_Implementation(FARSSettingsData Settings)
{
	if (AARSClientManager* ClientManager = AARSClientManager::Get(GetWorld()))
	{
		ClientManager->ApplyServerSettings(Settings);
	}
}

void UARSRemoteCallObject::Server_ApplyAimedRail_Implementation(AFGBuildableRailroadTrack* Track)
{
	AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld());
	AFGPlayerController* PlayerController = Cast<AFGPlayerController>(GetOuter());
	if (!Subsystem || !PlayerController || !Track)
	{
		return;
	}
	AFGCharacterPlayer* Character = Cast<AFGCharacterPlayer>(PlayerController->GetPawn());
	if (!Character)
	{
		return;
	}
	Subsystem->ApplyToTrack(Track, Character);
	Client_Notify(FString::Printf(TEXT("Auto Rail Supports: generating supports under the aimed rail segment (%.1f m)."), Track->GetLength() / 100.0f), /*bConfirmation*/ true);
}

void UARSRemoteCallObject::Server_SetPreviewSource_Implementation(AActor* Source)
{
	AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld());
	AFGPlayerController* PlayerController = Cast<AFGPlayerController>(GetOuter());
	AFGCharacterPlayer* Character = PlayerController ? Cast<AFGCharacterPlayer>(PlayerController->GetPawn()) : nullptr;
	if (!Subsystem || !Character)
	{
		return;
	}
	Subsystem->SetPreviewSource(Character, Source);
}

void UARSRemoteCallObject::Server_SetPreviewSpline_Implementation(const TArray<FARSSplinePoint>& Points, uint8 Stencil)
{
	AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld());
	AFGPlayerController* PlayerController = Cast<AFGPlayerController>(GetOuter());
	AFGCharacterPlayer* Character = PlayerController ? Cast<AFGCharacterPlayer>(PlayerController->GetPawn()) : nullptr;
	if (!Subsystem || !Character)
	{
		return;
	}
	Subsystem->SetPreviewSpline(Character, Points, Stencil);
}

void UARSRemoteCallObject::Server_RequestBlueprintSize_Implementation(const FString& BlueprintName)
{
	FVector SizeMeters = FVector::ZeroVector;
	AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld());
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::GetBlueprintSubsystem(GetWorld());
	if (Subsystem && BlueprintSubsystem && !BlueprintName.IsEmpty())
	{
		if (UFGBlueprintDescriptor* Descriptor = BlueprintSubsystem->GetBlueprintDescriptorByNameString(BlueprintName))
		{
			const FBox& LocalBounds = Subsystem->GetMeasuredBlueprint(Descriptor).LocalBounds;
			if (LocalBounds.IsValid)
			{
				SizeMeters = LocalBounds.GetSize() / 100.0f;
			}
		}
	}
	Client_BlueprintSize(BlueprintName, SizeMeters);
}

void UARSRemoteCallObject::Client_BlueprintSize_Implementation(const FString& BlueprintName, FVector SizeMeters)
{
	if (AARSClientManager* ClientManager = AARSClientManager::Get(GetWorld()))
	{
		if (UARSConfigWidget* Widget = ClientManager->GetOpenConfigWidget())
		{
			Widget->HandleBlueprintSize(BlueprintName, SizeMeters);
		}
	}
}

void UARSRemoteCallObject::Client_OpenUI_Implementation()
{
	if (AARSClientManager* ClientManager = AARSClientManager::Get(GetWorld()))
	{
		ClientManager->OpenUI();
	}
}

void UARSRemoteCallObject::Client_Notify_Implementation(const FString& Message, bool bConfirmation)
{
	if (bConfirmation && FARSModConfigStruct::GetActiveConfig(GetWorld()).bMuteChatConfirmations)
	{
		return;
	}
	if (AFGChatManager* ChatManager = AFGChatManager::Get(GetWorld()))
	{
		FChatMessageStruct ChatMessage;
		ChatMessage.MessageText = FText::FromString(Message);
		ChatMessage.MessageType = EFGChatMessageType::CMT_SystemMessage;
		ChatManager->AddChatMessageToReceived(ChatMessage);
	}
}
