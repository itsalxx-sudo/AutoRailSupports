#include "ARSSideModePickerWidget.h"

#include "ARSClientManager.h"
#include "Components/ComboBoxString.h"

const TArray<FString>& UARSSideModePickerWidget::Options()
{
	static const TArray<FString> SideModeOptions = {
		TEXT("Centered, spans both tracks"),
		TEXT("Both tracks, mirrored"),
		TEXT("Both tracks, same facing"),
		TEXT("Left track only"),
		TEXT("Right track only")
	};
	return SideModeOptions;
}

FString UARSSideModePickerWidget::ToOption(EARSSideMode Mode)
{
	const int32 Index = static_cast<int32>(Mode);
	return Options().IsValidIndex(Index) ? Options()[Index] : Options()[0];
}

EARSSideMode UARSSideModePickerWidget::FromOption(const FString& Option)
{
	const int32 Index = Options().IndexOfByKey(Option);
	return Index == INDEX_NONE ? EARSSideMode::Center : static_cast<EARSSideMode>(Index);
}

void UARSSideModePickerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (SideModeCombo)
	{
		SideModeCombo->ClearOptions();
		for (const FString& Option : Options())
		{
			SideModeCombo->AddOption(Option);
		}
		SideModeCombo->OnSelectionChanged.AddDynamic(this, &UARSSideModePickerWidget::HandleSelectionChanged);
		AARSClientManager::BindComboBoxSounds(this, SideModeCombo);
	}
}

void UARSSideModePickerWidget::SetSideMode(EARSSideMode Mode)
{
	if (SideModeCombo)
	{
		SideModeCombo->SetSelectedOption(ToOption(Mode));
	}
}

EARSSideMode UARSSideModePickerWidget::GetSideMode() const
{
	return SideModeCombo ? FromOption(SideModeCombo->GetSelectedOption()) : EARSSideMode::Center;
}

void UARSSideModePickerWidget::HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	OnSideModePicked.Broadcast(FromOption(SelectedItem));
}
