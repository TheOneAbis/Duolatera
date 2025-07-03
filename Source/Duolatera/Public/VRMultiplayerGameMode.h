// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "VRMultiplayerGameMode.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameModeFunctionFired);

UCLASS()
class DUOLATERA_API AVRMultiplayerGameMode : public AGameMode
{
	GENERATED_BODY()

protected: 
	void PlayerStartBubbleSort(TArray<APlayerStart*>& arr);

public:
	//broadcasted from Logout
	//Subscribed by: Pistol
	UPROPERTY(BlueprintAssignable)
	FOnGameModeFunctionFired OnPlayerLogoutDelegate;

	//broadcasted from HandleBothPlayerReady
	//Subscribed by: VRPawn
	UPROPERTY(BlueprintAssignable)
	FOnGameModeFunctionFired OnBothPlayerReadyDelegate;

	// Sets default values for this actor's properties
	AVRMultiplayerGameMode();

	UFUNCTION(BlueprintCallable, meta = (Description =
		"input should be PlayerController, returns 0 or 1 based on all player's PlayerState's ID"))
	int GetRelativeIDFromPlayerStates(AController* controller);

	//override FindPlayerStart, which calls ChoosePlayerStart to choose an unoccupied spot
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	//override Logout, to boradcast an delegate for player realated object to destory
	virtual void Logout(AController* Exiting);

	UFUNCTION(BlueprintCallable)
	virtual void HandleBothPlayerReady();

	//virtual void HandleMatchHasStarted();
	// Useful functions to be overriden might be
	// HandleStartingNewPlayer_Implementation
	// PostSeamlessTravel
};
