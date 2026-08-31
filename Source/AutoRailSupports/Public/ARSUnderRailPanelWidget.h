#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ARSSettingsData.h"
#include "ARSUnderRailPanelWidget.generated.h"

class UARSBlueprintPickerWidget;
class UARSCheckRowWidget;
class UARSSliderWidget;
class UARSTripleSliderWidget;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARSUnderRailEdited, bool, bPartnerSide);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARSUnderRailBlueprintPicked, bool, bPartnerSide, const FString&, BlueprintName);

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSUnderRailPanelWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;

	void SetupPanel(bool bInPartnerSide, float TinyStep, float SmallStep, float LargeStep,
		float RotationTiny, float RotationSmall, float RotationLarge);

	void SetBlueprintOptions(const TArray<FString>& Options);

	void PushSide(const FARSUnderRailSide& Side);
	const FARSUnderRailSide& GetSide() const { return mSide; }

	bool IsPartnerSide() const { return bPartnerSide; }

	void SetHeader(const FText& Title);

	void SetBlueprintPickerDisabled(bool bDisabled);

	void SetDimensions(const FText& Dimensions);
	void SetLengthWarning(bool bWarn, const FText& WarningText);

	UPROPERTY(BlueprintAssignable, Category = "ARS Under-Rail Panel")
	FARSUnderRailEdited OnSideEdited;

	UPROPERTY(BlueprintAssignable, Category = "ARS Under-Rail Panel")
	FARSUnderRailBlueprintPicked OnSideBlueprintPicked;

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSBlueprintPickerWidget> BlueprintPicker;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> TilingLengthSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSTripleSliderWidget> RotationGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSTripleSliderWidget> OffsetGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSCheckRowWidget> CoverGapCheck;

private:
	UFUNCTION() void HandleBlueprintPicked(const FString& BlueprintName);
	UFUNCTION() void HandleTilingLengthCommitted(float Value);
	UFUNCTION() void HandleRotationCommitted(int32 AxisIndex, float Value);
	UFUNCTION() void HandleOffsetCommitted(int32 AxisIndex, float Value);
	UFUNCTION() void HandleCoverGapChanged(bool bIsChecked);

	FARSUnderRailSide mSide;
	bool bPartnerSide = false;
	bool bSuppressEvents = false;
};
