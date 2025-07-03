// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VRMultiplayerGameInstance.generated.h"

/**
 * 
 */
UCLASS(config = Game, transient, BlueprintType, Blueprintable)
class DUOLATERA_API UVRMultiplayerGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActLoad")
	int ActToLoad = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActLoad")
	bool isLastSession = false;
};
