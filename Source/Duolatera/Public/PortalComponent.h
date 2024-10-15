// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PortalComponent.generated.h"

// forward-declares
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UCameraComponent;
class USceneCaptureComponent2D;
class IHeadMountedDisplay;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPortalTraversedSignature, UPortalComponent*, enteredPortal, UPortalComponent*, exitedPortal);

UCLASS(ClassGroup=(DuolateraComponents), meta=(BlueprintSpawnableComponent))
class DUOLATERA_API UPortalComponent : public UActorComponent
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

	UFUNCTION(BlueprintCallable, Category = "Portals")
	// Sets this portal's destination. If one is not set, this portal is unusable.
	void SetDestinationPortal(AActor* portal);

	UFUNCTION(BlueprintPure, Category = "Portals")
	// Returns this portal's destination portal.
	AActor* GetDestinationPortal();

	UFUNCTION(BlueprintPure, Category = "Portals")
	// Transforms a given transform from this portal to its exit
	FTransform PortalTransform(FTransform initTM);

	UFUNCTION(BlueprintPure, Category = "Portals")
	// Transforms a given vector from this portal to its exit
	FVector PortalTransformPoint(FVector initPt);

	UFUNCTION(BlueprintPure, Category = "Portals")
	// Transforms a given vector from this portal to its exit
	FVector PortalTransformVector(FVector initVec);

	UFUNCTION(BlueprintPure, Category = "Portals")
	// Transforms a given quaternion from this portal to its exit
	FQuat PortalTransformRotation(FQuat initRot);

	UFUNCTION(BlueprintCallable, Category = "Portals")
	void SetPlaneOffset(float offset);

	UPROPERTY(BlueprintAssignable)
	FOnPortalTraversedSignature OnPortalTraversed;

private:

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Updates the Scene Capture transforms, applying VR eye offsets
	void UpdateEye(float dt, int stereoIndex, FVector initCameraLoc, FQuat initCameraRot);

	bool GetScaledViewProjection(FMatrix& viewProj, FMatrix& proj);

	bool IsInFrustum(AActor* Actor, int stereoIndex);

	UPROPERTY(EditDefaultsOnly, Category = "Portal Properties")
	UMaterialInterface* portalMatParent;

	UPROPERTY(EditInstanceOnly, Category = "Portal Properties")
	AActor* destinationPortal;

	UPROPERTY(EditDefaultsOnly, Category = "Portal Properties")
	float warpAmount = -4.f;

	float planeOffset = 20.f;

	// Portal material ptr and render targets for each eye
	UMaterialInstanceDynamic* portalMat;
	UStaticMeshComponent* portalPlane;
	UTextureRenderTarget2D* portalRTs[2];
	AActor* player;
	
	// Cameras
	UCameraComponent *playerCam;
	USceneCaptureComponent2D* destinationCams[2];
	IHeadMountedDisplay* hmd = nullptr;

	// for detection and teleportation of objects through the portal
	UBoxComponent* detector;
	TMap<USceneComponent*, bool> overlapMap;

	//FTimerHandle occlusionTmrHandle;
};
