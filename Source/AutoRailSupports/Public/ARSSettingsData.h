#pragma once
#include "CoreMinimal.h"
#include "Algo/Reverse.h"
#include "ARSSettingsData.generated.h"

UENUM()
enum class EARSSideMode : uint8
{
	// centered between the tracks
	Center,
	// on each rail, the second rotated 180 degrees
	BothMirrored,
	// on each rail, both pointing the same way
	BothSameFacing,
	LeftOnly,
	RightOnly
};

USTRUCT()
struct AUTORAILSUPPORTS_API FARSUnderRailSide
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FString BlueprintName;

	UPROPERTY(SaveGame)
	float Length = 800.0f;

	UPROPERTY(SaveGame)
	bool bCoverEndGap = true;

	UPROPERTY(SaveGame)
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(SaveGame)
	float ZOffset = 0.0f;

	UPROPERTY(SaveGame)
	float SideOffset = 0.0f;

	UPROPERTY(SaveGame)
	float ForwardOffset = 0.0f;
};

USTRUCT()
struct AUTORAILSUPPORTS_API FARSPillarSlotData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FString BlueprintName;

	UPROPERTY(SaveGame)
	int32 EveryN = 4;

	UPROPERTY(SaveGame)
	int32 SpacingOffset = 0;

	UPROPERTY(SaveGame)
	bool bAlignToRail = false;

	UPROPERTY(SaveGame)
	bool bPreventOverlap = true;

	UPROPERTY(SaveGame)
	bool bAutoRoll = false;

	UPROPERTY(SaveGame)
	bool bExpanded = false;

	UPROPERTY(SaveGame)
	EARSSideMode SideMode = EARSSideMode::Center;

	UPROPERTY(SaveGame)
	float RotationPitch = 0.0f;

	UPROPERTY(SaveGame)
	float RotationYaw = 0.0f;

	UPROPERTY(SaveGame)
	float RotationRoll = 0.0f;

	UPROPERTY(SaveGame)
	float ZOffset = 0.0f;

	UPROPERTY(SaveGame)
	float SideOffset = 0.0f;

	UPROPERTY(SaveGame)
	float ForwardOffset = 0.0f;

	FRotator GetRotation() const { return FRotator(RotationPitch, RotationYaw, RotationRoll); }

	void SetRotation(const FRotator& Rotation)
	{
		RotationPitch = Rotation.Pitch;
		RotationYaw = Rotation.Yaw;
		RotationRoll = Rotation.Roll;
	}
};

USTRUCT()
struct AUTORAILSUPPORTS_API FARSSettingsData
{
	GENERATED_BODY()

	static constexpr int32 MaxPillarSlots = 100;

	static constexpr int32 DefaultMaxPillarSlots = 32;

	static constexpr float MinTrackSeparation = 100.0f;

	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(SaveGame)
	int32 Version = 0;

	// master toggle
	UPROPERTY(SaveGame)
	bool bEnabled = true;

	// combined blueprint
	UPROPERTY(SaveGame)
	bool bCombineIntoOneBlueprint = false;

	// double rails
	UPROPERTY(SaveGame)
	bool bDoubleRails = false;

	// track separation
	UPROPERTY(SaveGame)
	float TrackSeparation = 600.0f;

	// under-rail blueprint
	UPROPERTY(SaveGame)
	FString UnderRailBlueprintName;

	// tiling length
	UPROPERTY(SaveGame)
	float UnderRailLength = 800.0f;

	// cover end gap
	UPROPERTY(SaveGame)
	bool bCoverEndGap = true;

	// rotation
	UPROPERTY(SaveGame)
	FRotator UnderRailRotationOffset = FRotator::ZeroRotator;

	// vertical offset
	UPROPERTY(SaveGame)
	float UnderRailZOffset = 0.0f;

	// perpendicular offset
	UPROPERTY(SaveGame)
	float UnderRailSideOffset = 0.0f;

	// forward/backward offset
	UPROPERTY(SaveGame)
	float UnderRailForwardOffset = 0.0f;

	// under-rail side
	UPROPERTY(SaveGame)
	EARSSideMode UnderRailSideMode = EARSSideMode::BothMirrored;

	// under-rail auto roll
	UPROPERTY(SaveGame)
	bool bUnderRailAutoRoll = false;

	// split under-rail sides
	UPROPERTY(SaveGame)
	bool bSplitUnderRailSides = false;

	// second track's under-rail
	UPROPERTY(SaveGame)
	FARSUnderRailSide UnderRailPartner;

	// pillar slots, highest priority first
	UPROPERTY(SaveGame)
	TArray<FARSPillarSlotData> PillarSlots;

	//pre-1.1
	UPROPERTY(SaveGame) TArray<FString> PillarBlueprintNames;
	UPROPERTY(SaveGame) TArray<int32> PillarEveryNs;
	UPROPERTY(SaveGame) TArray<int32> PillarSpacingOffsets;
	UPROPERTY(SaveGame) TArray<bool> PillarAlignToRail;
	UPROPERTY(SaveGame) TArray<bool> PillarPreventOverlap;
	UPROPERTY(SaveGame) TArray<float> PillarRotations;
	UPROPERTY(SaveGame) TArray<FRotator> PillarRotationOffsets;
	UPROPERTY(SaveGame) TArray<float> PillarZOffsets;
	UPROPERTY(SaveGame) TArray<float> PillarSideOffsets;
	UPROPERTY(SaveGame) TArray<float> PillarForwardOffsets;
	UPROPERTY(SaveGame) TArray<EARSSideMode> PillarSideModes;
	UPROPERTY(SaveGame) TArray<bool> PillarAutoRoll;

	int32 NumPillarSlots() const { return PillarSlots.Num(); }

	static int32 ClampPillarSlotLimit(int32 Limit) { return FMath::Clamp(Limit, 1, MaxPillarSlots); }

	bool CanAddPillarSlot(int32 Limit) const { return PillarSlots.Num() < ClampPillarSlotLimit(Limit); }

	void ClampPillarSlotsTo(int32 Limit)
	{
		const int32 Clamped = ClampPillarSlotLimit(Limit);
		if (PillarSlots.Num() > Clamped)
		{
			PillarSlots.SetNum(Clamped);
		}
	}

	void Normalize()
	{
		MigrateLegacyPillarSlots();
		if (PillarSlots.Num() > MaxPillarSlots)
		{
			PillarSlots.SetNum(MaxPillarSlots);
		}
	}

	int32 AddPillarSlot(int32 Limit)
	{
		return CanAddPillarSlot(Limit) ? PillarSlots.AddDefaulted() : INDEX_NONE;
	}

	void RemovePillarSlot(int32 Index)
	{
		if (PillarSlots.IsValidIndex(Index))
		{
			PillarSlots.RemoveAt(Index);
		}
	}

	bool MovePillarSlot(int32 Index, int32 Delta)
	{
		const int32 Target = Index + Delta;
		if (!PillarSlots.IsValidIndex(Index) || !PillarSlots.IsValidIndex(Target))
		{
			return false;
		}
		PillarSlots.Swap(Index, Target);
		return true;
	}

	FARSUnderRailSide GetUnderRailSide(bool bPartner) const
	{
		if (bPartner)
		{
			FARSUnderRailSide Partner = UnderRailPartner;
			if (PartnerSharesBlueprint())
			{
				Partner.BlueprintName = UnderRailBlueprintName;
			}
			return Partner;
		}
		FARSUnderRailSide Side;
		Side.BlueprintName = UnderRailBlueprintName;
		Side.Length = UnderRailLength;
		Side.bCoverEndGap = bCoverEndGap;
		Side.RotationOffset = UnderRailRotationOffset;
		Side.ZOffset = UnderRailZOffset;
		Side.SideOffset = UnderRailSideOffset;
		Side.ForwardOffset = UnderRailForwardOffset;
		return Side;
	}

	void SetUnderRailSide(bool bPartner, const FARSUnderRailSide& Side)
	{
		if (bPartner)
		{
			const FString Kept = UnderRailPartner.BlueprintName;
			UnderRailPartner = Side;
			if (PartnerSharesBlueprint())
			{
				UnderRailPartner.BlueprintName = Kept;
			}
			return;
		}
		UnderRailBlueprintName = Side.BlueprintName;
		UnderRailLength = Side.Length;
		bCoverEndGap = Side.bCoverEndGap;
		UnderRailRotationOffset = Side.RotationOffset;
		UnderRailZOffset = Side.ZOffset;
		UnderRailSideOffset = Side.SideOffset;
		UnderRailForwardOffset = Side.ForwardOffset;
	}

	bool HasSplitUnderRailSides() const
	{
		return bDoubleRails && bSplitUnderRailSides && UnderRailSideMode != EARSSideMode::Center;
	}

	bool PartnerSharesBlueprint() const { return UnderRailSideMode == EARSSideMode::BothMirrored; }

	static void ClampSideMode(EARSSideMode& Mode)
	{
		if (static_cast<uint8>(Mode) > static_cast<uint8>(EARSSideMode::RightOnly))
		{
			Mode = EARSSideMode::Center;
		}
	}

	static void NormalizeRotation(FRotator& Rotation)
	{
		Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw);
		Rotation.Pitch = FRotator::NormalizeAxis(Rotation.Pitch);
		Rotation.Roll = FRotator::NormalizeAxis(Rotation.Roll);
	}

	// server-side sanitation of whatever the client sent
	void ClampValues()
	{
		Normalize();
		UnderRailLength = FMath::Clamp(UnderRailLength, 50.0f, 20000.0f);
		UnderRailZOffset = FMath::Clamp(UnderRailZOffset, -10000.0f, 10000.0f);
		UnderRailSideOffset = FMath::Clamp(UnderRailSideOffset, -10000.0f, 10000.0f);
		UnderRailForwardOffset = FMath::Clamp(UnderRailForwardOffset, -10000.0f, 10000.0f);
		NormalizeRotation(UnderRailRotationOffset);
		for (FARSPillarSlotData& Slot : PillarSlots)
		{
			Slot.EveryN = FMath::Clamp(Slot.EveryN, 1, 100);
			Slot.SpacingOffset = FMath::Clamp(Slot.SpacingOffset, 0, 100);
			Slot.ZOffset = FMath::Clamp(Slot.ZOffset, -10000.0f, 10000.0f);
			Slot.SideOffset = FMath::Clamp(Slot.SideOffset, -10000.0f, 10000.0f);
			Slot.ForwardOffset = FMath::Clamp(Slot.ForwardOffset, -10000.0f, 10000.0f);
			FRotator Rotation = Slot.GetRotation();
			NormalizeRotation(Rotation);
			Slot.SetRotation(Rotation);
			ClampSideMode(Slot.SideMode);
		}
		const float SeparationMagnitude = FMath::Clamp(FMath::Abs(TrackSeparation), MinTrackSeparation, 10000.0f);
		TrackSeparation = TrackSeparation < 0.0f ? -SeparationMagnitude : SeparationMagnitude;
		ClampSideMode(UnderRailSideMode);
		UnderRailPartner.Length = FMath::Clamp(UnderRailPartner.Length, 50.0f, 20000.0f);
		UnderRailPartner.ZOffset = FMath::Clamp(UnderRailPartner.ZOffset, -10000.0f, 10000.0f);
		UnderRailPartner.SideOffset = FMath::Clamp(UnderRailPartner.SideOffset, -10000.0f, 10000.0f);
		UnderRailPartner.ForwardOffset = FMath::Clamp(UnderRailPartner.ForwardOffset, -10000.0f, 10000.0f);
		NormalizeRotation(UnderRailPartner.RotationOffset);
	}

	void SeedPartnerFromUnderRail()
	{
		UnderRailPartner = GetUnderRailSide(/*bPartner*/ false);
	}

private:
	void MigrateLegacyPillarSlots()
	{
		if (Version >= CurrentVersion)
		{
			return;
		}
		Version = CurrentVersion;

		const int32 LegacyCount = FMath::Max(PillarBlueprintNames.Num(),
			FMath::Max(PillarEveryNs.Num(), PillarSideModes.Num()));
		auto At = [](const auto& Array, int32 Index, auto Fallback)
		{
			return Array.IsValidIndex(Index) ? Array[Index] : Fallback;
		};
		for (int32 i = 0; i < LegacyCount; i++)
		{
			FARSPillarSlotData Slot;
			Slot.BlueprintName = At(PillarBlueprintNames, i, FString());
			Slot.EveryN = At(PillarEveryNs, i, 4);
			Slot.SpacingOffset = At(PillarSpacingOffsets, i, 0);
			Slot.bAlignToRail = At(PillarAlignToRail, i, false);
			Slot.bPreventOverlap = At(PillarPreventOverlap, i, true);
			Slot.bAutoRoll = At(PillarAutoRoll, i, false);
			Slot.SideMode = At(PillarSideModes, i, EARSSideMode::Center);
			// two legacy rotation shapes, the FRotator array only exists in pre-1.0.2 data
			Slot.RotationPitch = At(PillarRotations, i * 3 + 0, 0.0f);
			Slot.RotationYaw = At(PillarRotations, i * 3 + 1, 0.0f);
			Slot.RotationRoll = At(PillarRotations, i * 3 + 2, 0.0f);
			if (PillarRotationOffsets.IsValidIndex(i))
			{
				Slot.SetRotation(PillarRotationOffsets[i]);
			}
			Slot.ZOffset = At(PillarZOffsets, i, 0.0f);
			Slot.SideOffset = At(PillarSideOffsets, i, 0.0f);
			Slot.ForwardOffset = At(PillarForwardOffsets, i, 0.0f);
			if (!Slot.BlueprintName.IsEmpty())
			{
				PillarSlots.Add(Slot);
			}
		}
		Algo::Reverse(PillarSlots);

		PillarBlueprintNames.Reset();
		PillarEveryNs.Reset();
		PillarSpacingOffsets.Reset();
		PillarAlignToRail.Reset();
		PillarPreventOverlap.Reset();
		PillarRotations.Reset();
		PillarRotationOffsets.Reset();
		PillarZOffsets.Reset();
		PillarSideOffsets.Reset();
		PillarForwardOffsets.Reset();
		PillarSideModes.Reset();
		PillarAutoRoll.Reset();
	}
};

USTRUCT()
struct AUTORAILSUPPORTS_API FARSSplinePoint
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FVector ArriveTangent = FVector::ZeroVector;

	UPROPERTY()
	FVector LeaveTangent = FVector::ZeroVector;
};

USTRUCT()
struct AUTORAILSUPPORTS_API FARSRemotePreview
{
	GENERATED_BODY()

	// whose preview this is
	UPROPERTY()
	TObjectPtr<class AFGCharacterPlayer> Player = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY()
	TArray<FARSSplinePoint> SplinePoints;

	UPROPERTY()
	uint8 Stencil = 0;

	UPROPERTY()
	FARSSettingsData Settings;
};

USTRUCT()
struct AUTORAILSUPPORTS_API FARSPreset
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FARSSettingsData Settings;
};

USTRUCT()
struct AUTORAILSUPPORTS_API FARSGlobalConfig
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FARSPreset> Presets;
};
