#pragma once
#include "CoreMinimal.h"
#include "Command/ChatCommandInstance.h"
#include "ARSCommand.generated.h"

class UCommandSender;

UCLASS()
class AUTORAILSUPPORTS_API AARSCommand : public AChatCommandInstance
{
	GENERATED_BODY()
public:
	AARSCommand();

	virtual EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
