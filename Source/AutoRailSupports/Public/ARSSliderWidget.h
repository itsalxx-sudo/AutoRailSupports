#pragma once
#include "CoreMinimal.h"
#include "ARSRowWidget.h"
#include "Types/SlateEnums.h"
#include "ARSSliderWidget.generated.h"

class UEditableTextBox;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSSliderValueCommitted, float, Value);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSSliderWidget : public UARSRowWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	void Configure(float InMin, float InMax, float InStep, int32 InFractionalDigits, bool bInAllowFineStep = false,
		float InTinyStep = 0.01f, float InSmallStep = 0.1f, float InLargeStep = 4.0f);

	void SetGears(float InTinyStep, float InSmallStep, float InLargeStep);
	void SetBipolar(bool bInBipolar);
	void SetUnit(const FText& InUnit);

	void SetValue(float NewValue);
	float GetValue() const { return mValue; }

	void SetWarning(bool bWarn, const FText& WarningText);

	UPROPERTY(BlueprintAssignable, Category = "ARS Slider")
	FARSSliderValueCommitted OnValueCommitted;

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> TrackElement;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> FillElement;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ThumbElement;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> CenterTick;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UEditableTextBox> ValueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> WarningIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Slider", meta = (ExposeOnSpawn)) float MinValue = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Slider", meta = (ExposeOnSpawn)) float MaxValue = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Slider", meta = (ExposeOnSpawn)) float StepSize = 0.01f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Slider", meta = (ExposeOnSpawn)) int32 FractionalDigits = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Slider", meta = (ExposeOnSpawn)) bool bAllowFineStep = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Slider", meta = (ExposeOnSpawn)) bool bBipolar = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARS Slider", meta = (ExposeOnSpawn)) FText Unit;

	UPROPERTY(EditAnywhere, Category = "ARS Slider") float TinyStep = 0.01f;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") float SmallStep = 0.1f;
	UPROPERTY(EditAnywhere, Category = "ARS Slider") float LargeStep = 4.0f;

private:
	UFUNCTION() void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	float SnapAndClamp(float RawValue, float Step) const;
	float SnapStep(bool bAltDown, bool bControlDown, bool bShiftDown) const;
	float GearStep(bool bAltDown, bool bControlDown, bool bShiftDown) const;
	float FinestStep() const { return bAllowFineStep ? TinyStep : StepSize; }
	float Fraction(float Value) const;
	float ZeroFraction() const;
	FText FormatValue(float Value) const;
	static bool ParseValue(const FString& Text, float& OutValue);
	void UpdateVisuals(bool bForceReadout = false);
	void PositionFill();

	bool IsInGrabArea(const FVector2D& ScreenPosition) const;
	bool IsOnThumb(const FVector2D& ScreenPosition) const;
	bool TrackFractionAt(const FVector2D& ScreenPosition, float& OutFraction) const;
	void SetGrabCursor(bool bGrab);

	float mValue = 0.0f;
	bool bDragging = false;
	float mDragValue = 0.0f;
	float mDragStartValue = 0.0f;
	float mCursorIdealX = 0.0f;
	float mCursorParkedX = 0.0f;
	bool bValueBoxHovered = false;
	bool mGrabCursor = false;
};
