// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Interactable.generated.h"

class UMotionControllerComponent;
class AActivatable;

UCLASS(Abstract)
class DUOLATERA_API AInteractable : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInteractable();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactables", meta = (Description =
		"Called when the player begins interacting with a held object. Override this in a child Actor."))
	void BeginInteract(UMotionControllerComponent* heldController = nullptr);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactables", meta = (Description =
		"Called when the player stops interacting with a held object. Override this in a child Actor."))
	void EndInteract(UMotionControllerComponent* heldController = nullptr);

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Interactables", meta = (Description = "List of activatables to activate when this actor is interacted with."))
	TArray<AActivatable*> objectsToActivate;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Interactables", meta = (Description = "List of activatables to deactivate when this actor is interacted with."))
	TArray<AActivatable*> objectsToDeactivate;
};
