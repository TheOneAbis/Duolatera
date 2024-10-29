// Fill out your copyright notice in the Description page of Project Settings.

#include "Interactable.h"

#include "Activatable.h"
#include "MotionControllerComponent.h"

// Sets default values
AInteractable::AInteractable()
{
}

// Called when the game starts or when spawned
void AInteractable::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AInteractable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AInteractable::BeginInteract_Implementation(UMotionControllerComponent* heldController)
{
	for (AActivatable* a : objectsToActivate)
	{
		a->AddActivator(this);
	}
	for (AActivatable* a : objectsToDeactivate)
	{
		a->RemoveActivator(this);
	}
}

void AInteractable::EndInteract_Implementation(UMotionControllerComponent* heldController)
{
	for (AActivatable* a : objectsToActivate)
	{
		a->RemoveActivator(this);
	}
	for (AActivatable* a : objectsToDeactivate)
	{
		a->AddActivator(this);
	}
}