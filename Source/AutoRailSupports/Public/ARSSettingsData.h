#pragma once
#include "CoreMinimal.h"
#include "ARSSettingsData.generated.h"

USTRUCT()
struct AUTORAILSUPPORTS_API FARSSettingsData
{
	GENERATED_BODY()

	static constexpr int32 NumPillarSlots = 4;

	// master toggle
	UPROPERTY(SaveGame)
	bool bEnabled = true;

	// combined blueprint
	UPROPERTY(SaveGame)
	bool bCombineIntoOneBlueprint = false;

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
