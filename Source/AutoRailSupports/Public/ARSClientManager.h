#pragma once
#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "ItemAmount.h"
#include "ARSSettingsData.h"
#include "ARSModConfigStruct.h"
#include "Components/SlateWrapperTypes.h"
#include "Input/FGBoundMappingContextHandle.h"
#include "ARSClientManager.generated.h"

class AARSPreviewActor;
class AFGBuildableRailroadTrack;
class AFGHologram;
class AFGPlayerController;
class UARSConfigWidget;
class UARSGizmo;
class UARSInfoPanelWidget;
class UARSRemoteCallObject;
class UTextBlock;
class URichTextBlock;
class UAkAudioEvent;
class UFGGameUI;
class UFGInputMappingContext;
class UInputAction;
class UWidget;

// which gun opened in-world ARS Mode
enum class EARSModeGun : uint8 { Build, Dismantle };

// adjustment mode
enum class EARSSettingsMode : uint8 { General, TilingLength, PillarSpacing, SpacingOffset, TrackSeparation };

enum class EARSArrow : uint8 { Forward, Backward, Left, Right };

UCLASS()
class AUTORAILSUPPORTS_API AARSClientManager : public AModSubsystem
{
	GENERATED_BODY()
public:
	AARSClientManager();

	static AARSClientManager* Get(UWorld* World);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// UI
	void OpenUI();
	void CloseUI();

	// Keybind actions
	void TriggerApplyAimedRail();
	void TriggerToggleEnabled();

	//SETTINGS
	const FARSSettingsData& GetSettings() const { return mSettings; }

	uint32 GetSettingsRevision() const { return mSettingsRevision; }

	bool HasServerSettings() const { return bHaveServerSettings; }

	void ApplyServerSettings(const FARSSettingsData& Settings);

	// Global config (presets)
	const FARSGlobalConfig& GetGlobalConfig() const { return mConfig; }
	void SavePreset(const FString& Name, const FARSSettingsData& Settings);
	bool DeletePreset(const FString& Name);
	const FARSPreset* FindPreset(const FString& Name) const;

	// Helpers for the widget
	AFGPlayerController* GetLocalController() const;
	UARSRemoteCallObject* GetRCO() const;

	UARSConfigWidget* GetOpenConfigWidget() const;

	static AFGBuildableRailroadTrack* TraceAimedRail(AFGPlayerController* PlayerController);

	const TArray<FItemAmount>& GetPlannedSupportCost() const { return mPlannedSupportCost; }

protected:
	virtual void Init() override;

private:
	void TryBindInput();
	void RebuildInputBindings(AFGPlayerController* PlayerController);

	UFGGameUI* GetGameUI() const;

	void UpdateSettingsRequest(float DeltaSeconds);

	bool IsLocalPlayerDismantling() const;

	AFGHologram* GetActiveRailHologram();

	void UpdateRailPreview();

	void UpdateRemotePreviews();

	void PushPreviewSource(AActor* Source);

	void PushPreviewSpline(class USplineComponent* Spline, uint8 Stencil);

	UFUNCTION() void OnOpenUIAction();
	UFUNCTION() void OnApplyAction();
	UFUNCTION() void OnToggleAction();

	//ARS MODE ACTIONS
	// only enters while placing a rail, or with the dismantle gun out
	UFUNCTION() void OnToggleModeAction();
	// part cycling and settings-mode cyling
	UFUNCTION() void OnPrevPartAction();
	UFUNCTION() void OnNextPartAction();
	UFUNCTION() void OnCycleSettingsModeAction();
	// cycles which axis: Yaw, Pitch, Roll
	UFUNCTION() void OnCycleRotationAxisAction();
	// position arrows, vertical steps and rotation steps
	UFUNCTION() void OnAdjustForwardAction();
	UFUNCTION() void OnAdjustBackwardAction();
	UFUNCTION() void OnAdjustLeftAction();
	UFUNCTION() void OnAdjustRightAction();
	UFUNCTION() void OnVerticalUpAction();
	UFUNCTION() void OnVerticalDownAction();
	UFUNCTION() void OnRotateUpAction();
	UFUNCTION() void OnRotateDownAction();

	UFUNCTION() void OnToggleHologramLockAction();

	void EnterARSMode(EARSModeGun Gun);
	void ExitARSMode();

	void UpdateARSModeValidity();

	// info panel
	void ShowModeHUD();
	void HideModeHUD();
	void RefreshInfoPanel();

	// rotation gizmo
	void UpdateRotationGizmo();
	void DestroyRotationGizmo();

	//HINT BAR + MODE INDICATOR
	UUserWidget* FindLiveModeWidget();
	UUserWidget* FindLiveBuildModeWidget();
	void BuildARSHintRows(TArray<TPair<FText, FText>>& OutRows) const;
	bool IsActiveRailLocked();
	static bool ResolveHintArray(UUserWidget* Widget, class FArrayProperty*& OutArray, class FTextProperty*& OutAction, class FTextProperty*& OutKeyBinding, const TCHAR* ArrayName = TEXT("mStandardKeybinds"));
	static void ReadHintRows(UUserWidget* Widget, TArray<TPair<FText, FText>>& OutRows, const TCHAR* ArrayName = TEXT("mStandardKeybinds"));
	static void WriteHintRows(UUserWidget* Widget, const TArray<TPair<FText, FText>>& Rows);
	bool PatchHintRow(UUserWidget* Widget, int32 RowIndex, const TPair<FText, FText>& OldRow, const TPair<FText, FText>& NewRow) const;
	static bool PatchLiveHintText(UUserWidget* BarWidget, const FText& OldText, const FText& NewText);
	static bool SetHintRowInArray(UUserWidget* Widget, int32 RowIndex, const TPair<FText, FText>& Row, const TCHAR* ArrayName);
	void ApplyARSHintsToBar();
	void EnsureBuildGunAdjustHint();
	void RestoreVanillaHints();
	void ApplyModeIndicator();
	void RestoreModeIndicator();
	void ApplyDismantleDressing();
	void RestoreDismantleDressing();
	static FText GetSettingsModeDisplayText(EARSSettingsMode Mode);
	FText GetKeyLabel(const UInputAction* Action) const;
	FString GetSelectedBlueprintName() const;
	FVector GetSelectedBlueprintSizeMeters();
	void BuildSelectedSettingsRows(TArray<struct FARSInfoPanelRow>& OutRows) const;

	int32 NumSettingsModesForPart(int32 Part) const;
	EARSSettingsMode SettingsModeForPartIndex(int32 Part, int32 Index) const;

	struct FARSUnderRailEdit
	{
		float* Length = nullptr;
		FRotator* Rotation = nullptr;
		float* ZOffset = nullptr;
		float* SideOffset = nullptr;
		float* ForwardOffset = nullptr;
	};
	bool IsEditingPartnerUnderRail() const;
	FARSUnderRailEdit SelectedUnderRailEdit();
	EARSSettingsMode GetCurrentSettingsMode() const { return SettingsModeForPartIndex(mSelectedPart, mSettingsModeIndex); }

	FVector GetActiveRailForward();
	void ApplyArrow(EARSArrow Arrow);
	void ApplyRotate(float Sign);
	void ApplyVertical(float Sign);
	void MaybeShowTilingWarning();
	float GetPositionStepCm() const;
	float GetRotationStepDegrees() const;
	void SendWorkingSettings();
	void RefreshModConfig();

	void InjectAdjustHints();

	UWidget* FindLiveDismantleRefundsWidget();

	void PlayUISound(TObjectPtr<UAkAudioEvent>& InOutEvent, bool& bInOutLoaded, const TCHAR* EventPath);
	void PlayApplySound();
	void PlayCycleModeSound();

	void LoadGlobalConfig();
	void SaveGlobalConfig() const;
	static FString GetConfigFilePath();

	TSubclassOf<UARSConfigWidget> GetConfigWidgetClass();

	UPROPERTY() TObjectPtr<UFGInputMappingContext> mMappingContext;
	UPROPERTY() TObjectPtr<UInputAction> mOpenUIAction;
	UPROPERTY() TObjectPtr<UInputAction> mApplyAction;
	UPROPERTY() TObjectPtr<UInputAction> mToggleAction;

	UPROPERTY() TObjectPtr<UInputAction> mToggleModeAction;
	UPROPERTY() TObjectPtr<UInputAction> mPrevPartAction;
	UPROPERTY() TObjectPtr<UInputAction> mNextPartAction;
	UPROPERTY() TObjectPtr<UInputAction> mCycleSettingsModeAction;
	UPROPERTY() TObjectPtr<UInputAction> mCycleRotationAxisAction;
	UPROPERTY() TObjectPtr<UInputAction> mAdjustForwardAction;
	UPROPERTY() TObjectPtr<UInputAction> mAdjustBackwardAction;
	UPROPERTY() TObjectPtr<UInputAction> mAdjustLeftAction;
	UPROPERTY() TObjectPtr<UInputAction> mAdjustRightAction;
	UPROPERTY() TObjectPtr<UInputAction> mVerticalUpAction;
	UPROPERTY() TObjectPtr<UInputAction> mVerticalDownAction;
	UPROPERTY() TObjectPtr<UInputAction> mRotateUpAction;
	UPROPERTY() TObjectPtr<UInputAction> mRotateDownAction;
	UPROPERTY() TObjectPtr<UInputAction> mToggleHologramLockAction;
	UPROPERTY() TObjectPtr<UFGInputMappingContext> mModeSharedContext;
	UPROPERTY() TObjectPtr<UFGInputMappingContext> mModeDismantleContext;
	bool mInputAssetsLoaded = false;

	UPROPERTY() TSubclassOf<UARSConfigWidget> mConfigWidgetClass;
	UPROPERTY() TSubclassOf<class UFGInteractWidget> mDismantleWidgetClass;

	UPROPERTY() TObjectPtr<AARSPreviewActor> mPreviewActor;

	UPROPERTY() TMap<TObjectPtr<class AFGCharacterPlayer>, TObjectPtr<AARSPreviewActor>> mRemotePreviews;

	UPROPERTY() TWeakObjectPtr<AActor> mPushedPreviewSource;

	double mLastPreviewSplineSent = 0.0;
	bool mSendingPreviewSpline = false;
	static constexpr double PreviewSplineSendInterval = 0.1;

	UPROPERTY() TSubclassOf<UARSGizmo> mGizmoClass;
	UPROPERTY() TObjectPtr<UARSGizmo> mGizmo;
	bool mGizmoClassSearched = false;

	// support cost mirrored from the preview each tick
	TArray<FItemAmount> mPlannedSupportCost;

	//SETTINGS
	FARSSettingsData mSettings;
	uint32 mSettingsRevision = 0;
	bool bHaveServerSettings = false;
	float mSettingsRequestCooldown = 0.0f;
	float mSettingsIdentityWait = 0.0f;

	bool mLastPreviewEnabled = true;

	FARSGlobalConfig mConfig;
	FBoundMappingContextHandle mContextHandle;
	TWeakObjectPtr<AFGPlayerController> mBoundController;
	FTimerHandle mBindRetryTimer;

	//ARS MODE STATE
	bool mARSModeActive = false;
	EARSModeGun mARSModeGun = EARSModeGun::Build;
	int32 mSelectedPart = 0;
	int32 mSettingsModeIndex = 0;
	int32 mRotationAxis = 0;
	// which rail the nearest under-rail stamp sits on
	bool mEditingPartnerSide = false;
	FARSSettingsData mWorking;
	FBoundMappingContextHandle mModeSharedHandle;
	FBoundMappingContextHandle mModeDismantleHandle;


	FARSModConfigStruct mModConfig;

	// info panel
	UPROPERTY() TSubclassOf<UARSInfoPanelWidget> mInfoPanelClass;
	UPROPERTY() TObjectPtr<UARSInfoPanelWidget> mInfoPanel;
	bool mInfoPanelClassSearched = false;
	bool mTilingWarnActive = false;

	//HINT BAR + MODE INDICATOR STATE
	UPROPERTY() TSubclassOf<class UFGInteractWidget> mBuildModeWidgetClass;
	TWeakObjectPtr<UUserWidget> mHintBarWidget;
	TWeakObjectPtr<UUserWidget> mBuildBarWidget;
	TArray<TPair<FText, FText>> mSavedVanillaHints;
	bool mVanillaHintsSaved = false;
	uint32 mLastHintSig = 0xFFFFFFFF;
	int32 mHintReinjectFramesLeft = 0;
	TWeakObjectPtr<UTextBlock> mModeTextBlock;
	FText mOrigModeText;
	bool mModeTextCached = false;
	TWeakObjectPtr<UWidget> mModeContainerWidget;
	ESlateVisibility mModeContainerOrigVisibility = ESlateVisibility::Collapsed;
	bool mModeContainerCached = false;
	TWeakObjectPtr<URichTextBlock> mDismantleNameText;
	FText mOrigDismantleName;
	bool mDismantleNameCached = false;
	TArray<TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>> mDismantleHiddenWidgets;
	bool mHologramLocked = false;
	TWeakObjectPtr<AFGBuildableRailroadTrack> mDismantleLockedTrack;
	bool bHintInjectionAttempted = false;

	// vanilla Wwise cues, each loaded on first use
	UPROPERTY() TObjectPtr<UAkAudioEvent> mApplySound;
	bool mApplyAudioLoaded = false;
	UPROPERTY() TObjectPtr<UAkAudioEvent> mCycleModeSound;
	bool mCycleModeAudioLoaded = false;
};
