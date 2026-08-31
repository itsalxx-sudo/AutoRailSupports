#include "ARSUnderRailPanelWidget.h"

#include "ARSBlueprintPickerWidget.h"
#include "ARSCheckRowWidget.h"
#include "ARSSliderWidget.h"
#include "ARSTripleSliderWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UARSUnderRailPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BlueprintPicker) { BlueprintPicker->OnBlueprintPicked.AddDynamic(this, &UARSUnderRailPanelWidget::HandleBlueprintPicked); }
	if (TilingLengthSlider) { TilingLengthSlider->OnValueCommitted.AddDynamic(this, &UARSUnderRailPanelWidget::HandleTilingLengthCommitted); }
	if (RotationGroup) { RotationGroup->OnAxisCommitted.AddDynamic(this, &UARSUnderRailPanelWidget::HandleRotationCommitted); }
	if (OffsetGroup) { OffsetGroup->OnAxisCommitted.AddDynamic(this, &UARSUnderRailPanelWidget::HandleOffsetCommitted); }
	if (CoverGapCheck) { CoverGapCheck->OnCheckChanged.AddDynamic(this, &UARSUnderRailPanelWidget::HandleCoverGapChanged); }
}

void UARSUnderRailPanelWidget::SetupPanel(bool bInPartnerSide, float TinyStep, float SmallStep, float LargeStep,
	float RotationTiny, float RotationSmall, float RotationLarge)
{
	bPartnerSide = bInPartnerSide;

	if (TilingLengthSlider)
	{
		TilingLengthSlider->Configure(0.5f, 48.0f, 1.0f, 2, /*fine*/ true, TinyStep, SmallStep, LargeStep);
		TilingLengthSlider->SetUnit(NSLOCTEXT("AutoRailSupports", "UnitMeters", "m"));
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

void UARSUnderRailPanelWidget::SetBlueprintOptions(const TArray<FString>& Options)
{
	if (BlueprintPicker)
	{
		BlueprintPicker->SetOptions(Options);
		BlueprintPicker->SetSelectedBlueprint(mSide.BlueprintName);
	}
}

void UARSUnderRailPanelWidget::PushSide(const FARSUnderRailSide& Side)
{
	mSide = Side;
	TGuardValue<bool> Guard(bSuppressEvents, true);
	if (BlueprintPicker) { BlueprintPicker->SetSelectedBlueprint(mSide.BlueprintName); }
	if (TilingLengthSlider) { TilingLengthSlider->SetValue(mSide.Length / 100.0f); }
	if (RotationGroup) { RotationGroup->SetValues(mSide.RotationOffset.Yaw, mSide.RotationOffset.Pitch, mSide.RotationOffset.Roll); }
	if (OffsetGroup) { OffsetGroup->SetValues(mSide.ForwardOffset / 100.0f, mSide.SideOffset / 100.0f, mSide.ZOffset / 100.0f); }
	if (CoverGapCheck) { CoverGapCheck->SetChecked(mSide.bCoverEndGap); }
}

void UARSUnderRailPanelWidget::SetHeader(const FText& Title)
{
	if (HeaderText) { HeaderText->SetText(Title); }
}

void UARSUnderRailPanelWidget::SetBlueprintPickerDisabled(bool bDisabled)
{
	if (BlueprintPicker)
	{
		BlueprintPicker->SetDisabled(bDisabled);
	}
}

void UARSUnderRailPanelWidget::SetDimensions(const FText& Dimensions)
{
	if (BlueprintPicker)
	{
		BlueprintPicker->SetDimensions(Dimensions);
	}
}

void UARSUnderRailPanelWidget::SetLengthWarning(bool bWarn, const FText& WarningText)
{
	if (TilingLengthSlider)
	{
		TilingLengthSlider->SetWarning(bWarn, WarningText);
	}
}

void UARSUnderRailPanelWidget::HandleBlueprintPicked(const FString& BlueprintName)
{
	if (bSuppressEvents) { return; }
	mSide.BlueprintName = BlueprintName;
	OnSideBlueprintPicked.Broadcast(bPartnerSide, BlueprintName);
}

void UARSUnderRailPanelWidget::HandleTilingLengthCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mSide.Length = Value * 100.0f;
	OnSideEdited.Broadcast(bPartnerSide);
}

void UARSUnderRailPanelWidget::HandleRotationCommitted(int32 AxisIndex, float Value)
{
	if (bSuppressEvents) { return; }
	switch (AxisIndex)
	{
	case 1:  mSide.RotationOffset.Pitch = Value; break;
	case 2:  mSide.RotationOffset.Roll = Value; break;
	default: mSide.RotationOffset.Yaw = Value; break;
	}
	OnSideEdited.Broadcast(bPartnerSide);
}

void UARSUnderRailPanelWidget::HandleOffsetCommitted(int32 AxisIndex, float Value)
{
	if (bSuppressEvents) { return; }
	switch (AxisIndex)
	{
	case 1:  mSide.SideOffset = Value * 100.0f; break;
	case 2:  mSide.ZOffset = Value * 100.0f; break;
	default: mSide.ForwardOffset = Value * 100.0f; break;
	}
	OnSideEdited.Broadcast(bPartnerSide);
}

void UARSUnderRailPanelWidget::HandleCoverGapChanged(bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mSide.bCoverEndGap = bIsChecked;
	OnSideEdited.Broadcast(bPartnerSide);
}
