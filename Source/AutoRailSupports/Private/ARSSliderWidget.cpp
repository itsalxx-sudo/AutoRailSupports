#include "ARSSliderWidget.h"

#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"

void UARSSliderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ValueSlider)
	{
		ValueSlider->OnValueChanged.AddDynamic(this, &UARSSliderWidget::OnSliderValueChanged);
		ValueSlider->OnMouseCaptureBegin.AddDynamic(this, &UARSSliderWidget::OnSliderCaptureBegin);
		ValueSlider->OnMouseCaptureEnd.AddDynamic(this, &UARSSliderWidget::OnSliderCaptureEnd);
		ValueSlider->OnControllerCaptureBegin.AddDynamic(this, &UARSSliderWidget::OnSliderCaptureBegin);
		ValueSlider->OnControllerCaptureEnd.AddDynamic(this, &UARSSliderWidget::OnSliderCaptureEnd);
	}
	if (ValueSlider && FillBar)
	{
		ValueSlider->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetVisibility(ESlateVisibility::Visible);
	}
	if (ValueText)
	{
		ValueText->OnTextCommitted.AddDynamic(this, &UARSSliderWidget::OnTextCommitted);
	}
	ApplyRangeToSlider();
	mValue = SnapAndClamp(mValue, FinestStep());
	UpdateVisuals();
}

void UARSSliderWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyRangeToSlider();
	UpdateVisuals();
}

void UARSSliderWidget::Configure(float InMin, float InMax, float InStep, int32 InFractionalDigits, bool bInAllowFineStep,
	float InTinyStep, float InSmallStep, float InLargeStep)
{
	MinValue = InMin;
	MaxValue = InMax;
	StepSize = InStep;
	FractionalDigits = InFractionalDigits;
	bAllowFineStep = bInAllowFineStep;
	TinyStep = InTinyStep;
	SmallStep = InSmallStep;
	LargeStep = InLargeStep;
	ApplyRangeToSlider();
	mValue = SnapAndClamp(mValue, FinestStep());
	UpdateVisuals();
}

void UARSSliderWidget::SetValue(float NewValue)
{
	mValue = SnapAndClamp(NewValue, FinestStep());
	UpdateVisuals();
}

float UARSSliderWidget::SnapAndClamp(float RawValue, float Step) const
{
	float Value = RawValue;
	if (Step > 0.0f)
	{
		Value = MinValue + FMath::RoundToFloat((Value - MinValue) / Step) * Step;
	}
	return FMath::Clamp(Value, MinValue, MaxValue);
}

float UARSSliderWidget::GearStep(bool bAltDown, bool bControlDown, bool bShiftDown) const
{
	if (bControlDown) { return TinyStep; }
	if (bAltDown) { return SmallStep; }
	if (bShiftDown) { return LargeStep; }
	return StepSize;
}

float UARSSliderWidget::SnapStep(bool bAltDown, bool bControlDown, bool bShiftDown) const
{
	return bAllowFineStep ? GearStep(bAltDown, bControlDown, bShiftDown) : StepSize;
}

void UARSSliderWidget::ApplyRangeToSlider()
{
	if (ValueSlider)
	{
		ValueSlider->SetMinValue(MinValue);
		ValueSlider->SetMaxValue(MaxValue);
		ValueSlider->SetStepSize(StepSize);
	}
}

void UARSSliderWidget::UpdateVisuals()
{
	if (ValueSlider && !bDragging)
	{
		ValueSlider->SetValue(mValue);
	}
	if (FillBar)
	{
		const float Range = MaxValue - MinValue;
		FillBar->SetPercent(Range > 0.0f ? (mValue - MinValue) / Range : 0.0f);
	}
	if (ValueText && !ValueText->HasKeyboardFocus())
	{
		FString Text = FString::Printf(TEXT("%.*f"), FMath::Max(FractionalDigits, 0), mValue);
		if (FractionalDigits > 0)
		{
			while (Text.EndsWith(TEXT("0")))
			{
				Text = Text.LeftChop(1);
			}
			if (Text.EndsWith(TEXT(".")))
			{
				Text = Text.LeftChop(1);
			}
		}
		if (Text == TEXT("-0"))
		{
			Text = TEXT("0");
		}
		ValueText->SetText(FText::FromString(Text));
	}
}

FReply UARSSliderWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (FillBar && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FGeometry& BarGeometry = FillBar->GetCachedGeometry();
		const FVector2D Local = BarGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D BarSize = BarGeometry.GetLocalSize();
		if (BarSize.X > 0.0f
			&& Local.X >= 0.0f && Local.X <= BarSize.X
			&& Local.Y >= 0.0f && Local.Y <= BarSize.Y)
		{
			bDragging = true;
			mDragStartValue = mValue;
			mDragValue = mValue;
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UARSSliderWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && FillBar)
	{
		const float BarWidth = FillBar->GetCachedGeometry().GetLocalSize().X;
		if (BarWidth > 0.0f)
		{
			const bool bAlt = InMouseEvent.IsAltDown();
			const bool bCtrl = InMouseEvent.IsControlDown();
			const bool bShift = InMouseEvent.IsShiftDown();
			const float Gear = GearStep(bAlt, bCtrl, bShift);
			float ValuePerPixel = (MaxValue - MinValue) / BarWidth;
			if (Gear > 0.0f)
			{
				ValuePerPixel = FMath::Min(ValuePerPixel, Gear);
			}
			mDragValue = FMath::Clamp(mDragValue + InMouseEvent.GetCursorDelta().X * ValuePerPixel, MinValue, MaxValue);
			const float Snap = SnapStep(bAlt, bCtrl, bShift);
			const float Snapped = (Snap > 0.0f)
				? mDragStartValue + FMath::RoundToFloat((mDragValue - mDragStartValue) / Snap) * Snap
				: mDragValue;
			mValue = FMath::Clamp(Snapped, MinValue, MaxValue);
			UpdateVisuals();
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UARSSliderWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		UpdateVisuals();
		OnValueCommitted.Broadcast(mValue);
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UARSSliderWidget::OnSliderValueChanged(float NewValue)
{
	mValue = SnapAndClamp(NewValue, FinestStep());
	UpdateVisuals();
	if (!bDragging)
	{
		OnValueCommitted.Broadcast(mValue);
	}
}

void UARSSliderWidget::OnSliderCaptureBegin()
{
	bDragging = true;
}

void UARSSliderWidget::OnSliderCaptureEnd()
{
	bDragging = false;
	UpdateVisuals();
	OnValueCommitted.Broadcast(mValue);
}

void UARSSliderWidget::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnCleared)
	{
		UpdateVisuals();
		return;
	}
	const FString Trimmed = Text.ToString().TrimStartAndEnd();
	if (Trimmed.IsNumeric())
	{
		mValue = SnapAndClamp(FCString::Atof(*Trimmed), FinestStep());
		OnValueCommitted.Broadcast(mValue);
	}
	UpdateVisuals();
}
