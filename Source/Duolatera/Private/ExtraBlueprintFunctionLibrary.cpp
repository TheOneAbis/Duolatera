// Fill out your copyright notice in the Description page of Project Settings.


#include "ExtraBlueprintFunctionLibrary.h"

#include "Components/SceneComponent.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "PortalComponent.h"

void UExtraBlueprintFunctionLibrary::UpdateNavComponentData(USceneComponent* component)
{
	if (component->IsRegistered())
	{
		if (component->GetWorld() != nullptr)
		{
			FNavigationSystem::UpdateComponentData(*component);
		}
	}
}

bool UExtraBlueprintFunctionLibrary::PortalLineTrace(const UObject* WorldContextObject, const FVector Start, const FVector Direction, const float Distance,
	ETraceTypeQuery TraceChannel, bool bTraceComplex, TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector normalDir = Direction.GetSafeNormal();
	bool hit = UKismetSystemLibrary::LineTraceSingle(WorldContextObject, Start, Start + (normalDir * Distance), TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
	if (!hit) return false;

	// if we hit a portal plane, do another trace through it
	AActor* hitActor = OutHit.GetActor();
	if (UPortalComponent* p = hitActor->GetComponentByClass<UPortalComponent>())
	{
		// ignore the exit portal so it's not accidentally blocking
		ActorsToIgnore.Add(p->GetDestinationPortal());

		// transform impact point across portal and calculate new start and end points
		FVector newStart = p->PortalTransformPoint(OutHit.ImpactPoint);
		FVector newDir = p->PortalTransformVector(normalDir) * (Distance - OutHit.Distance);

		OutHit = {}; // clear the hit struct
		hit = UKismetSystemLibrary::LineTraceSingle(WorldContextObject, newStart, newStart + newDir, TraceChannel, bTraceComplex,
			ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
	}
	return hit;
}

bool UExtraBlueprintFunctionLibrary::PortalPredictProjectilePath(
	const UObject* WorldContextObject, 
	FHitResult& OutHit,
	TArray<FVector>& OutPathPositions, 
	FVector& OutLastTraceDestination, 
	UPortalComponent*& tracedPortal,
	int& portalStartIndex, 
	FVector StartPos, 
	FVector LaunchVelocity, 
	bool bTracePath,
	float ProjectileRadius, 
	const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, 
	bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore, 
	EDrawDebugTrace::Type DrawDebugType, 
	float DrawDebugTime, 
	float SimFrequency,
	float MaxSimTime, 
	float OverrideGravityZ)
{
	FPredictProjectilePathParams Params = FPredictProjectilePathParams(ProjectileRadius, StartPos, LaunchVelocity, MaxSimTime);
	Params.bTraceWithCollision = bTracePath;
	Params.bTraceComplex = bTraceComplex;
	Params.ActorsToIgnore = ActorsToIgnore;
	Params.DrawDebugType = DrawDebugType;
	Params.DrawDebugTime = DrawDebugTime;
	Params.SimFrequency = SimFrequency;
	Params.OverrideGravityZ = OverrideGravityZ;
	Params.ObjectTypes = ObjectTypes; // Object trace
	Params.bTraceWithChannel = false;

	// trace
	FPredictProjectilePathResult Result;
	bool hit = UGameplayStatics::PredictProjectilePath(WorldContextObject, Params, Result);

	// fill results
	OutPathPositions.Empty(Result.PathData.Num());
	for (auto& datapt : Result.PathData)
	{
		OutPathPositions.Add(datapt.Location);
	}
	portalStartIndex = OutPathPositions.Num();
	if (!hit) return false;

	// if hit portal plane, do another prediction on the other side
	AActor* hitActor = Result.HitResult.GetActor();
	if (hitActor->ActorHasTag("Portal"))
	{
		UPortalComponent* p = hitActor->GetComponentByClass<UPortalComponent>();
		// ignore the exit portal so it's not accidentally blocking
		Params.ActorsToIgnore.Add(p->GetDestinationPortal());
		Params.StartLocation = p->PortalTransformPoint(Result.HitResult.ImpactPoint);
		FVector interpVel = FMath::Lerp(Result.PathData.Last(1).Velocity, Result.LastTraceDestination.Velocity, Result.HitResult.Time);
		Params.LaunchVelocity = p->PortalTransformVector(interpVel);
		hit = UGameplayStatics::PredictProjectilePath(WorldContextObject, Params, Result);
		if (hit)
		{
			for (auto& datapt : Result.PathData)
			{
				OutPathPositions.Add(datapt.Location);
			}
		}
		tracedPortal = p;
	}

	OutLastTraceDestination = Result.LastTraceDestination.Location;
	OutHit = Result.HitResult;
	return hit;
}