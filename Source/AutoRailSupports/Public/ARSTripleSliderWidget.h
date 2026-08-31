#pragma once
#include "CoreMinimal.h"
#include "ARSRowWidget.h"
#include "ARSTripleSliderWidget.generated.h"

class UARSSliderWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARSTripleAxisCommitted, int32, AxisIndex, float, Value);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSTripleSliderWidget : public UARSRowWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;

	static FText DegreeUnit();

	void ConfigureAxes(float InMin, float InMax, float InStep, int32 InFractionalDigits, bool bInAllowFineStep,
		bool bInBipolar, const FText& InUnit, float InTinyStep, float InSmallStep, float InLargeStep);

	void SetValues(float Value0, float Value1, float Value2);

	UPROPERTY(BlueprintAssignable, Category = "ARS Triple Slider")
	FARSTripleAxisCommitted OnAxisCommitted;

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Slider0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Slider1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Slider2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Triple Slider", meta = (ExposeOnSpawn))
	FText Axis0Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Triple Slider", meta = (ExposeOnSpawn))
	FText Axis1Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Triple Slider", meta = (ExposeOnSpawn))
	FText Axis2Name;

private:
	void ApplyAxisNames();

	UFUNCTION() void OnAxis0Committed(float Value);
	UFUNCTION() void OnAxis1Committed(float Value);
	UFUNCTION() void OnAxis2Committed(float Value);

	UARSSliderWidget* AxisSlider(int32 Index) const;
};
