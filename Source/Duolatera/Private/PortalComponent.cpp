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
#include "GlobalPostProcess.h"


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

	frontPlane = GetOwner()->FindComponentByTag<UPortalPlane>("FrontPlane");
	backPlane = GetOwner()->FindComponentByTag<UPortalPlane>("BackPlane");

	detector = GetOwner()->GetComponentByClass<UBoxComponent>();
	detector->OnComponentBeginOverlap.AddDynamic(this, &UPortalComponent::OnBeginOverlap);
	detector->OnComponentEndOverlap.AddDynamic(this, &UPortalComponent::OnEndOverlap);

	// create the portal plane's material instance and set the plane's material
	portalOffsetMat = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, portalMatParent);
	frontPlane->SetMaterial(0, portalOffsetMat);
	backPlane->SetMaterial(0, portalOffsetMat);

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
	
	// set plane destinations
	frontPlane->destination = destinationPortal->backPlane;
	backPlane->destination = destinationPortal->frontPlane;

	// set destination cams
	AActor* owner = destinationPortal->GetOwner();
	destinationCams[0] = owner->FindComponentByTag<USceneCaptureComponent2D>("Left");
	destinationCams[1] = owner->FindComponentByTag<USceneCaptureComponent2D>("Right");

	// Create the RT's based on viewport size and set destination scene captures to render to them
	for (int i = 0; i < 2; i++)
	{
		destinationCams[i]->TextureTarget = portalRTs[i];
		destinationCams[i]->bUseCustomProjectionMatrix = true;
	}
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UPortalComponent::RenderThisPortal()
{
	if (!portalPPMat)
	{
		portalPPMat = ((AGlobalPostProcess*)UGameplayStatics::GetActorOfClass(this, AGlobalPostProcess::StaticClass()))->GetPortalRenderMaterial();
	}

	destinationPortal->renderEyes = false;

	// The following needs to be on the NEXT tick, or the player may experience flashing
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			portalPPMat->SetTextureParameterValue("PortalTextureLeft", portalRTs[0]);
			portalPPMat->SetTextureParameterValue("PortalTextureRight", portalRTs[1]);
			renderEyes = true;
			// Disable warp on other portal so it can't be seen from this portal.
			destinationPortal->ResetWarp();
		}));
}

void UPortalComponent::SetPlaneOffset(float offset)
{
	frontPlane->SetRelativeLocation(FVector(offset, 0, 0));
	backPlane->SetRelativeLocation(FVector(-offset, 0, 0));
}

// Portal Transformation helpers
FTransform UPortalComponent::PortalTransform(FTransform initTM, UPortalPlane* portalPlane)
{
	FTransform t = portalPlane->GetComponentTransform();
	// new location = player cam relative to this portal, w/ X and Y negated
	FVector loc = t.InverseTransformPositionNoScale(initTM.GetLocation()) * FVector(-1, -1, 1);
	// new rotation = player cam relative to this portal, rotated 180 deg around Z
	FQuat rot = FQuat(0, 0, 1, 0) * t.InverseTransformRotation(initTM.GetRotation());
	// return the result, moved back into world space
	FTransform destT = portalPlane->destination->GetComponentTransform();
	destT.SetScale3D(FVector(1, 1, 1));
	return FTransform(rot, loc, initTM.GetScale3D()) * destT;
}
FVector UPortalComponent::PortalTransformPoint(FVector initPt, UPortalPlane* portalPlane)
{
	checkf(portalPlane, TEXT("Portal plane passed in was null."));
	if (!portalPlane) return FVector(0);
	return portalPlane->destination->GetComponentTransform().TransformPosition(
		portalPlane->GetComponentTransform().InverseTransformPosition(initPt) * FVector(-1, -1, 1));
}
FVector UPortalComponent::PortalTransformVector(FVector initVec, UPortalPlane* portalPlane)
{
	checkf(portalPlane, TEXT("Portal plane passed in was null."));
	if (!portalPlane) return FVector(0);
	return portalPlane->destination->GetComponentTransform().TransformVector(
		portalPlane->GetComponentTransform().InverseTransformVector(initVec) * FVector(-1, -1, 1));
}
FRotator UPortalComponent::PortalTransformRotation(FRotator initRot, UPortalPlane* portalPlane)
{
	checkf(portalPlane, TEXT("Portal plane passed in was null."));
	if (!portalPlane) return FRotator(0);
	return portalPlane->destination->GetComponentTransform().TransformRotation(
		FQuat(0, 0, 1, 0) * portalPlane->GetComponentTransform().InverseTransformRotation(
			initRot.Quaternion())).Rotator();
}
UPortalPlane* UPortalComponent::GetEnterPlane(FVector location)
{
	bool inFront = GetOwner()->GetActorForwardVector().Dot((location - GetOwner()->GetActorLocation()).GetSafeNormal()) >= 0.f;
	return inFront ? frontPlane : backPlane;
}

// Called every frame
void UPortalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!player) // didn't exist last frame, so check now
	{
		player = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!player) return; // still doesn't exist, stop here
		playerCam = player->GetComponentByClass<UCameraComponent>();
	}
	
	if (renderEyes)
	{
		// Check which planes to use for scene captures based on where the player is
		FVector playerCamLoc = playerCam->GetComponentLocation();
		UPortalPlane* enterPlane = GetEnterPlane(playerCamLoc);
		FVector portalForward = enterPlane->GetForwardVector();

		// Update portal "warp"
		portalOffsetMat->SetVectorParameterValue("OffsetDistance", FVector4(portalForward * warpAmount, 1.f));

		// If needed, switch sides
		if (destinationCams[0]->GetAttachParent() != enterPlane->destination) 
		{
			destinationCams[0]->AttachToComponent(enterPlane->destination, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
			destinationCams[1]->AttachToComponent(enterPlane->destination, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
		}

		// update the destination portal's scene captures
		FTransform t = enterPlane->GetComponentTransform();
		FVector loc = t.InverseTransformPosition(playerCamLoc) * FVector(-1, -1, 1);
		FQuat rot = FQuat(0, 0, 1, 0) * t.InverseTransformRotation(playerCam->GetComponentQuat());

		UpdateEye(DeltaTime, 0, loc, rot, enterPlane);
#ifdef UE_BUILD_DEBUG
		if (hmd && hmd->IsHMDEnabled() && GEngine->StereoRenderingDevice->IsStereoEnabled())
#endif
			UpdateEye(DeltaTime, 1, loc, rot, enterPlane);
	}

	// Check if objects should teleport to the other side of the portal
	TMap<AActor*, UPortalPlane*> actorsToMove;
	actorsToMove.Reserve(overlapMap.Num());
	for (auto& pair : overlapMap)
	{
		// Add actors that should be teleported to actorsToMove. Omit the player's hands specifically
		if (pair.Key->ComponentHasTag("VRHand")) continue;

		FVector toObj = pair.Key->GetComponentLocation() - pair.Value->GetComponentLocation();
		toObj.Normalize();
		if (FVector::DotProduct(toObj, pair.Value->GetForwardVector()) < 0.f)
			actorsToMove.Add(pair.Key->GetOwner(), pair.Value);

		pair.Value = GetEnterPlane(pair.Key->GetComponentLocation());
	}
	
	// TP actors that went through portal
	for (auto& a : actorsToMove)
	{
		// Skip actors being controlled by a parent
		if (a.Key->GetAttachParentActor()) continue;

		if (a.Key == player)
		{
			// Skip the player if they shouldn't be allowed to move through
			if (!traversable) continue;
		}
		else
		{
			// teleport the actor, retaining its physics properly across portals
			UPrimitiveComponent* c = a.Key->GetComponentByClass<UPrimitiveComponent>();
			c->SetPhysicsLinearVelocity(PortalTransformVector(c->GetPhysicsLinearVelocity(), a.Value));
			c->SetPhysicsAngularVelocityInDegrees(PortalTransformVector(c->GetPhysicsAngularVelocityInDegrees(), a.Value));
		}
		a.Key->SetActorTransform(PortalTransform(a.Key->GetTransform(), a.Value), false, nullptr, ETeleportType::TeleportPhysics);
		OnPortalTraversed.Broadcast(a.Key, this, destinationPortal);
	}
}

void UPortalComponent::UpdateEye(float dt, int stereoIndex, FVector initCameraLoc, FQuat initCameraRot, UPortalPlane* enterPlane)
{
	if (!GEngine->XRSystem) return;

	auto stereo = GEngine->XRSystem->GetStereoRenderingDevice();
	UPortalPlane* exit = enterPlane->destination;

	// update the eye's clip plane, in case the portal itself moved
	FVector forward = exit->GetForwardVector();
	destinationCams[stereoIndex]->ClipPlaneBase = exit->GetComponentLocation() + (forward * -3.f);
	destinationCams[stereoIndex]->ClipPlaneNormal = forward;
	
	// Get eye projection and set custom projection matrix
	FMatrix eyeProj = stereo->GetStereoProjectionMatrix(stereoIndex);
	destinationCams[stereoIndex]->CustomProjectionMatrix = eyeProj;

	// Get view info
	FMinimalViewInfo viewInfo;
	playerCam->GetCameraView(dt, viewInfo);

	// Update view info with left or right eye offset
	stereo->CalculateStereoViewOffset(stereoIndex, viewInfo.Rotation, 
		GetWorld()->GetWorldSettings()->WorldToMeters, viewInfo.Location);

	// Get the resulting view matrix
	FMatrix view, junk;
	UGameplayStatics::GetViewProjectionMatrix(viewInfo, view, junk, junk);

	// Combine into ViewProjection
	FMatrix viewProj = view * eyeProj;

	// We cannot rely on normal TexCoord since scene captures don't have
	// any late-latching. Therefore we need to perform the world-to-screen-space 
	// calculations ourselves, which we do in the Portal Post-Process material 
	// by providing the render target's warped ViewProj matrix, one column at a time.
	portalPPMat->SetVectorParameterValue(stereoIndex ? "VPCol0Right" : "VPCol0Left", 
		FVector4(viewProj.M[0][0], viewProj.M[1][0], viewProj.M[2][0], viewProj.M[3][0]));
	portalPPMat->SetVectorParameterValue(stereoIndex ? "VPCol1Right" : "VPCol1Left", 
		FVector4(viewProj.M[0][1], viewProj.M[1][1], viewProj.M[2][1], viewProj.M[3][1]));
	portalPPMat->SetVectorParameterValue(stereoIndex ? "VPCol3Right" : "VPCol3Left", 
		FVector4(viewProj.M[0][3], viewProj.M[1][3], viewProj.M[2][3], viewProj.M[3][3]));

	// update transform, view and projection
	FRotator eyeRot = initCameraRot.Rotator();
	FVector eyeLoc(0);
	stereo->CalculateStereoViewOffset(stereoIndex, eyeRot,
		GetWorld()->GetWorldSettings()->WorldToMeters, eyeLoc);
	destinationCams[stereoIndex]->SetRelativeLocationAndRotation(
		initCameraLoc + eyeLoc * (FVector(1.f) / exit->GetComponentScale()), eyeRot.Quaternion());

	// render the scene from this eye's point of view onto its Render Target
	destinationCams[stereoIndex]->CaptureScene();
}

bool UPortalComponent::IsInFrustum(UStaticMeshComponent* plane)
{
	APlayerController* pctrl = GetWorld()->GetFirstPlayerController();
	FTransform tm = plane->GetComponentTransform();
	FVector extents = plane->Bounds.GetBox().GetExtent();
	extents.X = warpAmount;
	FVector verts[4]
	{
		extents, -extents,
		FVector(extents.X, extents.Y, -extents.Z),
		FVector(extents.X, -extents.Y, extents.Z),
	};

	for (int i = 0; i < 4; i++)
	{
		FVector2D screenLoc;
		if (pctrl->ProjectWorldLocationToScreen(tm.TransformPosition(verts[i]), screenLoc)) return true;
	}
	return false;
}

void UPortalComponent::OnBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (otherActor == GetOwner()) return;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, OtherComp->GetName());
	overlapMap.Add(OtherComp, GetEnterPlane(OtherComp->GetComponentLocation()));
}
void UPortalComponent::OnEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (otherActor == GetOwner()) return;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, OtherComp->GetName());
	overlapMap.Remove(OtherComp);
}

void UPortalComponent::ResetWarp()
{
	portalOffsetMat->SetVectorParameterValue("OffsetDistance", FVector4(0));
}