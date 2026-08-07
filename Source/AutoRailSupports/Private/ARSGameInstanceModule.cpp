#include "ARSGameInstanceModule.h"
#include "ARSRemoteCallObject.h"
#include "Configuration/ModConfiguration.h"

UARSGameInstanceModule::UARSGameInstanceModule()
{
	bRootModule = true;
	RemoteCallObjects.Add(UARSRemoteCallObject::StaticClass());
}

void UARSGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	if (Phase == ELifecyclePhase::INITIALIZATION)
	{
		if (UClass* ConfigClass = LoadClass<UModConfiguration>(nullptr, TEXT("/AutoRailSupports/Config/BP_ARSModConfig.BP_ARSModConfig_C")))
		{
			ModConfigurations.AddUnique(ConfigClass);
		}
	}

	Super::DispatchLifecycleEvent(Phase);
}
