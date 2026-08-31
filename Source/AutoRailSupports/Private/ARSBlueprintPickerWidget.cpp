#include "ARSBlueprintPickerWidget.h"

#include "ARSClientManager.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"

const FString& UARSBlueprintPickerWidget::NoneOption()
{
	static const FString None = TEXT("None");
	return None;
}

void UARSBlueprintPickerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BlueprintCombo)
	{
		BlueprintCombo->OnSelectionChanged.AddDynamic(this, &UARSBlueprintPickerWidget::HandleSelectionChanged);
		AARSClientManager::BindComboBoxSounds(this, BlueprintCombo);
	}
	SetDisabled(false);
}

void UARSBlueprintPickerWidget::SetDisabled(bool bDisabled)
{
	if (BlueprintCombo)
	{
		BlueprintCombo->SetIsEnabled(!bDisabled);
	}
	SetRenderOpacity(bDisabled ? 0.5f : 1.0f);
	SetToolTipText(bDisabled ? DisabledTooltip : FText::GetEmpty());
}

void UARSBlueprintPickerWidget::SetOptions(const TArray<FString>& Options)
{
	if (!BlueprintCombo)
	{
		return;
	}
	const FString Previous = BlueprintCombo->GetSelectedOption();
	BlueprintCombo->ClearOptions();
	for (const FString& Option : Options)
	{
		BlueprintCombo->AddOption(Option);
	}
	if (!Previous.IsEmpty())
	{
		SetSelectedBlueprint(Previous == NoneOption() ? FString() : Previous);
	}
}

void UARSBlueprintPickerWidget::SetSelectedBlueprint(const FString& BlueprintName)
{
	if (!BlueprintCombo)
	{
		return;
	}
	const FString Wanted = BlueprintName.IsEmpty() ? NoneOption() : BlueprintName;
	if (BlueprintCombo->FindOptionIndex(Wanted) == INDEX_NONE)
	{
		BlueprintCombo->AddOption(Wanted); // blueprint is missing from this save, but keep the name visible
	}
	BlueprintCombo->SetSelectedOption(Wanted);
}

FString UARSBlueprintPickerWidget::GetSelectedBlueprint() const
{
	if (!BlueprintCombo)
	{
		return FString();
	}
	const FString Selected = BlueprintCombo->GetSelectedOption();
	return Selected == NoneOption() ? FString() : Selected;
}

void UARSBlueprintPickerWidget::SetDimensions(const FText& Dimensions)
{
	if (DimensionsText)
	{
		DimensionsText->SetText(Dimensions);
	}
}

void UARSBlueprintPickerWidget::HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	OnBlueprintPicked.Broadcast(SelectedItem == NoneOption() ? FString() : SelectedItem);
}
