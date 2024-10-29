// Fill out your copyright notice in the Description page of Project Settings.


#include "Activatable.h"

// Sets default values
AActivatable::AActivatable()
{
}

// Called when the game starts or when spawned
void AActivatable::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AActivatable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AActivatable::AddActivator(AActor* activator)
{
	if (!activator) return false;

	int prevCount = GetNumActivators();

	bool exists = false;
	if (deactivatorIds.Remove(activator->GetUniqueID()) == 0) // nothing to remove
	{
		exists = true;
		activatorIds.Add(activator->GetUniqueID(), &exists);
	}
	
	// new ID was added, so check if it's enough to activate
	bool shouldActivate = !exists && prevCount == RequiredActivators - 1;
	if (shouldActivate)
	{
		Activate();
		OnPuzzleActivated.Broadcast();
	}

	return shouldActivate;
}

bool AActivatable::RemoveActivator(AActor* activator)
{
	if (!activator) return false;

	int prevCount = GetNumActivators();

	bool exists = false;
	if (activatorIds.Remove(activator->GetUniqueID()) == 0) // nothing to remove
	{
		exists = true;
		deactivatorIds.Add(activator->GetUniqueID(), &exists);
	}

	// ID was removed, so check if it's no longer enough to stay active
	bool shouldDeactivate = !RemainActive && !exists && prevCount == RequiredActivators;
	if (shouldDeactivate)
	{
		Deactivate();
		OnPuzzleDeactivated.Broadcast();
	}

	return shouldDeactivate;
}

int AActivatable::GetNumActivators()
{
	return activatorIds.Num() - deactivatorIds.Num();
}

