#include "ARSRootModule.h"
#include "ARSClientManager.h"
#include "ARSSubsystem.h"
#include "ARSCommand.h"

UARSRootModule::UARSRootModule()
{
	bRootModule = true;
	ModSubsystems.Add(AARSSubsystem::StaticClass());
	ModSubsystems.Add(AARSClientManager::StaticClass());
	mChatCommands.Add(AARSCommand::StaticClass());
}
