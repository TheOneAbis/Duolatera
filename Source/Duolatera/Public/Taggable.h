// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Taggable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UTaggable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DUOLATERA_API ITaggable
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Taggables)
	void OnTagged(FHitResult tagHit);

};
