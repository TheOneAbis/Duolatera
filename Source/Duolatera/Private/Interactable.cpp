// Fill out your copyright notice in the Description page of Project Settings.

#include "Interactable.h"

#include "Activatable.h"
#include "MotionControllerComponent.h"
#include "Net/UnrealNetwork.h" // for multicast

// Sets default values
AInteractable::AInteractable()
{
}

void AInteractable::OnConstruction(const FTransform& Transform)
{
	UpdateInteractableMat_Implementation();
}

// Called when the game starts or when spawned
void AInteractable::BeginPlay()
{
	Super::BeginPlay();
	
}

void AInteractable::SetCanInteract(bool value)
{
	if (!HasAuthority()) return;

	// if set to false but player is already interacting with the object
	if (!value)
	{
		EndInteract_Implementation(nullptr);
	}
	canInteract = value;
	UpdateInteractableMat();
}

void AInteractable::UpdateInteractableMat_Implementation()
{
	TArray<UMeshComponent*> meshes;
	GetComponents<UMeshComponent*>(meshes);

	for (auto& m : meshes)
		m->SetScalarParameterValueOnMaterials("IsInteractable", canInteract ? 1.f : 0.f);
}

void AInteractable::BeginInteract_Implementation(UMotionControllerComponent* heldController)
{
	if (HasAuthority() && canInteract && !interacting)
	{
		for (AActivatable* a : objectsToActivate)
		{
			if (a)
				a->AddActivator(this);
		}
		for (AActivatable* a : objectsToDeactivate)
		{
			if (a)
				a->RemoveActivator(this);
		}
		interacting = true;

		// Notify interaction begin
		OnInteractBegin.Broadcast(this);
	}
}

void AInteractable::EndInteract_Implementation(UMotionControllerComponent* heldController)
{
	// If end interact was called without begin interact, we shouldn't be here.
	if (HasAuthority() && interacting)
	{
		for (AActivatable* a : objectsToActivate)
		{
			if (a)
				a->RemoveActivator(this);
		}
		for (AActivatable* a : objectsToDeactivate)
		{
			if (a)
				a->AddActivator(this);
		}
		interacting = false;

		// Notify interaction end
		OnInteractEnd.Broadcast(this);
	}
}