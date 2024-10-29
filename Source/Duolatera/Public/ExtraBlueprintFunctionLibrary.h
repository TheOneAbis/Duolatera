// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ExtraBlueprintFunctionLibrary.generated.h"

enum ETraceTypeQuery : int;
namespace EDrawDebugTrace
{
	enum Type : int;
}

class USceneComponent;

/**
 * 
 */
UCLASS()
class DUOLATERA_API UExtraBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	UFUNCTION(BlueprintCallable, Category = "Extra Utilities")
	// Manually update this component's data for navigation. Components automatically update when moved, so likely use case for this is if mesh topology changes.
	static void UpdateNavComponentData(USceneComponent* component);

	UFUNCTION(BlueprintCallable, Category = "Extra Utilities", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Portal Line Trace", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	// Line trace that accounts for portals
	static bool PortalLineTrace(const UObject* WorldContextObject, const FVector Start, const FVector Direction, const float Distance, 
		ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, 
		FHitResult& OutHit, FHitResult& OutHitThroughPortal, bool& tracedThroughPortal, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, 
		FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	UFUNCTION(BlueprintCallable, Category = "Extra Utilities", DisplayName = "Portal Predict Projectile Path", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", AdvancedDisplay = "DrawDebugTime, DrawDebugType, SimFrequency, MaxSimTime, OverrideGravityZ", bTracePath = true))
	static bool PortalPredictProjectilePath(const UObject* WorldContextObject, FHitResult& OutHit, 
		TArray<FVector>& OutPathPositions, FVector& OutLastTraceDestination, UPortalComponent*& tracedPortal, int& portalStartIndex, FVector StartPos, FVector LaunchVelocity, bool bTracePath,
		float ProjectileRadius, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, bool bTraceComplex, 
		const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, float DrawDebugTime, float SimFrequency = 15.f, 
		float MaxSimTime = 2.f, float OverrideGravityZ = 0);
};
