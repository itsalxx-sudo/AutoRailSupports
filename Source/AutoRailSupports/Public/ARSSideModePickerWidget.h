#pragma once
#include "CoreMinimal.h"
#include "ARSRowWidget.h"
#include "ARSSettingsData.h"
#include "Types/SlateEnums.h"
#include "ARSSideModePickerWidget.generated.h"

class UComboBoxString;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSSideModePicked, EARSSideMode, SideMode);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSSideModePickerWidget : public UARSRowWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;

	void SetSideMode(EARSSideMode Mode);
	EARSSideMode GetSideMode() const;

	UPROPERTY(BlueprintAssignable, Category = "ARS Side Mode Picker")
	FARSSideModePicked OnSideModePicked;

	static const TArray<FString>& Options();
	static FString ToOption(EARSSideMode Mode);
	static EARSSideMode FromOption(const FString& Option);

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> SideModeCombo;

private:
	UFUNCTION() void HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
};
