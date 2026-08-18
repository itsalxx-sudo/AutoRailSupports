#include "ARSConfigWidget.h"

#include "ARSClientManager.h"
#include "ARSModConfigStruct.h"
#include "ARSRemoteCallObject.h"
#include "ARSSliderWidget.h"
#include "ARSSubsystem.h"
#include "FGBlueprintSubsystem.h"
#include "FGFactoryBlueprintTypes.h"
#include "UI/FGWindow.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

namespace
{
	const FLinearColor ARSAccent = FLinearColor(FColor(0x4C, 0xBB, 0x17));
	const FLinearColor ARSNeutral = FLinearColor(FColor(0x3A, 0x3A, 0x3A));
}

UARSConfigWidget::UARSConfigWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mUseKeyboard = true;
	mUseMouse = true;
	mDisablePlayerActions = true;
	mDisableBuildGunActions = true;
	mMouseLockMode = EMouseLockMode::DoNotLock;
	mRestoreFocusWhenLost = true;
	mDesiredHorizontalAlignment = HAlign_Center;
	mDesiredVerticalAlignment = VAlign_Center;
	SetIsFocusable(true);
}

void UARSConfigWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (HoverTitleText) { mDefaultHoverTitle = HoverTitleText->GetText(); }
	if (HoverDescriptionText) { mDefaultHoverDescription = HoverDescriptionText->GetText(); }

	mPillarCombos = { Pillar1Combo, Pillar2Combo, Pillar3Combo, Pillar4Combo };
	mPillarEverySpins = { Pillar1EverySpin, Pillar2EverySpin, Pillar3EverySpin, Pillar4EverySpin };
	mPillarSpacingSpins = { Pillar1SpacingSpin, Pillar2SpacingSpin, Pillar3SpacingSpin, Pillar4SpacingSpin };
	mPillarAlignChecks = { Pillar1InclineCheck, Pillar2InclineCheck, Pillar3InclineCheck, Pillar4InclineCheck };
	mPillarPriorityChecks = { Pillar1PriorityCheck, Pillar2PriorityCheck, Pillar3PriorityCheck, Pillar4PriorityCheck };
	mPillarLengthTexts = { Pillar1BlueprintLengthText, Pillar2BlueprintLengthText, Pillar3BlueprintLengthText, Pillar4BlueprintLengthText };
	mPillarWidthTexts = { Pillar1BlueprintWidthText, Pillar2BlueprintWidthText, Pillar3BlueprintWidthText, Pillar4BlueprintWidthText };
	mPillarHeightTexts = { Pillar1BlueprintHeightText, Pillar2BlueprintHeightText, Pillar3BlueprintHeightText, Pillar4BlueprintHeightText };
	mPillarYawSpins = { Pillar1YawSpin, Pillar2YawSpin, Pillar3YawSpin, Pillar4YawSpin };
	mPillarPitchSpins = { Pillar1PitchSpin, Pillar2PitchSpin, Pillar3PitchSpin, Pillar4PitchSpin };
	mPillarRollSpins = { Pillar1RollSpin, Pillar2RollSpin, Pillar3RollSpin, Pillar4RollSpin };
	mPillarVerticalSpins = { Pillar1VerticalSpin, Pillar2VerticalSpin, Pillar3VerticalSpin, Pillar4VerticalSpin };
	mPillarSidewaysSpins = { Pillar1SidewaysSpin, Pillar2SidewaysSpin, Pillar3SidewaysSpin, Pillar4SidewaysSpin };
	mPillarForwardSpins = { Pillar1ForwardSpin, Pillar2ForwardSpin, Pillar3ForwardSpin, Pillar4ForwardSpin };

	const FARSModConfigStruct ModConfig = FARSModConfigStruct::GetActiveConfig(this);

	auto ConfigureSlider = [&ModConfig](UARSSliderWidget* Slider, float Min, float Max, float Step, int32 Digits, bool bAllowFineStep = false)
	{
		if (Slider)
		{
			Slider->Configure(Min, Max, Step, Digits, bAllowFineStep,
				ModConfig.PositionStepTiny, ModConfig.PositionStepSmall, ModConfig.PositionStepLarge);
		}
	};
	auto ConfigureRotationSlider = [&ModConfig](UARSSliderWidget* Slider)
	{
		if (Slider)
		{
			Slider->Configure(-180.0f, 180.0f, 15.0f, 1, /*fine*/ true,
				ModConfig.RotationStepTiny, ModConfig.RotationStepSmall, ModConfig.RotationStepLarge);
		}
	};
	ConfigureSlider(UnderLengthSpin, 0.5f, 48.0f, 1.0f, 2, /*fine*/ true);
	ConfigureRotationSlider(UnderYawSpin);
	ConfigureRotationSlider(UnderPitchSpin);
	ConfigureRotationSlider(UnderRollSpin);
	ConfigureSlider(UnderVerticalSpin, -100.0f, 100.0f, 1.0f, 2, /*fine*/ true);
	ConfigureSlider(UnderSidewaysSpin, -100.0f, 100.0f, 1.0f, 2, /*fine*/ true);
	ConfigureSlider(UnderForwardSpin, -100.0f, 100.0f, 1.0f, 2, /*fine*/ true);
	for (UARSSliderWidget* EverySpin : mPillarEverySpins)
	{
		ConfigureSlider(EverySpin, 1.0f, 100.0f, 1.0f, 0);
	}
	for (UARSSliderWidget* SpacingSpin : mPillarSpacingSpins)
	{
		ConfigureSlider(SpacingSpin, 0.0f, 100.0f, 1.0f, 0);
	}
	for (int32 SlotIndex = 0; SlotIndex < FARSSettingsData::NumPillarSlots; SlotIndex++)
	{
		ConfigureRotationSlider(mPillarYawSpins[SlotIndex]);
		ConfigureRotationSlider(mPillarPitchSpins[SlotIndex]);
		ConfigureRotationSlider(mPillarRollSpins[SlotIndex]);
		ConfigureSlider(mPillarVerticalSpins[SlotIndex], -100.0f, 100.0f, 1.0f, 2, /*fine*/ true);
		ConfigureSlider(mPillarSidewaysSpins[SlotIndex], -100.0f, 100.0f, 1.0f, 2, /*fine*/ true);
		ConfigureSlider(mPillarForwardSpins[SlotIndex], -100.0f, 100.0f, 1.0f, 2, /*fine*/ true);
	}

	if (MasterButton) { MasterButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnMasterToggleClicked); }
	if (CloseButton) { CloseButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnCloseClicked); }

	if (UnderBlueprintCombo) { UnderBlueprintCombo->OnSelectionChanged.AddDynamic(this, &UARSConfigWidget::OnUnderBlueprintChanged); }
	if (UnderLengthSpin) { UnderLengthSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnUnderLengthCommitted); }
	if (UnderYawSpin) { UnderYawSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnUnderYawCommitted); }
	if (UnderPitchSpin) { UnderPitchSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnUnderPitchCommitted); }
	if (UnderRollSpin) { UnderRollSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnUnderRollCommitted); }
	if (UnderVerticalSpin) { UnderVerticalSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnUnderVOffsetCommitted); }
	if (UnderSidewaysSpin) { UnderSidewaysSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnUnderHOffsetCommitted); }
	if (UnderForwardSpin) { UnderForwardSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnUnderFOffsetCommitted); }
	if (UnderCoverGapCheck) { UnderCoverGapCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnUnderCoverGapChanged); }
	if (CombineBlueprintCheck) { CombineBlueprintCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnCombineBlueprintChanged); }

	if (Pillar1Combo) { Pillar1Combo->OnSelectionChanged.AddDynamic(this, &UARSConfigWidget::OnPillarBlueprintChanged0); }
	if (Pillar2Combo) { Pillar2Combo->OnSelectionChanged.AddDynamic(this, &UARSConfigWidget::OnPillarBlueprintChanged1); }
	if (Pillar3Combo) { Pillar3Combo->OnSelectionChanged.AddDynamic(this, &UARSConfigWidget::OnPillarBlueprintChanged2); }
	if (Pillar4Combo) { Pillar4Combo->OnSelectionChanged.AddDynamic(this, &UARSConfigWidget::OnPillarBlueprintChanged3); }
	if (Pillar1EverySpin) { Pillar1EverySpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarEveryCommitted0); }
	if (Pillar2EverySpin) { Pillar2EverySpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarEveryCommitted1); }
	if (Pillar3EverySpin) { Pillar3EverySpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarEveryCommitted2); }
	if (Pillar4EverySpin) { Pillar4EverySpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarEveryCommitted3); }
	if (Pillar1SpacingSpin) { Pillar1SpacingSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarSpacingCommitted0); }
	if (Pillar2SpacingSpin) { Pillar2SpacingSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarSpacingCommitted1); }
	if (Pillar3SpacingSpin) { Pillar3SpacingSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarSpacingCommitted2); }
	if (Pillar4SpacingSpin) { Pillar4SpacingSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarSpacingCommitted3); }
	if (Pillar1InclineCheck) { Pillar1InclineCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnPillarAlignChanged0); }
	if (Pillar2InclineCheck) { Pillar2InclineCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnPillarAlignChanged1); }
	if (Pillar3InclineCheck) { Pillar3InclineCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnPillarAlignChanged2); }
	if (Pillar4InclineCheck) { Pillar4InclineCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnPillarAlignChanged3); }
	if (Pillar1PriorityCheck) { Pillar1PriorityCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnPillarPriorityChanged0); }
	if (Pillar2PriorityCheck) { Pillar2PriorityCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnPillarPriorityChanged1); }
	if (Pillar3PriorityCheck) { Pillar3PriorityCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnPillarPriorityChanged2); }
	if (Pillar4PriorityCheck) { Pillar4PriorityCheck->OnCheckStateChanged.AddDynamic(this, &UARSConfigWidget::OnPillarPriorityChanged3); }

	if (Pillar1YawSpin) { Pillar1YawSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarYawCommitted0); }
	if (Pillar2YawSpin) { Pillar2YawSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarYawCommitted1); }
	if (Pillar3YawSpin) { Pillar3YawSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarYawCommitted2); }
	if (Pillar4YawSpin) { Pillar4YawSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarYawCommitted3); }
	if (Pillar1PitchSpin) { Pillar1PitchSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarPitchCommitted0); }
	if (Pillar2PitchSpin) { Pillar2PitchSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarPitchCommitted1); }
	if (Pillar3PitchSpin) { Pillar3PitchSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarPitchCommitted2); }
	if (Pillar4PitchSpin) { Pillar4PitchSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarPitchCommitted3); }
	if (Pillar1RollSpin) { Pillar1RollSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarRollCommitted0); }
	if (Pillar2RollSpin) { Pillar2RollSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarRollCommitted1); }
	if (Pillar3RollSpin) { Pillar3RollSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarRollCommitted2); }
	if (Pillar4RollSpin) { Pillar4RollSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarRollCommitted3); }
	if (Pillar1VerticalSpin) { Pillar1VerticalSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarVOffsetCommitted0); }
	if (Pillar2VerticalSpin) { Pillar2VerticalSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarVOffsetCommitted1); }
	if (Pillar3VerticalSpin) { Pillar3VerticalSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarVOffsetCommitted2); }
	if (Pillar4VerticalSpin) { Pillar4VerticalSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarVOffsetCommitted3); }
	if (Pillar1SidewaysSpin) { Pillar1SidewaysSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarHOffsetCommitted0); }
	if (Pillar2SidewaysSpin) { Pillar2SidewaysSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarHOffsetCommitted1); }
	if (Pillar3SidewaysSpin) { Pillar3SidewaysSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarHOffsetCommitted2); }
	if (Pillar4SidewaysSpin) { Pillar4SidewaysSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarHOffsetCommitted3); }
	if (Pillar1ForwardSpin) { Pillar1ForwardSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarFOffsetCommitted0); }
	if (Pillar2ForwardSpin) { Pillar2ForwardSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarFOffsetCommitted1); }
	if (Pillar3ForwardSpin) { Pillar3ForwardSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarFOffsetCommitted2); }
	if (Pillar4ForwardSpin) { Pillar4ForwardSpin->OnValueCommitted.AddDynamic(this, &UARSConfigWidget::OnPillarFOffsetCommitted3); }

	if (SavePresetButton) { SavePresetButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnSavePresetClicked); }
	if (LoadPresetButton) { LoadPresetButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnApplyPresetClicked); }
	if (DeletePresetButton) { DeletePresetButton->OnClicked.AddDynamic(this, &UARSConfigWidget::OnDeletePresetClicked); }


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

void UARSConfigWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshBlueprintOptions();
	RefreshPresetOptions();

	bHaveServerData = false;
	if (const AARSClientManager* ClientManager = GetClientManager())
	{
		if (ClientManager->HasServerSettings())
		{
			mWorking = ClientManager->GetSettings();
			mWorking.EnsureSlotArraySizes();
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
		mWorking.EnsureSlotArraySizes();
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

void UARSConfigWidget::UpdateBlueprintDimsText()
{
	UpdateDimsTextFor(mWorking.UnderRailBlueprintName, UnderBlueprintLengthText, UnderBlueprintWidthText, UnderBlueprintHeightText);
	for (int32 SlotIndex = 0; SlotIndex < FARSSettingsData::NumPillarSlots; SlotIndex++)
	{
		UpdateDimsTextFor(mWorking.PillarBlueprintNames[SlotIndex],
			mPillarLengthTexts.IsValidIndex(SlotIndex) ? mPillarLengthTexts[SlotIndex] : nullptr,
			mPillarWidthTexts.IsValidIndex(SlotIndex) ? mPillarWidthTexts[SlotIndex] : nullptr,
			mPillarHeightTexts.IsValidIndex(SlotIndex) ? mPillarHeightTexts[SlotIndex] : nullptr);
	}
}

void UARSConfigWidget::UpdateDimsTextFor(const FString& BlueprintName, UTextBlock* LengthText, UTextBlock* WidthText, UTextBlock* HeightText)
{
	if (!BlueprintName.IsEmpty() && !mMeasuredSizes.Contains(BlueprintName))
	{
		RequestBlueprintSize(BlueprintName);
	}
	FString Length = TEXT("-");
	FString Width = TEXT("-");
	FString Height = TEXT("-");
	if (!BlueprintName.IsEmpty())
	{
		if (const FVector* Size = mMeasuredSizes.Find(BlueprintName))
		{
			if (Size->IsNearlyZero())
			{
				Length = Width = Height = TEXT("?"); // measurement failed
			}
			else
			{
				Length = FString::Printf(TEXT("%.1f m"), Size->X);
				Width = FString::Printf(TEXT("%.1f m"), Size->Y);
				Height = FString::Printf(TEXT("%.1f m"), Size->Z);
			}
		}
		else
		{
			Length = Width = Height = TEXT("..."); // multiplayer, searching for the server's measurement
		}
	}
	auto SetDim = [](UTextBlock* Text, const TCHAR* Label, const FString& Value)
	{
		if (Text)
		{
			Text->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), Label, *Value)));
		}
	};
	SetDim(LengthText, TEXT("Length"), Length);
	SetDim(WidthText, TEXT("Width"), Width);
	SetDim(HeightText, TEXT("Height"), Height);
}

void UARSConfigWidget::RequestBlueprintSize(const FString& BlueprintName)
{
	if (BlueprintName.IsEmpty() || mRequestedSizeNames.Contains(BlueprintName))
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

	if (bAutoLengthOnSize && BlueprintName == mWorking.UnderRailBlueprintName && SizeMeters.X > 0.0f)
	{
		mWorking.UnderRailLength = SizeMeters.X * 100.0f;
		if (UnderLengthSpin)
		{
			UnderLengthSpin->SetValue(SizeMeters.X);
		}
		SendSettings();
		bAutoLengthOnSize = false;
	}
	UpdateBlueprintDimsText();
	UpdateLengthWarning();
}

void UARSConfigWidget::UpdateLengthWarning()
{
	if (!UnderLengthWarning)
	{
		return;
	}
	bool bWarn = false;
	if (!mWorking.UnderRailBlueprintName.IsEmpty())
	{
		float BlueprintLengthMeters = 0.0f;
		if (const FVector* Size = mMeasuredSizes.Find(mWorking.UnderRailBlueprintName))
		{
			BlueprintLengthMeters = Size->X;
		}
		bWarn = AARSSubsystem::ShouldWarnStampCount(mWorking.UnderRailLength, BlueprintLengthMeters);
	}
	UnderLengthWarning->SetVisibility(bWarn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bWarn)
	{
		UnderLengthWarning->SetToolTipText(AARSSubsystem::GetStampCountWarningText());
	}
}

void UARSConfigWidget::RefreshBlueprintOptions()
{
	TArray<FString> Options;
	Options.Add(TEXT("None"));
	TArray<UFGBlueprintDescriptor*> Descriptors;
	AFGBlueprintSubsystem::GetBlueprintDescriptors(Descriptors, const_cast<UARSConfigWidget*>(this));
	for (UFGBlueprintDescriptor* Descriptor : Descriptors)
	{
		if (Descriptor)
		{
			Options.Add(Descriptor->GetBlueprintNameAsString());
		}
	}
	Options.Sort([](const FString& A, const FString& B) { return A == TEXT("None") ? true : (B == TEXT("None") ? false : A < B); });

	auto FillCombo = [&Options](UComboBoxString* Combo)
	{
		if (!Combo)
		{
			return;
		}
		Combo->ClearOptions();
		for (const FString& Option : Options)
		{
			Combo->AddOption(Option);
		}
	};
	FillCombo(UnderBlueprintCombo);
	for (UComboBoxString* Combo : mPillarCombos)
	{
		FillCombo(Combo);
	}
}

void UARSConfigWidget::RefreshPresetOptions()
{
	if (!PresetCombo)
	{
		return;
	}
	const FString Previous = PresetCombo->GetSelectedOption();
	PresetCombo->ClearOptions();
	if (const AARSClientManager* Manager = GetClientManager())
	{
		for (const FARSPreset& Preset : Manager->GetGlobalConfig().Presets)
		{
			PresetCombo->AddOption(Preset.Name);
		}
	}
	if (!Previous.IsEmpty())
	{
		PresetCombo->SetSelectedOption(Previous);
	}
}

void UARSConfigWidget::SetMasterButtonState(bool bEnabled)
{
	if (MasterButtonText)
	{
		MasterButtonText->SetText(FText::FromString(bEnabled ? TEXT("ENABLED") : TEXT("DISABLED")));
	}
	if (MasterButton)
	{
		MasterButton->SetBackgroundColor(bEnabled ? ARSAccent : ARSNeutral);
	}
}

void UARSConfigWidget::PushWorkingToControls()
{
	TGuardValue<bool> Guard(bSuppressEvents, true);

	auto SelectBlueprint = [](UComboBoxString* Combo, const FString& Name)
	{
		if (!Combo)
		{
			return;
		}
		const FString Wanted = Name.IsEmpty() ? TEXT("None") : Name;
		if (Combo->FindOptionIndex(Wanted) == INDEX_NONE)
		{
			Combo->AddOption(Wanted); // blueprint is missing from this save, but keep the name visible
		}
		Combo->SetSelectedOption(Wanted);
	};
	auto SetSpin = [](UARSSliderWidget* Spin, float Value)
	{
		if (Spin)
		{
			Spin->SetValue(Value);
		}
	};

	SetMasterButtonState(mWorking.bEnabled);
	SelectBlueprint(UnderBlueprintCombo, mWorking.UnderRailBlueprintName);
	SetSpin(UnderLengthSpin, mWorking.UnderRailLength / 100.0f);
	SetSpin(UnderYawSpin, mWorking.UnderRailRotationOffset.Yaw);
	SetSpin(UnderPitchSpin, mWorking.UnderRailRotationOffset.Pitch);
	SetSpin(UnderRollSpin, mWorking.UnderRailRotationOffset.Roll);
	SetSpin(UnderVerticalSpin, mWorking.UnderRailZOffset / 100.0f);
	SetSpin(UnderSidewaysSpin, mWorking.UnderRailSideOffset / 100.0f);
	SetSpin(UnderForwardSpin, mWorking.UnderRailForwardOffset / 100.0f);
	if (UnderCoverGapCheck) { UnderCoverGapCheck->SetIsChecked(mWorking.bCoverEndGap); }
	if (CombineBlueprintCheck) { CombineBlueprintCheck->SetIsChecked(mWorking.bCombineIntoOneBlueprint); }

	for (int32 SlotIndex = 0; SlotIndex < FARSSettingsData::NumPillarSlots; SlotIndex++)
	{
		SelectBlueprint(mPillarCombos.IsValidIndex(SlotIndex) ? mPillarCombos[SlotIndex] : nullptr, mWorking.PillarBlueprintNames[SlotIndex]);
		SetSpin(mPillarEverySpins.IsValidIndex(SlotIndex) ? mPillarEverySpins[SlotIndex] : nullptr, mWorking.PillarEveryNs[SlotIndex]);
		SetSpin(mPillarSpacingSpins.IsValidIndex(SlotIndex) ? mPillarSpacingSpins[SlotIndex] : nullptr, mWorking.PillarSpacingOffsets[SlotIndex]);
		if (mPillarAlignChecks.IsValidIndex(SlotIndex) && mPillarAlignChecks[SlotIndex])
		{
			mPillarAlignChecks[SlotIndex]->SetIsChecked(mWorking.PillarAlignToRail[SlotIndex]);
		}
		if (mPillarPriorityChecks.IsValidIndex(SlotIndex) && mPillarPriorityChecks[SlotIndex])
		{
			mPillarPriorityChecks[SlotIndex]->SetIsChecked(mWorking.PillarPreventOverlap[SlotIndex]);
		}
		const FRotator PillarRotation = mWorking.GetPillarRotation(SlotIndex);
		SetSpin(mPillarYawSpins.IsValidIndex(SlotIndex) ? mPillarYawSpins[SlotIndex] : nullptr, PillarRotation.Yaw);
		SetSpin(mPillarPitchSpins.IsValidIndex(SlotIndex) ? mPillarPitchSpins[SlotIndex] : nullptr, PillarRotation.Pitch);
		SetSpin(mPillarRollSpins.IsValidIndex(SlotIndex) ? mPillarRollSpins[SlotIndex] : nullptr, PillarRotation.Roll);
		SetSpin(mPillarVerticalSpins.IsValidIndex(SlotIndex) ? mPillarVerticalSpins[SlotIndex] : nullptr, mWorking.PillarZOffsets[SlotIndex] / 100.0f);
		SetSpin(mPillarSidewaysSpins.IsValidIndex(SlotIndex) ? mPillarSidewaysSpins[SlotIndex] : nullptr, mWorking.PillarSideOffsets[SlotIndex] / 100.0f);
		SetSpin(mPillarForwardSpins.IsValidIndex(SlotIndex) ? mPillarForwardSpins[SlotIndex] : nullptr, mWorking.PillarForwardOffsets[SlotIndex] / 100.0f);
	}

	UpdateBlueprintDimsText();
	UpdateLengthWarning();
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
}

void UARSConfigWidget::RequestClose()
{
	CloseSelf();
}

void UARSConfigWidget::SetHoverInfo(const FText& Name, const FText& Description)
{
	if (HoverTitleText) { HoverTitleText->SetText(Name); }
	if (HoverDescriptionText) { HoverDescriptionText->SetText(Description); }
}

void UARSConfigWidget::ClearHoverInfo()
{
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

void UARSConfigWidget::OnUnderBlueprintChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bSuppressEvents || SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	mWorking.UnderRailBlueprintName = (SelectedItem == TEXT("None")) ? FString() : SelectedItem;
	bAutoLengthOnSize = !mWorking.UnderRailBlueprintName.IsEmpty();
	UpdateBlueprintDimsText();
	UpdateLengthWarning();
	SendSettings();
}

void UARSConfigWidget::OnUnderLengthCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.UnderRailLength = Value * 100.0f;
	UpdateLengthWarning();
	SendSettings();
}

void UARSConfigWidget::OnUnderYawCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.UnderRailRotationOffset.Yaw = Value;
	SendSettings();
}

void UARSConfigWidget::OnUnderPitchCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.UnderRailRotationOffset.Pitch = Value;
	SendSettings();
}

void UARSConfigWidget::OnUnderRollCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.UnderRailRotationOffset.Roll = Value;
	SendSettings();
}

void UARSConfigWidget::OnUnderVOffsetCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.UnderRailZOffset = Value * 100.0f;
	SendSettings();
}

void UARSConfigWidget::OnUnderHOffsetCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.UnderRailSideOffset = Value * 100.0f;
	SendSettings();
}

void UARSConfigWidget::OnUnderFOffsetCommitted(float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.UnderRailForwardOffset = Value * 100.0f;
	SendSettings();
}

void UARSConfigWidget::OnUnderCoverGapChanged(bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mWorking.bCoverEndGap = bIsChecked;
	SendSettings();
}

void UARSConfigWidget::OnCombineBlueprintChanged(bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mWorking.bCombineIntoOneBlueprint = bIsChecked;
	SendSettings();
}

void UARSConfigWidget::HandlePillarBlueprintChanged(int32 SlotIndex, const FString& SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bSuppressEvents || SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	mWorking.EnsureSlotArraySizes();
	mWorking.PillarBlueprintNames[SlotIndex] = (SelectedItem == TEXT("None")) ? FString() : SelectedItem;
	SendSettings();
}

void UARSConfigWidget::HandlePillarEveryCommitted(int32 SlotIndex, float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	mWorking.PillarEveryNs[SlotIndex] = FMath::Clamp(FMath::RoundToInt(Value), 1, 100);
	SendSettings();
}

void UARSConfigWidget::HandlePillarSpacingCommitted(int32 SlotIndex, float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	mWorking.PillarSpacingOffsets[SlotIndex] = FMath::Clamp(FMath::RoundToInt(Value), 0, 100);
	SendSettings();
}

void UARSConfigWidget::HandlePillarAlignChanged(int32 SlotIndex, bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	mWorking.PillarAlignToRail[SlotIndex] = bIsChecked;
	SendSettings();
}

void UARSConfigWidget::HandlePillarPriorityChanged(int32 SlotIndex, bool bIsChecked)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	mWorking.PillarPreventOverlap[SlotIndex] = bIsChecked;
	SendSettings();
}

void UARSConfigWidget::HandlePillarYawCommitted(int32 SlotIndex, float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	FRotator Rotation = mWorking.GetPillarRotation(SlotIndex);
	Rotation.Yaw = Value;
	mWorking.SetPillarRotation(SlotIndex, Rotation);
	SendSettings();
}

void UARSConfigWidget::HandlePillarPitchCommitted(int32 SlotIndex, float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	FRotator Rotation = mWorking.GetPillarRotation(SlotIndex);
	Rotation.Pitch = Value;
	mWorking.SetPillarRotation(SlotIndex, Rotation);
	SendSettings();
}

void UARSConfigWidget::HandlePillarRollCommitted(int32 SlotIndex, float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	FRotator Rotation = mWorking.GetPillarRotation(SlotIndex);
	Rotation.Roll = Value;
	mWorking.SetPillarRotation(SlotIndex, Rotation);
	SendSettings();
}

void UARSConfigWidget::HandlePillarVOffsetCommitted(int32 SlotIndex, float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	mWorking.PillarZOffsets[SlotIndex] = Value * 100.0f;
	SendSettings();
}

void UARSConfigWidget::HandlePillarHOffsetCommitted(int32 SlotIndex, float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	mWorking.PillarSideOffsets[SlotIndex] = Value * 100.0f;
	SendSettings();
}

void UARSConfigWidget::HandlePillarFOffsetCommitted(int32 SlotIndex, float Value)
{
	if (bSuppressEvents) { return; }
	mWorking.EnsureSlotArraySizes();
	mWorking.PillarForwardOffsets[SlotIndex] = Value * 100.0f;
	SendSettings();
}

void UARSConfigWidget::OnPillarBlueprintChanged0(FString SelectedItem, ESelectInfo::Type SelectionType) { HandlePillarBlueprintChanged(0, SelectedItem, SelectionType); }
void UARSConfigWidget::OnPillarBlueprintChanged1(FString SelectedItem, ESelectInfo::Type SelectionType) { HandlePillarBlueprintChanged(1, SelectedItem, SelectionType); }
void UARSConfigWidget::OnPillarBlueprintChanged2(FString SelectedItem, ESelectInfo::Type SelectionType) { HandlePillarBlueprintChanged(2, SelectedItem, SelectionType); }
void UARSConfigWidget::OnPillarBlueprintChanged3(FString SelectedItem, ESelectInfo::Type SelectionType) { HandlePillarBlueprintChanged(3, SelectedItem, SelectionType); }
void UARSConfigWidget::OnPillarEveryCommitted0(float Value) { HandlePillarEveryCommitted(0, Value); }
void UARSConfigWidget::OnPillarEveryCommitted1(float Value) { HandlePillarEveryCommitted(1, Value); }
void UARSConfigWidget::OnPillarEveryCommitted2(float Value) { HandlePillarEveryCommitted(2, Value); }
void UARSConfigWidget::OnPillarEveryCommitted3(float Value) { HandlePillarEveryCommitted(3, Value); }
void UARSConfigWidget::OnPillarSpacingCommitted0(float Value) { HandlePillarSpacingCommitted(0, Value); }
void UARSConfigWidget::OnPillarSpacingCommitted1(float Value) { HandlePillarSpacingCommitted(1, Value); }
void UARSConfigWidget::OnPillarSpacingCommitted2(float Value) { HandlePillarSpacingCommitted(2, Value); }
void UARSConfigWidget::OnPillarSpacingCommitted3(float Value) { HandlePillarSpacingCommitted(3, Value); }
void UARSConfigWidget::OnPillarAlignChanged0(bool bIsChecked) { HandlePillarAlignChanged(0, bIsChecked); }
void UARSConfigWidget::OnPillarAlignChanged1(bool bIsChecked) { HandlePillarAlignChanged(1, bIsChecked); }
void UARSConfigWidget::OnPillarAlignChanged2(bool bIsChecked) { HandlePillarAlignChanged(2, bIsChecked); }
void UARSConfigWidget::OnPillarAlignChanged3(bool bIsChecked) { HandlePillarAlignChanged(3, bIsChecked); }
void UARSConfigWidget::OnPillarPriorityChanged0(bool bIsChecked) { HandlePillarPriorityChanged(0, bIsChecked); }
void UARSConfigWidget::OnPillarPriorityChanged1(bool bIsChecked) { HandlePillarPriorityChanged(1, bIsChecked); }
void UARSConfigWidget::OnPillarPriorityChanged2(bool bIsChecked) { HandlePillarPriorityChanged(2, bIsChecked); }
void UARSConfigWidget::OnPillarPriorityChanged3(bool bIsChecked) { HandlePillarPriorityChanged(3, bIsChecked); }
void UARSConfigWidget::OnPillarYawCommitted0(float Value) { HandlePillarYawCommitted(0, Value); }
void UARSConfigWidget::OnPillarYawCommitted1(float Value) { HandlePillarYawCommitted(1, Value); }
void UARSConfigWidget::OnPillarYawCommitted2(float Value) { HandlePillarYawCommitted(2, Value); }
void UARSConfigWidget::OnPillarYawCommitted3(float Value) { HandlePillarYawCommitted(3, Value); }
void UARSConfigWidget::OnPillarPitchCommitted0(float Value) { HandlePillarPitchCommitted(0, Value); }
void UARSConfigWidget::OnPillarPitchCommitted1(float Value) { HandlePillarPitchCommitted(1, Value); }
void UARSConfigWidget::OnPillarPitchCommitted2(float Value) { HandlePillarPitchCommitted(2, Value); }
void UARSConfigWidget::OnPillarPitchCommitted3(float Value) { HandlePillarPitchCommitted(3, Value); }
void UARSConfigWidget::OnPillarRollCommitted0(float Value) { HandlePillarRollCommitted(0, Value); }
void UARSConfigWidget::OnPillarRollCommitted1(float Value) { HandlePillarRollCommitted(1, Value); }
void UARSConfigWidget::OnPillarRollCommitted2(float Value) { HandlePillarRollCommitted(2, Value); }
void UARSConfigWidget::OnPillarRollCommitted3(float Value) { HandlePillarRollCommitted(3, Value); }
void UARSConfigWidget::OnPillarVOffsetCommitted0(float Value) { HandlePillarVOffsetCommitted(0, Value); }
void UARSConfigWidget::OnPillarVOffsetCommitted1(float Value) { HandlePillarVOffsetCommitted(1, Value); }
void UARSConfigWidget::OnPillarVOffsetCommitted2(float Value) { HandlePillarVOffsetCommitted(2, Value); }
void UARSConfigWidget::OnPillarVOffsetCommitted3(float Value) { HandlePillarVOffsetCommitted(3, Value); }
void UARSConfigWidget::OnPillarHOffsetCommitted0(float Value) { HandlePillarHOffsetCommitted(0, Value); }
void UARSConfigWidget::OnPillarHOffsetCommitted1(float Value) { HandlePillarHOffsetCommitted(1, Value); }
void UARSConfigWidget::OnPillarHOffsetCommitted2(float Value) { HandlePillarHOffsetCommitted(2, Value); }
void UARSConfigWidget::OnPillarHOffsetCommitted3(float Value) { HandlePillarHOffsetCommitted(3, Value); }
void UARSConfigWidget::OnPillarFOffsetCommitted0(float Value) { HandlePillarFOffsetCommitted(0, Value); }
void UARSConfigWidget::OnPillarFOffsetCommitted1(float Value) { HandlePillarFOffsetCommitted(1, Value); }
void UARSConfigWidget::OnPillarFOffsetCommitted2(float Value) { HandlePillarFOffsetCommitted(2, Value); }
void UARSConfigWidget::OnPillarFOffsetCommitted3(float Value) { HandlePillarFOffsetCommitted(3, Value); }

void UARSConfigWidget::OnSavePresetClicked()
{
	AARSClientManager* Manager = GetClientManager();
	if (!Manager)
	{
		return;
	}
	FString Name = PresetNameBox ? PresetNameBox->GetText().ToString().TrimStartAndEnd() : FString();
	if (Name.IsEmpty() && PresetCombo)
	{
		Name = PresetCombo->GetSelectedOption();
	}
	if (Name.IsEmpty())
	{
		return;
	}
	Manager->SavePreset(Name, mWorking);
	RefreshPresetOptions();
	if (PresetCombo)
	{
		PresetCombo->SetSelectedOption(Name);
	}
	if (PresetNameBox)
	{
		PresetNameBox->SetText(FText::GetEmpty());
	}
}

void UARSConfigWidget::OnApplyPresetClicked()
{
	AARSClientManager* Manager = GetClientManager();
	if (!Manager || !PresetCombo)
	{
		return;
	}
	if (const FARSPreset* Preset = Manager->FindPreset(PresetCombo->GetSelectedOption()))
	{
		const bool bKeepEnabled = mWorking.bEnabled;
		mWorking = Preset->Settings;
		mWorking.EnsureSlotArraySizes();
		mWorking.bEnabled = bKeepEnabled;
		SendSettings();
		PushWorkingToControls();
	}
}

void UARSConfigWidget::OnDeletePresetClicked()
{
	AARSClientManager* Manager = GetClientManager();
	if (!Manager || !PresetCombo)
	{
		return;
	}
	if (Manager->DeletePreset(PresetCombo->GetSelectedOption()))
	{
		RefreshPresetOptions();
	}
}
