// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Activatable.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPuzzleActivatedDelegate);

UCLASS(Abstract)
class DUOLATERA_API AActivatable : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AActivatable();

private:

	TSet<uint32> activatorIds;
	TSet<uint32> deactivatorIds;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Activatables", meta = (Description =
		"Activation logic. Override this in a child Actor."))
	void Activate();
	UFUNCTION(BlueprintImplementableEvent, Category = "Activatables", meta = (Description =
		"Deactivation logic. Override this in a child Actor."))
	void Deactivate();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Activatables", meta = (Description = 
		"Adds an activator to the activator set. If the activator in question previously existed in this actor's deactivator list, it is removed instead. Returns true if this activator activated the object."))
	bool AddActivator(AActor* activator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Activatables", meta = (Description = 
		"Removes an activator from the activator set. If the activator in question was not activating this actor to begin with, it is added to this actor's set of deactivators. Returns true if this activator deactivated the object."))
	bool RemoveActivator(AActor* activator = nullptr);

	UFUNCTION(BlueprintPure, Category = "Activatables", meta = (Description = 
		"The number of activators currently applied to this actor. The result is num activators - num deactivators"))
	int GetNumActivators();

	// Notification Events
	UPROPERTY(BlueprintAssignable)
	FPuzzleActivatedDelegate OnPuzzleActivated;

	UPROPERTY(BlueprintAssignable)
	FPuzzleActivatedDelegate OnPuzzleDeactivated;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Activatables", meta = (Description =
		"The number of activators (e.g. interactables) required to activate this actor's Puzzle mechanic."))
	int RequiredActivators = 1;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Activatables", meta = (Description =
		"Whether this activatable actor should remain active after it's been activated once, disregarding deactivations."))
	bool RemainActive = false;
};
