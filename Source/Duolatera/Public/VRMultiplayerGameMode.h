// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "VRMultiplayerGameMode.generated.h"

/**
 * 
 */
UCLASS()
class DUOLATERA_API AVRMultiplayerGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AVRMultiplayerGameMode();

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};
