#include "ARSSubsystem.h"

#include "ARSDebug.h"
#include "ARSRemoteCallObject.h"
#include "FGBlueprintProxy.h"
#include "FGBlueprintSubsystem.h"
#include "FGBuildableSubsystem.h"
#include "FGChatManager.h"
#include "FGCharacterPlayer.h"
#include "FGFactoryBlueprintTypes.h"
#include "FGPlayerController.h"
#include "FGPlayerState.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "Subsystem/SubsystemActorManager.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

AARSSubsystem::AARSSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

AARSSubsystem* AARSSubsystem::Get(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	if (USubsystemActorManager* SubsystemActorManager = World->GetSubsystem<USubsystemActorManager>())
	{
		return SubsystemActorManager->GetSubsystemActor<AARSSubsystem>();
	}
	return nullptr;
}

void AARSSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	mSettings.EnsureSlotArraySizes();
	for (TPair<FString, FARSSettingsData>& Entry : mPlayerSettings)
	{
		Entry.Value.EnsureSlotArraySizes();
	}
}

FString AARSSubsystem::GetPlayerIdentityString(const APlayerState* PlayerState)
{
	return PlayerState ? PlayerState->GetUniqueId().ToString() : FString();
}

bool AARSSubsystem::IsPlayerIdentityReady(const APlayerState* PlayerState)
{
	return !GetPlayerIdentityString(PlayerState).IsEmpty();
}

FString AARSSubsystem::GetPlayerKey(const APlayerState* PlayerState)
{
	const FString UserId = GetPlayerIdentityString(PlayerState);
	return UserId.IsEmpty() ? TEXT("LocalPlayer") : UserId;
}

FString AARSSubsystem::GetPlayerKeyFor(const AFGCharacterPlayer* Player)
{
	return GetPlayerKey(Player ? Player->GetPlayerState() : nullptr);
}

const FARSSettingsData& AARSSubsystem::GetSettingsFor(const FString& UserId) const
{
	const FARSSettingsData* Found = mPlayerSettings.Find(UserId);
	return Found ? *Found : mSettings;
}

void AARSSubsystem::Init()
{
	Super::Init();
}

void AARSSubsystem::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		mSettings.EnsureSlotArraySizes();
		TryBindConstructionDelegate();
	}

	if (AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(GetWorld()))
	{
		BlueprintSubsystem->OnBlueprintDescriptorUpdated.AddDynamic(this, &AARSSubsystem::OnBlueprintDescriptorUpdated);
		bDescriptorUpdateBound = true;
	}
}

void AARSSubsystem::OnBlueprintDescriptorUpdated(UFGBlueprintDescriptor* Descriptor)
{
	if (!Descriptor)
	{
		// no idea which one changed, so re-measure everything on next use
		mMeasureCache.Reset();
		return;
	}
	const FString ChangedName = Descriptor->GetBlueprintNameAsString();
	for (auto It = mMeasureCache.CreateIterator(); It; ++It)
	{
		const UFGBlueprintDescriptor* Cached = It.Key().Get();
		if (!Cached || Cached == Descriptor || Cached->GetBlueprintNameAsString() == ChangedName)
		{
			It.RemoveCurrent();
		}
	}
}

void AARSSubsystem::TryBindConstructionDelegate()
{
	if (bDelegateBound)
	{
		return;
	}
	if (AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(GetWorld()))
	{
		BuildableSubsystem->BuildableConstructedGlobalDelegate.AddDynamic(this, &AARSSubsystem::OnBuildableConstructed);
		bDelegateBound = true;
	}
	else
	{
		// the buildable subsystem may not exist this early in world init
		GetWorldTimerManager().SetTimer(mBindRetryTimerHandle, this, &AARSSubsystem::TryBindConstructionDelegate, 0.5f, false);
	}
}

void AARSSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bDelegateBound)
	{
		if (AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(GetWorld()))
		{
			BuildableSubsystem->BuildableConstructedGlobalDelegate.RemoveDynamic(this, &AARSSubsystem::OnBuildableConstructed);
		}
		bDelegateBound = false;
	}
	if (bDescriptorUpdateBound)
	{
		if (AFGBlueprintSubsystem* BlueprintSubsystem = AFGBlueprintSubsystem::Get(GetWorld()))
		{
			BlueprintSubsystem->OnBlueprintDescriptorUpdated.RemoveDynamic(this, &AARSSubsystem::OnBlueprintDescriptorUpdated);
		}
		bDescriptorUpdateBound = false;
	}
	Super::EndPlay(EndPlayReason);
}

void AARSSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AARSSubsystem, mActivePreviews);
}

void AARSSubsystem::ApplySettingsFor(const FString& UserId, const FARSSettingsData& NewSettings)
{
	if (!HasAuthority())
	{
		return;
	}
	FARSSettingsData& Stored = mPlayerSettings.Add(UserId, NewSettings);
	Stored.ClampValues();
	// so an adjustment shows up for whoever is watching this player place
	RefreshPreviewSettings(UserId);
}

bool AARSSubsystem::SetEnabledSettingFor(const FString& UserId, bool bNewEnabled)
{
	if (!HasAuthority())
	{
		return bNewEnabled;
	}
	FARSSettingsData& Stored = mPlayerSettings.FindOrAdd(UserId, mSettings);
	Stored.bEnabled = bNewEnabled;
	RefreshPreviewSettings(UserId);
	return Stored.bEnabled;
}

void AARSSubsystem::SetPreviewSource(AFGCharacterPlayer* Player, AActor* Source)
{
	if (!HasAuthority() || !Player)
	{
		return;
	}
	const int32 Index = mActivePreviews.IndexOfByPredicate(
		[Player](const FARSRemotePreview& Preview) { return Preview.Player == Player; });

	if (!Source)
	{
		if (Index != INDEX_NONE)
		{
			mActivePreviews.RemoveAt(Index);
		}
		return;
	}

	const FString UserId = GetPlayerKeyFor(Player);
	FARSRemotePreview& Preview = (Index != INDEX_NONE) ? mActivePreviews[Index] : mActivePreviews.AddDefaulted_GetRef();
	Preview.Player = Player;
	Preview.Source = Source;
	Preview.SplinePoints.Reset();
	Preview.Settings = GetSettingsFor(UserId);
}

void AARSSubsystem::SetPreviewSpline(AFGCharacterPlayer* Player, const TArray<FARSSplinePoint>& Points, uint8 Stencil)
{
	if (!HasAuthority() || !Player)
	{
		return;
	}
	const int32 Index = mActivePreviews.IndexOfByPredicate(
		[Player](const FARSRemotePreview& Preview) { return Preview.Player == Player; });

	if (Points.Num() < 2)
	{
		if (Index != INDEX_NONE)
		{
			mActivePreviews.RemoveAt(Index);
		}
		return;
	}

	const FString UserId = GetPlayerKeyFor(Player);
	FARSRemotePreview& Preview = (Index != INDEX_NONE) ? mActivePreviews[Index] : mActivePreviews.AddDefaulted_GetRef();
	Preview.Player = Player;
	Preview.Source = nullptr;
	Preview.SplinePoints = Points;
	Preview.Stencil = Stencil;
	Preview.Settings = GetSettingsFor(UserId);
}

void AARSSubsystem::RefreshPreviewSettings(const FString& UserId)
{
	if (!HasAuthority())
	{
		return;
	}
	for (FARSRemotePreview& Preview : mActivePreviews)
	{
		if (Preview.Player && GetPlayerKeyFor(Preview.Player) == UserId)
		{
			Preview.Settings = GetSettingsFor(UserId);
			return;
		}
	}
}

void AARSSubsystem::OnBuildableConstructed(AFGBuildable* Buildable)
{
	if (bIsGenerating || !HasAuthority())
	{
		return;
	}
	AFGBuildableRailroadTrack* Track = Cast<AFGBuildableRailroadTrack>(Buildable);
	if (!Track || Track->GetWorld() != GetWorld())
	{
		return;
	}
	if (Track->GetBlueprintProxy() != nullptr || Track->IsBuildableInsideBlueprintDesigner() || Track->GetIsOwnedByPlatform())
	{
		return;
	}
	FPendingTrack Pending;
	Pending.Track = Track;
	Pending.Payer = Cast<AFGCharacterPlayer>(Track->GetmBuildEffectInstignator());
	Pending.FrameNumber = GFrameCounter;
	mPendingTracks.Add(Pending);

	SetActorTickEnabled(true);
}

void AARSSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && mActivePreviews.Num() > 0)
	{
		mActivePreviews.RemoveAll([](const FARSRemotePreview& Preview)
		{
			return !IsValid(Preview.Player)
				|| (!IsValid(Preview.Source) && Preview.SplinePoints.Num() < 2);
		});
	}

	PlanPendingTracks();

	int32 StampsThisTick = 0;
	while (NumPendingStamps() > 0 && StampsThisTick < MaxStampsPerTick)
	{
		const FARSStampJob Job = mStampQueue[mStampQueueHead++];
		StampsThisTick++;

		AFGBlueprintProxy* SharedProxy = Job.bCombineIntoOneBlueprint ? mBatchProxies.FindRef(Job.BatchId).Get() : nullptr;

		FText FailReason;
		bIsGenerating = true;
		const bool bStamped = FARSGenerator::ExecuteStampJob(GetWorld(), Job, SharedProxy, FailReason);
		bIsGenerating = false;

		if (Job.bCombineIntoOneBlueprint && SharedProxy)
		{
			mBatchProxies.Add(Job.BatchId, SharedProxy);
		}

		if (!bStamped && !FailReason.IsEmpty() && !mWarnedBatches.Contains(Job.BatchId))
		{
			mWarnedBatches.Add(Job.BatchId);
			SendPlayerChatMessage(Job.Payer.Get(), FString::Printf(TEXT("Auto Rail Supports: %s"), *FailReason.ToString()));
		}
	}

	if (NumPendingStamps() == 0)
	{
		mStampQueue.Reset();
		mStampQueueHead = 0;
	}

	if (mPendingTracks.Num() == 0 && mStampQueue.Num() == 0)
	{
		mWarnedBatches.Empty();
		mBatchProxies.Empty();
		SetActorTickEnabled(false);
	}
}

void AARSSubsystem::PlanPendingTracks()
{
	if (mPendingTracks.Num() == 0)
	{
		return;
	}

	TArray<FPendingTrack> Ready;
	for (int32 i = mPendingTracks.Num() - 1; i >= 0; i--)
	{
		if (mPendingTracks[i].FrameNumber < GFrameCounter)
		{
			Ready.Add(mPendingTracks[i]);
			mPendingTracks.RemoveAt(i);
		}
	}
	if (Ready.Num() == 0)
	{
		return;
	}

	TMap<uint64, TArray<FPendingTrack>> Batches;
	for (const FPendingTrack& Pending : Ready)
	{
		Batches.FindOrAdd(Pending.FrameNumber).Add(Pending);
	}

	for (TPair<uint64, TArray<FPendingTrack>>& Batch : Batches)
	{
		const uint64 BatchId = mNextBatchId++;

		TArray<AFGBuildableRailroadTrack*> Tracks;
		AFGCharacterPlayer* Payer = nullptr;
		for (const FPendingTrack& Pending : Batch.Value)
		{
			if (AFGBuildableRailroadTrack* Track = Pending.Track.Get())
			{
				Tracks.Add(Track);
				if (!Payer)
				{
					Payer = Cast<AFGCharacterPlayer>(Track->GetmBuildEffectInstignator());
				}
			}
			if (!Payer)
			{
				Payer = Pending.Payer.Get();
			}
		}
		if (Tracks.Num() == 0)
		{
			continue;
		}
		if (!Payer)
		{
			AFGCharacterPlayer* OnlyPlayer = nullptr;
			int32 PlayerCount = 0;
			for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
			{
				OnlyPlayer = *It;
				PlayerCount++;
			}
			if (PlayerCount == 1)
			{
				Payer = OnlyPlayer;
			}
		}
		if (!Payer)
		{
			SendSystemChatMessage(TEXT("Auto Rail Supports: could not determine the building player, supports were skipped."));
			continue;
		}

		const FString UserId = GetPlayerKeyFor(Payer);
		if (!GetSettingsFor(UserId).bEnabled)
		{
			continue;
		}
		TArray<FString> Warnings;
		const FARSSettings Settings = ResolveSettingsFor(UserId, &Warnings);
		for (const FString& Warning : Warnings)
		{
			SendPlayerChatMessage(Payer, FString::Printf(TEXT("Auto Rail Supports: %s"), *Warning));
		}
		if (!Settings.UnderRailDescriptor && Settings.PillarSlots.Num() == 0)
		{
			continue;
		}

		const TArray<TArray<FARSChainSegment>> Chains = FARSGenerator::BuildChains(Tracks);
		for (const TArray<FARSChainSegment>& Chain : Chains)
		{
			FARSGenerator::PlanChain(Chain, Settings, Payer, BatchId, mStampQueue);
		}
	}
}

void AARSSubsystem::ApplyToTrack(AFGBuildableRailroadTrack* Track, AFGCharacterPlayer* Payer)
{
	if (!Track || !Payer || !HasAuthority())
	{
		return;
	}

	TArray<FString> Warnings;
	const FARSSettings Settings = ResolveSettingsFor(GetPlayerKeyFor(Payer), &Warnings);
	for (const FString& Warning : Warnings)
	{
		SendPlayerChatMessage(Payer, FString::Printf(TEXT("Auto Rail Supports: %s"), *Warning));
	}
	if (!Settings.UnderRailDescriptor && Settings.PillarSlots.Num() == 0)
	{
		return;
	}

	TArray<AFGBuildableRailroadTrack*> Tracks = { Track };
	const TArray<TArray<FARSChainSegment>> Chains = FARSGenerator::BuildChains(Tracks);
	const uint64 BatchId = mNextBatchId++;
	for (const TArray<FARSChainSegment>& Chain : Chains)
	{
		FARSGenerator::PlanChain(Chain, Settings, Payer, BatchId, mStampQueue);
	}
	SetActorTickEnabled(true);
}

const AARSSubsystem::FARSMeasuredBlueprint& AARSSubsystem::GetMeasuredBlueprint(UFGBlueprintDescriptor* Descriptor)
{
	const double Now = FPlatformTime::Seconds();
	if (const FARSMeasuredBlueprint* Existing = mMeasureCache.Find(Descriptor))
	{
		const bool bCanRetry = !Existing->bMeasured && !HasAuthority()
			&& Existing->Attempts < MaxMeasureAttempts
			&& Now - Existing->LastAttemptSeconds >= MeasureRetrySeconds;
		if (!bCanRetry)
		{
			return *Existing;
		}
	}

	FARSMeasuredBlueprint& Entry = mMeasureCache.FindOrAdd(Descriptor);
	Entry.LastAttemptSeconds = Now;
	Entry.Attempts++;

	{
		TGuardValue<bool> GenerationGuard(bIsGenerating, true);
		Entry.bMeasured = HasAuthority()
			? FARSGenerator::MeasureBlueprintLocalBounds(GetWorld(), Descriptor, Entry.LocalBounds, &Entry.Pieces)
			: FARSGenerator::MeasureBlueprintInHologramWorld(GetWorld(), Descriptor, Entry.LocalBounds, &Entry.Pieces);
	}

	for (const FARSPreviewPiece& Piece : Entry.Pieces)
	{
		if (Piece.Mesh.IsValid())
		{
			mMeasureKeepAlive.Add(Piece.Mesh.Get());
		}
	}

	return Entry;
}

FARSSettings AARSSubsystem::ResolveSettingsFor(const FString& UserId, TArray<FString>* OutWarnings)
{
	return ResolveSettingsFrom(GetSettingsFor(UserId), AFGBlueprintSubsystem::Get(GetWorld()),
		[this](UFGBlueprintDescriptor* Descriptor, FBox& OutBounds)
		{
			OutBounds = GetMeasuredBlueprint(Descriptor).LocalBounds;
		},
		OutWarnings);
}

FARSSettings AARSSubsystem::ResolveSettingsFrom(FARSSettingsData Data, AFGBlueprintSubsystem* BlueprintSubsystem,
                                                const FARSMeasureFn& Measure, TArray<FString>* OutWarnings)
{
	ARS_SCOPE_PERF(ResolveSettings);
	Data.EnsureSlotArraySizes();

	FARSSettings Settings;
	Settings.UnderRailLength = FMath::Clamp(Data.UnderRailLength, 50.0f, 20000.0f);
	Settings.bCoverEndGap = Data.bCoverEndGap;
	Settings.bCombineIntoOneBlueprint = Data.bCombineIntoOneBlueprint;
	Settings.UnderRailRotationOffset = Data.UnderRailRotationOffset;
	Settings.UnderRailZOffset = Data.UnderRailZOffset;
	Settings.UnderRailSideOffset = Data.UnderRailSideOffset;
	Settings.UnderRailForwardOffset = Data.UnderRailForwardOffset;

	if (!BlueprintSubsystem)
	{
		if (OutWarnings)
		{
			OutWarnings->Add(TEXT("blueprint subsystem unavailable."));
		}
		return Settings;
	}

	if (!Data.UnderRailBlueprintName.IsEmpty())
	{
		Settings.UnderRailDescriptor = BlueprintSubsystem->GetBlueprintDescriptorByNameString(Data.UnderRailBlueprintName);
		if (Settings.UnderRailDescriptor)
		{
			// measured bounds drive the top-at-rail anchoring and the dismantle outline
			Measure(Settings.UnderRailDescriptor, Settings.UnderRailLocalBounds);
		}
		else if (OutWarnings)
		{
			OutWarnings->Add(FString::Printf(TEXT("under-rail blueprint '%s' not found, slot disabled."), *Data.UnderRailBlueprintName));
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < FARSSettingsData::NumPillarSlots; SlotIndex++)
	{
		const FString& PillarName = Data.PillarBlueprintNames[SlotIndex];
		if (PillarName.IsEmpty())
		{
			continue;
		}
		UFGBlueprintDescriptor* Descriptor = BlueprintSubsystem->GetBlueprintDescriptorByNameString(PillarName);
		if (!Descriptor)
		{
			if (OutWarnings)
			{
				OutWarnings->Add(FString::Printf(TEXT("pillar %d blueprint '%s' not found, slot disabled."), SlotIndex + 1, *PillarName));
			}
			continue;
		}
		FARSPillarSlot& Slot = Settings.PillarSlots.AddDefaulted_GetRef();
		Slot.Descriptor = Descriptor;
		Slot.EveryN = FMath::Max(1, Data.PillarEveryNs[SlotIndex]);
		Slot.SpacingOffset = FMath::Max(0, Data.PillarSpacingOffsets[SlotIndex]);
		Slot.bAlignToRail = Data.PillarAlignToRail[SlotIndex];
		Slot.Priority = SlotIndex; // slot 4 outranks slot 1 when both land on the same segment
		Slot.bPreventOverlap = Data.PillarPreventOverlap[SlotIndex];
		Slot.RotationOffset = Data.PillarRotationOffsets[SlotIndex];
		Slot.ZOffset = Data.PillarZOffsets[SlotIndex];
		Slot.SideOffset = Data.PillarSideOffsets[SlotIndex];
		Slot.ForwardOffset = Data.PillarForwardOffsets[SlotIndex];
		Measure(Descriptor, Slot.LocalBounds);
	}
	return Settings;
}

bool AARSSubsystem::ShouldWarnStampCount(float UnderRailLengthCm, float BlueprintLengthMeters)
{
	// warn on a very small value, or on heavy overlap
	const float LengthMeters = UnderRailLengthCm / 100.0f;
	bool bWarn = LengthMeters <= 1.0f;
	if (BlueprintLengthMeters > 0.0f)
	{
		bWarn |= LengthMeters < BlueprintLengthMeters * 0.5f;
	}
	return bWarn;
}

FText AARSSubsystem::GetStampCountWarningText()
{
	return NSLOCTEXT("AutoRailSupports", "LowLengthWarning",
		"Very low tiling length: every rail stamps many overlapping copies of the blueprint, which can create a huge number of building parts.");
}

void AARSSubsystem::SendPlayerChatMessage(AFGCharacterPlayer* Player, const FString& Message) const
{
	if (!Player)
	{
		return;
	}
	if (AFGPlayerController* PlayerController = Cast<AFGPlayerController>(Player->GetController()))
	{
		if (UARSRemoteCallObject* RCO = PlayerController->GetRemoteCallObjectOfClass<UARSRemoteCallObject>())
		{
			RCO->Client_Notify(Message);
		}
	}
}

void AARSSubsystem::SendSystemChatMessage(const FString& Message) const
{
	if (AFGChatManager* ChatManager = AFGChatManager::Get(GetWorld()))
	{
		FChatMessageStruct ChatMessage;
		ChatMessage.MessageText = FText::FromString(Message);
		ChatMessage.MessageType = EFGChatMessageType::CMT_SystemMessage;
		ChatManager->BroadcastChatMessage(ChatMessage);
	}
}
