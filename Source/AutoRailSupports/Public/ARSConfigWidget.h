#pragma once
#include "CoreMinimal.h"
#include "UI/FGInteractWidget.h"
#include "ARSSettingsData.h"
#include "Types/SlateEnums.h"
#include "ARSConfigWidget.generated.h"

class AARSClientManager;
class UARSBlueprintPickerWidget;
class UARSCheckRowWidget;
class UARSPillarSlotWidget;
class UARSPresetRowWidget;
class UARSSideModePickerWidget;
class UARSSegmentedSwitchWidget;
class UARSSliderWidget;
class UARSUnderRailPanelWidget;
class UButton;
class UComboBoxString;
class UEditableTextBox;
class UFGWindow;
class UPanelWidget;
class UScrollBox;
class UTextBlock;
class UWidget;
class UWidgetAnimation;

enum class EARSPresetConfirm : uint8 { None, LoadOverUnsaved, Delete, Overwrite };

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

	void SetHoverInfo(const FText& Name, const FText& Description, const UObject* Source);
	void ClearHoverInfo(const UObject* Source);

	void HandleBlueprintSize(const FString& BlueprintName, const FVector& SizeMeters);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Auto Rail Supports")
	TSubclassOf<UARSPillarSlotWidget> PillarSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Auto Rail Supports")
	TSubclassOf<UARSPresetRowWidget> PresetRowWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText UnderRailHeader = NSLOCTEXT("AutoRailSupports", "SideUnderRail", "Main Rail");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText SplitMainHeader = NSLOCTEXT("AutoRailSupports", "SideMainRail", "Main Rail");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText PartnerHeader = NSLOCTEXT("AutoRailSupports", "SideSecondRail", "Second Rail");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText MasterOnLabel = NSLOCTEXT("AutoRailSupports", "MasterEnabled", "ENABLED");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText MasterOffLabel = NSLOCTEXT("AutoRailSupports", "MasterDisabled", "DISABLED");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText MasterOffHelp = NSLOCTEXT("AutoRailSupports", "MasterOffHelp", "Rails build plain. Applying to an existing rail still works.");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText PillarCountFormat = NSLOCTEXT("AutoRailSupports", "PillarCount", "{0} of {1}");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText DimensionsPendingText = NSLOCTEXT("AutoRailSupports", "DimsPending", "measuring...");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Text")
	FText DimensionsUnknownText = NSLOCTEXT("AutoRailSupports", "DimsUnknown", "size unknown");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Preset Text")
	FText PresetCountFormat = NSLOCTEXT("AutoRailSupports", "PresetCount", "{0} saved");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Preset Text")
	FText NoPresetLoadedText = NSLOCTEXT("AutoRailSupports", "NoPresetLoaded", "Unsaved");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Preset Text", meta = (MultiLine = true))
	FText UnsavedChangesQuestion = NSLOCTEXT("AutoRailSupports", "UnsavedChanges",
		"You have unsaved changes. Loading a preset will discard them.");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Preset Text", meta = (MultiLine = true))
	FText DeletePresetQuestion = NSLOCTEXT("AutoRailSupports", "DeletePreset", "Delete the preset \"{0}\"?");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Preset Text")
	FText SavePresetLabel = NSLOCTEXT("AutoRailSupports", "SavePreset", "Save Changes");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Preset Text")
	FText SaveNewPresetLabel = NSLOCTEXT("AutoRailSupports", "SaveNewPreset", "Save as New");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports|Preset Text", meta = (MultiLine = true))
	FText OverwritePresetQuestion = NSLOCTEXT("AutoRailSupports", "OverwritePreset",
		"A preset named \"{0}\" already exists. Overwrite it?");

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports")
	FLinearColor MasterOnColor = FLinearColor(FColor(0x4C, 0xBB, 0x17));

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports")
	FLinearColor MasterOffColor = FLinearColor(FColor(0x3A, 0x3A, 0x3A));

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports")
	FLinearColor MasterOnTextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Auto Rail Supports")
	FLinearColor MasterOffTextColor = FLinearColor(FColor(0x9A, 0x9A, 0x9A));

	//FRAME
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UFGWindow> WindowFrame;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CloseButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UScrollBox> ContentScrollBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HoverTitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HoverDescriptionText;

	//HEADER
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MasterButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MasterButtonText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MasterHelperText;

	//PRESETS
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PresetHeaderButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PresetCountText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PresetActiveNameText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PresetDirtyIndicator;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PresetExpandedChevron;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PresetCollapsedChevron;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PresetDropdownPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PresetClickCatcher;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> PresetListBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UEditableTextBox> PresetNameBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PresetSaveButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PresetSaveButtonText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PresetResetButton;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PresetConfirmPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PresetConfirmText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PresetConfirmButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PresetCancelButton;
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> PresetConfirmFlash;

	//TRACK LAYOUT
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSegmentedSwitchWidget> TrackLayoutSwitch;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSliderWidget> TrackSeparationSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> TrackSeparationHint;

	//GENERAL
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSCheckRowWidget> CombineBlueprintCheck;

	//UNDER-RAIL
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSSideModePickerWidget> UnderRailSideModeRow;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSCheckRowWidget> SplitSidesCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSUnderRailPanelWidget> MainUnderRailPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UARSUnderRailPanelWidget> PartnerUnderRailPanel;

	//PILLARS
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> PillarSlotBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AddPillarButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PillarCountText;

private:
	AARSClientManager* GetClientManager() const;

	void ApplyMenuInputMode();
	void CloseSelf();
	void SendSettings();
	void SetMasterButtonState(bool bEnabled);

	void RefreshBlueprintOptions();
	void PushWorkingToControls();
	void RebuildPillarList();
	void UpdateConditionalVisibility();
	void UpdateBlueprintInfo();
	void RequestBlueprintSize(const FString& BlueprintName);
	void BindMenuSounds();
	void UpdateSaveButton();
	void UpdatePresetHeader();
	void TickPresetNameMarquee(float DeltaTime);

	FText DimensionsTextFor(const FString& BlueprintName) const;
	float UnderRailStepCm() const;

	//PRESETS
	void SetPresetMenuOpen(bool bOpen);
	void RebuildPresetList();
	void LoadPreset(const FString& PresetName);
	void SavePresetAs(const FString& PresetName);
	void AskConfirmation(EARSPresetConfirm Action, const FString& PresetName, const FText& Question);
	void CloseConfirmation();
	bool IsWorkingDirty() const;

	UFUNCTION() void OnMasterToggleClicked();
	UFUNCTION() void OnCloseClicked();

	UFUNCTION() void OnTrackLayoutSelected(int32 Index);
	UFUNCTION() void OnTrackSeparationCommitted(float Value);
	UFUNCTION() void OnCombineBlueprintChanged(bool bIsChecked);
	UFUNCTION() void OnUnderRailSideModeChanged(EARSSideMode SideMode);
	UFUNCTION() void OnSplitSidesChanged(bool bIsChecked);

	UFUNCTION() void OnUnderRailSideEdited(bool bPartnerSide);
	UFUNCTION() void OnUnderRailBlueprintPicked(bool bPartnerSide, const FString& BlueprintName);

	UFUNCTION() void OnSlotEdited(int32 SlotIndex);
	UFUNCTION() void OnSlotBlueprintPicked(int32 SlotIndex, const FString& BlueprintName);
	UFUNCTION() void OnSlotMoveRequested(int32 SlotIndex, int32 Delta);
	UFUNCTION() void OnSlotRemoveRequested(int32 SlotIndex);
	UFUNCTION() void OnAddPillarClicked();

	UFUNCTION() void OnPresetHeaderClicked();
	UFUNCTION() void OnPresetClickCatcherClicked();
	UFUNCTION() void OnPresetSaveClicked();
	UFUNCTION() void OnPresetNameTextChanged(const FText& Text);
	UFUNCTION() void OnPresetResetClicked();
	UFUNCTION() void OnPresetConfirmClicked();
	UFUNCTION() void OnPresetCancelClicked();

	UFUNCTION() void OnPresetRowChosen(const FString& PresetName);
	UFUNCTION() void OnPresetRowRenamed(const FString& OldName, const FString& NewName);
	UFUNCTION() void OnPresetRowDeleteRequested(const FString& PresetName);
	UFUNCTION() void OnPresetRowRenameStarted(const FString& PresetName);

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
	FString mAutoLengthBlueprint;
	bool bAutoLengthForPartner = false;

	UPROPERTY() TArray<TObjectPtr<UARSPillarSlotWidget>> mSlotWidgets;

	UPROPERTY() TArray<TObjectPtr<UARSPresetRowWidget>> mPresetRows;

	TWeakObjectPtr<const UObject> mHoverSource;

	FString mActivePresetName;
	float mNameMarqueeTime = 0.0f;
	FARSSettingsData mPresetBaseline;
	EARSPresetConfirm mConfirmAction = EARSPresetConfirm::None;
	FString mConfirmPresetName;

	float mPositionTiny = 0.01f;
	float mPositionSmall = 0.1f;
	float mPositionLarge = 4.0f;
	float mRotationTiny = 0.1f;
	float mRotationSmall = 1.0f;
	float mRotationLarge = 45.0f;
};
