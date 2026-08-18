#include "ARSCurve.h"

#include "ARSGenerator.h"
#include "Algo/Reverse.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "Components/SplineComponent.h"

namespace
{
	constexpr float MinArcAngleRadians = 0.0005f;
	constexpr float DuplicatePointToleranceSq = 1.0f;

	FVector FlatDirection(const FVector& Tangent)
	{
		return FVector(Tangent.X, Tangent.Y, 0.0f).GetSafeNormal();
	}

	// how much longer the partner's tangents are on this segment
	float SegmentScale(const FVector& LeaveTangent, const FVector& NextArriveTangent, float Offset)
	{
		const FVector Start(LeaveTangent.X, LeaveTangent.Y, 0.0f);
		const FVector End(NextArriveTangent.X, NextArriveTangent.Y, 0.0f);
		const FVector StartDirection = Start.GetSafeNormal();
		const FVector EndDirection = End.GetSafeNormal();
		if (StartDirection.IsNearlyZero() || EndDirection.IsNearlyZero())
		{
			return 1.0f;
		}

		const float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(StartDirection, EndDirection), -1.0f, 1.0f));
		if (Angle < MinArcAngleRadians)
		{
			return 1.0f; // straight, so parallel rails get equal tangents
		}

		const float Magnitude = 0.5f * (Start.Size() + End.Size());
		const float Radius = Magnitude / (4.0f * FMath::Tan(Angle * 0.25f));
		if (Radius < KINDA_SMALL_NUMBER)
		{
			return 1.0f;
		}

		// a positive cross is a turn towards +Y
		const float Turn = FVector::CrossProduct(StartDirection, EndDirection).Z >= 0.0f ? -1.0f : 1.0f;
		return FMath::Max(0.01f, 1.0f + Offset * Turn / Radius);
	}
}

void FARSCurve::ReadPoints(const USplineComponent* Spline, TArray<FARSCurvePoint>& OutPoints)
{
	OutPoints.Reset();
	if (!Spline)
	{
		return;
	}
	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	OutPoints.Reserve(NumPoints);
	for (int32 i = 0; i < NumPoints; i++)
	{
		FARSCurvePoint& Point = OutPoints.AddDefaulted_GetRef();
		Point.Location = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		Point.ArriveTangent = Spline->GetArriveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
		Point.LeaveTangent = Spline->GetLeaveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
	}
}

void FARSCurve::ReadChainPoints(const TArray<FARSChainSegment>& Chain, TArray<FARSCurvePoint>& OutPoints)
{
	OutPoints.Reset();

	TArray<FARSCurvePoint> TrackPoints;
	for (const FARSChainSegment& Segment : Chain)
	{
		if (!Segment.Track)
		{
			continue;
		}
		ReadPoints(Segment.Track->FindComponentByClass<USplineComponent>(), TrackPoints);
		if (TrackPoints.Num() == 0)
		{
			continue;
		}

		if (Segment.bReversed)
		{
			Algo::Reverse(TrackPoints);
			for (FARSCurvePoint& Point : TrackPoints)
			{
				const FVector Arrive = Point.ArriveTangent;
				Point.ArriveTangent = -Point.LeaveTangent;
				Point.LeaveTangent = -Arrive;
			}
		}

		int32 FirstNew = 0;
		if (OutPoints.Num() > 0 &&
			FVector::DistSquared(OutPoints.Last().Location, TrackPoints[0].Location) <= DuplicatePointToleranceSq)
		{
			OutPoints.Last().LeaveTangent = TrackPoints[0].LeaveTangent;
			FirstNew = 1;
		}
		for (int32 i = FirstNew; i < TrackPoints.Num(); i++)
		{
			OutPoints.Add(TrackPoints[i]);
		}
	}
}

void FARSCurve::OffsetPoints(const TArray<FARSCurvePoint>& InPoints, float Offset, TArray<FARSCurvePoint>& OutPoints)
{
	OutPoints.Reset();
	const int32 NumPoints = InPoints.Num();
	if (NumPoints < 2)
	{
		return;
	}

	TArray<float> SegmentScales;
	SegmentScales.Reserve(NumPoints - 1);
	for (int32 i = 0; i < NumPoints - 1; i++)
	{
		SegmentScales.Add(SegmentScale(InPoints[i].LeaveTangent, InPoints[i + 1].ArriveTangent, Offset));
	}

	OutPoints.Reserve(NumPoints);
	for (int32 i = 0; i < NumPoints; i++)
	{
		const FARSCurvePoint& In = InPoints[i];

		FVector Heading = FlatDirection(In.ArriveTangent) + FlatDirection(In.LeaveTangent);
		Heading = Heading.GetSafeNormal();
		if (Heading.IsNearlyZero())
		{
			Heading = FVector::ForwardVector;
		}
		const FVector Normal = FVector::CrossProduct(FVector::UpVector, Heading);

		const float ArriveScale = SegmentScales[FMath::Max(0, i - 1)];
		const float LeaveScale = SegmentScales[FMath::Min(i, SegmentScales.Num() - 1)];

		FARSCurvePoint& Out = OutPoints.AddDefaulted_GetRef();
		Out.Location = In.Location + Normal * Offset; // normal is horizontal
		Out.ArriveTangent = In.ArriveTangent * ArriveScale;
		Out.LeaveTangent = In.LeaveTangent * LeaveScale;
	}
}

USplineComponent* FARSCurve::MakeScratchSpline(const TArray<FARSCurvePoint>& Points)
{
	if (Points.Num() < 2)
	{
		return nullptr;
	}
	USplineComponent* Spline = NewObject<USplineComponent>(GetTransientPackage());
	Spline->ReparamStepsPerSegment = 32;
	Spline->ClearSplinePoints(false);
	for (int32 i = 0; i < Points.Num(); i++)
	{
		Spline->AddSplinePoint(Points[i].Location, ESplineCoordinateSpace::World, false);
		Spline->SetTangentsAtSplinePoint(i, Points[i].ArriveTangent, Points[i].LeaveTangent,
			ESplineCoordinateSpace::World, false);
	}
	Spline->UpdateSpline();
	return Spline;
}
