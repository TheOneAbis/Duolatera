// Fill out your copyright notice in the Description page of Project Settings.


#include "VRMultiplayerGameMode.h"

AVRMultiplayerGameMode::AVRMultiplayerGameMode()
{
}

void AVRMultiplayerGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) 
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("override HandleStartingNewPlayer")));

}



