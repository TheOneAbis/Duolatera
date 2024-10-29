// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DialogueManager.generated.h"

UCLASS()
class DUOLATERA_API ADialogueManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADialogueManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString AllDialogues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UUserWidget* DialogueWidgetComponent;

protected:

	TQueue<TPair<FString, FString>> DialogueQueue;

	TPair<FString, FString> CurrentDialogue;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogueQueue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void HandlePlayerInput(const FString& PlayerInput);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void UpdateWidget(const FString& Text);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void DisplayDialogue(const FString& Text);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void ClearDialogueUI();

	void ShowNextDialogue();

private:

	void PopulateDialogueQueue();
};
