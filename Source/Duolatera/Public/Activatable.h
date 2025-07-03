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

	TSet<AActor*> activators;

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
		"Adds an activator to the activator set. Returns true if this activator activated the object."))
	bool AddActivator(AActor* activator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Activatables", meta = (Description = 
		"Removes an activator from the activator set. Returns true if this activator deactivated the object."))
	bool RemoveActivator(AActor* activator = nullptr);

	UFUNCTION(BlueprintPure, Category = "Activatables", meta = (Description = 
		"The number of activators currently applied to this actor. The result is num activators - num deactivators"))
	int GetNumActivators();

	UFUNCTION(BlueprintPure, Category = "Activatables", meta = (Description =
		"Returns the list of all activators contributing toward this actor's activation state."))
	TArray<AActor*> GetActivatorList();

	UFUNCTION(BlueprintCallable, Category = "Activatables", meta = (Description =
		"Set The number of required activators to activate this actor. This will automatically update its activation state."))
	void SetRequiredActivators(int newCount);

	UFUNCTION(BlueprintPure, Category = "Activatables", meta = (Description = "Checks if the specified actor is currently an activator for this activatable."))
	bool IsActivator(AActor* activator);

	UFUNCTION(BlueprintCallable, Category = "Activatables", meta = (Description = "Sets if this activatable should stay active after being activated, even if an activator is removed from it later on."))
	void SetRemainActive(bool remainActive);

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

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Activatables", meta = (Description =
		"If false, this actor will only activate if it meets the required activator count exactly. Going over will deactivate it."))
	bool CanOverflow = true;
};
