#include "ARSClientManager.h"

#include "ARSConfigWidget.h"
#include "ARSDebug.h"
#include "ARSGizmo.h"
#include "ARSInfoPanelWidget.h"
#include "ARSPreviewActor.h"
#include "ARSRemoteCallObject.h"
#include "ARSSubsystem.h"
#include "AbstractInstanceManager.h"
#include "FGBlueprintFunctionLibrary.h"
#include "FGBlueprintSubsystem.h"
#include "AkAudioEvent.h"
#include "AkGameplayTypes.h"
#include "Equipment/FGBuildGun.h"
#include "Equipment/FGBuildGunBuild.h"
#include "FGCharacterPlayer.h"
#include "Hologram/FGRailroadTrackHologram.h"
#include "FGHUD.h"
#include "FGPlayerController.h"
#include "UI/FGGameUI.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/SplineComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/FGInputMappingContext.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Patching/NativeHookManager.h"
#include "Subsystem/SubsystemActorManager.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"

static FText GetApplyHintActionText()
{
	return NSLOCTEXT("AutoRailSupports", "ARSAdjustHint", "ARS Adjustments");
}

static FText ARSKeyDisplayName(const FKey& Key)
{
	if (Key == EKeys::MouseScrollUp) { return NSLOCTEXT("AutoRailSupports", "ScrollUp", "Scroll Up"); }
	if (Key == EKeys::MouseScrollDown) { return NSLOCTEXT("AutoRailSupports", "ScrollDown", "Scroll Down"); }
	return Key.GetDisplayName();
}

AARSClientManager::AARSClientManager()
{
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

AARSClientManager* AARSClientManager::Get(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	if (USubsystemActorManager* SubsystemActorManager = World->GetSubsystem<USubsystemActorManager>())
	{
		return SubsystemActorManager->GetSubsystemActor<AARSClientManager>();
	}
	return nullptr;
}

namespace
{
	// Make the build-gun cost panel list the support materials too
	void RegisterHologramCostHook()
	{
		static bool bRegistered = false;
		if (bRegistered)
		{
			return;
		}
		bRegistered = true;

		SUBSCRIBE_METHOD(UFGBuildGunStateBuild::GetHologramCost, [](auto& Scope, const UFGBuildGunStateBuild* BuildState)
		{
			ARS_SCOPE_PERF(CostHook);
			if (!BuildState || !Cast<AFGRailroadTrackHologram>(BuildState->GetHologram()))
			{
				return;
			}

			if (const AFGBuildGun* BuildGun = BuildState->GetBuildGun())
			{
				if (const AFGCharacterPlayer* Owner = BuildGun->GetInstigatorCharacter())
				{
					if (!Owner->IsLocallyControlled())
					{
						return;
					}
				}
			}
			AARSClientManager* Manager = AARSClientManager::Get(BuildState->GetWorld());
			if (!Manager)
			{
				return;
			}
			const TArray<FItemAmount> SupportCost = Manager->GetPlannedSupportCost();
			if (SupportCost.Num() == 0)
			{
				return; // Mod off
			}

			TArray<FItemAmount> Combined = Scope(BuildState); // The rails own cost
			FARSGenerator::MergeItemAmounts(Combined, SupportCost);
			Scope.Override(Combined);
		});
	}
}

void AARSClientManager::Init()
{
	Super::Init();

	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	LoadGlobalConfig();
	TryBindInput();
	InjectAdjustHints();
	RegisterHologramCostHook();
	SetActorTickEnabled(true);
}

void AARSClientManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideModeHUD();
	DestroyRotationGizmo();
	if (mPreviewActor)
	{
		mPreviewActor->Destroy();
		mPreviewActor = nullptr;
	}
	for (const TPair<TObjectPtr<AFGCharacterPlayer>, TObjectPtr<AARSPreviewActor>>& Pair : mRemotePreviews)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->Destroy();
		}
	}
	mRemotePreviews.Reset();
	if (AFGPlayerController* PlayerController = mBoundController.Get())
	{
		PlayerController->ClearActionBindingsForObject(this);
		PlayerController->UnbindAllMappingContextsByObject(this);
	}
	Super::EndPlay(EndPlayReason);
}

AFGPlayerController* AARSClientManager::GetLocalController() const
{
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	return GameInstance ? Cast<AFGPlayerController>(GameInstance->GetFirstLocalPlayerController(GetWorld())) : nullptr;
}

void AARSClientManager::ApplyServerSettings(const FARSSettingsData& Settings)
{
	mSettings = Settings;
	mSettings.EnsureSlotArraySizes();
	mSettingsRevision++;
	bHaveServerSettings = true;
	if (mARSModeActive)
	{
		mWorking = mSettings;
	}
}

void AARSClientManager::UpdateSettingsRequest(float DeltaSeconds)
{
	if (bHaveServerSettings)
	{
		return;
	}

	mSettingsRequestCooldown -= DeltaSeconds;
	if (mSettingsRequestCooldown > 0.0f)
	{
		return;
	}
	mSettingsRequestCooldown = 1.0f;

	AFGPlayerController* PlayerController = GetLocalController();
	mSettingsIdentityWait += 1.0f;
	if (!AARSSubsystem::IsPlayerIdentityReady(PlayerController ? PlayerController->PlayerState : nullptr)
		&& mSettingsIdentityWait < 15.0f)
	{
		return;
	}
	if (UARSRemoteCallObject* RCO = GetRCO())
	{
		RCO->Server_RequestSettings();
	}
}

UARSRemoteCallObject* AARSClientManager::GetRCO() const
{
	if (AFGPlayerController* PlayerController = GetLocalController())
	{
		return PlayerController->GetRemoteCallObjectOfClass<UARSRemoteCallObject>();
	}
	return nullptr;
}

void AARSClientManager::TryBindInput()
{
	AFGPlayerController* PlayerController = GetLocalController();
	if (!PlayerController || !PlayerController->HasActorBegunPlay())
	{
		GetWorldTimerManager().SetTimer(mBindRetryTimer, this, &AARSClientManager::TryBindInput, 0.5f, false);
		return;
	}
	RebuildInputBindings(PlayerController);
}

void AARSClientManager::RebuildInputBindings(AFGPlayerController* PlayerController)
{
	if (!PlayerController)
	{
		PlayerController = GetLocalController();
	}
	if (!PlayerController)
	{
		return;
	}

	// Drop any previous registration
	PlayerController->ClearActionBindingsForObject(this);
	PlayerController->UnbindAllMappingContextsByObject(this);
	mContextHandle = FBoundMappingContextHandle();

	if (!mMappingContext)
	{
		mMappingContext = LoadObject<UFGInputMappingContext>(nullptr, TEXT("/AutoRailSupports/Input/IMC_ARS.IMC_ARS"));
		mOpenUIAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_OpenMenu.IA_ARS_OpenMenu"));
		mApplyAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_ApplyRailSupports.IA_ARS_ApplyRailSupports"));
		mToggleAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_ToggleEnabled.IA_ARS_ToggleEnabled"));
	}
	if (!mMappingContext || !mOpenUIAction || !mApplyAction || !mToggleAction)
	{
		return;
	}

	mContextHandle = PlayerController->BindMappingContext(mMappingContext, PlayerController->GetMappingContextPriority() + 1, this);

	FFGRuntimeInputActionDelegate OpenDelegate;
	OpenDelegate.BindDynamic(this, &AARSClientManager::OnOpenUIAction);
	PlayerController->BindToAction(mOpenUIAction, ETriggerEvent::Started, OpenDelegate);

	FFGRuntimeInputActionDelegate ApplyDelegate;
	ApplyDelegate.BindDynamic(this, &AARSClientManager::OnApplyAction);
	PlayerController->BindToAction(mApplyAction, ETriggerEvent::Started, ApplyDelegate);

	FFGRuntimeInputActionDelegate ToggleDelegate;
	ToggleDelegate.BindDynamic(this, &AARSClientManager::OnToggleAction);
	PlayerController->BindToAction(mToggleAction, ETriggerEvent::Started, ToggleDelegate);

	// reset ARS Mode to a clean state
	mARSModeActive = false;
	mModeSharedHandle = FBoundMappingContextHandle();
	mModeDismantleHandle = FBoundMappingContextHandle();

	if (!mInputAssetsLoaded)
	{
		mInputAssetsLoaded = true;
		mToggleModeAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_ToggleMode.IA_ARS_ToggleMode"));
		mPrevPartAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_PrevPart.IA_ARS_PrevPart"));
		mNextPartAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_NextPart.IA_ARS_NextPart"));
		mCycleSettingsModeAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_CycleSettingsMode.IA_ARS_CycleSettingsMode"));
		mCycleRotationAxisAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_CycleRotationAxis.IA_ARS_CycleRotationAxis"));
		mAdjustForwardAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_AdjustForward.IA_ARS_AdjustForward"));
		mAdjustBackwardAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_AdjustBackward.IA_ARS_AdjustBackward"));
		mAdjustLeftAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_AdjustLeft.IA_ARS_AdjustLeft"));
		mAdjustRightAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_AdjustRight.IA_ARS_AdjustRight"));
		mVerticalUpAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_VerticalUp.IA_ARS_VerticalUp"));
		mVerticalDownAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_VerticalDown.IA_ARS_VerticalDown"));
		mRotateUpAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_RotateUp.IA_ARS_RotateUp"));
		mRotateDownAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_RotateDown.IA_ARS_RotateDown"));
		mToggleHologramLockAction = LoadObject<UInputAction>(nullptr, TEXT("/AutoRailSupports/Input/IA_ARS_ToggleHologramLock.IA_ARS_ToggleHologramLock"));
		mModeSharedContext = LoadObject<UFGInputMappingContext>(nullptr, TEXT("/AutoRailSupports/Input/IMC_ARS_Mode_Shared.IMC_ARS_Mode_Shared"));
		mModeDismantleContext = LoadObject<UFGInputMappingContext>(nullptr, TEXT("/AutoRailSupports/Input/IMC_ARS_Mode_Dismantle.IMC_ARS_Mode_Dismantle"));
	}

	if (mToggleModeAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnToggleModeAction); PlayerController->BindToAction(mToggleModeAction, ETriggerEvent::Started, D); }
	if (mPrevPartAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnPrevPartAction); PlayerController->BindToAction(mPrevPartAction, ETriggerEvent::Started, D); }
	if (mNextPartAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnNextPartAction); PlayerController->BindToAction(mNextPartAction, ETriggerEvent::Started, D); }
	if (mCycleSettingsModeAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnCycleSettingsModeAction); PlayerController->BindToAction(mCycleSettingsModeAction, ETriggerEvent::Started, D); }
	if (mCycleRotationAxisAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnCycleRotationAxisAction); PlayerController->BindToAction(mCycleRotationAxisAction, ETriggerEvent::Started, D); }
	if (mAdjustForwardAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnAdjustForwardAction); PlayerController->BindToAction(mAdjustForwardAction, ETriggerEvent::Started, D); }
	if (mAdjustBackwardAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnAdjustBackwardAction); PlayerController->BindToAction(mAdjustBackwardAction, ETriggerEvent::Started, D); }
	if (mAdjustLeftAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnAdjustLeftAction); PlayerController->BindToAction(mAdjustLeftAction, ETriggerEvent::Started, D); }
	if (mAdjustRightAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnAdjustRightAction); PlayerController->BindToAction(mAdjustRightAction, ETriggerEvent::Started, D); }
	if (mVerticalUpAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnVerticalUpAction); PlayerController->BindToAction(mVerticalUpAction, ETriggerEvent::Started, D); }
	if (mVerticalDownAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnVerticalDownAction); PlayerController->BindToAction(mVerticalDownAction, ETriggerEvent::Started, D); }
	if (mRotateUpAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnRotateUpAction); PlayerController->BindToAction(mRotateUpAction, ETriggerEvent::Started, D); }
	if (mRotateDownAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnRotateDownAction); PlayerController->BindToAction(mRotateDownAction, ETriggerEvent::Started, D); }
	if (mToggleHologramLockAction) { FFGRuntimeInputActionDelegate D; D.BindDynamic(this, &AARSClientManager::OnToggleHologramLockAction); PlayerController->BindToAction(mToggleHologramLockAction, ETriggerEvent::Started, D); }

	mBoundController = PlayerController;
}

bool AARSClientManager::IsLocalPlayerDismantling() const
{
	AFGPlayerController* PlayerController = GetLocalController();
	AFGCharacterPlayer* Character = PlayerController ? Cast<AFGCharacterPlayer>(PlayerController->GetPawn()) : nullptr;
	AFGBuildGun* BuildGun = Character ? Character->GetBuildGun() : nullptr;
	return BuildGun && BuildGun->IsEquipped() && BuildGun->IsInState(EBuildGunState::BGS_DISMANTLE);
}

void AARSClientManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateSettingsRequest(DeltaSeconds);
	UpdateARSModeValidity();
	if (mARSModeActive)
	{
		mHologramLocked = IsActiveRailLocked();
		RefreshInfoPanel();
		ApplyModeIndicator();
		if (mARSModeGun == EARSModeGun::Dismantle)
		{
			ApplyDismantleDressing();
		}
		const uint32 HintSig = (static_cast<uint32>(mARSModeGun) << 5)
			| (static_cast<uint32>(mRotationAxis) << 3)
			| (GetCurrentSettingsMode() == EARSSettingsMode::General ? 2u : 0u)
			| (mHologramLocked ? 1u : 0u);
		if (HintSig != mLastHintSig || !mHintBarWidget.IsValid())
		{
			mLastHintSig = HintSig;
			mHintReinjectFramesLeft = 8;
		}
		if (mHintReinjectFramesLeft > 0)
		{
			--mHintReinjectFramesLeft;
			ApplyARSHintsToBar();
		}
	}

	EnsureBuildGunAdjustHint();
	UpdateRailPreview();
	UpdateRemotePreviews();
	UpdateRotationGizmo();
	ARSDebug::TickPerfReport();
}

AFGHologram* AARSClientManager::GetActiveRailHologram()
{
	AFGPlayerController* PlayerController = GetLocalController();
	AFGCharacterPlayer* Character = PlayerController ? Cast<AFGCharacterPlayer>(PlayerController->GetPawn()) : nullptr;
	AFGBuildGun* BuildGun = Character ? Character->GetBuildGun() : nullptr;
	if (!BuildGun || !BuildGun->IsEquipped() || !BuildGun->IsInState(EBuildGunState::BGS_BUILD))
	{
		return nullptr;
	}
	UFGBuildGunStateBuild* BuildState = Cast<UFGBuildGunStateBuild>(BuildGun->GetBuildGunStateFor(EBuildGunState::BGS_BUILD));
	AFGHologram* Hologram = BuildState ? BuildState->GetHologram() : nullptr;

	// only rails get supports, so only preview while a rail is being placed
	return Cast<AFGRailroadTrackHologram>(Hologram) ? Hologram : nullptr;
}

void AARSClientManager::UpdateRailPreview()
{
	if (!bHaveServerSettings)
	{
		return;
	}
	// Build gun: preview while the mod is on and a rail is actually being placed
	const bool bEnabled = mSettings.bEnabled;
	AFGHologram* RailHologram = bEnabled ? GetActiveRailHologram() : nullptr;

	// fix: turning auto back on has to rebuild even while the hologram is locked
	const bool bEnabledChanged = (bEnabled != mLastPreviewEnabled);
	mLastPreviewEnabled = bEnabled;

	AFGBuildableRailroadTrack* AimedTrack = nullptr;
	if (!RailHologram && mARSModeActive && mARSModeGun == EARSModeGun::Dismantle)
	{
		AimedTrack = mDismantleLockedTrack.IsValid() ? mDismantleLockedTrack.Get() : TraceAimedRail(GetLocalController());
	}

	if (!RailHologram && !AimedTrack)
	{
		if (mPreviewActor)
		{
			mPreviewActor->ClearPreview();
		}
		mPlannedSupportCost.Reset();

		PushPreviewSource(nullptr);
		PushPreviewSpline(nullptr, 0);
		return;
	}

	if (RailHologram)
	{
		PushPreviewSource(nullptr);
		PushPreviewSpline(RailHologram->FindComponentByClass<USplineComponent>(),
			AARSPreviewActor::GetHologramStencil(RailHologram));
	}
	else
	{
		PushPreviewSpline(nullptr, 0);
		PushPreviewSource(AimedTrack);
	}

	if (!mPreviewActor)
	{
		mPreviewActor = GetWorld()->SpawnActor<AARSPreviewActor>();
	}
	if (mPreviewActor)
	{
		if (RailHologram)
		{
			mPreviewActor->UpdateFromHologram(RailHologram, mSettings, mARSModeActive || bEnabledChanged, mARSModeActive);
		}
		else
		{
			mPreviewActor->UpdateFromTrack(AimedTrack, mSettings, mARSModeActive);
		}
		// Mirror the preview's cost so the GetHologramCost hook can surface it in the build-gun panel
		mPlannedSupportCost = mPreviewActor->GetPlannedCost();
	}
}

void AARSClientManager::PushPreviewSource(AActor* Source)
{
	if (mPushedPreviewSource.Get() == Source)
	{
		return;
	}
	mPushedPreviewSource = Source;
	if (UARSRemoteCallObject* RCO = GetRCO())
	{
		RCO->Server_SetPreviewSource(Source);
	}
}

void AARSClientManager::PushPreviewSpline(USplineComponent* Spline, uint8 Stencil)
{
	const double Now = FPlatformTime::Seconds();
	const bool bStopping = (Spline == nullptr);
	if (!bStopping && Now - mLastPreviewSplineSent < PreviewSplineSendInterval)
	{
		return;
	}
	if (bStopping && !mSendingPreviewSpline)
	{
		return;
	}
	mLastPreviewSplineSent = Now;
	mSendingPreviewSpline = !bStopping;

	TArray<FARSSplinePoint> Points;
	if (Spline)
	{
		const int32 NumPoints = Spline->GetNumberOfSplinePoints();
		Points.Reserve(NumPoints);
		for (int32 i = 0; i < NumPoints; i++)
		{
			FARSSplinePoint& Point = Points.AddDefaulted_GetRef();
			Point.Location = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			Point.ArriveTangent = Spline->GetArriveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
			Point.LeaveTangent = Spline->GetLeaveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
		}
	}
	if (UARSRemoteCallObject* RCO = GetRCO())
	{
		RCO->Server_SetPreviewSpline(Points, Stencil);
	}
}

void AARSClientManager::UpdateRemotePreviews()
{
	AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld());
	if (!Subsystem)
	{
		return;
	}
	AFGPlayerController* PlayerController = GetLocalController();
	const AFGCharacterPlayer* LocalCharacter = PlayerController ? Cast<AFGCharacterPlayer>(PlayerController->GetPawn()) : nullptr;

	TSet<TObjectPtr<AFGCharacterPlayer>> StillPreviewing;
	for (const FARSRemotePreview& Remote : Subsystem->GetActivePreviews())
	{
		const bool bHasSource = IsValid(Remote.Source) || Remote.SplinePoints.Num() >= 2;
		if (!IsValid(Remote.Player) || !bHasSource || Remote.Player == LocalCharacter)
		{
			continue;
		}
		StillPreviewing.Add(Remote.Player);

		TObjectPtr<AARSPreviewActor>& Preview = mRemotePreviews.FindOrAdd(Remote.Player);
		if (!Preview)
		{
			Preview = GetWorld()->SpawnActor<AARSPreviewActor>();
		}
		if (!Preview)
		{
			continue;
		}
		if (AFGBuildableRailroadTrack* Track = Cast<AFGBuildableRailroadTrack>(Remote.Source.Get()))
		{
			Preview->UpdateFromTrack(Track, Remote.Settings, /*bCollectAnchors*/ false);
		}
		else
		{
			Preview->UpdateFromSplinePoints(Remote.SplinePoints, Remote.Settings, Remote.Stencil);
		}
	}

	for (auto It = mRemotePreviews.CreateIterator(); It; ++It)
	{
		if (StillPreviewing.Contains(It.Key()))
		{
			continue;
		}
		if (IsValid(It.Value()))
		{
			It.Value()->Destroy();
		}
		It.RemoveCurrent();
	}
}

void AARSClientManager::OnOpenUIAction()
{
	OpenUI();
}

void AARSClientManager::OnApplyAction()
{
	if (mARSModeActive && IsLocalPlayerDismantling())
	{
		PlayApplySound();
		TriggerApplyAimedRail();
	}
}

void AARSClientManager::OnToggleAction()
{
	TriggerToggleEnabled();
}

static void ARSPatchAdjustHint(UObject* WidgetObject)
{
	if (!WidgetObject)
	{
		return;
	}
	FArrayProperty* HintsProperty = FindFProperty<FArrayProperty>(WidgetObject->GetClass(), TEXT("mStandardKeybinds"));
	FStructProperty* EntryProperty = HintsProperty ? CastField<FStructProperty>(HintsProperty->Inner) : nullptr;
	if (!EntryProperty)
	{
		return;
	}
	FTextProperty* ActionProperty = nullptr;
	FTextProperty* KeyBindingProperty = nullptr;
	for (TFieldIterator<FProperty> It(EntryProperty->Struct); It; ++It)
	{
		if (FTextProperty* TextProperty = CastField<FTextProperty>(*It))
		{
			if (TextProperty->GetName().StartsWith(TEXT("Action"))) { ActionProperty = TextProperty; }
			else if (TextProperty->GetName().StartsWith(TEXT("KeyBinding"))) { KeyBindingProperty = TextProperty; }
		}
	}
	if (!ActionProperty || !KeyBindingProperty)
	{
		return;
	}
	const FText ActionText = GetApplyHintActionText();
	FScriptArrayHelper Entries(HintsProperty, HintsProperty->ContainerPtrToValuePtr<void>(WidgetObject));
	for (int32 Index = 0; Index < Entries.Num(); Index++)
	{
		if (ActionProperty->GetPropertyValue_InContainer(Entries.GetRawPtr(Index)).EqualTo(ActionText))
		{
			return; // Already patched
		}
	}
	const int32 NewIndex = Entries.AddValue();
	uint8* NewEntry = Entries.GetRawPtr(NewIndex);
	ActionProperty->SetPropertyValue_InContainer(NewEntry, ActionText);
	KeyBindingProperty->SetPropertyValue_InContainer(NewEntry, FText::FromString(TEXT("{ARS.ToggleMode}")));
}

void AARSClientManager::InjectAdjustHints()
{
	if (bHintInjectionAttempted)
	{
		return;
	}
	bHintInjectionAttempted = true;

	if (!mDismantleWidgetClass)
	{
		mDismantleWidgetClass = LoadClass<UFGInteractWidget>(nullptr, TEXT("/Game/FactoryGame/Interface/UI/InGame/Widget_DismantleMode.Widget_DismantleMode_C"));
	}
	if (UWidgetBlueprintGeneratedClass* DismantleClass = Cast<UWidgetBlueprintGeneratedClass>(mDismantleWidgetClass.Get()))
	{
		if (UWidgetTree* TemplateTree = DismantleClass->GetWidgetTreeArchetype())
		{
			ARSPatchAdjustHint(TemplateTree->FindWidget(FName(TEXT("Widget_DismantleRefunds"))));
		}
	}
}

UWidget* AARSClientManager::FindLiveDismantleRefundsWidget()
{
	UFGGameUI* GameUI = GetGameUI();
	if (!GameUI)
	{
		return nullptr;
	}
	if (!mDismantleWidgetClass)
	{
		mDismantleWidgetClass = LoadClass<UFGInteractWidget>(nullptr, TEXT("/Game/FactoryGame/Interface/UI/InGame/Widget_DismantleMode.Widget_DismantleMode_C"));
	}
	UFGInteractWidget* DismantleWidget = mDismantleWidgetClass ? GameUI->GetInteractWidgetOfClass(mDismantleWidgetClass) : nullptr;
	if (!DismantleWidget || !DismantleWidget->WidgetTree)
	{
		return nullptr;
	}
	return DismantleWidget->WidgetTree->FindWidget(FName(TEXT("Widget_DismantleRefunds")));
}

static const TCHAR* const ARSApplySoundPath =
	TEXT("/Game/WwiseAudio/Events/Interface/Hologram/Play_UI_Hologram_HalfPlaced.Play_UI_Hologram_HalfPlaced");
static const TCHAR* const ARSCycleModeSoundPath =
	TEXT("/Game/WwiseAudio/Events/Interface/BuildMenu/Play_UI_BuildGun_SwitchState.Play_UI_BuildGun_SwitchState");

void AARSClientManager::PlayUISound(TObjectPtr<UAkAudioEvent>& InOutEvent, bool& bInOutLoaded, const TCHAR* EventPath)
{
	if (!bInOutLoaded)
	{
		bInOutLoaded = true;
		InOutEvent = LoadObject<UAkAudioEvent>(nullptr, EventPath);
	}
	AFGPlayerController* PlayerController = GetLocalController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (InOutEvent && Pawn)
	{
		InOutEvent->PostOnActor(Pawn, FOnAkPostEventCallback(), 0, false);
	}
}

void AARSClientManager::PlayApplySound()
{
	PlayUISound(mApplySound, mApplyAudioLoaded, ARSApplySoundPath);
}

void AARSClientManager::PlayCycleModeSound()
{
	PlayUISound(mCycleModeSound, mCycleModeAudioLoaded, ARSCycleModeSoundPath);
}

void AARSClientManager::TriggerApplyAimedRail()
{
	AFGPlayerController* PlayerController = GetLocalController();
	UARSRemoteCallObject* RCO = GetRCO();
	if (!PlayerController || !RCO)
	{
		return;
	}
	// A locked target applies to that rail even if the crosshair has drifted off it
	AFGBuildableRailroadTrack* Track = mDismantleLockedTrack.IsValid() ? mDismantleLockedTrack.Get() : TraceAimedRail(PlayerController);
	if (Track)
	{
		RCO->Server_ApplyAimedRail(Track);
	}
	else
	{
		RCO->Client_Notify(TEXT("Auto Rail Supports: aim at a railway track and try again."), /*bConfirmation*/ false);
	}
}

void AARSClientManager::TriggerToggleEnabled()
{
	UARSRemoteCallObject* RCO = GetRCO();
	if (!RCO || !bHaveServerSettings)
	{
		return;
	}
	RCO->Server_SetEnabled(!mSettings.bEnabled);
}

AFGBuildableRailroadTrack* AARSClientManager::TraceAimedRail(AFGPlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return nullptr;
	}
	UWorld* World = PlayerController->GetWorld();

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARSApplyTrace), /*bTraceComplex*/ true);
	QueryParams.AddIgnoredActor(PlayerController->GetPawn());

	FHitResult Hit;
	constexpr float TraceRange = 50000.0f; // 500 m
	World->LineTraceSingleByChannel(Hit, ViewLocation, ViewLocation + ViewRotation.Vector() * TraceRange, ECC_Visibility, QueryParams);

	AFGBuildableRailroadTrack* Track = Cast<AFGBuildableRailroadTrack>(Hit.GetActor());
	if (!Track)
	{
		if (AAbstractInstanceManager* InstanceManager = AAbstractInstanceManager::GetInstanceManager(World))
		{
			FInstanceHandle Handle;
			if (InstanceManager->ResolveHit(Hit, Handle))
			{
				Track = Handle.GetOwner<AFGBuildableRailroadTrack>();
			}
		}
	}
	return Track;
}

TSubclassOf<UARSConfigWidget> AARSClientManager::GetConfigWidgetClass()
{
	if (!mConfigWidgetClass)
	{
		mConfigWidgetClass = LoadClass<UARSConfigWidget>(nullptr, TEXT("/AutoRailSupports/Interface/Widget_ARSConfig.Widget_ARSConfig_C"));
	}
	return mConfigWidgetClass;
}

void AARSClientManager::OpenUI()
{
	AFGPlayerController* PlayerController = GetLocalController();
	if (!PlayerController)
	{
		return;
	}
	AFGHUD* HUD = PlayerController->GetHUD<AFGHUD>();
	UFGGameUI* GameUI = HUD ? HUD->GetGameUI() : nullptr;
	TSubclassOf<UARSConfigWidget> WidgetClass = GetConfigWidgetClass();
	if (!GameUI || !WidgetClass)
	{
		return;
	}
	if (GameUI->ContainsInteractWidgetOfClass(WidgetClass))
	{
		return;
	}
	HUD->OpenInteractUI(WidgetClass, this);
}

UFGGameUI* AARSClientManager::GetGameUI() const
{
	AFGPlayerController* PlayerController = GetLocalController();
	AFGHUD* HUD = PlayerController ? PlayerController->GetHUD<AFGHUD>() : nullptr;
	return HUD ? HUD->GetGameUI() : nullptr;
}

void AARSClientManager::CloseUI()
{
	UFGGameUI* GameUI = GetGameUI();
	if (!GameUI || !mConfigWidgetClass)
	{
		return;
	}
	if (UFGInteractWidget* Widget = GameUI->GetInteractWidgetOfClass(mConfigWidgetClass))
	{
		if (!GameUI->PopWidget(Widget))
		{
			GameUI->RemoveInteractWidget(Widget);
		}
	}
}

UARSConfigWidget* AARSClientManager::GetOpenConfigWidget() const
{
	UFGGameUI* GameUI = GetGameUI();
	if (!GameUI || !mConfigWidgetClass)
	{
		return nullptr;
	}
	return Cast<UARSConfigWidget>(GameUI->GetInteractWidgetOfClass(mConfigWidgetClass));
}

void AARSClientManager::SavePreset(const FString& Name, const FARSSettingsData& Settings)
{
	const FString Trimmed = Name.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return;
	}
	for (FARSPreset& Preset : mConfig.Presets)
	{
		if (Preset.Name.Equals(Trimmed, ESearchCase::IgnoreCase))
		{
			Preset.Settings = Settings;
			SaveGlobalConfig();
			return;
		}
	}
	FARSPreset& NewPreset = mConfig.Presets.AddDefaulted_GetRef();
	NewPreset.Name = Trimmed;
	NewPreset.Settings = Settings;
	SaveGlobalConfig();
}

bool AARSClientManager::DeletePreset(const FString& Name)
{
	const int32 Removed = mConfig.Presets.RemoveAll([&Name](const FARSPreset& Preset)
	{
		return Preset.Name.Equals(Name, ESearchCase::IgnoreCase);
	});
	if (Removed > 0)
	{
		SaveGlobalConfig();
		return true;
	}
	return false;
}

const FARSPreset* AARSClientManager::FindPreset(const FString& Name) const
{
	return mConfig.Presets.FindByPredicate([&Name](const FARSPreset& Preset)
	{
		return Preset.Name.Equals(Name, ESearchCase::IgnoreCase);
	});
}

FString AARSClientManager::GetConfigFilePath()
{
	return FPaths::ProjectDir() / TEXT("Configs") / TEXT("AutoRailSupports.json");
}

void AARSClientManager::LoadGlobalConfig()
{
	mConfig = FARSGlobalConfig();
	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *GetConfigFilePath()))
	{
		FARSGlobalConfig Loaded;
		if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Loaded))
		{
			mConfig = Loaded;
			for (FARSPreset& Preset : mConfig.Presets)
			{
				Preset.Settings.EnsureSlotArraySizes();
			}
		}
		else
		{
			UE_LOG(LogARSClient, Warning, TEXT("Failed to parse %s, using defaults"), *GetConfigFilePath());
		}
	}
}

void AARSClientManager::SaveGlobalConfig() const
{
	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(mConfig, JsonString))
	{
		FFileHelper::SaveStringToFile(JsonString, *GetConfigFilePath());
	}
}

//IN-WORLD ARS MODE

void AARSClientManager::OnToggleModeAction()
{
	if (mARSModeActive)
	{
		ExitARSMode();
		return;
	}
	// Only enter with a rail to adjust - placing one, or aiming at one
	if (GetActiveRailHologram())
	{
		EnterARSMode(EARSModeGun::Build);
	}
	else if (IsLocalPlayerDismantling())
	{
		EnterARSMode(EARSModeGun::Dismantle);
	}
}

void AARSClientManager::EnterARSMode(EARSModeGun Gun)
{
	AFGPlayerController* PlayerController = GetLocalController();
	if (!PlayerController)
	{
		return;
	}
	mARSModeActive = true;
	mARSModeGun = Gun;
	mSelectedPart = 0;
	mSettingsModeIndex = 0;
	mRotationAxis = 0;
	mTilingWarnActive = false;
	mDismantleLockedTrack.Reset();
	mLastHintSig = 0xFFFFFFFF; // Force a fresh hint inject on the first tick
	mHintReinjectFramesLeft = 0;
	mWorking = mSettings;
	mWorking.EnsureSlotArraySizes();

	const int32 Priority = PlayerController->GetMappingContextPriority() + 1000;
	if (mModeSharedContext)
	{
		mModeSharedHandle = PlayerController->BindMappingContext(mModeSharedContext, Priority, this);
	}
	if (Gun == EARSModeGun::Dismantle && mModeDismantleContext)
	{
		mModeDismantleHandle = PlayerController->BindMappingContext(mModeDismantleContext, Priority + 1, this);
	}

	ShowModeHUD();
}

void AARSClientManager::ExitARSMode()
{
	if (!mARSModeActive)
	{
		return;
	}
	mARSModeActive = false;
	if (AFGPlayerController* PlayerController = GetLocalController())
	{
		if (mModeSharedContext)
		{
			PlayerController->UnbindMappingContext(mModeSharedContext, mModeSharedHandle);
		}
		if (mModeDismantleContext)
		{
			PlayerController->UnbindMappingContext(mModeDismantleContext, mModeDismantleHandle);
		}
	}
	mModeSharedHandle = FBoundMappingContextHandle();
	mModeDismantleHandle = FBoundMappingContextHandle();
	HideModeHUD();
}

void AARSClientManager::UpdateARSModeValidity()
{
	if (!mARSModeActive)
	{
		return;
	}
	const bool bStillValid = (mARSModeGun == EARSModeGun::Build)
		? (GetActiveRailHologram() != nullptr)
		: IsLocalPlayerDismantling();
	if (!bStillValid)
	{
		ExitARSMode();
	}
}

int32 AARSClientManager::NumSettingsModesForPart(int32 Part) const
{
	if (Part == 0)
	{
		return mWorking.bDoubleRails ? 3 : 2;
	}
	return 3;
}

bool AARSClientManager::IsEditingPartnerUnderRail() const
{
	return mSelectedPart == 0 && mWorking.bDoubleRails && mWorking.bSplitUnderRailSides && mEditingPartnerSide;
}

AARSClientManager::FARSUnderRailEdit AARSClientManager::SelectedUnderRailEdit()
{
	FARSUnderRailEdit Edit;
	if (IsEditingPartnerUnderRail())
	{
		Edit.Length = &mWorking.UnderRailPartner.Length;
		Edit.Rotation = &mWorking.UnderRailPartner.RotationOffset;
		Edit.ZOffset = &mWorking.UnderRailPartner.ZOffset;
		Edit.SideOffset = &mWorking.UnderRailPartner.SideOffset;
		Edit.ForwardOffset = &mWorking.UnderRailPartner.ForwardOffset;
		return Edit;
	}
	Edit.Length = &mWorking.UnderRailLength;
	Edit.Rotation = &mWorking.UnderRailRotationOffset;
	Edit.ZOffset = &mWorking.UnderRailZOffset;
	Edit.SideOffset = &mWorking.UnderRailSideOffset;
	Edit.ForwardOffset = &mWorking.UnderRailForwardOffset;
	return Edit;
}

EARSSettingsMode AARSClientManager::SettingsModeForPartIndex(int32 Part, int32 Index) const
{
	if (Part == 0)
	{
		if (Index == 2 && mWorking.bDoubleRails)
		{
			return EARSSettingsMode::TrackSeparation;
		}
		return Index == 1 ? EARSSettingsMode::TilingLength : EARSSettingsMode::General;
	}
	switch (Index)
	{
	case 1:  return EARSSettingsMode::PillarSpacing;
	case 2:  return EARSSettingsMode::SpacingOffset;
	default: return EARSSettingsMode::General;
	}
}

void AARSClientManager::OnPrevPartAction()
{
	if (!mARSModeActive) { return; }
	mSelectedPart = (mSelectedPart + (FARSSettingsData::NumPillarSlots + 1) - 1) % (FARSSettingsData::NumPillarSlots + 1);
	// keep the current mode if the new part has it, otherwise fall back to general
	if (mSettingsModeIndex >= NumSettingsModesForPart(mSelectedPart)) { mSettingsModeIndex = 0; }
	PlayCycleModeSound();
}

void AARSClientManager::OnNextPartAction()
{
	if (!mARSModeActive) { return; }
	mSelectedPart = (mSelectedPart + 1) % (FARSSettingsData::NumPillarSlots + 1);
	if (mSettingsModeIndex >= NumSettingsModesForPart(mSelectedPart)) { mSettingsModeIndex = 0; }
	PlayCycleModeSound();
}

void AARSClientManager::OnCycleSettingsModeAction()
{
	if (!mARSModeActive) { return; }
	const int32 Num = NumSettingsModesForPart(mSelectedPart);
	mSettingsModeIndex = (mSettingsModeIndex + 1) % Num;
	PlayCycleModeSound();
}

void AARSClientManager::OnCycleRotationAxisAction()
{
	if (!mARSModeActive) { return; }
	// Yaw -> Pitch -> Roll -> Yaw
	mRotationAxis = (mRotationAxis + 1) % 3;
	PlayCycleModeSound();
}

void AARSClientManager::RefreshModConfig()
{
	mModConfig = FARSModConfigStruct::GetActiveConfig(this);
}

float AARSClientManager::GetPositionStepCm() const
{
	const FModifierKeysState Mods = FSlateApplication::Get().GetModifierKeys();
	if (Mods.IsControlDown()) { return mModConfig.PositionStepTiny * 100.0f; }
	if (Mods.IsAltDown()) { return mModConfig.PositionStepSmall * 100.0f; }
	if (Mods.IsShiftDown()) { return mModConfig.PositionStepLarge * 100.0f; }
	return 100.0f;
}

float AARSClientManager::GetRotationStepDegrees() const
{
	const FModifierKeysState Mods = FSlateApplication::Get().GetModifierKeys();
	if (Mods.IsControlDown()) { return mModConfig.RotationStepTiny; }
	if (Mods.IsAltDown()) { return mModConfig.RotationStepSmall; }
	if (Mods.IsShiftDown()) { return mModConfig.RotationStepLarge; }
	return 15.0f;
}

void AARSClientManager::SendWorkingSettings()
{
	mWorking.ClampValues();
	if (UARSRemoteCallObject* RCO = GetRCO())
	{
		RCO->Server_SetSettings(mWorking);
	}
}

FVector AARSClientManager::GetActiveRailForward()
{
	USplineComponent* Spline = nullptr;
	if (mARSModeGun == EARSModeGun::Build)
	{
		if (AFGHologram* Hologram = GetActiveRailHologram())
		{
			Spline = Hologram->FindComponentByClass<USplineComponent>();
		}
	}
	else if (AFGPlayerController* PlayerController = GetLocalController())
	{
		AFGBuildableRailroadTrack* Track = mDismantleLockedTrack.IsValid() ? mDismantleLockedTrack.Get() : TraceAimedRail(PlayerController);
		if (Track)
		{
			Spline = Track->FindComponentByClass<USplineComponent>();
		}
	}
	if (!Spline)
	{
		return FVector::ZeroVector;
	}
	FVector Direction = Spline->GetDirectionAtDistanceAlongSpline(Spline->GetSplineLength() * 0.5f, ESplineCoordinateSpace::World);
	Direction.Z = 0.0f;
	return Direction.GetSafeNormal();
}

void AARSClientManager::ApplyArrow(EARSArrow Arrow)
{
	if (!mARSModeActive) { return; }
	AFGPlayerController* PlayerController = GetLocalController();
	if (!PlayerController) { return; }

	const FVector RailForward = GetActiveRailForward();
	if (RailForward.IsNearlyZero()) { return; }
	const FVector RailRight = FVector::CrossProduct(FVector::UpVector, RailForward).GetSafeNormal();

	FVector ViewLocation; FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	FVector ViewForward = ViewRotation.Vector();
	ViewForward.Z = 0.0f;
	ViewForward = ViewForward.GetSafeNormal();
	if (ViewForward.IsNearlyZero()) { ViewForward = RailForward; }
	const FVector ViewRight = FVector::CrossProduct(FVector::UpVector, ViewForward).GetSafeNormal();

	FVector WorldDir;
	switch (Arrow)
	{
	case EARSArrow::Forward:  WorldDir = ViewForward; break;
	case EARSArrow::Backward: WorldDir = -ViewForward; break;
	case EARSArrow::Right:    WorldDir = ViewRight; break;
	case EARSArrow::Left:     WorldDir = -ViewRight; break;
	default: return;
	}

	const float AlongDot = FVector::DotProduct(WorldDir, RailForward);
	const float SideDot = FVector::DotProduct(WorldDir, RailRight);
	const bool bAlong = FMath::Abs(AlongDot) >= FMath::Abs(SideDot);
	const float AlongSign = AlongDot >= 0.0f ? 1.0f : -1.0f;
	const float SideSign = SideDot >= 0.0f ? 1.0f : -1.0f;

	mWorking.EnsureSlotArraySizes();
	const bool bUnderRail = (mSelectedPart == 0);
	const int32 Slot = mSelectedPart - 1;
	const EARSSettingsMode Mode = GetCurrentSettingsMode();

	if (Mode == EARSSettingsMode::General)
	{
		const float StepCm = GetPositionStepCm();
		const FARSUnderRailEdit Edit = SelectedUnderRailEdit();
		if (bAlong)
		{
			float& Forward = bUnderRail ? *Edit.ForwardOffset : mWorking.PillarForwardOffsets[Slot];
			Forward += AlongSign * StepCm;
		}
		else
		{
			float& Side = bUnderRail ? *Edit.SideOffset : mWorking.PillarSideOffsets[Slot];
			Side += SideSign * StepCm;
		}
	}
	else
	{
		const bool bWantsSide = (Mode == EARSSettingsMode::TrackSeparation);
		if (bAlong == bWantsSide) { return; }
		const float Sign = bWantsSide ? SideSign : AlongSign;
		const int32 IntStep = static_cast<int32>(Sign);
		switch (Mode)
		{
		case EARSSettingsMode::TilingLength:  *SelectedUnderRailEdit().Length += Sign * GetPositionStepCm(); break;
		case EARSSettingsMode::PillarSpacing: mWorking.PillarEveryNs[Slot] += IntStep; break;
		case EARSSettingsMode::SpacingOffset: mWorking.PillarSpacingOffsets[Slot] += IntStep; break;
		case EARSSettingsMode::TrackSeparation:
		{
			float Next = mWorking.TrackSeparation + Sign * GetPositionStepCm();
			// stepping into the gap swaps to the other side
			if (FMath::Abs(Next) < FARSSettingsData::MinTrackSeparation)
			{
				Next = (mWorking.TrackSeparation > 0.0f ? -1.0f : 1.0f) * FARSSettingsData::MinTrackSeparation;
			}
			mWorking.TrackSeparation = Next;
			break;
		}
		default: return;
		}
	}
	SendWorkingSettings();
	if (Mode == EARSSettingsMode::TilingLength)
	{
		MaybeShowTilingWarning();
	}
}

void AARSClientManager::ApplyVertical(float Sign)
{
	if (!mARSModeActive) { return; }
	// Vertical offset is General mode only
	if (GetCurrentSettingsMode() != EARSSettingsMode::General) { return; }
	mWorking.EnsureSlotArraySizes();
	float& ZOffset = (mSelectedPart == 0) ? *SelectedUnderRailEdit().ZOffset : mWorking.PillarZOffsets[mSelectedPart - 1];
	ZOffset += Sign * GetPositionStepCm();
	SendWorkingSettings();
}

void AARSClientManager::ApplyRotate(float Sign)
{
	if (!mARSModeActive) { return; }
	if (GetCurrentSettingsMode() != EARSSettingsMode::General) { return; }
	mWorking.EnsureSlotArraySizes();
	FRotator Rotation = (mSelectedPart == 0) ? *SelectedUnderRailEdit().Rotation : mWorking.GetPillarRotation(mSelectedPart - 1);
	const float Delta = Sign * GetRotationStepDegrees();
	switch (mRotationAxis)
	{
	case 1:  Rotation.Pitch += Delta; break;
	case 2:  Rotation.Roll += Delta; break;
	default: Rotation.Yaw += Delta; break;
	}
	if (mSelectedPart == 0)
	{
		mWorking.UnderRailRotationOffset = Rotation;
	}
	else
	{
		mWorking.SetPillarRotation(mSelectedPart - 1, Rotation);
	}
	SendWorkingSettings();
}

void AARSClientManager::OnAdjustForwardAction() { ApplyArrow(EARSArrow::Forward); }
void AARSClientManager::OnAdjustBackwardAction() { ApplyArrow(EARSArrow::Backward); }
void AARSClientManager::OnAdjustLeftAction() { ApplyArrow(EARSArrow::Left); }
void AARSClientManager::OnAdjustRightAction() { ApplyArrow(EARSArrow::Right); }
void AARSClientManager::OnVerticalUpAction() { ApplyVertical(+1.0f); }
void AARSClientManager::OnVerticalDownAction() { ApplyVertical(-1.0f); }
void AARSClientManager::OnRotateUpAction() { ApplyRotate(+1.0f); }
void AARSClientManager::OnRotateDownAction() { ApplyRotate(-1.0f); }

void AARSClientManager::OnToggleHologramLockAction()
{
	if (!mARSModeActive) { return; }
	if (mARSModeGun == EARSModeGun::Build)
	{
		if (AFGHologram* Hologram = GetActiveRailHologram())
		{
			if (Hologram->CanLockHologram())
			{
				Hologram->ToggleHologramPositionLock();
			}
		}
	}
	else
	{
		if (mDismantleLockedTrack.IsValid())
		{
			mDismantleLockedTrack.Reset();
		}
		else if (AFGBuildableRailroadTrack* Track = TraceAimedRail(GetLocalController()))
		{
			mDismantleLockedTrack = Track;
		}
	}
}

void AARSClientManager::ShowModeHUD()
{
	RefreshModConfig();

	if (!mInfoPanelClassSearched)
	{
		mInfoPanelClassSearched = true;
		mInfoPanelClass = LoadClass<UARSInfoPanelWidget>(nullptr, TEXT("/AutoRailSupports/Interface/Widget_ARSInfoPanel.Widget_ARSInfoPanel_C"));
	}
	if (mInfoPanelClass && !mInfoPanel)
	{
		mInfoPanel = CreateWidget<UARSInfoPanelWidget>(GetLocalController(), mInfoPanelClass);
		if (mInfoPanel)
		{
			mInfoPanel->AddToViewport(30);
		}
	}
	if (mInfoPanel)
	{
		mInfoPanel->SetBackgroundOpacity(mModConfig.InfoPanelBackgroundOpacity);
	}

	RefreshInfoPanel();
}

void AARSClientManager::HideModeHUD()
{
	if (mInfoPanel)
	{
		mInfoPanel->RemoveFromParent();
		mInfoPanel = nullptr;
	}
	DestroyRotationGizmo();
	RestoreVanillaHints();
	RestoreModeIndicator();
	RestoreDismantleDressing();
}

FString AARSClientManager::GetSelectedBlueprintName() const
{
	if (mSelectedPart == 0)
	{
		return mWorking.UnderRailBlueprintName;
	}
	const int32 Slot = mSelectedPart - 1;
	return mWorking.PillarBlueprintNames.IsValidIndex(Slot) ? mWorking.PillarBlueprintNames[Slot] : FString();
}

FVector AARSClientManager::GetSelectedBlueprintSizeMeters()
{
	const FString Name = GetSelectedBlueprintName();
	if (Name.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(GetWorld());
	UFGBlueprintDescriptor* Descriptor = BlueprintSubsystem ? BlueprintSubsystem->GetBlueprintDescriptorByNameString(Name) : nullptr;
	AARSSubsystem* Subsystem = AARSSubsystem::Get(GetWorld());
	if (!Descriptor || !Subsystem)
	{
		return FVector::ZeroVector;
	}
	const FBox Bounds = Subsystem->GetMeasuredBlueprint(Descriptor).LocalBounds;
	return Bounds.IsValid ? (Bounds.GetSize() / 100.0f) : FVector::ZeroVector;
}

void AARSClientManager::BuildSelectedSettingsRows(TArray<FARSInfoPanelRow>& OutRows) const
{
	OutRows.Reset();
	const bool bUnderRail = (mSelectedPart == 0);
	const int32 Slot = mSelectedPart - 1;
	const EARSSettingsMode Mode = GetCurrentSettingsMode();
	const bool bGeneral = (Mode == EARSSettingsMode::General);

	auto Degrees = [](float Value) { return FText::FromString(FString::Printf(TEXT("%.1f%c"), Value, static_cast<TCHAR>(0x00B0))); };
	auto Meters = [](float Centimeters) { return FText::FromString(FString::Printf(TEXT("%.2f m"), Centimeters / 100.0f)); };
	
	auto AddSimpleRow = [&OutRows](const FText& Label, const FText& Value, bool bActive)
	{
		FARSInfoPanelRow& Row = OutRows.AddDefaulted_GetRef();
		Row.Label = Label;
		Row.bActive = bActive;
		Row.Cells.Add({ FText::GetEmpty(), Value, true });
	};

	auto AddTripleRow = [&OutRows](const FText& Label, bool bActive, int32 SelectedCell,
		const FText& Type0, const FText& Value0, const FText& Type1, const FText& Value1, const FText& Type2, const FText& Value2)
	{
		FARSInfoPanelRow& Row = OutRows.AddDefaulted_GetRef();
		Row.Label = Label;
		Row.bActive = bActive;
		Row.Cells.Add({ Type0, Value0, SelectedCell == INDEX_NONE || SelectedCell == 0 });
		Row.Cells.Add({ Type1, Value1, SelectedCell == INDEX_NONE || SelectedCell == 1 });
		Row.Cells.Add({ Type2, Value2, SelectedCell == INDEX_NONE || SelectedCell == 2 });
	};

	auto AddOffsetAndRotationRows = [&](const FRotator& Rotation, const FVector& Offsets)
	{
		// Only the axis scroll is on stays lit, and only while the row itself is live
		AddTripleRow(NSLOCTEXT("AutoRailSupports", "RowRotation", "Rotation:"), bGeneral, bGeneral ? mRotationAxis : INDEX_NONE,
			NSLOCTEXT("AutoRailSupports", "CellYaw", "Yaw"), Degrees(Rotation.Yaw),
			NSLOCTEXT("AutoRailSupports", "CellPitch", "Pitch"), Degrees(Rotation.Pitch),
			NSLOCTEXT("AutoRailSupports", "CellRoll", "Roll"), Degrees(Rotation.Roll));
		AddTripleRow(NSLOCTEXT("AutoRailSupports", "RowPositionOffset", "Position Offset:"), bGeneral, INDEX_NONE,
			NSLOCTEXT("AutoRailSupports", "CellForward", "Fwd"), Meters(Offsets.X),
			NSLOCTEXT("AutoRailSupports", "CellSide", "Side"), Meters(Offsets.Y),
			NSLOCTEXT("AutoRailSupports", "CellVertical", "Vert"), Meters(Offsets.Z));
	};

	if (bUnderRail)
	{
		const bool bPartner = mSelectedPart == 0 && mWorking.bDoubleRails
			&& mWorking.bSplitUnderRailSides && mEditingPartnerSide;
		const FARSUnderRailSide& Partner = mWorking.UnderRailPartner;
		const float Length = bPartner ? Partner.Length : mWorking.UnderRailLength;
		const FRotator Rotation = bPartner ? Partner.RotationOffset : mWorking.UnderRailRotationOffset;
		const FVector Offsets = bPartner
			? FVector(Partner.ForwardOffset, Partner.SideOffset, Partner.ZOffset)
			: FVector(mWorking.UnderRailForwardOffset, mWorking.UnderRailSideOffset, mWorking.UnderRailZOffset);

		AddSimpleRow(NSLOCTEXT("AutoRailSupports", "RowTilingLength", "Tiling Length:"),
			Meters(Length), Mode == EARSSettingsMode::TilingLength);
		if (mWorking.bDoubleRails)
		{
			const FText Side = mWorking.TrackSeparation >= 0.0f
				? NSLOCTEXT("AutoRailSupports", "SideRight", "right")
				: NSLOCTEXT("AutoRailSupports", "SideLeft", "left");
			AddSimpleRow(NSLOCTEXT("AutoRailSupports", "RowTrackSeparation", "Track Separation:"),
				FText::Format(NSLOCTEXT("AutoRailSupports", "SeparationValue", "{0} ({1})"),
					Meters(FMath::Abs(mWorking.TrackSeparation)), Side),
				Mode == EARSSettingsMode::TrackSeparation);
		}
		AddOffsetAndRotationRows(Rotation, Offsets);
	}
	else if (mWorking.PillarForwardOffsets.IsValidIndex(Slot))
	{
		AddSimpleRow(NSLOCTEXT("AutoRailSupports", "RowPillarSpacing", "Pillar Spacing:"),
			FText::AsNumber(mWorking.PillarEveryNs[Slot]), Mode == EARSSettingsMode::PillarSpacing);
		AddSimpleRow(NSLOCTEXT("AutoRailSupports", "RowSpacingOffset", "Spacing Offset:"),
			FText::AsNumber(mWorking.PillarSpacingOffsets[Slot]), Mode == EARSSettingsMode::SpacingOffset);
		AddOffsetAndRotationRows(
			mWorking.GetPillarRotation(Slot),
			FVector(mWorking.PillarForwardOffsets[Slot], mWorking.PillarSideOffsets[Slot], mWorking.PillarZOffsets[Slot]));
	}
}

void AARSClientManager::RefreshInfoPanel()
{
	if (!mInfoPanel)
	{
		return;
	}
	const bool bUnderRail = (mSelectedPart == 0);
	FText PartLabel = bUnderRail
		? NSLOCTEXT("AutoRailSupports", "PartUnderRail", "Under-Rail")
		: FText::Format(NSLOCTEXT("AutoRailSupports", "PartPillar", "Pillar Slot {0}"), FText::AsNumber(mSelectedPart));
	if (bUnderRail && mWorking.bDoubleRails && mWorking.bSplitUnderRailSides)
	{
		PartLabel = FText::Format(NSLOCTEXT("AutoRailSupports", "PartUnderRailSide", "Under-Rail ({0})"),
			mEditingPartnerSide
				? NSLOCTEXT("AutoRailSupports", "SideSecondTrack", "Second Track")
				: NSLOCTEXT("AutoRailSupports", "SideMainTrack", "Main Track"));
	}

	const FString Name = GetSelectedBlueprintName();
	const FText BlueprintName = Name.IsEmpty() ? NSLOCTEXT("AutoRailSupports", "PartNone", "None") : FText::FromString(Name);

	const FVector Size = GetSelectedBlueprintSizeMeters();
	const FText Dimensions = Size.IsZero()
		? FText::FromString(TEXT("-"))
		: FText::FromString(FString::Printf(TEXT("%.1f x %.1f x %.1f m"), Size.X, Size.Y, Size.Z));

	mInfoPanel->SetInfo(PartLabel, BlueprintName, Dimensions);

	TArray<FARSInfoPanelRow> Rows;
	BuildSelectedSettingsRows(Rows);
	mInfoPanel->SetSettingsRows(Rows);
}

void AARSClientManager::UpdateRotationGizmo()
{
	// needs ARS Mode and a live preview to read the segment anchors from
	if (!mARSModeActive || !mPreviewActor)
	{
		DestroyRotationGizmo();
		return;
	}
	AFGPlayerController* PlayerController = GetLocalController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		DestroyRotationGizmo();
		return;
	}
	// anchor on the selected part's nearest stamp
	FTransform SegmentTransform;
	if (!mPreviewActor->GetClosestStampTransform(mSelectedPart, Pawn->GetActorLocation(), SegmentTransform, &mEditingPartnerSide))
	{
		DestroyRotationGizmo();
		return;
	}

	if (!mGizmoClassSearched)
	{
		mGizmoClassSearched = true;
		mGizmoClass = LoadClass<UARSGizmo>(nullptr, TEXT("/AutoRailSupports/Gizmo/BP_ARSGizmo.BP_ARSGizmo_C"));
	}
	if (!mGizmoClass)
	{
		return;
	}
	if (!mGizmo)
	{
		mGizmo = NewObject<UARSGizmo>(mPreviewActor, mGizmoClass);
		if (mGizmo)
		{
			mGizmo->AttachToComponent(mPreviewActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			mGizmo->RegisterComponent();
			mGizmo->InitCircles();
		}
	}
	if (mGizmo)
	{
		mGizmo->SetWorldLocationAndRotation(SegmentTransform.GetLocation(), SegmentTransform.GetRotation());
		mGizmo->SetActiveAxis(mRotationAxis);
		mGizmo->SetVisibility(true, /*bPropagateToChildren*/ true);
	}
}

void AARSClientManager::DestroyRotationGizmo()
{
	if (mGizmo)
	{
		mGizmo->DestroyComponent();
		mGizmo = nullptr;
	}
}

void AARSClientManager::MaybeShowTilingWarning()
{
	const FVector Size = GetSelectedBlueprintSizeMeters();
	const bool bWarn = AARSSubsystem::ShouldWarnStampCount(mWorking.UnderRailLength, Size.X);
	if (bWarn && !mTilingWarnActive)
	{
		if (AFGPlayerController* PlayerController = GetLocalController())
		{
			UFGBlueprintFunctionLibrary::AddPopupWithCloseDelegate(PlayerController,
				NSLOCTEXT("AutoRailSupports", "TilingWarnTitle", "Auto Rail Supports"),
				AARSSubsystem::GetStampCountWarningText(),
				FPopupClosed());
		}
	}
	mTilingWarnActive = bWarn;
}

FText AARSClientManager::GetSettingsModeDisplayText(EARSSettingsMode Mode)
{
	switch (Mode)
	{
	case EARSSettingsMode::TilingLength:  return NSLOCTEXT("AutoRailSupports", "ModeTiling", "Settings Mode: Tiling Length");
	case EARSSettingsMode::PillarSpacing: return NSLOCTEXT("AutoRailSupports", "ModeSpacing", "Settings Mode: Pillar Spacing");
	case EARSSettingsMode::SpacingOffset: return NSLOCTEXT("AutoRailSupports", "ModeOffset", "Settings Mode: Spacing Offset");
	case EARSSettingsMode::TrackSeparation: return NSLOCTEXT("AutoRailSupports", "ModeSeparation", "Settings Mode: Track Separation");
	default:                              return NSLOCTEXT("AutoRailSupports", "ModeRotationOffset", "Settings Mode: Rotation & Offset");
	}
}

FText AARSClientManager::GetKeyLabel(const UInputAction* Action) const
{
	if (!Action)
	{
		return FText::GetEmpty();
	}
	auto FindIn = [Action](const UFGInputMappingContext* Context) -> FText
	{
		if (Context)
		{
			for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
			{
				if (Mapping.Action == Action)
				{
					return ARSKeyDisplayName(Mapping.Key);
				}
			}
		}
		return FText::GetEmpty();
	};
	FText Label = FindIn(mModeSharedContext);
	if (Label.IsEmpty()) { Label = FindIn(mMappingContext); }
	if (Label.IsEmpty()) { Label = FindIn(mModeDismantleContext); }
	return Label;
}

void AARSClientManager::BuildARSHintRows(TArray<TPair<FText, FText>>& OutRows) const
{
	const bool bBuild = (mARSModeGun == EARSModeGun::Build);
	const bool bGeneral = (GetCurrentSettingsMode() == EARSSettingsMode::General);

	auto Add = [&OutRows](const TCHAR* Action, const FText& Key) { OutRows.Emplace(FText::FromString(Action), Key); };
	auto AddT = [&OutRows](const TCHAR* Action, const TCHAR* Key) { OutRows.Emplace(FText::FromString(Action), FText::FromString(Key)); };
	auto Two = [](const FText& A, const FText& B)
	{
		const FString SA = A.ToString();
		const FString SB = B.ToString();
		if (SA.IsEmpty()) { return FText::FromString(SB); }
		if (SB.IsEmpty()) { return FText::FromString(SA); }
		return FText::FromString(FString::Printf(TEXT("%s / %s"), *SA, *SB));
	};

	AddT(bBuild ? TEXT("Build") : TEXT("Apply Supports"), TEXT("LMB"));
	Add(TEXT("Back"), GetKeyLabel(mToggleModeAction));
	AddT(TEXT("Close"), bBuild ? TEXT("Q / Escape") : TEXT("F / Escape"));

	{
		TArray<FString> ArrowKeys;
		for (const UInputAction* A : { mAdjustForwardAction.Get(), mAdjustBackwardAction.Get(), mAdjustLeftAction.Get(), mAdjustRightAction.Get() })
		{
			const FString Label = GetKeyLabel(A).ToString();
			if (!Label.IsEmpty()) { ArrowKeys.Add(Label); }
		}
		Add(TEXT("Adjust"), FText::FromString(FString::Join(ArrowKeys, TEXT(" / "))));
	}

	if (bGeneral)
	{
		Add(TEXT("Vertical Adjust"), Two(GetKeyLabel(mVerticalUpAction), GetKeyLabel(mVerticalDownAction)));
		const FText Scroll = Two(GetKeyLabel(mRotateUpAction), GetKeyLabel(mRotateDownAction));
		const TCHAR* RotateLabel = (mRotationAxis == 1) ? TEXT("Rotate Pitch")
			: (mRotationAxis == 2) ? TEXT("Rotate Roll") : TEXT("Rotate Yaw");
		Add(RotateLabel, Scroll);
		Add(TEXT("Cycle Rotation Axis"), GetKeyLabel(mCycleRotationAxisAction));
	}

	Add(TEXT("Previous Part"), GetKeyLabel(mPrevPartAction));
	Add(TEXT("Next Part"), GetKeyLabel(mNextPartAction));
	Add(TEXT("Cycle Settings Mode"), GetKeyLabel(mCycleSettingsModeAction));
	AddT(TEXT("Tiny Adjustment"), TEXT("Left Ctrl"));
	AddT(TEXT("Small Adjustment"), TEXT("Left Alt"));
	AddT(TEXT("Large Adjustment"), TEXT("Left Shift"));
	Add(mHologramLocked ? TEXT("Unlock Hologram") : TEXT("Lock Hologram"), GetKeyLabel(mToggleHologramLockAction));
}

UUserWidget* AARSClientManager::FindLiveModeWidget()
{
	if (mARSModeGun == EARSModeGun::Dismantle)
	{
		return Cast<UUserWidget>(FindLiveDismantleRefundsWidget());
	}
	return FindLiveBuildModeWidget();
}

UUserWidget* AARSClientManager::FindLiveBuildModeWidget()
{
	if (!mBuildModeWidgetClass)
	{
		// Path confirmed from the live dump
		mBuildModeWidgetClass = LoadClass<UFGInteractWidget>(nullptr, TEXT("/Game/FactoryGame/Interface/UI/InGame/BuildMenu/Widget_BuildMode.Widget_BuildMode_C"));
	}
	if (!mBuildModeWidgetClass)
	{
		return nullptr;
	}
	UWorld* World = GetWorld();
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Candidate = *It;
		if (!Candidate || Candidate->GetWorld() != World
			|| Candidate->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			continue;
		}
		if (Candidate->GetClass() != mBuildModeWidgetClass || !Candidate->GetCachedWidget().IsValid())
		{
			continue;
		}
		if (Candidate->GetName().Contains(TEXT("Dismantle")))
		{
			continue;
		}
		const ESlateVisibility Visibility = Candidate->GetVisibility();
		if (Visibility == ESlateVisibility::Collapsed || Visibility == ESlateVisibility::Hidden)
		{
			continue;
		}
		return Candidate;
	}
	return nullptr;
}

bool AARSClientManager::ResolveHintArray(UUserWidget* Widget, FArrayProperty*& OutArray, FTextProperty*& OutAction, FTextProperty*& OutKeyBinding, const TCHAR* ArrayName)
{
	OutArray = Widget ? FindFProperty<FArrayProperty>(Widget->GetClass(), ArrayName) : nullptr;
	FStructProperty* EntryProperty = OutArray ? CastField<FStructProperty>(OutArray->Inner) : nullptr;
	if (!EntryProperty)
	{
		return false;
	}
	OutAction = nullptr;
	OutKeyBinding = nullptr;
	for (TFieldIterator<FProperty> It(EntryProperty->Struct); It; ++It)
	{
		if (FTextProperty* TextProperty = CastField<FTextProperty>(*It))
		{
			if (TextProperty->GetName().StartsWith(TEXT("Action"))) { OutAction = TextProperty; }
			else if (TextProperty->GetName().StartsWith(TEXT("KeyBinding"))) { OutKeyBinding = TextProperty; }
		}
	}
	return OutAction != nullptr && OutKeyBinding != nullptr;
}

void AARSClientManager::ReadHintRows(UUserWidget* Widget, TArray<TPair<FText, FText>>& OutRows, const TCHAR* ArrayName)
{
	FArrayProperty* Array; FTextProperty* ActionProp; FTextProperty* KeyProp;
	if (!ResolveHintArray(Widget, Array, ActionProp, KeyProp, ArrayName))
	{
		return;
	}
	FScriptArrayHelper Entries(Array, Array->ContainerPtrToValuePtr<void>(Widget));
	for (int32 i = 0; i < Entries.Num(); i++)
	{
		const uint8* Entry = Entries.GetRawPtr(i);
		OutRows.Emplace(ActionProp->GetPropertyValue_InContainer(Entry), KeyProp->GetPropertyValue_InContainer(Entry));
	}
}

void AARSClientManager::WriteHintRows(UUserWidget* Widget, const TArray<TPair<FText, FText>>& Rows)
{
	if (!Widget)
	{
		return;
	}
	UFunction* Function = Widget->FindFunction(FName(TEXT("SetKeybindingHints")));
	if (!Function || Function->NumParms != 1)
	{
		UE_LOG(LogARSClient, Warning, TEXT("SetKeybindingHints(1) not found on %s; ARS hints can't update."), *GetNameSafe(Widget->GetClass()));
		return;
	}

	FArrayProperty* ArrayParm = nullptr;
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		ArrayParm = CastField<FArrayProperty>(*It);
		break;
	}
	FStructProperty* EntryProp = ArrayParm ? CastField<FStructProperty>(ArrayParm->Inner) : nullptr;
	if (!EntryProp)
	{
		return;
	}
	FTextProperty* ActionProp = nullptr;
	FTextProperty* KeyProp = nullptr;
	for (TFieldIterator<FProperty> It(EntryProp->Struct); It; ++It)
	{
		if (FTextProperty* TextProp = CastField<FTextProperty>(*It))
		{
			if (TextProp->GetName().StartsWith(TEXT("Action"))) { ActionProp = TextProp; }
			else if (TextProp->GetName().StartsWith(TEXT("KeyBinding"))) { KeyProp = TextProp; }
		}
	}
	if (!ActionProp || !KeyProp)
	{
		return;
	}

	void* Params = FMemory::Malloc(FMath::Max<int32>(Function->ParmsSize, 1));
	FMemory::Memzero(Params, Function->ParmsSize);
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->InitializeValue_InContainer(Params);
	}
	{
		FScriptArrayHelper Entries(ArrayParm, ArrayParm->ContainerPtrToValuePtr<void>(Params));
		Entries.EmptyValues();
		for (const TPair<FText, FText>& Row : Rows)
		{
			const int32 Index = Entries.AddValue();
			uint8* Entry = Entries.GetRawPtr(Index);
			ActionProp->SetPropertyValue_InContainer(Entry, Row.Key);
			KeyProp->SetPropertyValue_InContainer(Entry, Row.Value);
		}
	}
	Widget->ProcessEvent(Function, Params);
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Params);
	}
	FMemory::Free(Params);
}

bool AARSClientManager::SetHintRowInArray(UUserWidget* Widget, int32 RowIndex, const TPair<FText, FText>& Row, const TCHAR* ArrayName)
{
	FArrayProperty* Array; FTextProperty* ActionProp; FTextProperty* KeyProp;
	if (!ResolveHintArray(Widget, Array, ActionProp, KeyProp, ArrayName))
	{
		return false;
	}
	FScriptArrayHelper Entries(Array, Array->ContainerPtrToValuePtr<void>(Widget));
	if (!Entries.IsValidIndex(RowIndex))
	{
		return false;
	}
	uint8* Entry = Entries.GetRawPtr(RowIndex);
	ActionProp->SetPropertyValue_InContainer(Entry, Row.Key);
	KeyProp->SetPropertyValue_InContainer(Entry, Row.Value);
	return true;
}

bool AARSClientManager::PatchLiveHintText(UUserWidget* BarWidget, const FText& OldText, const FText& NewText)
{
	const FString Needle = OldText.ToString();
	if (!BarWidget || !BarWidget->WidgetTree || Needle.IsEmpty())
	{
		return false;
	}
	UTextBlock* FoundText = nullptr;
	URichTextBlock* FoundRichText = nullptr;
	int32 Matches = 0;
	BarWidget->WidgetTree->ForEachWidgetAndDescendants([&Needle, &Matches, &FoundText, &FoundRichText](UWidget* Child)
	{
		if (!Child || !Child->GetCachedWidget().IsValid())
		{
			return;
		}
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Child))
		{
			if (TextBlock->GetText().ToString().Equals(Needle, ESearchCase::CaseSensitive))
			{
				++Matches;
				FoundText = TextBlock;
			}
		}
		else if (URichTextBlock* RichText = Cast<URichTextBlock>(Child))
		{
			if (RichText->GetText().ToString().Equals(Needle, ESearchCase::CaseSensitive))
			{
				++Matches;
				FoundRichText = RichText;
			}
		}
	});
	if (Matches != 1)
	{
		UE_LOG(LogARSClient, Warning, TEXT("ARS hint patch: %d live text widget(s) show \"%s\" in %s; rebuilding the whole bar instead."),
			Matches, *Needle, *GetNameSafe(BarWidget));
		return false;
	}
	if (FoundText) { FoundText->SetText(NewText); }
	else if (FoundRichText) { FoundRichText->SetText(NewText); }
	return true;
}

bool AARSClientManager::PatchHintRow(UUserWidget* Widget, int32 RowIndex, const TPair<FText, FText>& OldRow, const TPair<FText, FText>& NewRow) const
{
	if (!OldRow.Key.EqualTo(NewRow.Key) && !PatchLiveHintText(Widget, OldRow.Key, NewRow.Key))
	{
		return false;
	}
	if (!OldRow.Value.EqualTo(NewRow.Value) && !PatchLiveHintText(Widget, OldRow.Value, NewRow.Value))
	{
		return false;
	}
	SetHintRowInArray(Widget, RowIndex, NewRow, TEXT("mCachedKeybinds"));
	return true;
}

void AARSClientManager::ApplyARSHintsToBar()
{
	UUserWidget* Widget = FindLiveModeWidget();
	if (!Widget)
	{
		return;
	}
	// save the vanilla rows once per ARS Mode session
	if (!mVanillaHintsSaved)
	{
		mSavedVanillaHints.Reset();
		ReadHintRows(Widget, mSavedVanillaHints);
		mVanillaHintsSaved = true;
	}
	mHintBarWidget = Widget;
	// replace the whole bar with only the ARS Mode hints
	TArray<TPair<FText, FText>> Rows;
	BuildARSHintRows(Rows);

	TArray<TPair<FText, FText>> Live;
	ReadHintRows(Widget, Live, TEXT("mCachedKeybinds"));
	if (Live.Num() == Rows.Num())
	{
		bool bPatched = true;
		for (int32 i = 0; i < Rows.Num() && bPatched; i++)
		{
			if (Live[i].Key.EqualTo(Rows[i].Key) && Live[i].Value.EqualTo(Rows[i].Value))
			{
				continue;
			}
			bPatched = PatchHintRow(Widget, i, Live[i], Rows[i]);
		}
		if (bPatched)
		{
			return;
		}
	}
	WriteHintRows(Widget, Rows);
}

void AARSClientManager::EnsureBuildGunAdjustHint()
{
	if (mARSModeActive || !GetActiveRailHologram())
	{
		mBuildBarWidget = nullptr;
		return;
	}
	if (!mBuildBarWidget.IsValid())
	{
		mBuildBarWidget = FindLiveBuildModeWidget();
	}
	UUserWidget* Widget = mBuildBarWidget.Get();
	if (!Widget)
	{
		return;
	}
	TArray<TPair<FText, FText>> Rows;
	ReadHintRows(Widget, Rows, TEXT("mCachedKeybinds"));
	if (Rows.Num() == 0)
	{
		return;
	}
	const FText ActionText = GetApplyHintActionText();
	for (const TPair<FText, FText>& Row : Rows)
	{
		if (Row.Key.EqualTo(ActionText))
		{
			return; // Already there, so leave the bar alone
		}
	}
	Rows.Emplace(ActionText, FText::FromString(TEXT("{ARS.ToggleMode}")));
	WriteHintRows(Widget, Rows);
}

bool AARSClientManager::IsActiveRailLocked()
{
	if (mARSModeGun == EARSModeGun::Build)
	{
		AFGHologram* Hologram = GetActiveRailHologram();
		return Hologram && Hologram->IsHologramLocked();
	}
	return mDismantleLockedTrack.IsValid();
}

void AARSClientManager::RestoreVanillaHints()
{
	if (UUserWidget* Widget = mHintBarWidget.Get())
	{
		if (mVanillaHintsSaved)
		{
			WriteHintRows(Widget, mSavedVanillaHints);
		}
	}
	mHintBarWidget = nullptr;
	mSavedVanillaHints.Reset();
	mVanillaHintsSaved = false;
}

void AARSClientManager::ApplyModeIndicator()
{
	UUserWidget* Widget = FindLiveModeWidget();
	if (!Widget)
	{
		return;
	}
	UTextBlock* Text = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("mBuildModeText")));
	if (!Text)
	{
		return;
	}
	if (mModeTextBlock.Get() != Text)
	{
		mModeTextBlock = Text;
		mOrigModeText = Text->GetText();
		mModeTextCached = true;
	}
	Text->SetText(GetSettingsModeDisplayText(GetCurrentSettingsMode()));

	if (mARSModeGun == EARSModeGun::Build)
	{
		if (UWidget* Container = Widget->GetWidgetFromName(TEXT("mBuildModeContainer")))
		{
			if (mModeContainerWidget.Get() != Container)
			{
				mModeContainerWidget = Container;
				mModeContainerOrigVisibility = Container->GetVisibility();
				mModeContainerCached = true;
			}
			if (Container->GetVisibility() != ESlateVisibility::SelfHitTestInvisible)
			{
				Container->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}
}

void AARSClientManager::RestoreModeIndicator()
{
	if (UTextBlock* Text = mModeTextBlock.Get())
	{
		if (mModeTextCached)
		{
			Text->SetText(mOrigModeText);
		}
	}
	mModeTextBlock = nullptr;
	mModeTextCached = false;

	if (UWidget* Container = mModeContainerWidget.Get())
	{
		if (mModeContainerCached)
		{
			Container->SetVisibility(mModeContainerOrigVisibility);
		}
	}
	mModeContainerWidget = nullptr;
	mModeContainerCached = false;
}

void AARSClientManager::ApplyDismantleDressing()
{
	UUserWidget* Widget = Cast<UUserWidget>(FindLiveDismantleRefundsWidget());
	if (!Widget)
	{
		return;
	}
	URichTextBlock* Name = Cast<URichTextBlock>(Widget->GetWidgetFromName(TEXT("mBuildingNameText")));
	if (!Name)
	{
		return;
	}
	static const FText ARSName = FText::FromString(TEXT("<White12Bold>Currently Building:</>\n<White20Reg>ARS Supports</>"));
	if (mDismantleNameText.Get() != Name)
	{
		mDismantleNameText = Name;
		mOrigDismantleName = Name->GetText();
		mDismantleNameCached = true;
	}
	if (!Name->GetText().ToString().Equals(ARSName.ToString()))
	{
		Name->SetText(ARSName);
	}

	auto HideDismantleIndicator = [this, Widget](const TCHAR* WidgetName)
	{
		UWidget* Target = Widget->GetWidgetFromName(FName(WidgetName));
		if (!Target)
		{
			return;
		}
		bool bTracked = false;
		for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Entry : mDismantleHiddenWidgets)
		{
			if (Entry.Key.Get() == Target) { bTracked = true; break; }
		}
		if (!bTracked)
		{
			mDismantleHiddenWidgets.Emplace(Target, Target->GetVisibility());
		}
		if (Target->GetVisibility() != ESlateVisibility::Collapsed)
		{
			Target->SetVisibility(ESlateVisibility::Collapsed);
		}
	};
	HideDismantleIndicator(TEXT("mCircle"));
	HideDismantleIndicator(TEXT("mDismantleBorder"));
	HideDismantleIndicator(TEXT("mCostSlotsGrid"));
}

void AARSClientManager::RestoreDismantleDressing()
{
	if (URichTextBlock* Name = mDismantleNameText.Get())
	{
		if (mDismantleNameCached)
		{
			Name->SetText(mOrigDismantleName);
		}
	}
	mDismantleNameText = nullptr;
	mDismantleNameCached = false;

	for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Entry : mDismantleHiddenWidgets)
	{
		if (UWidget* Target = Entry.Key.Get())
		{
			Target->SetVisibility(Entry.Value);
		}
	}
	mDismantleHiddenWidgets.Empty();
}
