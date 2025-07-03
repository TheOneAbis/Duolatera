// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlobalPostProcess.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;

UCLASS(NotPlaceable, NotBlueprintable)
class DUOLATERA_API AGlobalPostProcess : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGlobalPostProcess();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

public:	

	UFUNCTION(BlueprintPure, Category = "Global PostProcess")
	UMaterialInstanceDynamic* GetPortalRenderMaterial();

private:
	UMaterialInstanceDynamic* portalMat;
};
