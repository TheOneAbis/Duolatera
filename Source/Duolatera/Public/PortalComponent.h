// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Taggable.h"
#include "PortalComponent.generated.h"


UCLASS(ClassGroup=(DuolateraComponents), meta=(BlueprintSpawnableComponent))
class DUOLATERA_API UPortalComponent : public UActorComponent, public ITaggable
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPortalComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Override OnTagged from Taggable interface
	// This should be called every time a player fires a laser tool at an actor with this component
	void OnTagged_Implementation(FHitResult tagHit) override;
};
