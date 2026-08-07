#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "ARSSliderWidget.generated.h"

class UEditableTextBox;
class UProgressBar;
class USlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSSliderValueCommitted, float, Value);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSSliderWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	void Configure(float InMin, float InMax, float InStep, int32 InFractionalDigits, bool bInAllowFineStep = false,
		float InTinyStep = 0.01f, float InSmallStep = 0.1f, float InLargeStep = 4.0f);
	void SetValue(float NewValue);

	UPROPERTY(BlueprintAssignable, Category = "ARS Slider")
	FARSSliderValueCommitted OnValueCommitted;

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> ValueSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> FillBar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UEditableTextBox> ValueText;

	UPROPERTY(EditAnywhere, Category = "ARS Slider") float MinValue = 0.0f;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") float MaxValue = 100.0f;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") float StepSize = 0.01f;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") int32 FractionalDigits = 2;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") bool bAllowFineStep = false;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") float TinyStep = 0.01f;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") float SmallStep = 0.1f;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") float LargeStep = 4.0f;

private:
	UFUNCTION() void OnSliderValueChanged(float NewValue);
	UFUNCTION() void OnSliderCaptureBegin();
	UFUNCTION() void OnSliderCaptureEnd();
	UFUNCTION() void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	float SnapAndClamp(float RawValue, float Step) const;
	float SnapStep(bool bAltDown, bool bControlDown, bool bShiftDown) const;
	float GearStep(bool bAltDown, bool bControlDown, bool bShiftDown) const;
	float FinestStep() const { return bAllowFineStep ? TinyStep : StepSize; }
	void ApplyRangeToSlider();
	void UpdateVisuals();

	float mValue = 0.0f;
	bool bDragging = false;
	float mDragValue = 0.0f;
	float mDragStartValue = 0.0f;
};
