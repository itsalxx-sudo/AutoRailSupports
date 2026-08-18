#pragma once
#include "CoreMinimal.h"
#include "UI/FGInteractWidget.h"
#include "ARSSettingsData.h"
#include "Types/SlateEnums.h"
#include "ARSConfigWidget.generated.h"

class AARSClientManager;
class AARSSubsystem;
class UARSSliderWidget;
class UButton;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UFGWindow;
class UScrollBox;
class UTextBlock;
class UWidget;

UCLASS(Abstract)
class AUTORAILSUPPORTS_API UARSConfigWidget : public UFGInteractWidget
{
	GENERATED_BODY()
public:
	UARSConfigWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void OnEscapePressed_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "Auto Rail Supports")
	void RequestClose();

	void SetHoverInfo(const FText& Name, const FText& Description);
	void ClearHoverInfo();

	void HandleBlueprintSize(const FString& BlueprintName, const FVector& SizeMeters);

protected:
	//BOUND CONTROLS
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UFGWindow> WindowFrame;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MasterButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MasterButtonText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CloseButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UScrollBox> ContentScrollBox;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HoverTitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HoverDescriptionText;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> UnderBlueprintCombo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> UnderBlueprintLengthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> UnderBlueprintWidthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> UnderBlueprintHeightText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> UnderLengthSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> UnderCoverGapCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> UnderYawSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> UnderPitchSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> UnderRollSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> UnderVerticalSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> UnderSidewaysSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> UnderForwardSpin;

	//PILLAR SLOTS 1-4
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Pillar1Combo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Pillar2Combo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Pillar3Combo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Pillar4Combo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar1EverySpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar2EverySpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar3EverySpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar4EverySpin;
	// under-rail segments to skip before this slot's first pillar
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar1SpacingSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar2SpacingSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar3SpacingSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar4SpacingSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar1InclineCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar2InclineCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar3InclineCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar4InclineCheck;

	// on = this slot is removed by a higher-priority slot on the same segment
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar1PriorityCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar2PriorityCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar3PriorityCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar4PriorityCheck;

	// per-slot dimensions
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar1BlueprintLengthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar1BlueprintWidthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar1BlueprintHeightText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar2BlueprintLengthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar2BlueprintWidthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar2BlueprintHeightText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar3BlueprintLengthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar3BlueprintWidthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar3BlueprintHeightText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar4BlueprintLengthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar4BlueprintWidthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Pillar4BlueprintHeightText;

	// per-slot rotation and position offset
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar1YawSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar1PitchSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar1RollSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar1VerticalSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar1SidewaysSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar1ForwardSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar2YawSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar2PitchSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar2RollSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar2VerticalSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar2SidewaysSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar2ForwardSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar3YawSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar3PitchSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar3RollSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar3VerticalSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar3SidewaysSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar3ForwardSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar4YawSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar4PitchSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar4RollSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar4VerticalSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar4SidewaysSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> Pillar4ForwardSpin;

	// checked = the next run's blueprints group into one blueprint for dismantling
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> CombineBlueprintCheck;

	//DOUBLE RAILS
	// checked = supports are placed for a second track alongside the one you build
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> DoubleRailsCheck;
	// distance between the two tracks
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> TrackSeparationSpin;
	// placement of the blueprint
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> UnderSideModeCombo;
	// checked = tilt when the two rails sit at different heights
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> UnderAutoRollCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Pillar1SideModeCombo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Pillar2SideModeCombo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Pillar3SideModeCombo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Pillar4SideModeCombo;
	// checked = the second track's under-rail gets its own settings
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> SplitUnderRailSidesCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> PartnerLengthSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> PartnerCoverGapCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> PartnerYawSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> PartnerPitchSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> PartnerRollSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> PartnerVerticalSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> PartnerSidewaysSpin;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> PartnerForwardSpin;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar1AutoRollCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar2AutoRollCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar3AutoRollCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Pillar4AutoRollCheck;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> PresetCombo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UEditableTextBox> PresetNameBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SavePresetButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> LoadPresetButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DeletePresetButton;

	// shown while the tiling length is low enough to stamp a huge number of parts per rail
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> UnderLengthWarning;

private:
	AARSClientManager* GetClientManager() const;
	void UpdateBlueprintDimsText();
	void UpdateDimsTextFor(const FString& BlueprintName, UTextBlock* LengthText, UTextBlock* WidthText, UTextBlock* HeightText);
	// skipped if the size is cached or already in flight
	void RequestBlueprintSize(const FString& BlueprintName);
	void UpdateLengthWarning();
	void RefreshBlueprintOptions();
	void RefreshPresetOptions();
	void PushWorkingToControls();
	void SendSettings();
	void SetMasterButtonState(bool bEnabled);
	void CloseSelf();
	void ApplyMenuInputMode();

	UFUNCTION() void OnMasterToggleClicked();
	UFUNCTION() void OnCloseClicked();

	UFUNCTION() void OnUnderBlueprintChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnUnderLengthCommitted(float Value);
	UFUNCTION() void OnUnderYawCommitted(float Value);
	UFUNCTION() void OnUnderPitchCommitted(float Value);
	UFUNCTION() void OnUnderRollCommitted(float Value);
	UFUNCTION() void OnUnderVOffsetCommitted(float Value);
	UFUNCTION() void OnUnderHOffsetCommitted(float Value);
	UFUNCTION() void OnUnderFOffsetCommitted(float Value);
	UFUNCTION() void OnUnderCoverGapChanged(bool bIsChecked);

	UFUNCTION() void OnCombineBlueprintChanged(bool bIsChecked);

	UFUNCTION() void OnDoubleRailsChanged(bool bIsChecked);
	UFUNCTION() void OnTrackSeparationCommitted(float Value);
	UFUNCTION() void OnUnderSideModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnUnderAutoRollChanged(bool bIsChecked);
	UFUNCTION() void OnPillarSideModeChanged0(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPillarSideModeChanged1(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPillarSideModeChanged2(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPillarSideModeChanged3(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnSplitUnderRailSidesChanged(bool bIsChecked);
	UFUNCTION() void OnPartnerLengthCommitted(float Value);
	UFUNCTION() void OnPartnerCoverGapChanged(bool bIsChecked);
	UFUNCTION() void OnPartnerYawCommitted(float Value);
	UFUNCTION() void OnPartnerPitchCommitted(float Value);
	UFUNCTION() void OnPartnerRollCommitted(float Value);
	UFUNCTION() void OnPartnerVOffsetCommitted(float Value);
	UFUNCTION() void OnPartnerHOffsetCommitted(float Value);
	UFUNCTION() void OnPartnerFOffsetCommitted(float Value);

	UFUNCTION() void OnPillarAutoRollChanged0(bool bIsChecked);
	UFUNCTION() void OnPillarAutoRollChanged1(bool bIsChecked);
	UFUNCTION() void OnPillarAutoRollChanged2(bool bIsChecked);
	UFUNCTION() void OnPillarAutoRollChanged3(bool bIsChecked);

	UFUNCTION() void OnPillarBlueprintChanged0(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPillarBlueprintChanged1(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPillarBlueprintChanged2(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPillarBlueprintChanged3(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPillarEveryCommitted0(float Value);
	UFUNCTION() void OnPillarEveryCommitted1(float Value);
	UFUNCTION() void OnPillarEveryCommitted2(float Value);
	UFUNCTION() void OnPillarEveryCommitted3(float Value);
	UFUNCTION() void OnPillarSpacingCommitted0(float Value);
	UFUNCTION() void OnPillarSpacingCommitted1(float Value);
	UFUNCTION() void OnPillarSpacingCommitted2(float Value);
	UFUNCTION() void OnPillarSpacingCommitted3(float Value);
	UFUNCTION() void OnPillarAlignChanged0(bool bIsChecked);
	UFUNCTION() void OnPillarAlignChanged1(bool bIsChecked);
	UFUNCTION() void OnPillarAlignChanged2(bool bIsChecked);
	UFUNCTION() void OnPillarAlignChanged3(bool bIsChecked);
	UFUNCTION() void OnPillarPriorityChanged0(bool bIsChecked);
	UFUNCTION() void OnPillarPriorityChanged1(bool bIsChecked);
	UFUNCTION() void OnPillarPriorityChanged2(bool bIsChecked);
	UFUNCTION() void OnPillarPriorityChanged3(bool bIsChecked);

	UFUNCTION() void OnPillarYawCommitted0(float Value);
	UFUNCTION() void OnPillarYawCommitted1(float Value);
	UFUNCTION() void OnPillarYawCommitted2(float Value);
	UFUNCTION() void OnPillarYawCommitted3(float Value);
	UFUNCTION() void OnPillarPitchCommitted0(float Value);
	UFUNCTION() void OnPillarPitchCommitted1(float Value);
	UFUNCTION() void OnPillarPitchCommitted2(float Value);
	UFUNCTION() void OnPillarPitchCommitted3(float Value);
	UFUNCTION() void OnPillarRollCommitted0(float Value);
	UFUNCTION() void OnPillarRollCommitted1(float Value);
	UFUNCTION() void OnPillarRollCommitted2(float Value);
	UFUNCTION() void OnPillarRollCommitted3(float Value);
	UFUNCTION() void OnPillarVOffsetCommitted0(float Value);
	UFUNCTION() void OnPillarVOffsetCommitted1(float Value);
	UFUNCTION() void OnPillarVOffsetCommitted2(float Value);
	UFUNCTION() void OnPillarVOffsetCommitted3(float Value);
	UFUNCTION() void OnPillarHOffsetCommitted0(float Value);
	UFUNCTION() void OnPillarHOffsetCommitted1(float Value);
	UFUNCTION() void OnPillarHOffsetCommitted2(float Value);
	UFUNCTION() void OnPillarHOffsetCommitted3(float Value);
	UFUNCTION() void OnPillarFOffsetCommitted0(float Value);
	UFUNCTION() void OnPillarFOffsetCommitted1(float Value);
	UFUNCTION() void OnPillarFOffsetCommitted2(float Value);
	UFUNCTION() void OnPillarFOffsetCommitted3(float Value);

	UFUNCTION() void OnSavePresetClicked();
	UFUNCTION() void OnApplyPresetClicked();
	UFUNCTION() void OnDeletePresetClicked();

	void HandlePillarBlueprintChanged(int32 SlotIndex, const FString& SelectedItem, ESelectInfo::Type SelectionType);
	void HandlePillarEveryCommitted(int32 SlotIndex, float Value);
	void HandlePillarSpacingCommitted(int32 SlotIndex, float Value);
	void HandlePillarAlignChanged(int32 SlotIndex, bool bIsChecked);
	void HandlePillarPriorityChanged(int32 SlotIndex, bool bIsChecked);
	void HandlePillarYawCommitted(int32 SlotIndex, float Value);
	void HandlePillarPitchCommitted(int32 SlotIndex, float Value);
	void HandlePillarRollCommitted(int32 SlotIndex, float Value);
	void HandlePillarVOffsetCommitted(int32 SlotIndex, float Value);
	void HandlePillarHOffsetCommitted(int32 SlotIndex, float Value);
	void HandlePillarFOffsetCommitted(int32 SlotIndex, float Value);
	void HandlePillarSideModeChanged(int32 SlotIndex, const FString& SelectedItem, ESelectInfo::Type SelectionType);
	void HandlePillarAutoRollChanged(int32 SlotIndex, bool bIsChecked);

	static void FillSideModeOptions(UComboBoxString* Combo);
	static FString SideModeToOption(EARSSideMode Mode);
	static EARSSideMode OptionToSideMode(const FString& Option);

private:
	FARSSettingsData mWorking;
	uint32 mSeenRevision = 0;
	bool bHaveServerData = false;
	bool bPendingRefresh = false;
	bool bSuppressEvents = false;

	FText mDefaultHoverTitle;
	FText mDefaultHoverDescription;

	TMap<FString, FVector> mMeasuredSizes;
	TSet<FString> mRequestedSizeNames;
	bool bAutoLengthOnSize = false;

	TArray<UComboBoxString*> mPillarCombos;
	TArray<UARSSliderWidget*> mPillarEverySpins;
	TArray<UARSSliderWidget*> mPillarSpacingSpins;
	TArray<UCheckBox*> mPillarAlignChecks;
	TArray<UCheckBox*> mPillarPriorityChecks;
	TArray<UTextBlock*> mPillarLengthTexts;
	TArray<UTextBlock*> mPillarWidthTexts;
	TArray<UTextBlock*> mPillarHeightTexts;
	TArray<UARSSliderWidget*> mPillarYawSpins;
	TArray<UARSSliderWidget*> mPillarPitchSpins;
	TArray<UARSSliderWidget*> mPillarRollSpins;
	TArray<UARSSliderWidget*> mPillarVerticalSpins;
	TArray<UARSSliderWidget*> mPillarSidewaysSpins;
	TArray<UARSSliderWidget*> mPillarForwardSpins;
	TArray<UComboBoxString*> mPillarSideModeCombos;
	TArray<UCheckBox*> mPillarAutoRollChecks;
};
