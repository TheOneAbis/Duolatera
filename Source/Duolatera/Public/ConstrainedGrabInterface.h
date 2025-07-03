// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ConstrainedGrabInterface.generated.h"

/**
 * 
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UConstrainedGrabInterface : public UInterface
{
	GENERATED_BODY()
};

class DUOLATERA_API IConstrainedGrabInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = ConstrainedGrab)
	void OnConstrainedGrabbed(FTransform grabbingCompRef);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = ConstrainedGrab)
	void OnConstrainedReleased();
};
