#include "ARSTripleSliderWidget.h"

#include "ARSSliderWidget.h"

FText UARSTripleSliderWidget::DegreeUnit()
{
	return FText::FromString(FString::Chr(static_cast<TCHAR>(0x00B0)));
}

void UARSTripleSliderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Slider0) { Slider0->OnValueCommitted.AddDynamic(this, &UARSTripleSliderWidget::OnAxis0Committed); }
	if (Slider1) { Slider1->OnValueCommitted.AddDynamic(this, &UARSTripleSliderWidget::OnAxis1Committed); }
	if (Slider2) { Slider2->OnValueCommitted.AddDynamic(this, &UARSTripleSliderWidget::OnAxis2Committed); }
	ApplyAxisNames();
}

void UARSTripleSliderWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyAxisNames();
}

UARSSliderWidget* UARSTripleSliderWidget::AxisSlider(int32 Index) const
{
	switch (Index)
	{
	case 0:  return Slider0;
	case 1:  return Slider1;
	case 2:  return Slider2;
	default: return nullptr;
	}
}

void UARSTripleSliderWidget::ConfigureAxes(float InMin, float InMax, float InStep, int32 InFractionalDigits,
	bool bInAllowFineStep, bool bInBipolar, const FText& InUnit, float InTinyStep, float InSmallStep, float InLargeStep)
{
	for (int32 Axis = 0; Axis < 3; Axis++)
	{
		if (UARSSliderWidget* Slider = AxisSlider(Axis))
		{
			Slider->Configure(InMin, InMax, InStep, InFractionalDigits, bInAllowFineStep, InTinyStep, InSmallStep, InLargeStep);
			Slider->SetBipolar(bInBipolar);
			Slider->SetUnit(InUnit);
		}
	}
}

void UARSTripleSliderWidget::ApplyAxisNames()
{
	const FText Names[3] = { Axis0Name, Axis1Name, Axis2Name };
	for (int32 Axis = 0; Axis < 3; Axis++)
	{
		if (UARSSliderWidget* Slider = AxisSlider(Axis))
		{
			Slider->SetSettingText(Names[Axis], SettingDescription);
		}
	}
}

void UARSTripleSliderWidget::SetValues(float Value0, float Value1, float Value2)
{
	const float Values[3] = { Value0, Value1, Value2 };
	for (int32 Axis = 0; Axis < 3; Axis++)
	{
		if (UARSSliderWidget* Slider = AxisSlider(Axis))
		{
			Slider->SetValue(Values[Axis]);
		}
	}
}

void UARSTripleSliderWidget::OnAxis0Committed(float Value) { OnAxisCommitted.Broadcast(0, Value); }
void UARSTripleSliderWidget::OnAxis1Committed(float Value) { OnAxisCommitted.Broadcast(1, Value); }
void UARSTripleSliderWidget::OnAxis2Committed(float Value) { OnAxisCommitted.Broadcast(2, Value); }
