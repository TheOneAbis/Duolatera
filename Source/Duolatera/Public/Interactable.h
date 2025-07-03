// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Interactable.generated.h"

class UMotionControllerComponent;
class AActivatable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractDelegate, AInteractable*, interactedActor);

UCLASS(Abstract)
class DUOLATERA_API AInteractable : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInteractable();
	
protected:

	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	UFUNCTION(BlueprintCallable, Category = "Interactables", meta = (Description = 
		"Sets if this item can be interacted with. Deactivates automatically if it's currently being interacted with when set to false."))
	void SetCanInteract(bool value);

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactables", meta = (Description = "If false, this interactable will not activate or deactivate any of its activatables."))
	bool canInteract = true;

	UPROPERTY(BlueprintReadOnly , Category = "Interactables", meta = (Description = "Tracks if this interactable is currently being interacted with."))
	bool interacting = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interactables", meta = (Description = "If false, this object cannot be interacted with by holding it and pressing Trigger."))
	bool heldInteraction = true;

	// Notification Events
	UPROPERTY(BlueprintAssignable)
	FInteractDelegate OnInteractBegin;

	UPROPERTY(BlueprintAssignable)
	FInteractDelegate OnInteractEnd;

private:

	UFUNCTION(NetMulticast, Reliable)
	void UpdateInteractableMat();
};
