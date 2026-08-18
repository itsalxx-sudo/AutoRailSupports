#pragma once
#include "CoreMinimal.h"
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
struct AUTORAILSUPPORTS_API FARSSettingsData
{
	GENERATED_BODY()

	static constexpr int32 NumPillarSlots = 4;

	static constexpr float MinTrackSeparation = 100.0f;

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

	// pillar blueprint
	UPROPERTY(SaveGame)
	TArray<FString> PillarBlueprintNames;

	// pillar spacing
	UPROPERTY(SaveGame)
	TArray<int32> PillarEveryNs;

	// spacing offset
	UPROPERTY(SaveGame)
	TArray<int32> PillarSpacingOffsets;

	// follow incline
	UPROPERTY(SaveGame)
	TArray<bool> PillarAlignToRail;

	// prevent overlap
	UPROPERTY(SaveGame)
	TArray<bool> PillarPreventOverlap;

	// pillar rotation
	UPROPERTY(SaveGame)
	TArray<FRotator> PillarRotationOffsets;

	// pillar vertical offset
	UPROPERTY(SaveGame)
	TArray<float> PillarZOffsets;

	// pillar perpendicular offset
	UPROPERTY(SaveGame)
	TArray<float> PillarSideOffsets;

	// pillar forward/backward offset
	UPROPERTY(SaveGame)
	TArray<float> PillarForwardOffsets;

	// pillar side
	UPROPERTY(SaveGame)
	TArray<EARSSideMode> PillarSideModes;

	// pillar auto roll
	UPROPERTY(SaveGame)
	TArray<bool> PillarAutoRoll;

	FARSSettingsData()
	{
		EnsureSlotArraySizes();
	}

	void EnsureSlotArraySizes()
	{
		if (PillarBlueprintNames.Num() < NumPillarSlots)
		{
			PillarBlueprintNames.SetNum(NumPillarSlots);
		}
		while (PillarEveryNs.Num() < NumPillarSlots)
		{
			PillarEveryNs.Add(4);
		}
		while (PillarSpacingOffsets.Num() < NumPillarSlots)
		{
			PillarSpacingOffsets.Add(0);
		}
		if (PillarAlignToRail.Num() < NumPillarSlots)
		{
			PillarAlignToRail.SetNum(NumPillarSlots);
		}
		while (PillarPreventOverlap.Num() < NumPillarSlots)
		{
			PillarPreventOverlap.Add(true); // defaults on for every slot
		}
		while (PillarRotationOffsets.Num() < NumPillarSlots)
		{
			PillarRotationOffsets.Add(FRotator::ZeroRotator);
		}
		while (PillarZOffsets.Num() < NumPillarSlots)
		{
			PillarZOffsets.Add(0.0f);
		}
		while (PillarSideOffsets.Num() < NumPillarSlots)
		{
			PillarSideOffsets.Add(0.0f);
		}
		while (PillarForwardOffsets.Num() < NumPillarSlots)
		{
			PillarForwardOffsets.Add(0.0f);
		}
		while (PillarSideModes.Num() < NumPillarSlots)
		{
			PillarSideModes.Add(EARSSideMode::Center);
		}
		if (PillarAutoRoll.Num() < NumPillarSlots)
		{
			PillarAutoRoll.SetNum(NumPillarSlots);
		}
	}

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
		EnsureSlotArraySizes();
		UnderRailLength = FMath::Clamp(UnderRailLength, 50.0f, 20000.0f);
		UnderRailZOffset = FMath::Clamp(UnderRailZOffset, -10000.0f, 10000.0f);
		UnderRailSideOffset = FMath::Clamp(UnderRailSideOffset, -10000.0f, 10000.0f);
		UnderRailForwardOffset = FMath::Clamp(UnderRailForwardOffset, -10000.0f, 10000.0f);
		for (int32 i = 0; i < PillarEveryNs.Num(); i++)
		{
			PillarEveryNs[i] = FMath::Clamp(PillarEveryNs[i], 1, 100);
		}
		for (int32& SpacingOffset : PillarSpacingOffsets)
		{
			SpacingOffset = FMath::Clamp(SpacingOffset, 0, 100);
		}
		for (float& ZOffset : PillarZOffsets)
		{
			ZOffset = FMath::Clamp(ZOffset, -10000.0f, 10000.0f);
		}
		for (float& SideOffset : PillarSideOffsets)
		{
			SideOffset = FMath::Clamp(SideOffset, -10000.0f, 10000.0f);
		}
		for (float& ForwardOffset : PillarForwardOffsets)
		{
			ForwardOffset = FMath::Clamp(ForwardOffset, -10000.0f, 10000.0f);
		}
		NormalizeRotation(UnderRailRotationOffset);
		for (FRotator& Rotation : PillarRotationOffsets)
		{
			NormalizeRotation(Rotation);
		}
		const float SeparationMagnitude = FMath::Clamp(FMath::Abs(TrackSeparation), MinTrackSeparation, 10000.0f);
		TrackSeparation = TrackSeparation < 0.0f ? -SeparationMagnitude : SeparationMagnitude;
		ClampSideMode(UnderRailSideMode);
		for (EARSSideMode& Mode : PillarSideModes)
		{
			ClampSideMode(Mode);
		}
		UnderRailPartner.Length = FMath::Clamp(UnderRailPartner.Length, 50.0f, 20000.0f);
		UnderRailPartner.ZOffset = FMath::Clamp(UnderRailPartner.ZOffset, -10000.0f, 10000.0f);
		UnderRailPartner.SideOffset = FMath::Clamp(UnderRailPartner.SideOffset, -10000.0f, 10000.0f);
		UnderRailPartner.ForwardOffset = FMath::Clamp(UnderRailPartner.ForwardOffset, -10000.0f, 10000.0f);
		NormalizeRotation(UnderRailPartner.RotationOffset);
	}

	void SeedPartnerFromUnderRail()
	{
		UnderRailPartner.Length = UnderRailLength;
		UnderRailPartner.bCoverEndGap = bCoverEndGap;
		UnderRailPartner.RotationOffset = UnderRailRotationOffset;
		UnderRailPartner.ZOffset = UnderRailZOffset;
		UnderRailPartner.SideOffset = UnderRailSideOffset;
		UnderRailPartner.ForwardOffset = UnderRailForwardOffset;
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
