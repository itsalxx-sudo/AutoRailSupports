#include "ARSConfigWidget.h"

#include "ARSCheckRowWidget.h"
#include "ARSClientManager.h"
#include "ARSModConfigStruct.h"
#include "ARSPillarSlotWidget.h"
#include "ARSPresetRowWidget.h"
#include "ARSRemoteCallObject.h"
#include "ARSSegmentedSwitchWidget.h"
#include "ARSSideModePickerWidget.h"
#include "ARSSliderWidget.h"
#include "ARSSubsystem.h"
#include "ARSUnderRailPanelWidget.h"
#include "FGBlueprintSubsystem.h"
#include "FGFactoryBlueprintTypes.h"
#include "UI/FGWindow.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Animation/WidgetAnimation.h"
#include "UObject/PropertyPortFlags.h"

namespace
{
	//MARQUEE
	constexpr float MarqueeHoldSeconds = 1.5f;
	constexpr float MarqueeSpeed = 30.0f;
	constexpr float MarqueeSlack = 1.0f;
}

UARSConfigWidget::UARSConfigWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mUseKeyboard = true;
	mUseMouse = true;
	mDisablePlayerActions = true;
	mDisableBuildGunActions = true;
	mMouseLockMode = EMouseLockMode::DoNotLock;
	mHideCursorDuringCapture = false;
	mRestoreFocusWhenLost = true;
	mDesiredHorizontalAlignment = HAlign_Center;
	mDesiredVerticalAlignment = VAlign_Center;
	SetIsFocusable(true);
}

//SETUP

void UARSConfigWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (HoverTitleText) { mDefaultHoverTitle = HoverTitleText->GetText(); }
	if (HoverDescriptionText) { mDefaultHoverDescription = HoverDescriptionText->GetText(); }

	const FARSModConfigStruct ModConfig = FARSModConfigStruct::GetActiveConfig(this);
	mPositionTiny = ModConfig.PositionStepTiny;
	mPositionSmall = ModConfig.PositionStepSmall;
	mPositionLarge = ModConfig.PositionStepLarge;
	mRotationTiny = ModConfig.RotationStepTiny;
	mRotationSmall = ModConfig.RotationStepSmall;
	mRotationLarge = ModConfig.RotationStepLarge;

	if (TrackSeparationSlider)
	{
		// signed separation
		TrackSeparationSlider->Configure(-20.0f, 20.0f, 1.0f, 2, /*fine*/ true, mPositionTiny, mPositionSmall, mPositionLarge);
		TrackSeparationSlider->SetBipolar(true);
		TrackSeparationSlider->SetUnit(NSLOCTEXT("AutoRailSupports", "UnitMeters", "m"));
	}

	if (MainUnderRailPanel)
	{
		MainUnderRailPanel->SetupPanel(/*bPartner*/ false, mPositionTiny, mPositionSmall, mPositionLarge,
			mRotationTiny, mRotationSmall, mRotationLarge);
		MainUnderRailPanel->OnSideEdited.AddDynamic(this, &UARSConfigWidget::OnUnderRailSideEdited);
		MainUnderRailPanel->OnSideBlueprintPicked.AddDynamic(this, &UARSConfigWidget::OnUnderRailBlueprintPicked);
	}
	if (PartnerUnderRailPanel)
	{
		PartnerUnderRailPanel->SetupPanel(/*bPartner*/ true, mPositionTiny, mPositionSmall, mPositionLarge,
			mRotationTiny, mRotationSmall, mRotationLarge);
		PartnerUnderRailPanel->OnSideEdited.AddDynamic(this, &UARSConfigWidget::OnUnderRailSideEdited);
		PartnerUnderRailPanel->OnSideBlueprintPicked.AddDynamic(this, &UARSConfigWidget::OnUnderRailBlueprintPicked);
	}

	if (MasterButton) { MasterButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnMasterToggleClicked); }
	BindMenuSounds();
	if (CloseButton) { CloseButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnCloseClicked); }
	if (TrackLayoutSwitch) { TrackLayoutSwitch->OnSegmentSelected.AddDynamic(this, &UARSConfigWidget::OnTrackLayoutSelected); }
	if (TrackSeparationSlider) { TrackSeparationSlider->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnTrackSeparationCommitted); }
	if (CombineBlueprintCheck) { CombineBlueprintCheck->OnCheckChanged.AddDynamic(this, &UARSConfigWidget::OnCombineBlueprintChanged); }
	if (UnderRailSideModeRow) { UnderRailSideModeRow->OnSideModePicked.AddDynamic(this, &UARSConfigWidget::OnUnderRailSideModeChanged); }
	if (SplitSidesCheck) { SplitSidesCheck->OnCheckChanged.AddDynamic(this, &UARSConfigWidget::OnSplitSidesChanged); }
	if (AddPillarButton) { AddPillarButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnAddPillarClicked); }

	if (PresetHeaderButton) { PresetHeaderButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnPresetHeaderClicked); }
	if (PresetClickCatcher) { PresetClickCatcher->OnClicked.AddDynamic(this, &UARSConfigWidget::OnPresetClickCatcherClicked); }
	if (PresetSaveButton) { PresetSaveButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnPresetSaveClicked); }
	if (PresetNameBox) { PresetNameBox->OnTextChanged.AddDynamic(this, &UARSConfigWidget::OnPresetNameTextChanged); }
	if (PresetResetButton) { PresetResetButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnPresetResetClicked); }
	if (PresetConfirmButton) { PresetConfirmButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnPresetConfirmClicked); }
	if (PresetCancelButton) { PresetCancelButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnPresetCancelClicked); }
	CloseConfirmation();

	SetPresetMenuOpen(false);

	if (WindowFrame)
	{
		if (FMulticastDelegateProperty* CloseProperty = CastField<FMulticastDelegateProperty>(
			WindowFrame->GetClass()->FindPropertyByName(TEXT("OnClose"))))
		{
			FScriptDelegate CloseDelegate;
			CloseDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UARSConfigWidget, RequestClose));
			CloseProperty->AddDelegate(CloseDelegate, WindowFrame);
		}
	}
}

void UARSConfigWidget::BindMenuSounds()
{
	for (UButton* Button : { MasterButton.Get(), CloseButton.Get(), AddPillarButton.Get(),
		PresetHeaderButton.Get(), PresetSaveButton.Get(), PresetResetButton.Get(),
		PresetConfirmButton.Get(), PresetCancelButton.Get() })
	{
		AARSClientManager::BindButtonSounds(this, Button);
	}
}

void UARSConfigWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshBlueprintOptions();
	RebuildPresetList();

	bHaveServerData = false;
	if (const AARSClientManager* ClientManager = GetClientManager())
	{
		if (ClientManager->HasServerSettings())
		{
			mWorking = ClientManager->GetSettings();
			mWorking.Normalize();
			mSeenRevision = ClientManager->GetSettingsRevision();
			bHaveServerData = true;
			PushWorkingToControls();
		}
	}
	if (StatusText) { StatusText->SetVisibility(bHaveServerData ? ESlateVisibility::Collapsed : ESlateVisibility::Visible); }
	if (ContentScrollBox) { ContentScrollBox->SetIsEnabled(bHaveServerData); }

	ApplyMenuInputMode();
}

void UARSConfigWidget::ApplyMenuInputMode()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
		PlayerController->SetIgnoreLookInput(true);
		PlayerController->SetIgnoreMoveInput(true);
	}
	SetFocus();
}

void UARSConfigWidget::NativeDestruct()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->SetIgnoreMoveInput(false);
	}
	Super::NativeDestruct();
}

void UARSConfigWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TickPresetNameMarquee(InDeltaTime);

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (!PlayerController->bShowMouseCursor)
		{
			ApplyMenuInputMode();
		}
	}

	const AARSClientManager* ClientManager = GetClientManager();
	if (!ClientManager || !ClientManager->HasServerSettings())
	{
		return;
	}
	const bool bRevisionChanged = ClientManager->GetSettingsRevision() != mSeenRevision;
	if (!bHaveServerData || bRevisionChanged || bPendingRefresh)
	{
		if (HasFocusedDescendants() && bHaveServerData)
		{
			bPendingRefresh = true;
			mSeenRevision = ClientManager->GetSettingsRevision();
			return;
		}
		mWorking = ClientManager->GetSettings();
		mWorking.Normalize();
		mSeenRevision = ClientManager->GetSettingsRevision();
		bPendingRefresh = false;
		if (!bHaveServerData)
		{
			bHaveServerData = true;
			if (StatusText) { StatusText->SetVisibility(ESlateVisibility::Collapsed); }
			if (ContentScrollBox) { ContentScrollBox->SetIsEnabled(true); }
		}
		PushWorkingToControls();
	}
}

AARSClientManager* UARSConfigWidget::GetClientManager() const
{
	return AARSClientManager::Get(GetWorld());
}

//CONTROLS

void UARSConfigWidget::PushWorkingToControls()
{
	TGuardValue<bool> Guard(bSuppressEvents, true);

	SetMasterButtonState(mWorking.bEnabled);
	if (TrackLayoutSwitch) { TrackLayoutSwitch->SetSelectedIndex(mWorking.bDoubleRails ? 1 : 0); }
	if (TrackSeparationSlider) { TrackSeparationSlider->SetValue(mWorking.TrackSeparation / 100.0f); }
	if (CombineBlueprintCheck) { CombineBlueprintCheck->SetChecked(mWorking.bCombineIntoOneBlueprint); }
	if (UnderRailSideModeRow) { UnderRailSideModeRow->SetSideMode(mWorking.UnderRailSideMode); }
	if (SplitSidesCheck) { SplitSidesCheck->SetChecked(mWorking.bSplitUnderRailSides); }

	if (MainUnderRailPanel) { MainUnderRailPanel->PushSide(mWorking.GetUnderRailSide(/*bPartner*/ false)); }
	if (PartnerUnderRailPanel) { PartnerUnderRailPanel->PushSide(mWorking.GetUnderRailSide(/*bPartner*/ true)); }

	RebuildPillarList();
	UpdateConditionalVisibility();
	UpdateBlueprintInfo();
	UpdatePresetHeader();
}

void UARSConfigWidget::RebuildPillarList()
{
	const int32 SlotLimit = AARSSubsystem::GetMaxPillarSlots(this);
	mWorking.ClampPillarSlotsTo(SlotLimit);
	if (!PillarSlotBox || !PillarSlotWidgetClass)
	{
		return;
	}
	const int32 NumSlots = mWorking.NumPillarSlots();
	while (mSlotWidgets.Num() > NumSlots)
	{
		if (UARSPillarSlotWidget* Card = mSlotWidgets.Pop())
		{
			Card->RemoveFromParent();
		}
	}
	while (mSlotWidgets.Num() < NumSlots)
	{
		UARSPillarSlotWidget* Card = CreateWidget<UARSPillarSlotWidget>(this, PillarSlotWidgetClass);
		if (!Card)
		{
			break;
		}
		Card->SetupCard(mSlotWidgets.Num(), mPositionTiny, mPositionSmall, mPositionLarge,
			mRotationTiny, mRotationSmall, mRotationLarge);
		Card->OnSlotEdited.AddDynamic(this, &UARSConfigWidget::OnSlotEdited);
		Card->OnSlotBlueprintPicked.AddDynamic(this, &UARSConfigWidget::OnSlotBlueprintPicked);
		Card->OnSlotMoveRequested.AddDynamic(this, &UARSConfigWidget::OnSlotMoveRequested);
		Card->OnSlotRemoveRequested.AddDynamic(this, &UARSConfigWidget::OnSlotRemoveRequested);
		PillarSlotBox->AddChild(Card);
		mSlotWidgets.Add(Card);
	}

	RefreshBlueprintOptions();
	for (int32 SlotIndex = 0; SlotIndex < mSlotWidgets.Num(); SlotIndex++)
	{
		UARSPillarSlotWidget* Card = mSlotWidgets[SlotIndex];
		if (!Card)
		{
			continue;
		}
		Card->SetListPosition(SlotIndex, NumSlots, SlotIndex > 0, SlotIndex < NumSlots - 1);
		Card->SetDoubleRailsControlsVisible(mWorking.bDoubleRails);
		Card->PushSlot(mWorking.PillarSlots[SlotIndex]);
	}

	if (AddPillarButton)
	{
		AddPillarButton->SetVisibility(mWorking.CanAddPillarSlot(SlotLimit) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (PillarCountText)
	{
		PillarCountText->SetText(FText::Format(PillarCountFormat,
			FText::AsNumber(NumSlots), FText::AsNumber(SlotLimit)));
	}
}

void UARSConfigWidget::UpdateConditionalVisibility()
{
	const ESlateVisibility DoubleOnly = mWorking.bDoubleRails ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (TrackSeparationSlider) { TrackSeparationSlider->SetVisibility(DoubleOnly); }
	if (TrackSeparationHint) { TrackSeparationHint->SetVisibility(DoubleOnly); }
	if (UnderRailSideModeRow) { UnderRailSideModeRow->SetVisibility(DoubleOnly); }
	if (SplitSidesCheck)
	{
		SplitSidesCheck->SetVisibility(DoubleOnly);
		SplitSidesCheck->SetDisabled(mWorking.UnderRailSideMode == EARSSideMode::Center);
	}
	if (PartnerUnderRailPanel)
	{
		PartnerUnderRailPanel->SetVisibility(mWorking.HasSplitUnderRailSides()
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		PartnerUnderRailPanel->SetBlueprintPickerDisabled(mWorking.PartnerSharesBlueprint());
	}

	const FText MainHeader = mWorking.HasSplitUnderRailSides() ? SplitMainHeader : UnderRailHeader;
	if (MainUnderRailPanel) { MainUnderRailPanel->SetHeader(MainHeader); }
	if (PartnerUnderRailPanel) { PartnerUnderRailPanel->SetHeader(PartnerHeader); }
}

float UARSConfigWidget::UnderRailStepCm() const
{
	return FMath::Max(mWorking.UnderRailLength, 50.0f);
}

FText UARSConfigWidget::DimensionsTextFor(const FString& BlueprintName) const
{
	if (BlueprintName.IsEmpty())
	{
		return FText::GetEmpty();
	}
	const FVector* Size = mMeasuredSizes.Find(BlueprintName);
	if (!Size)
	{
		return DimensionsPendingText;
	}
	if (Size->IsNearlyZero())
	{
		return DimensionsUnknownText;
	}
	return FText::FromString(FString::Printf(TEXT("%.1f x %.1f x %.1f m"), Size->X, Size->Y, Size->Z));
}

void UARSConfigWidget::UpdateBlueprintInfo()
{
	auto UpdatePanel = [this](UARSUnderRailPanelWidget* Panel, bool bPartner)
	{
		if (!Panel)
		{
			return;
		}
		const FARSUnderRailSide Side = mWorking.GetUnderRailSide(bPartner);
		RequestBlueprintSize(Side.BlueprintName);
		Panel->SetDimensions(DimensionsTextFor(Side.BlueprintName));

		float BlueprintLengthMeters = 0.0f;
		if (const FVector* Size = mMeasuredSizes.Find(Side.BlueprintName))
		{
			BlueprintLengthMeters = Size->X;
		}
		const bool bWarn = !Side.BlueprintName.IsEmpty()
			&& AARSSubsystem::ShouldWarnStampCount(Side.Length, BlueprintLengthMeters);
		Panel->SetLengthWarning(bWarn, AARSSubsystem::GetStampCountWarningText());
	};
	UpdatePanel(MainUnderRailPanel, /*bPartner*/ false);
	UpdatePanel(PartnerUnderRailPanel, /*bPartner*/ true);

	for (int32 SlotIndex = 0; SlotIndex < mSlotWidgets.Num(); SlotIndex++)
	{
		UARSPillarSlotWidget* Card = mSlotWidgets[SlotIndex];
		if (!Card || !mWorking.PillarSlots.IsValidIndex(SlotIndex))
		{
			continue;
		}
		const FARSPillarSlotData& SlotData = mWorking.PillarSlots[SlotIndex];
		RequestBlueprintSize(SlotData.BlueprintName);
		Card->SetDimensions(DimensionsTextFor(SlotData.BlueprintName));
	}
}

void UARSConfigWidget::RequestBlueprintSize(const FString& BlueprintName)
{
	if (BlueprintName.IsEmpty() || mMeasuredSizes.Contains(BlueprintName) || mRequestedSizeNames.Contains(BlueprintName))
	{
		return;
	}
	mRequestedSizeNames.Add(BlueprintName);
	if (AARSClientManager* Manager = GetClientManager())
	{
		if (UARSRemoteCallObject* RCO = Manager->GetRCO())
		{
			RCO->Server_RequestBlueprintSize(BlueprintName);
		}
	}
}

void UARSConfigWidget::HandleBlueprintSize(const FString& BlueprintName, const FVector& SizeMeters)
{
	mRequestedSizeNames.Remove(BlueprintName);
	mMeasuredSizes.Add(BlueprintName, SizeMeters);

	if (BlueprintName == mAutoLengthBlueprint && SizeMeters.X > 0.0f)
	{
		FARSUnderRailSide Side = mWorking.GetUnderRailSide(bAutoLengthForPartner);
		Side.Length = SizeMeters.X * 100.0f;
		mWorking.SetUnderRailSide(bAutoLengthForPartner, Side);
		UARSUnderRailPanelWidget* Panel = bAutoLengthForPartner ? PartnerUnderRailPanel : MainUnderRailPanel;
		if (Panel)
		{
			Panel->PushSide(Side);
		}
		mAutoLengthBlueprint.Reset();
		SendSettings();
	}
	UpdateBlueprintInfo();
}

void UARSConfigWidget::RefreshBlueprintOptions()
{
	TArray<FString> Options;
	Options.Add(TEXT("None"));
	TArray<UFGBlueprintDescriptor*> Descriptors;
	AFGBlueprintSubsystem::GetBlueprintDescriptors(Descriptors, this);
	for (UFGBlueprintDescriptor* Descriptor : Descriptors)
	{
		if (Descriptor)
		{
			Options.Add(Descriptor->GetBlueprintNameAsString());
		}
	}
	Options.Sort([](const FString& A, const FString& B) { return A == TEXT("None") ? true : (B == TEXT("None") ? false : A < B); });

	if (MainUnderRailPanel) { MainUnderRailPanel->SetBlueprintOptions(Options); }
	if (PartnerUnderRailPanel) { PartnerUnderRailPanel->SetBlueprintOptions(Options); }
	for (UARSPillarSlotWidget* Card : mSlotWidgets)
	{
		if (Card)
		{
			Card->SetBlueprintOptions(Options);
		}
	}
}

void UARSConfigWidget::SetMasterButtonState(bool bEnabled)
{
	if (MasterButtonText)
	{
		MasterButtonText->SetText(bEnabled ? MasterOnLabel : MasterOffLabel);
		MasterButtonText->SetColorAndOpacity(FSlateColor(bEnabled ? MasterOnTextColor : MasterOffTextColor));
	}
	if (MasterButton)
	{
		MasterButton->SetBackgroundColor(bEnabled ? MasterOnColor : MasterOffColor);
	}
	if (MasterHelperText)
	{
		MasterHelperText->SetText(bEnabled ? FText::GetEmpty() : MasterOffHelp);
	}
}

void UARSConfigWidget::SendSettings()
{
	if (bSuppressEvents)
	{
		return;
	}
	if (AARSClientManager* Manager = GetClientManager())
	{
		if (UARSRemoteCallObject* RCO = Manager->GetRCO())
		{
			RCO->Server_SetSettings(mWorking);
		}
	}
	UpdatePresetHeader();
}

//FRAME

void UARSConfigWidget::RequestClose()
{
	CloseSelf();
}

void UARSConfigWidget::SetHoverInfo(const FText& Name, const FText& Description, const UObject* Source)
{
	mHoverSource = Source;
	if (HoverTitleText) { HoverTitleText->SetText(Name); }
	if (HoverDescriptionText) { HoverDescriptionText->SetText(Description); }
}

void UARSConfigWidget::ClearHoverInfo(const UObject* Source)
{
	if (mHoverSource.Get() != Source)
	{
		return;
	}
	mHoverSource = nullptr;
	if (HoverTitleText) { HoverTitleText->SetText(mDefaultHoverTitle); }
	if (HoverDescriptionText) { HoverDescriptionText->SetText(mDefaultHoverDescription); }
}


void UARSConfigWidget::CloseSelf()
{
	if (AARSClientManager* Manager = GetClientManager())
	{
		Manager->CloseUI();
	}
	else
	{
		RemoveFromParent();
	}
}

FReply UARSConfigWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseSelf();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UARSConfigWidget::OnEscapePressed_Implementation()
{
	CloseSelf();
}

void UARSConfigWidget::OnMasterToggleClicked()
{
	mWorking.bEnabled = !mWorking.bEnabled;
	SetMasterButtonState(mWorking.bEnabled);
	if (AARSClientManager* Manager = GetClientManager())
	{
		if (UARSRemoteCallObject* RCO = Manager->GetRCO())
		{
			RCO->Server_SetEnabled(mWorking.bEnabled);
		}
	}
}

void UARSConfigWidget::OnCloseClicked()
{
	CloseSelf();
}

//SETTINGS

void UARSConfigWidget::OnTrackLayoutSelected(int32 Index)
{
	if (bSuppressEvents) { return; }
	mWorking.bDoubleRails = (Index == 1);
	UpdateConditionalVisibility();
	for (UARSPillarSlotWidget* Card : mSlotWidgets)
	{
		if (Card)
		{
			Card->SetDoubleRailsControlsVisible(mWorking.bDoubleRails);
		}
	}
	UpdateBlueprintInfo();
	SendSettings();
}

void UARSConfigWidget::OnTrackSeparationCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.TrackSeparation = Value * 100.0f;
	UpdateConditionalVisibility();
	SendSettings();
}

void UARSConfigWidget::OnCombineBlueprintChanged(bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mWorking.bCombineIntoOneBlueprint = bIsChecked;
	SendSettings();
}

void UARSConfigWidget::OnUnderRailSideModeChanged(EARSSideMode SideMode)
{
	if (bSuppressEvents) { return; }
	mWorking.UnderRailSideMode = SideMode;
	if (PartnerUnderRailPanel)
	{
		PartnerUnderRailPanel->PushSide(mWorking.GetUnderRailSide(/*bPartner*/ true));
	}
	UpdateConditionalVisibility();
	UpdateBlueprintInfo();
	SendSettings();
}

void UARSConfigWidget::OnSplitSidesChanged(bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	if (bIsChecked && !mWorking.bSplitUnderRailSides)
	{
		mWorking.SeedPartnerFromUnderRail();
		if (PartnerUnderRailPanel)
		{
			PartnerUnderRailPanel->PushSide(mWorking.GetUnderRailSide(/*bPartner*/ true));
		}
	}
	mWorking.bSplitUnderRailSides = bIsChecked;
	UpdateConditionalVisibility();
	UpdateBlueprintInfo();
	SendSettings();
}

void UARSConfigWidget::OnUnderRailSideEdited(bool bPartnerSide)
{
	if (bSuppressEvents) { return; }
	UARSUnderRailPanelWidget* Panel = bPartnerSide ? PartnerUnderRailPanel : MainUnderRailPanel;
	if (!Panel)
	{
		return;
	}
	mWorking.SetUnderRailSide(bPartnerSide, Panel->GetSide());
	UpdateBlueprintInfo();
	SendSettings();
}

void UARSConfigWidget::OnUnderRailBlueprintPicked(bool bPartnerSide, const FString& BlueprintName)
{
	if (bSuppressEvents) { return; }
	UARSUnderRailPanelWidget* Panel = bPartnerSide ? PartnerUnderRailPanel : MainUnderRailPanel;
	if (!Panel)
	{
		return;
	}
	mWorking.SetUnderRailSide(bPartnerSide, Panel->GetSide());
	mAutoLengthBlueprint = BlueprintName;
	bAutoLengthForPartner = bPartnerSide;
	if (const FVector* Size = mMeasuredSizes.Find(BlueprintName))
	{
		// already measured
		HandleBlueprintSize(BlueprintName, *Size);
	}
	else
	{
		RequestBlueprintSize(BlueprintName);
	}
	UpdateBlueprintInfo();
	SendSettings();
}

//PILLAR SLOTS

void UARSConfigWidget::OnSlotEdited(int32 SlotIndex)
{
	if (bSuppressEvents || !mSlotWidgets.IsValidIndex(SlotIndex) || !mWorking.PillarSlots.IsValidIndex(SlotIndex))
	{
		return;
	}
	mWorking.PillarSlots[SlotIndex] = mSlotWidgets[SlotIndex]->GetSlot();
	UpdateBlueprintInfo();
	SendSettings();
}

void UARSConfigWidget::OnSlotBlueprintPicked(int32 SlotIndex, const FString& BlueprintName)
{
	RequestBlueprintSize(BlueprintName);
	OnSlotEdited(SlotIndex);
}

void UARSConfigWidget::OnSlotMoveRequested(int32 SlotIndex, int32 Delta)
{
	if (bSuppressEvents || !mWorking.MovePillarSlot(SlotIndex, Delta))
	{
		return;
	}
	RebuildPillarList();
	UpdateBlueprintInfo();
	SendSettings();
}

void UARSConfigWidget::OnSlotRemoveRequested(int32 SlotIndex)
{
	if (bSuppressEvents)
	{
		return;
	}
	mWorking.RemovePillarSlot(SlotIndex);
	RebuildPillarList();
	UpdateBlueprintInfo();
	SendSettings();
}

void UARSConfigWidget::OnAddPillarClicked()
{
	if (mWorking.AddPillarSlot(AARSSubsystem::GetMaxPillarSlots(this)) == INDEX_NONE)
	{
		return;
	}
	RebuildPillarList();
	UpdateBlueprintInfo();
	SendSettings();
}

//PRESETS

bool UARSConfigWidget::IsWorkingDirty() const
{
	if (mActivePresetName.IsEmpty())
	{
		return false;
	}
	FARSSettingsData Baseline = mPresetBaseline;
	Baseline.bEnabled = mWorking.bEnabled;
	for (int32 i = 0; i < Baseline.PillarSlots.Num() && i < mWorking.PillarSlots.Num(); i++)
	{
		Baseline.PillarSlots[i].bExpanded = mWorking.PillarSlots[i].bExpanded;
	}
	return !FARSSettingsData::StaticStruct()->CompareScriptStruct(&Baseline, &mWorking, PPF_None);
}


void UARSConfigWidget::UpdatePresetHeader()
{
	const AARSClientManager* Manager = GetClientManager();
	const int32 Count = Manager ? Manager->GetGlobalConfig().Presets.Num() : 0;
	if (PresetCountText)
	{
		PresetCountText->SetText(FText::Format(PresetCountFormat, FText::AsNumber(Count)));
	}
	if (PresetActiveNameText)
	{
		const FText ActiveName = mActivePresetName.IsEmpty()
			? NoPresetLoadedText : FText::FromString(mActivePresetName);
		if (!ActiveName.EqualTo(PresetActiveNameText->GetText()))
		{
			mNameMarqueeTime = 0.0f;
		}
		PresetActiveNameText->SetText(ActiveName);
	}
	if (PresetDirtyIndicator)
	{
		PresetDirtyIndicator->SetVisibility(IsWorkingDirty()
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	UpdateSaveButton();
}

void UARSConfigWidget::TickPresetNameMarquee(float DeltaTime)
{
	if (!PresetActiveNameText)
	{
		return;
	}
	const float Window = static_cast<float>(PresetActiveNameText->GetCachedGeometry().GetLocalSize().X);
	const float Overflow = static_cast<float>(PresetActiveNameText->GetDesiredSize().X) - Window;
	if (Window <= 0.0f || Overflow <= MarqueeSlack)
	{
		mNameMarqueeTime = 0.0f;
		PresetActiveNameText->SetRenderTranslation(FVector2D::ZeroVector);
		return;
	}

	const float Travel = Overflow / MarqueeSpeed;
	const float Cycle = 2.0f * (MarqueeHoldSeconds + Travel);
	mNameMarqueeTime = FMath::Fmod(mNameMarqueeTime + DeltaTime, Cycle);

	float Offset = 0.0f;
	if (mNameMarqueeTime > 2.0f * MarqueeHoldSeconds + Travel)
	{
		Offset = Overflow * (1.0f - (mNameMarqueeTime - 2.0f * MarqueeHoldSeconds - Travel) / Travel);
	}
	else if (mNameMarqueeTime > MarqueeHoldSeconds + Travel)
	{
		Offset = Overflow;
	}
	else if (mNameMarqueeTime > MarqueeHoldSeconds)
	{
		Offset = Overflow * (mNameMarqueeTime - MarqueeHoldSeconds) / Travel;
	}
	PresetActiveNameText->SetRenderTranslation(FVector2D(-Offset, 0.0));
}

void UARSConfigWidget::RebuildPresetList()
{
	const AARSClientManager* Manager = GetClientManager();
	if (!PresetListBox || !PresetRowWidgetClass || !Manager)
	{
		return;
	}
	const TArray<FARSPreset>& Presets = Manager->GetGlobalConfig().Presets;

	while (mPresetRows.Num() > Presets.Num())
	{
		if (UARSPresetRowWidget* Row = mPresetRows.Pop())
		{
			Row->RemoveFromParent();
		}
	}
	while (mPresetRows.Num() < Presets.Num())
	{
		UARSPresetRowWidget* Row = CreateWidget<UARSPresetRowWidget>(this, PresetRowWidgetClass);
		if (!Row)
		{
			break;
		}
		Row->OnPresetChosen.AddDynamic(this, &UARSConfigWidget::OnPresetRowChosen);
		Row->OnPresetRenamed.AddDynamic(this, &UARSConfigWidget::OnPresetRowRenamed);
		Row->OnPresetDeleteRequested.AddDynamic(this, &UARSConfigWidget::OnPresetRowDeleteRequested);
		Row->OnPresetRenameStarted.AddDynamic(this, &UARSConfigWidget::OnPresetRowRenameStarted);
		PresetListBox->AddChild(Row);
		mPresetRows.Add(Row);
	}

	const bool bDirty = IsWorkingDirty();
	for (int32 Index = 0; Index < mPresetRows.Num(); Index++)
	{
		if (UARSPresetRowWidget* Row = mPresetRows[Index])
		{
			Row->SetPreset(Presets[Index].Name, Index, Presets[Index].Name == mActivePresetName, bDirty);
		}
	}
	UpdatePresetHeader();
}

void UARSConfigWidget::SetPresetMenuOpen(bool bOpen)
{
	if (PresetDropdownPanel)
	{
		PresetDropdownPanel->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (PresetClickCatcher)
	{
		PresetClickCatcher->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (PresetExpandedChevron)
	{
		PresetExpandedChevron->SetVisibility(bOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (PresetCollapsedChevron)
	{
		PresetCollapsedChevron->SetVisibility(bOpen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (!bOpen)
	{
		CloseConfirmation();
		for (UARSPresetRowWidget* Row : mPresetRows)
		{
			if (Row) { Row->EndRename(); }
		}
	}
}

void UARSConfigWidget::AskConfirmation(EARSPresetConfirm Action, const FString& PresetName, const FText& Question)
{
	mConfirmAction = Action;
	mConfirmPresetName = PresetName;
	if (PresetConfirmText) { PresetConfirmText->SetText(Question); }
	if (PresetConfirmPanel) { PresetConfirmPanel->SetVisibility(ESlateVisibility::Visible); }
	if (PresetConfirmFlash) { PlayAnimation(PresetConfirmFlash); }
}

void UARSConfigWidget::CloseConfirmation()
{
	mConfirmAction = EARSPresetConfirm::None;
	mConfirmPresetName.Reset();
	if (PresetConfirmPanel) { PresetConfirmPanel->SetVisibility(ESlateVisibility::Collapsed); }
}

void UARSConfigWidget::LoadPreset(const FString& PresetName)
{
	AARSClientManager* Manager = GetClientManager();
	const FARSPreset* Preset = Manager ? Manager->FindPreset(PresetName) : nullptr;
	if (!Preset)
	{
		return;
	}
	const bool bKeepEnabled = mWorking.bEnabled;
	mWorking = Preset->Settings;
	mWorking.Normalize();
	mWorking.bEnabled = bKeepEnabled;
	for (FARSPillarSlotData& PillarSlot : mWorking.PillarSlots)
	{
		PillarSlot.bExpanded = false;
	}
	mActivePresetName = Preset->Name;
	mPresetBaseline = mWorking;
	SendSettings();
	PushWorkingToControls();
	RebuildPresetList();
}

//PRESET INPUT

void UARSConfigWidget::OnPresetHeaderClicked()
{
	const bool bOpen = !PresetDropdownPanel || PresetDropdownPanel->GetVisibility() == ESlateVisibility::Collapsed;
	if (bOpen)
	{
		RebuildPresetList();
	}
	SetPresetMenuOpen(bOpen);
}

void UARSConfigWidget::OnPresetClickCatcherClicked()
{
	SetPresetMenuOpen(false);
}

void UARSConfigWidget::OnPresetRowChosen(const FString& PresetName)
{
	if (PresetName == mActivePresetName)
	{
		return;
	}
	if (IsWorkingDirty())
	{
		AskConfirmation(EARSPresetConfirm::LoadOverUnsaved, PresetName, UnsavedChangesQuestion);
		return;
	}
	LoadPreset(PresetName);
}

void UARSConfigWidget::OnPresetRowRenamed(const FString& OldName, const FString& NewName)
{
	AARSClientManager* Manager = GetClientManager();
	if (!Manager || !Manager->RenamePreset(OldName, NewName))
	{
		RebuildPresetList();
		return;
	}
	if (mActivePresetName == OldName)
	{
		mActivePresetName = NewName;
	}
	RebuildPresetList();
}

void UARSConfigWidget::OnPresetRowDeleteRequested(const FString& PresetName)
{
	AskConfirmation(EARSPresetConfirm::Delete, PresetName,
		FText::Format(DeletePresetQuestion, FText::FromString(PresetName)));
}

void UARSConfigWidget::OnPresetRowRenameStarted(const FString& PresetName)
{
	for (UARSPresetRowWidget* Row : mPresetRows)
	{
		if (Row && Row->GetPresetName() != PresetName)
		{
			Row->EndRename();
		}
	}
}

void UARSConfigWidget::OnPresetSaveClicked()
{
	AARSClientManager* Manager = GetClientManager();
	if (!Manager)
	{
		return;
	}
	const FString Typed = PresetNameBox ? PresetNameBox->GetText().ToString().TrimStartAndEnd() : FString();
	const FString Name = Typed.IsEmpty() ? mActivePresetName : Typed;
	if (Name.IsEmpty())
	{
		return;
	}
	if (Name != mActivePresetName && Manager->FindPreset(Name))
	{
		AskConfirmation(EARSPresetConfirm::Overwrite, Name,
			FText::Format(OverwritePresetQuestion, FText::FromString(Name)));
		return;
	}
	SavePresetAs(Name);
}

void UARSConfigWidget::OnPresetNameTextChanged(const FText& Text)
{
	UpdateSaveButton();
}

void UARSConfigWidget::UpdateSaveButton()
{
	const FString Typed = PresetNameBox ? PresetNameBox->GetText().ToString().TrimStartAndEnd() : FString();
	const bool bSavingNew = !Typed.IsEmpty();
	if (PresetSaveButtonText)
	{
		PresetSaveButtonText->SetText(bSavingNew ? SaveNewPresetLabel : SavePresetLabel);
	}
	if (PresetSaveButton)
	{
		PresetSaveButton->SetIsEnabled(bSavingNew || !mActivePresetName.IsEmpty());
	}
}

void UARSConfigWidget::SavePresetAs(const FString& PresetName)
{
	AARSClientManager* Manager = GetClientManager();
	if (!Manager || PresetName.IsEmpty())
	{
		return;
	}
	Manager->SavePreset(PresetName, mWorking);
	mActivePresetName = PresetName;
	mPresetBaseline = mWorking;
	if (PresetNameBox) { PresetNameBox->SetText(FText::GetEmpty()); }
	RebuildPresetList();
}

void UARSConfigWidget::OnPresetResetClicked()
{
	const bool bKeepEnabled = mWorking.bEnabled;
	mWorking = FARSSettingsData();
	mWorking.Normalize();
	mWorking.bEnabled = bKeepEnabled;
	mActivePresetName.Reset();
	mPresetBaseline = mWorking;
	SendSettings();
	PushWorkingToControls();
	RebuildPresetList();
}

void UARSConfigWidget::OnPresetConfirmClicked()
{
	const EARSPresetConfirm Action = mConfirmAction;
	const FString Name = mConfirmPresetName;
	CloseConfirmation();

	switch (Action)
	{
	case EARSPresetConfirm::LoadOverUnsaved:
		LoadPreset(Name);
		break;
	case EARSPresetConfirm::Delete:
		if (AARSClientManager* Manager = GetClientManager())
		{
			Manager->DeletePreset(Name);
		}
		if (mActivePresetName == Name)
		{
			mActivePresetName.Reset();
		}
		RebuildPresetList();
		break;
	case EARSPresetConfirm::Overwrite:
		SavePresetAs(Name);
		break;
	default:
		break;
	}
}

void UARSConfigWidget::OnPresetCancelClicked()
{
	CloseConfirmation();
}
