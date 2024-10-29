// Fill out your copyright notice in the Description page of Project Settings.

#include "PortalComponent.h"

#include "IXRTrackingSystem.h"
#include "IHeadMountedDisplay.h"

#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "MotionControllerComponent.h"


// Sets default values for this component's properties
UPortalComponent::UPortalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// set this component to tick after everything else
	SetTickGroup(ETickingGroup::TG_PostUpdateWork);
}

// Called when the game starts
void UPortalComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	frontPlane = GetOwner()->FindComponentByTag<UStaticMeshComponent>("FrontPlane");
	backPlane = GetOwner()->FindComponentByTag<UStaticMeshComponent>("BackPlane");

	detector = GetOwner()->GetComponentByClass<UBoxComponent>();
	detector->OnComponentBeginOverlap.AddDynamic(this, &UPortalComponent::OnBeginOverlap);
	detector->OnComponentEndOverlap.AddDynamic(this, &UPortalComponent::OnEndOverlap);

	// create the portal plane's material instance and set the plane's material
	portalMat = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, portalMatParent);
	frontPlane->SetMaterial(0, portalMat);
	backPlane->SetMaterial(0, portalMat);

	if (GEngine->XRSystem)
		hmd = GEngine->XRSystem->GetHMDDevice();

	// Viewport may not be created yet by the time we hit BeginPlay(), so have this happen next tick.
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]
		{
			// create render targets
			FVector2D viewport = FVector2D();
			if (GEngine->GameViewport)
				GEngine->GameViewport->GetViewportSize(viewport);
			int size = FMath::Max(viewport.X, viewport.Y);

			// Get the size of the viewport or HMD, depending on the context
			if (hmd && hmd->IsHMDEnabled() && GEngine->StereoRenderingDevice->IsStereoEnabled())
			{
				// Eye resolution X = one side of the HMD resolution, thus half
				IHeadMountedDisplay::MonitorInfo hmdInfo;
				hmd->GetHMDMonitorInfo(hmdInfo);
				size = FMath::Max(hmdInfo.ResolutionX / 2, hmdInfo.ResolutionY);
			}
			portalRTs[0] = UKismetRenderingLibrary::CreateRenderTarget2D(this, size, size);
			portalRTs[1] = UKismetRenderingLibrary::CreateRenderTarget2D(this, size, size);

			SetDestinationPortal(destinationPortal);
		});
}

void UPortalComponent::SetDestinationPortal(UPortalComponent* portal)
{
	destinationPortal = portal;
	if (!portal)
	{
		PrimaryComponentTick.SetTickFunctionEnable(false);
		return;
	}
	
	// set destination cams
	AActor* owner = destinationPortal->GetOwner();
	destinationCams[0] = owner->FindComponentByTag<USceneCaptureComponent2D>("Left");
	destinationCams[1] = owner->FindComponentByTag<USceneCaptureComponent2D>("Right");

	// Create the RT's based on viewport size and set destination scene captures to render to them
	for (int i = 0; i < 2; i++)
	{
		portalMat->SetTextureParameterValue(i ? "PortalTextureRight" : "PortalTextureLeft", portalRTs[i]);
		destinationCams[i]->TextureTarget = portalRTs[i];
		destinationCams[i]->bUseCustomProjectionMatrix = true;
	}
}
UPortalComponent* UPortalComponent::GetDestinationPortal()
{
	return destinationPortal;
}

void UPortalComponent::SetPlaneOffset(float offset)
{
	frontPlane->SetRelativeLocation(FVector(offset, 0, 0));
	backPlane->SetRelativeLocation(FVector(-offset, 0, 0));
}

// Portal Transformation helpers
FTransform UPortalComponent::PortalTransform(FTransform initTM, UStaticMeshComponent* portalPlane)
{
	FTransform t = portalPlane->GetComponentTransform();
	// new location = player cam relative to this portal, w/ X and Y negated
	FVector loc = t.InverseTransformPositionNoScale(initTM.GetLocation()) * FVector(-1, -1, 1);
	// new rotation = player cam relative to this portal, rotated 180 deg around Z
	FQuat rot = FQuat(0, 0, 1, 0) * t.InverseTransformRotation(initTM.GetRotation());
	// return the result, moved back into world space
	FTransform destT = (portalPlane == frontPlane ? destinationPortal->backPlane : destinationPortal->frontPlane)->GetComponentTransform();
	destT.SetScale3D(FVector(1, 1, 1));
	return FTransform(rot, loc, initTM.GetScale3D()) * destT;
}
FVector UPortalComponent::PortalTransformPoint(FVector initPt, UStaticMeshComponent* portalPlane)
{
	checkf(portalPlane, TEXT("Portal plane passed in was null."));
	return (portalPlane == frontPlane ? destinationPortal->backPlane : destinationPortal->frontPlane)->GetComponentTransform()
		.TransformPosition(portalPlane->GetComponentTransform().InverseTransformPosition(initPt) * FVector(-1, -1, 1));
}
FVector UPortalComponent::PortalTransformVector(FVector initVec, UStaticMeshComponent* portalPlane)
{
	checkf(portalPlane, TEXT("Portal plane passed in was null."));
	return (portalPlane == frontPlane ? destinationPortal->backPlane : destinationPortal->frontPlane)->GetComponentTransform()
		.TransformVector(portalPlane->GetComponentTransform().InverseTransformVector(initVec) * FVector(-1, -1, 1));
}
FQuat UPortalComponent::PortalTransformRotation(FQuat initRot, UStaticMeshComponent* portalPlane)
{
	checkf(portalPlane, TEXT("Portal plane passed in was null."));
	return (portalPlane == frontPlane ? destinationPortal->backPlane : destinationPortal->frontPlane)->GetComponentTransform()
		.TransformRotation(FQuat(0, 0, 1, 0) * portalPlane->GetComponentTransform().InverseTransformRotation(initRot));
}

// Called every frame
void UPortalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* canvas = GetOwner()->GetAttachParentActor();
	if (!canvas) return; // portal needs to be attached to a canvas

	if (!player) // didn't exist last frame, so check now
	{
		player = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!player) return; // still doesn't exist, stop here
		playerCam = player->GetComponentByClass<UCameraComponent>();
	}
	
	FVector playerCamLoc = playerCam->GetComponentLocation();
	UStaticMeshComponent* enterPlane, * exitPlane;

	// Check which planes to use for scene captures based on where the player is
	bool playerInFront = canvas->GetActorForwardVector().Dot((playerCamLoc - canvas->GetActorLocation()).GetSafeNormal()) >= 0.f;
	enterPlane = playerInFront ? frontPlane : backPlane;
	exitPlane = playerInFront ? destinationPortal->backPlane : destinationPortal->frontPlane;
	if (destinationCams[0]->GetAttachParent() != exitPlane) // switching sides
	{
		destinationCams[0]->AttachToComponent(exitPlane, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
		destinationCams[1]->AttachToComponent(exitPlane, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
	}

	FTransform t = enterPlane->GetComponentTransform();
	FVector loc = t.InverseTransformPosition(playerCamLoc) * FVector(-1, -1, 1);
	FQuat rot = FQuat(0, 0, 1, 0) * t.InverseTransformRotation(playerCam->GetComponentQuat());

	// update the destination portal's scene captures
	UpdateEye(DeltaTime, 0, loc, rot, enterPlane, exitPlane);
#ifdef UE_BUILD_DEBUG
	if (hmd && hmd->IsHMDEnabled() && GEngine->StereoRenderingDevice->IsStereoEnabled())
		UpdateEye(DeltaTime, 1, loc, rot, enterPlane, exitPlane);
#else
	UpdateEye(DeltaTime, 1, loc, rot, enterPlane, exitPlane);
#endif

	// Update portal "warp"
	FVector portalForward = enterPlane->GetForwardVector();
	portalMat->SetVectorParameterValue("OffsetDistance", FVector4(portalForward * warpAmount, 1.f));

	// Check if objects should teleport to the other side of the portal
	TArray<AActor*> actorsToMove;
	actorsToMove.Reserve(overlapMap.Num());
	for (auto& pair : overlapMap)
	{
		// Add actors that should be teleported to actorsToMove. Omit the player's hands specifically
		if (pair.Key->ComponentHasTag("VRHand")) continue;

		FVector toObj = pair.Key->GetComponentLocation() - enterPlane->GetComponentLocation();
		toObj.Normalize();
		bool inFront = FVector::DotProduct(toObj, portalForward) > 0.f;
		if (pair.Value && !inFront)
		{
			actorsToMove.Add(pair.Key->GetOwner());
		}
		pair.Value = inFront;
	}
	
	// TP actors that went through portal
	for (auto& a : actorsToMove)
	{
		// Skip actors being controlled by a parent
		if (a->GetAttachParentActor()) continue;

		if (a == player)
		{
			PrimaryComponentTick.SetTickFunctionEnable(false);
			destinationPortal->PrimaryComponentTick.SetTickFunctionEnable(true);
		}
		else
		{
			// teleport the actor, retaining its physics properly across portals
			UPrimitiveComponent* c = a->GetComponentByClass<UPrimitiveComponent>();
			c->SetPhysicsLinearVelocity(PortalTransformVector(c->GetPhysicsLinearVelocity(), enterPlane));
			c->SetPhysicsAngularVelocityInDegrees(PortalTransformVector(c->GetPhysicsAngularVelocityInDegrees(), enterPlane));
		}
		a->SetActorTransform(PortalTransform(a->GetTransform(), enterPlane), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UPortalComponent::UpdateEye(float dt, int stereoIndex, FVector initCameraLoc, FQuat initCameraRot, UStaticMeshComponent* enterPlane, UStaticMeshComponent* exitPlane)
{
	auto stereo = GEngine->XRSystem->GetStereoRenderingDevice();

	// update the eye's clip plane, in case the portal itself moved
	FVector forward = exitPlane->GetForwardVector();
	destinationCams[stereoIndex]->ClipPlaneBase = exitPlane->GetComponentLocation() + (forward * -3.f);
	destinationCams[stereoIndex]->ClipPlaneNormal = forward;
	
	// get eye projection and view matrices
	FMatrix eyeProj = stereo->GetStereoProjectionMatrix(stereoIndex);
	FMinimalViewInfo viewInfo;
	playerCam->GetCameraView(dt, viewInfo);
	stereo->CalculateStereoViewOffset(stereoIndex, viewInfo.Rotation, GetWorld()->GetWorldSettings()->WorldToMeters, viewInfo.Location);
	FMatrix view, junk;
	UGameplayStatics::GetViewProjectionMatrix(viewInfo, view, junk, junk);
	FMatrix viewProjT = view * eyeProj;
	
	// If plane not in view, don't render.
	//bool inView = IsInFrustum(GetOwner(), stereoIndex); //TODO: FIX THIS
	bool inView = true;
	if (inView)
	{
		// update transform, view and projection
		destinationCams[stereoIndex]->CustomProjectionMatrix = eyeProj;
		FRotator eyeRot = initCameraRot.Rotator();
		FVector eyeLoc(0);
		stereo->CalculateStereoViewOffset(stereoIndex, eyeRot, GetWorld()->GetWorldSettings()->WorldToMeters, eyeLoc);
		destinationCams[stereoIndex]->SetRelativeLocationAndRotation(initCameraLoc + eyeLoc * (FVector(1.f) / exitPlane->GetComponentScale()), eyeRot.Quaternion());

		// update the Render Target's View-Projection columns
		portalMat->SetVectorParameterValue(stereoIndex ? "VPCol0Right" : "VPCol0Left", FVector4(viewProjT.M[0][0], viewProjT.M[1][0], viewProjT.M[2][0], viewProjT.M[3][0]));
		portalMat->SetVectorParameterValue(stereoIndex ? "VPCol1Right" : "VPCol1Left", FVector4(viewProjT.M[0][1], viewProjT.M[1][1], viewProjT.M[2][1], viewProjT.M[3][1]));
		portalMat->SetVectorParameterValue(stereoIndex ? "VPCol3Right" : "VPCol3Left", FVector4(viewProjT.M[0][3], viewProjT.M[1][3], viewProjT.M[2][3], viewProjT.M[3][3]));

		// render the scene from this eye's point of view onto its Render Target
		destinationCams[stereoIndex]->CaptureScene();
	}
}

bool UPortalComponent::IsInFrustum(AActor* Actor, int stereoIndex)
{
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer != nullptr && LocalPlayer->ViewportClient != nullptr && LocalPlayer->ViewportClient->Viewport)
	{
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			LocalPlayer->ViewportClient->Viewport,
			GetWorld()->Scene,
			LocalPlayer->ViewportClient->EngineShowFlags)
			.SetRealtimeUpdate(true));

		FVector ViewLocation;
		FRotator ViewRotation;
		FSceneView* SceneView = LocalPlayer->CalcSceneView(&ViewFamily, ViewLocation, ViewRotation, LocalPlayer->ViewportClient->Viewport, 0, stereoIndex);
		if (SceneView != nullptr)
		{
			return SceneView->ViewFrustum.IntersectSphere(
				Actor->GetActorLocation(), Actor->GetSimpleCollisionRadius());
		}
	}
	return false;
}

void UPortalComponent::OnBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (otherActor == GetOwner()) return;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, OtherComp->GetName());
	overlapMap.Add(OtherComp, false);
}
void UPortalComponent::OnEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (otherActor == GetOwner()) return;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, OtherComp->GetName());
	overlapMap.Remove(OtherComp);
}