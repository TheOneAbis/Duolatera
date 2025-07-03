// Fill out your copyright notice in the Description page of Project Settings.


#include "GlobalPostProcess.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PostProcessComponent.h"

// Sets default values
AGlobalPostProcess::AGlobalPostProcess()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AGlobalPostProcess::OnConstruction(const FTransform& Transform)
{
	// add PostProcess component
	UPostProcessComponent* pp = (UPostProcessComponent*)AddComponentByClass(UPostProcessComponent::StaticClass(), false, FTransform(), false);
	pp->bUnbound = true;

	// Load and add Portal PP material
	UMaterialInterface* portalMatParent = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/Assets/Materials/Portals/M_PortalPP.M_PortalPP"));
	portalMat = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, portalMatParent);
	pp->Settings.WeightedBlendables = FWeightedBlendables({ FWeightedBlendable(1.f, portalMat) });
}

UMaterialInstanceDynamic* AGlobalPostProcess::GetPortalRenderMaterial()
{
	return portalMat;
}