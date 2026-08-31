#include "ARSSliderWidget.h"

#include "ARSClientManager.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Widget.h"

namespace
{
	constexpr float TrackGrabSlack = 5.0f;

	bool ContainsPoint(const UWidget* Widget, const FVector2D& ScreenPosition, float SlackY)
	{
		if (!Widget)
		{
			return false;
		}
		const FGeometry& Geometry = Widget->GetCachedGeometry();
		const FVector2D Size = Geometry.GetLocalSize();
		if (Size.X <= 0.0f || Size.Y <= 0.0f)
		{
			return false;
		}
		const FVector2D Local = Geometry.AbsoluteToLocal(ScreenPosition);
		return Local.X >= 0.0f && Local.X <= Size.X
			&& Local.Y >= -SlackY && Local.Y <= Size.Y + SlackY;
	}
}

void UARSSliderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ValueText)
	{
		ValueText->OnTextCommitted.AddDynamic(this, &UARSSliderWidget::OnTextCommitted);
	}
	mValue = SnapAndClamp(mValue, FinestStep());
	SetWarning(false, FText::GetEmpty());
	UpdateVisuals();
}

void UARSSliderWidget::SetWarning(bool bWarn, const FText& WarningText)
{
	if (!WarningIcon)
	{
		return;
	}
	WarningIcon->SetVisibility(bWarn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bWarn)
	{
		WarningIcon->SetToolTipText(WarningText);
	}
}

void UARSSliderWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
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
	mValue = SnapAndClamp(mValue, FinestStep());
	UpdateVisuals();
}

void UARSSliderWidget::SetGears(float InTinyStep, float InSmallStep, float InLargeStep)
{
	TinyStep = InTinyStep;
	SmallStep = InSmallStep;
	LargeStep = InLargeStep;
}

void UARSSliderWidget::SetBipolar(bool bInBipolar)
{
	bBipolar = bInBipolar;
	UpdateVisuals();
}

void UARSSliderWidget::SetUnit(const FText& InUnit)
{
	Unit = InUnit;
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

float UARSSliderWidget::Fraction(float Value) const
{
	const float Range = MaxValue - MinValue;
	return Range > 0.0f ? FMath::Clamp((Value - MinValue) / Range, 0.0f, 1.0f) : 0.0f;
}

float UARSSliderWidget::ZeroFraction() const
{
	return Fraction(0.0f);
}

void UARSSliderWidget::PositionFill()
{
	const float ValueFraction = Fraction(mValue);
	float Low = 0.0f;
	float High = ValueFraction;
	if (bBipolar)
	{
		const float Zero = ZeroFraction();
		Low = FMath::Min(Zero, ValueFraction);
		High = FMath::Max(Zero, ValueFraction);
	}

	auto DriveHorizontal = [](UWidget* Widget, float MinX, float MaxX, bool bStretched)
	{
		UCanvasPanelSlot* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (!Slot)
		{
			return;
		}
		FAnchors Anchors = Slot->GetAnchors();
		Anchors.Minimum.X = MinX;
		Anchors.Maximum.X = MaxX;
		Slot->SetAnchors(Anchors);
		if (bStretched)
		{
			FMargin Offsets = Slot->GetOffsets();
			Offsets.Left = 0.0f;
			Offsets.Right = 0.0f;
			Slot->SetOffsets(Offsets);
		}
	};

	DriveHorizontal(FillElement, Low, High, /*bStretched*/ true);
	if (FillElement)
	{
		FillElement->SetVisibility(High - Low > KINDA_SMALL_NUMBER
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	DriveHorizontal(ThumbElement, ValueFraction, ValueFraction, /*bStretched*/ false);

	const float Zero = ZeroFraction();
	DriveHorizontal(CenterTick, Zero, Zero, /*bStretched*/ false);
	if (CenterTick)
	{
		CenterTick->SetVisibility(bBipolar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

FText UARSSliderWidget::FormatValue(float Value) const
{
	FString Text = FString::Printf(TEXT("%.*f"), FMath::Max(FractionalDigits, 0), Value);
	if (FractionalDigits > 0 && Text.Contains(TEXT(".")))
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
	if (bBipolar && Value > 0.0f)
	{
		Text = TEXT("+") + Text;
	}
	if (!Unit.IsEmpty())
	{
		const FString UnitString = Unit.ToString();
		if (FChar::IsAlpha(UnitString[0]))
		{
			Text += TEXT(" ");
		}
		Text += UnitString;
	}
	return FText::FromString(Text);
}

bool UARSSliderWidget::ParseValue(const FString& Text, float& OutValue)
{
	FString Number;
	bool bAnyDigit = false;
	for (const TCHAR Char : Text)
	{
		if (FChar::IsDigit(Char))
		{
			Number.AppendChar(Char);
			bAnyDigit = true;
			continue;
		}
		if ((Char == TEXT('-') || Char == TEXT('+')) && Number.IsEmpty())
		{
			Number.AppendChar(Char);
			continue;
		}
		if ((Char == TEXT('.') || Char == TEXT(',')) && !Number.Contains(TEXT(".")))
		{
			Number.AppendChar(TEXT('.'));
			continue;
		}
		break;
	}
	if (!bAnyDigit)
	{
		return false;
	}
	OutValue = FCString::Atof(*Number);
	return true;
}

void UARSSliderWidget::UpdateVisuals(bool bForceReadout)
{
	PositionFill();

	if (ValueText && (bForceReadout || !ValueText->HasKeyboardFocus()))
	{
		ValueText->SetText(FormatValue(mValue));
	}
}

bool UARSSliderWidget::IsInGrabArea(const FVector2D& ScreenPosition) const
{
	return ContainsPoint(TrackElement, ScreenPosition, TrackGrabSlack)
		|| ContainsPoint(ThumbElement, ScreenPosition, 0.0f);
}

bool UARSSliderWidget::IsOnThumb(const FVector2D& ScreenPosition) const
{
	return ContainsPoint(ThumbElement, ScreenPosition, 0.0f);
}

bool UARSSliderWidget::TrackFractionAt(const FVector2D& ScreenPosition, float& OutFraction) const
{
	if (!TrackElement)
	{
		return false;
	}
	const FGeometry& Geometry = TrackElement->GetCachedGeometry();
	const float Width = Geometry.GetLocalSize().X;
	if (Width <= 0.0f)
	{
		return false;
	}
	OutFraction = FMath::Clamp(Geometry.AbsoluteToLocal(ScreenPosition).X / Width, 0.0f, 1.0f);
	return true;
}

void UARSSliderWidget::SetGrabCursor(bool bGrab)
{
	if (bGrab == mGrabCursor)
	{
		return;
	}
	mGrabCursor = bGrab;
	if (bGrab)
	{
		SetCursor(EMouseCursor::ResizeLeftRight);
	}
	else
	{
		ResetCursor();
	}
}

FReply UARSSliderWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	if (TrackElement && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsInGrabArea(ScreenPosition))
	{
		float ClickFraction = 0.0f;
		mDragValue = mValue;
		if (!IsOnThumb(ScreenPosition) && TrackFractionAt(ScreenPosition, ClickFraction))
		{
			const float Clicked = FMath::Clamp(MinValue + ClickFraction * (MaxValue - MinValue), MinValue, MaxValue);
			mValue = SnapAndClamp(Clicked,
				SnapStep(InMouseEvent.IsAltDown(), InMouseEvent.IsControlDown(), InMouseEvent.IsShiftDown()));
			mDragValue = Clicked;
			UpdateVisuals();
		}
		bDragging = true;
		mDragStartValue = mValue;
		mCursorIdealX = static_cast<float>(ScreenPosition.X);
		mCursorParkedX = mCursorIdealX;
		SetGrabCursor(true);
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UARSSliderWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDragging)
	{
		const FVector2D CursorPosition = InMouseEvent.GetScreenSpacePosition();
		SetGrabCursor(IsInGrabArea(CursorPosition));
		const bool bOverValue = ContainsPoint(ValueText, CursorPosition, 0.0f);
		if (bOverValue && !bValueBoxHovered)
		{
			AARSClientManager::PlayMenuSound(this, EARSMenuSound::Hover);
		}
		bValueBoxHovered = bOverValue;
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}
	if (!TrackElement)
	{
		return FReply::Handled();
	}

	const float TrackWidth = TrackElement->GetCachedGeometry().GetLocalSize().X;
	if (TrackWidth <= 0.0f)
	{
		return FReply::Handled();
	}

	const bool bAlt = InMouseEvent.IsAltDown();
	const bool bCtrl = InMouseEvent.IsControlDown();
	const bool bShift = InMouseEvent.IsShiftDown();
	const float Gear = GearStep(bAlt, bCtrl, bShift);
	const float NaturalPerPixel = (MaxValue - MinValue) / TrackWidth;
	float ValuePerPixel = NaturalPerPixel;
	if (Gear > 0.0f)
	{
		ValuePerPixel = FMath::Min(ValuePerPixel, Gear);
	}

	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	const float HandDelta = static_cast<float>(ScreenPosition.X) - mCursorParkedX;
	const float Before = mDragValue;
	mDragValue = FMath::Clamp(mDragValue + HandDelta * ValuePerPixel, MinValue, MaxValue);
	const float Snap = SnapStep(bAlt, bCtrl, bShift);
	const float Snapped = (Snap > 0.0f)
		? mDragStartValue + FMath::RoundToFloat((mDragValue - mDragStartValue) / Snap) * Snap
		: mDragValue;
	const float Previous = mValue;
	mValue = FMath::Clamp(Snapped, MinValue, MaxValue);
	if (!FMath::IsNearlyEqual(Previous, mValue))
	{
		AARSClientManager::PlayMenuSound(this, EARSMenuSound::SliderTick);
	}
	UpdateVisuals();

	mCursorIdealX += NaturalPerPixel > 0.0f ? (mDragValue - Before) / NaturalPerPixel : 0.0f;
	const int32 Parked = FMath::RoundToInt(mCursorIdealX);
	mCursorParkedX = static_cast<float>(Parked);
	if (Parked == FMath::RoundToInt(static_cast<float>(ScreenPosition.X)))
	{
		return FReply::Handled();
	}
	return FReply::Handled().SetMousePos(FIntPoint(Parked, FMath::RoundToInt(static_cast<float>(ScreenPosition.Y))));
}

FReply UARSSliderWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		UpdateVisuals();
		OnValueCommitted.Broadcast(mValue);
		SetGrabCursor(IsInGrabArea(InMouseEvent.GetScreenSpacePosition()));
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UARSSliderWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bValueBoxHovered = false;
	if (!bDragging)
	{
		SetGrabCursor(false);
	}
}

void UARSSliderWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
	if (bDragging)
	{
		bDragging = false;
		SetGrabCursor(false);
		UpdateVisuals();
		OnValueCommitted.Broadcast(mValue);
	}
}

void UARSSliderWidget::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnCleared)
	{
		UpdateVisuals(/*bForceReadout*/ true);
		return;
	}
	float Parsed = 0.0f;
	if (ParseValue(Text.ToString().TrimStartAndEnd(), Parsed))
	{
		mValue = SnapAndClamp(Parsed, FinestStep());
		OnValueCommitted.Broadcast(mValue);
	}
	UpdateVisuals(/*bForceReadout*/ true);
}
