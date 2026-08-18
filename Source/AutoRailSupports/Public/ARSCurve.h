#pragma once
#include "CoreMinimal.h"

class USplineComponent;
struct FARSChainSegment;

struct FARSCurvePoint
{
	FVector Location = FVector::ZeroVector;
	FVector ArriveTangent = FVector::ZeroVector;
	FVector LeaveTangent = FVector::ZeroVector;
};

class AUTORAILSUPPORTS_API FARSCurve
{
public:
	static void ReadPoints(const USplineComponent* Spline, TArray<FARSCurvePoint>& OutPoints);

	static void ReadChainPoints(const TArray<FARSChainSegment>& Chain, TArray<FARSCurvePoint>& OutPoints);

	static void OffsetPoints(const TArray<FARSCurvePoint>& InPoints, float Offset, TArray<FARSCurvePoint>& OutPoints);

	static USplineComponent* MakeScratchSpline(const TArray<FARSCurvePoint>& Points);
};
