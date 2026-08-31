#include "ARSSegmentedSwitchWidget.h"

#include "ARSClientManager.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UARSSegmentedSwitchWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Segment0Button) { Segment0Button->OnClicked.AddDynamic(this, &UARSSegmentedSwitchWidget::OnSegment0Clicked); }
	if (Segment1Button) { Segment1Button->OnClicked.AddDynamic(this, &UARSSegmentedSwitchWidget::OnSegment1Clicked); }
	for (UButton* Button : { Segment0Button.Get(), Segment1Button.Get() })
	{
		AARSClientManager::BindButtonSounds(this, Button);
	}
	ApplySelectionVisuals();
}

void UARSSegmentedSwitchWidget::SetSelectedIndex(int32 Index)
{
	mSelectedIndex = FMath::Clamp(Index, 0, 1);
	ApplySelectionVisuals();
}

void UARSSegmentedSwitchWidget::ApplySelectionVisuals()
{
	UWidget* Backgrounds[2] = { Segment0SelectedBackground, Segment1SelectedBackground };
	UWidget* Checks[2] = { Segment0Check, Segment1Check };
	UTextBlock* Titles[2] = { Segment0Title, Segment1Title };
	UTextBlock* Subtitles[2] = { Segment0Subtitle, Segment1Subtitle };

	for (int32 Index = 0; Index < 2; Index++)
	{
		const bool bSelected = (mSelectedIndex == Index);
		const ESlateVisibility SelectedOnly = bSelected
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (Backgrounds[Index]) { Backgrounds[Index]->SetVisibility(SelectedOnly); }
		if (Checks[Index]) { Checks[Index]->SetVisibility(SelectedOnly); }
		const FSlateColor TextColor(bSelected ? SelectedTextColor : UnselectedTextColor);
		if (Titles[Index]) { Titles[Index]->SetColorAndOpacity(TextColor); }
		if (Subtitles[Index]) { Subtitles[Index]->SetColorAndOpacity(TextColor); }
	}
}

void UARSSegmentedSwitchWidget::OnSegment0Clicked()
{
	SetSelectedIndex(0);
	OnSegmentSelected.Broadcast(0);
}

void UARSSegmentedSwitchWidget::OnSegment1Clicked()
{
	SetSelectedIndex(1);
	OnSegmentSelected.Broadcast(1);
}
