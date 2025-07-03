// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
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
		FHitResult& OutHit, FHitResult& OutPortalHit, bool& tracedThroughPortal, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, 
		FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	UFUNCTION(BlueprintCallable, Category = "Extra Utilities", DisplayName = "Portal Predict Projectile Path", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", AdvancedDisplay = "DrawDebugTime, DrawDebugType, SimFrequency, MaxSimTime, OverrideGravityZ", bTracePath = true))
	static bool PortalPredictProjectilePath(const UObject* WorldContextObject, FHitResult& OutHit, FHitResult& OutPortalHit,
		TArray<FVector>& OutPathPositions, FVector& OutLastTraceDestination, int& portalStartIndex, FVector StartPos, FVector LaunchVelocity, 
		bool bTracePath, float ProjectileRadius, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, bool bTraceComplex, 
		const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, float DrawDebugTime, float SimFrequency = 15.f, 
		float MaxSimTime = 2.f, float OverrideGravityZ = 0);
	UFUNCTION(BlueprintCallable, Category = "Extra Utilities")
	static FVector CalculateHolsterRelativeLocation(const FTransform cameraRelativeTrans, const FVector VROriginWorldLoc, const float playerHeight);

	UFUNCTION(BlueprintCallable, Category = "Extra Utilities", meta = (Description = "Return leaning angle, in radius"))
	static float CalculateTorsoInclinationAngle(const TArray<float>& socketHeights, const float playerHeight, const float cameraRelativeHeight);

	UFUNCTION(BlueprintPure, Category = "Extra Utilities", meta = (Description = "Return UObject's Unique ID"))
	static int64 GetObjectUniqueID(const UObject* obj);

	UFUNCTION(BlueprintPure, Category = "VR Player Animation", meta = (Description = "Fine tune elbow's joint target location"))
	static FVector ClampElbowTargetLocation(const FTransform handTransform, const FTransform headTransform, bool leftHand);

	UFUNCTION(BlueprintCallable, Category = "Procedural Texture Stripe", meta = (WorldContext = "WorldContextObject", Description = "adjust spline point based on hit result"))
	static void AdjustSplinePoint(
		const UObject* WorldContextObject,
		const AActor* selfRef, 
		USplineComponent* spline,
		const float detectRadius,
		const float expandDistance,
		const float snappingMargin,
		const bool bShowDebugDrawing = true
	);

	UFUNCTION(BlueprintPure, Category = "Custom Importer", meta = (Description = "Get Log Message"))
	static FString GetImportFbxWarnignMessages();
};
