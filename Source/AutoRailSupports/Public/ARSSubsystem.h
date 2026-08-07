#pragma once
#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "FGSaveInterface.h"
#include "ARSGenerator.h"
#include "ARSSettingsData.h"
#include "ARSSubsystem.generated.h"

class AFGBlueprintProxy;
class AFGBuildable;
class AFGBuildableRailroadTrack;
class AFGCharacterPlayer;
class APlayerState;

// server-side subsystem
UCLASS()
class AUTORAILSUPPORTS_API AARSSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()
public:
	AARSSubsystem();

	// authoritative on the server, a replicated copy on clients
	static AARSSubsystem* Get(UWorld* World);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {}
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {}
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {}
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {}
	virtual bool NeedTransform_Implementation() override { return false; }
	virtual bool ShouldSave_Implementation() const override { return true; }

	static FString GetPlayerIdentityString(const APlayerState* PlayerState);

	static bool IsPlayerIdentityReady(const APlayerState* PlayerState);

	static FString GetPlayerKey(const APlayerState* PlayerState);
	static FString GetPlayerKeyFor(const AFGCharacterPlayer* Player);

	const FARSSettingsData& GetSettingsFor(const FString& UserId) const;

	void ApplySettingsFor(const FString& UserId, const FARSSettingsData& NewSettings);

	bool SetEnabledSettingFor(const FString& UserId, bool bNewEnabled);

	void ApplyToTrack(AFGBuildableRailroadTrack* Track, AFGCharacterPlayer* Payer);

	void SetPreviewSource(AFGCharacterPlayer* Player, AActor* Source);

	void SetPreviewSpline(AFGCharacterPlayer* Player, const TArray<FARSSplinePoint>& Points, uint8 Stencil);

	void RefreshPreviewSettings(const FString& UserId);

	const TArray<FARSRemotePreview>& GetActivePreviews() const { return mActivePreviews; }

	struct FARSMeasuredBlueprint
	{
		FBox LocalBounds = FBox(ForceInit);
		TArray<FARSPreviewPiece> Pieces;
		bool bMeasured = false;
		double LastAttemptSeconds = 0.0;
		int32 Attempts = 0;
	};

	const FARSMeasuredBlueprint& GetMeasuredBlueprint(class UFGBlueprintDescriptor* Descriptor);

	using FARSMeasureFn = TFunction<void(class UFGBlueprintDescriptor*, FBox&)>;

	static FARSSettings ResolveSettingsFrom(FARSSettingsData Data, class AFGBlueprintSubsystem* BlueprintSubsystem,
	                                        const FARSMeasureFn& Measure, TArray<FString>* OutWarnings);

	static bool ShouldWarnStampCount(float UnderRailLengthCm, float BlueprintLengthMeters);

	static FText GetStampCountWarningText();

protected:
	virtual void Init() override;

private:
	UFUNCTION()
	void OnBuildableConstructed(AFGBuildable* Buildable);

	// drops the cached measurement for a blueprint edited in the designer, so the next use re-probes it
	UFUNCTION()
	void OnBlueprintDescriptorUpdated(class UFGBlueprintDescriptor* Descriptor);

	void TryBindConstructionDelegate();

	FARSSettings ResolveSettingsFor(const FString& UserId, TArray<FString>* OutWarnings = nullptr);

	void PlanPendingTracks();

	void SendPlayerChatMessage(AFGCharacterPlayer* Player, const FString& Message) const;

	void SendSystemChatMessage(const FString& Message) const;

	struct FPendingTrack
	{
		TWeakObjectPtr<AFGBuildableRailroadTrack> Track;
		TWeakObjectPtr<AFGCharacterPlayer> Payer;
		uint64 FrameNumber = 0;
	};

	UPROPERTY(SaveGame)
	FARSSettingsData mSettings;

	UPROPERTY(SaveGame)
	TMap<FString, FARSSettingsData> mPlayerSettings;

	UPROPERTY(Replicated)
	TArray<FARSRemotePreview> mActivePreviews;

	TArray<FPendingTrack> mPendingTracks;

	// planned stamps, executed a few per tick
	TArray<FARSStampJob> mStampQueue;

	int32 mStampQueueHead = 0;

	// stamps still waiting to execute
	int32 NumPendingStamps() const { return mStampQueue.Num() - mStampQueueHead; }

	TSet<uint64> mWarnedBatches;

	// shared proxy per batch
	TMap<uint64, TWeakObjectPtr<AFGBlueprintProxy>> mBatchProxies;

	uint64 mNextBatchId = 1;

	bool bIsGenerating = false;

	FTimerHandle mBindRetryTimerHandle;
	bool bDelegateBound = false;
	bool bDescriptorUpdateBound = false;

	static constexpr int32 MaxStampsPerTick = 6;

	static constexpr double MeasureRetrySeconds = 2.0;

	static constexpr int32 MaxMeasureAttempts = 8;

	// per-blueprint measurement cache
	TMap<TWeakObjectPtr<class UFGBlueprintDescriptor>, FARSMeasuredBlueprint> mMeasureCache;

	UPROPERTY()
	TSet<TObjectPtr<class UStaticMesh>> mMeasureKeepAlive;
};
