#pragma once
#include "CoreMinimal.h"
#include "ARSRowWidget.h"
#include "Types/SlateEnums.h"
#include "ARSBlueprintPickerWidget.generated.h"

class UComboBoxString;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSBlueprintPicked, const FString&, BlueprintName);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSBlueprintPickerWidget : public UARSRowWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;

	void SetOptions(const TArray<FString>& Options);
	void SetSelectedBlueprint(const FString& BlueprintName);
	FString GetSelectedBlueprint() const;

	void SetDimensions(const FText& Dimensions);

	void SetDisabled(bool bDisabled);

	UPROPERTY(BlueprintAssignable, Category = "ARS Blueprint Picker")
	FARSBlueprintPicked OnBlueprintPicked;

	static const FString& NoneOption();

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> BlueprintCombo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DimensionsText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Blueprint Picker", meta = (MultiLine = true, ExposeOnSpawn))
	FText DisabledTooltip;

private:
	UFUNCTION() void HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
};
