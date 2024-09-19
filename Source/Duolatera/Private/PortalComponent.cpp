// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalComponent.h"

// Sets default values for this component's properties
UPortalComponent::UPortalComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	drawnVerts.Reserve(64);
	// ...
}


// Called when the game starts
void UPortalComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	// ...
	
}


// Called every frame
void UPortalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UPortalComponent::OnTagged_Implementation(FHitResult tagHit)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
		FString::Printf(TEXT("%s tagged at position %f, %f, %f"), 
		*tagHit.GetActor()->GetName(), tagHit.ImpactPoint.X, tagHit.ImpactPoint.Y, tagHit.ImpactPoint.Z));


}

void UPortalComponent::ResetCanvas()
{
	drawnVerts.Empty(64);
}
