#include "ARSGameInstanceModule.h"
#include "ARSRemoteCallObject.h"
#include "Configuration/ModConfiguration.h"
#include "SessionSettings/SessionSetting.h"

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
		if (USMLSessionSetting* MaxPillarSlots = LoadObject<USMLSessionSetting>(nullptr,
			TEXT("/AutoRailSupports/Config/ARS_SessionSetting_MaxPillarSlots.ARS_SessionSetting_MaxPillarSlots")))
		{
			SessionSettings.AddUnique(MaxPillarSlots);
		}
	}

	Super::DispatchLifecycleEvent(Phase);
}
