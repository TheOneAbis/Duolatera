// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MovementInterpolation.generated.h"

/**
 * 
 */
UCLASS()
class DUOLATERA_API UMovementInterpolation : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static bool IterateTimer(float deltaTime, UPARAM(ref) float& currentTimer, float interval);

	UFUNCTION(BlueprintCallable, Category = MovementInterpolation)
	static FVector3f CatmullRomLocationInterploate(UPARAM(ref) TArray<FVector3f>& points, float currentTime, float interval);

	UFUNCTION(BlueprintCallable, Category = MovementInterpolation)
	static void CatmullRomPointsUpdate(FVector3f p, UPARAM(ref) TArray<FVector3f>& points);
};
