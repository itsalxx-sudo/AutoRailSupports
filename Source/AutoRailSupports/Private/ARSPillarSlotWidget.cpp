#include "ARSPillarSlotWidget.h"

#include "ARSBlueprintPickerWidget.h"
#include "ARSClientManager.h"
#include "ARSCheckRowWidget.h"
#include "ARSSideModePickerWidget.h"
#include "ARSSliderWidget.h"
#include "ARSTripleSliderWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UARSPillarSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BlueprintPicker) { BlueprintPicker->OnBlueprintPicked.AddDynamic(this, &UARSPillarSlotWidget::HandleBlueprintPicked); }
	if (PlaceEverySlider) { PlaceEverySlider->OnValueCommitted.AddDynamic(this, &UARSPillarSlotWidget::HandlePlaceEveryCommitted); }
	if (StartOffsetSlider) { StartOffsetSlider->OnValueCommitted.AddDynamic(this, &UARSPillarSlotWidget::HandleStartOffsetCommitted); }
	if (RotationGroup) { RotationGroup->OnAxisCommitted.AddDynamic(this, &UARSPillarSlotWidget::HandleRotationCommitted); }
	if (OffsetGroup) { OffsetGroup->OnAxisCommitted.AddDynamic(this, &UARSPillarSlotWidget::HandleOffsetCommitted); }
	if (SideModeRow) { SideModeRow->OnSideModePicked.AddDynamic(this, &UARSPillarSlotWidget::HandleSideModeChanged); }
	if (InclineCheck) { InclineCheck->OnCheckChanged.AddDynamic(this, &UARSPillarSlotWidget::HandleInclineChanged); }
	if (AutoRollCheck) { AutoRollCheck->OnCheckChanged.AddDynamic(this, &UARSPillarSlotWidget::HandleAutoRollChanged); }
	if (SkipCheck) { SkipCheck->OnCheckChanged.AddDynamic(this, &UARSPillarSlotWidget::HandleSkipChanged); }
	if (MoveUpButton) { MoveUpButton->OnClicked.AddDynamic(this, &UARSPillarSlotWidget::HandleMoveUpClicked); }
	if (MoveDownButton) { MoveDownButton->OnClicked.AddDynamic(this, &UARSPillarSlotWidget::HandleMoveDownClicked); }
	if (RemoveButton) { RemoveButton->OnClicked.AddDynamic(this, &UARSPillarSlotWidget::HandleRemoveClicked); }
	for (UButton* Button : { MoveUpButton.Get(), MoveDownButton.Get(), RemoveButton.Get() })
	{
		AARSClientManager::BindButtonSounds(this, Button);
	}
}

bool UARSPillarSlotWidget::IsOverHeader(const FVector2D& ScreenPosition) const
{
	if (!HeaderRow)
	{
		return false;
	}
	const FGeometry& Header = HeaderRow->GetCachedGeometry();
	return Header.AbsoluteToLocal(ScreenPosition).Y <= Header.GetLocalSize().Y;
}

FReply UARSPillarSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& (!mSlot.bExpanded || IsOverHeader(InMouseEvent.GetScreenSpacePosition())))
	{
		AARSClientManager::PlayMenuSound(this, EARSMenuSound::Click);
		ToggleExpanded();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UARSPillarSlotWidget::SetupCard(int32 InSlotIndex, float TinyStep, float SmallStep, float LargeStep,
	float RotationTiny, float RotationSmall, float RotationLarge)
{
	mSlotIndex = InSlotIndex;

	if (PlaceEverySlider)
	{
		PlaceEverySlider->Configure(1.0f, 100.0f, 1.0f, 0);
	}
	if (StartOffsetSlider)
	{
		StartOffsetSlider->Configure(0.0f, 100.0f, 1.0f, 0);
	}
	if (RotationGroup)
	{
		RotationGroup->ConfigureAxes(-180.0f, 180.0f, 15.0f, 1, /*fine*/ true, /*bipolar*/ true,
			UARSTripleSliderWidget::DegreeUnit(), RotationTiny, RotationSmall, RotationLarge);
	}
	if (OffsetGroup)
	{
		OffsetGroup->ConfigureAxes(-100.0f, 100.0f, 1.0f, 2, /*fine*/ true, /*bipolar*/ true,
			NSLOCTEXT("AutoRailSupports", "UnitMeters", "m"), TinyStep, SmallStep, LargeStep);
	}
}

void UARSPillarSlotWidget::SetBlueprintOptions(const TArray<FString>& Options)
{
	if (BlueprintPicker)
	{
		BlueprintPicker->SetOptions(Options);
		BlueprintPicker->SetSelectedBlueprint(mSlot.BlueprintName);
	}
}

void UARSPillarSlotWidget::PushSlot(const FARSPillarSlotData& InSlot)
{
	mSlot = InSlot;
	TGuardValue<bool> Guard(bSuppressEvents, true);
	if (BlueprintPicker) { BlueprintPicker->SetSelectedBlueprint(mSlot.BlueprintName); }
	if (PlaceEverySlider) { PlaceEverySlider->SetValue(mSlot.EveryN); }
	if (StartOffsetSlider) { StartOffsetSlider->SetValue(mSlot.SpacingOffset); }
	const FRotator Rotation = mSlot.GetRotation();
	if (RotationGroup) { RotationGroup->SetValues(Rotation.Yaw, Rotation.Pitch, Rotation.Roll); }
	if (OffsetGroup) { OffsetGroup->SetValues(mSlot.ForwardOffset / 100.0f, mSlot.SideOffset / 100.0f, mSlot.ZOffset / 100.0f); }
	if (SideModeRow) { SideModeRow->SetSideMode(mSlot.SideMode); }
	if (InclineCheck) { InclineCheck->SetChecked(mSlot.bAlignToRail); }
	if (AutoRollCheck) { AutoRollCheck->SetChecked(mSlot.bAutoRoll); }
	if (SkipCheck) { SkipCheck->SetChecked(mSlot.bPreventOverlap); }
	ApplyExpansion();
	ApplyHeaderSummary();
	ApplyAutoRollDependency();
}

void UARSPillarSlotWidget::SetListPosition(int32 InSlotIndex, int32 TotalSlots, bool bCanMoveUp, bool bCanMoveDown)
{
	mSlotIndex = InSlotIndex;
	if (MoveUpButton)
	{
		MoveUpButton->SetVisibility(bCanMoveUp ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (MoveDownButton)
	{
		MoveDownButton->SetVisibility(bCanMoveDown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (PriorityText)
	{
		PriorityText->SetText(FText::Format(PriorityFormat,
			FText::AsNumber(mSlotIndex + 1), FText::AsNumber(TotalSlots)));
	}
}

void UARSPillarSlotWidget::SetDoubleRailsControlsVisible(bool bVisible)
{
	const ESlateVisibility DoubleOnly = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (SideModeRow) { SideModeRow->SetVisibility(DoubleOnly); }
	if (AutoRollCheck) { AutoRollCheck->SetVisibility(DoubleOnly); }
	ApplyAutoRollDependency();
}

void UARSPillarSlotWidget::SetDimensions(const FText& Dimensions)
{
	if (BlueprintPicker)
	{
		BlueprintPicker->SetDimensions(Dimensions);
	}
}

void UARSPillarSlotWidget::ApplyExpansion()
{
	if (BodyContainer)
	{
		BodyContainer->SetVisibility(mSlot.bExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (ExpandedChevron)
	{
		ExpandedChevron->SetVisibility(mSlot.bExpanded ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (CollapsedChevron)
	{
		CollapsedChevron->SetVisibility(mSlot.bExpanded ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UARSPillarSlotWidget::ApplyHeaderSummary()
{
	if (BlueprintNameText)
	{
		BlueprintNameText->SetText(mSlot.BlueprintName.IsEmpty()
			? EmptySlotText
			: FText::FromString(mSlot.BlueprintName));
	}
}

void UARSPillarSlotWidget::ApplyAutoRollDependency()
{
	if (!AutoRollCheck)
	{
		return;
	}
	// rolling only for center lane that spans both rails
	AutoRollCheck->SetDisabled(mSlot.SideMode != EARSSideMode::Center);
}

void UARSPillarSlotWidget::HandleBlueprintPicked(const FString& BlueprintName)
{
	if (bSuppressEvents) { return; }
	mSlot.BlueprintName = BlueprintName;
	ApplyHeaderSummary();
	OnSlotBlueprintPicked.Broadcast(mSlotIndex, BlueprintName);
}

void UARSPillarSlotWidget::HandlePlaceEveryCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mSlot.EveryN = FMath::Clamp(FMath::RoundToInt(Value), 1, 100);
	ApplyHeaderSummary();
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::HandleStartOffsetCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mSlot.SpacingOffset = FMath::Clamp(FMath::RoundToInt(Value), 0, 100);
	ApplyHeaderSummary();
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::HandleRotationCommitted(int32 AxisIndex, float Value)
{
	if (bSuppressEvents) { return; }
	FRotator Rotation = mSlot.GetRotation();
	switch (AxisIndex)
	{
	case 1:  Rotation.Pitch = Value; break;
	case 2:  Rotation.Roll = Value; break;
	default: Rotation.Yaw = Value; break;
	}
	mSlot.SetRotation(Rotation);
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::HandleOffsetCommitted(int32 AxisIndex, float Value)
{
	if (bSuppressEvents) { return; }
	switch (AxisIndex)
	{
	case 1:  mSlot.SideOffset = Value * 100.0f; break;
	case 2:  mSlot.ZOffset = Value * 100.0f; break;
	default: mSlot.ForwardOffset = Value * 100.0f; break;
	}
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::HandleSideModeChanged(EARSSideMode SideMode)
{
	if (bSuppressEvents) { return; }
	mSlot.SideMode = SideMode;
	ApplyHeaderSummary();
	ApplyAutoRollDependency();
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::HandleInclineChanged(bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mSlot.bAlignToRail = bIsChecked;
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::HandleAutoRollChanged(bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mSlot.bAutoRoll = bIsChecked;
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::HandleSkipChanged(bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mSlot.bPreventOverlap = bIsChecked;
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::ToggleExpanded()
{
	mSlot.bExpanded = !mSlot.bExpanded;
	ApplyExpansion();
	OnSlotEdited.Broadcast(mSlotIndex);
}

void UARSPillarSlotWidget::HandleMoveUpClicked()
{
	OnSlotMoveRequested.Broadcast(mSlotIndex, -1);
}

void UARSPillarSlotWidget::HandleMoveDownClicked()
{
	OnSlotMoveRequested.Broadcast(mSlotIndex, 1);
}

void UARSPillarSlotWidget::HandleRemoveClicked()
{
	OnSlotRemoveRequested.Broadcast(mSlotIndex);
}
