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
	activators.Add(activator, &exists);
	
	// new ID was added, so check if it's enough to activate
	bool shouldActivate = !exists && prevCount == RequiredActivators - 1;
	if (shouldActivate)
	{
		Activate();
		OnPuzzleActivated.Broadcast();
	}

	if (!CanOverflow && !(RemainActive || exists) && prevCount == RequiredActivators)
	{
		Deactivate();
		OnPuzzleDeactivated.Broadcast();
	}

	return shouldActivate;
}

bool AActivatable::RemoveActivator(AActor* activator)
{
	if (!activator) return false;

	int prevCount = GetNumActivators();
	int removed = activators.Remove(activator);

	// ID was removed, so check if it's no longer enough to stay active
	bool shouldDeactivate = removed && !RemainActive && prevCount == RequiredActivators;
	if (shouldDeactivate)
	{
		Deactivate();
		OnPuzzleDeactivated.Broadcast();
	}

	if (!CanOverflow && removed && prevCount == RequiredActivators + 1)
	{
		Activate();
		OnPuzzleActivated.Broadcast();
	}

	return shouldDeactivate;
}

int AActivatable::GetNumActivators()
{
	return activators.Num();
}

TArray<AActor*> AActivatable::GetActivatorList()
{
	return activators.Array();
}

void AActivatable::SetRequiredActivators(int newReqCount)
{
	if (RequiredActivators == newReqCount) return;

	int prevReq = RequiredActivators;
	RequiredActivators = FMath::Max(newReqCount, 0);
	int currentCount = GetNumActivators();
	
	if (CanOverflow)
	{
		// deactivate if new requirement not equal to the count
		if (!RemainActive && prevReq == currentCount && RequiredActivators != currentCount)
		{
			Deactivate();
			OnPuzzleDeactivated.Broadcast();
		}
		// activate if new requirement is equal to the count
		else if (RequiredActivators == currentCount)
		{
			Activate();
			OnPuzzleActivated.Broadcast();
		}
	}
	else
	{
		// deactivate if new requirement is greater than the count
		if (!RemainActive && prevReq >= currentCount && RequiredActivators < currentCount)
		{
			Deactivate();
			OnPuzzleDeactivated.Broadcast();
		}
		// activate if new requirement is less or equal to the count
		else if (prevReq < currentCount && RequiredActivators >= currentCount)
		{
			Activate();
			OnPuzzleActivated.Broadcast();
		}
	}
}

bool AActivatable::IsActivator(AActor* activator)
{
	return activators.Find(activator) != nullptr;
}

void AActivatable::SetRemainActive(bool remainActive)
{
	if (RemainActive == remainActive) return;

	if (!remainActive && (activators.Num() < RequiredActivators || 
		(!CanOverflow && activators.Num() > RequiredActivators)))
	{
		Deactivate();
		OnPuzzleDeactivated.Broadcast();
	}
	RemainActive = remainActive;
}