#include "ARSCommand.h"

#include "ARSClientManager.h"
#include "ARSRemoteCallObject.h"
#include "ARSSubsystem.h"
#include "Command/CommandSender.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "Engine/World.h"

AARSCommand::AARSCommand()
{
	CommandName = TEXT("ars");
	Aliases.Add(TEXT("autorailsupports"));
	bOnlyUsableByPlayer = true;
	MinNumberOfArguments = 0;
	Usage = FText::FromString(TEXT(
		"/ars - open the configuration menu (default keybind U)\n"
		"/ars apply - generate supports under the rail you are aiming at (in-world ARS Adjustments: K to enter with the dismantle gun, then LMB)"));
}

EExecutionStatus AARSCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label)
{
	AFGPlayerController* PlayerController = Sender->GetPlayer();
	if (!PlayerController)
	{
		return EExecutionStatus::UNCOMPLETED;
	}
	UARSRemoteCallObject* RCO = PlayerController->GetRemoteCallObjectOfClass<UARSRemoteCallObject>();

	if (Arguments.Num() == 0)
	{
		if (RCO)
		{
			RCO->Client_OpenUI();
			return EExecutionStatus::COMPLETED;
		}
		return EExecutionStatus::UNCOMPLETED;
	}

	if (Arguments[0].Equals(TEXT("apply"), ESearchCase::IgnoreCase))
	{
		AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld());
		AFGCharacterPlayer* Character = Cast<AFGCharacterPlayer>(PlayerController->GetPawn());
		if (!Subsystem || !Character)
		{
			Sender->SendChatMessage(TEXT("Apply requires a player character."), FLinearColor::Red);
			return EExecutionStatus::UNCOMPLETED;
		}
		AFGBuildableRailroadTrack* Track = AARSClientManager::TraceAimedRail(PlayerController);
		if (!Track)
		{
			Sender->SendChatMessage(TEXT("Aim at a railway track and run /ars apply again."), FLinearColor::Red);
			return EExecutionStatus::UNCOMPLETED;
		}
		Subsystem->ApplyToTrack(Track, Character);
		Sender->SendChatMessage(FString::Printf(TEXT("Generating supports under the aimed rail segment (%.1f m)."), Track->GetLength() / 100.0f));
		return EExecutionStatus::COMPLETED;
	}

	PrintCommandUsage(Sender);
	return EExecutionStatus::BAD_ARGUMENTS;
}
