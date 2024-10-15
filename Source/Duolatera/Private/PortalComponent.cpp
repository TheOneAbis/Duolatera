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
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// set this component to tick after everything else
	SetTickGroup(ETickingGroup::TG_PostUpdateWork);
}

// Called when the game starts
void UPortalComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!player) return;

	playerCam = player->GetComponentByClass<UCameraComponent>();
	portalPlane = GetOwner()->GetComponentByClass<UStaticMeshComponent>();
	detector = GetOwner()->GetComponentByClass<UBoxComponent>();
	detector->OnComponentBeginOverlap.AddDynamic(this, &UPortalComponent::OnBeginOverlap);
	detector->OnComponentEndOverlap.AddDynamic(this, &UPortalComponent::OnEndOverlap);

	// create the portal plane's material instance and set the plane's material
	portalMat = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, portalMatParent);
	GetOwner()->GetComponentByClass<UStaticMeshComponent>()->SetMaterial(0, portalMat);

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

void UPortalComponent::SetDestinationPortal(AActor* portal)
{
	destinationPortal = portal;

	if (!destinationPortal || !player)
	{
		PrimaryComponentTick.SetTickFunctionEnable(false);
		return;
	}

	// set destination cams
	destinationCams[0] = destinationPortal->FindComponentByTag<USceneCaptureComponent2D>("Left");
	destinationCams[1] = destinationPortal->FindComponentByTag<USceneCaptureComponent2D>("Right");

	// Create the RT's based on viewport size and set destination scene captures to render to them
	for (int i = 0; i < 2; i++)
	{
		portalMat->SetTextureParameterValue(i ? "PortalTextureRight" : "PortalTextureLeft", portalRTs[i]);
		destinationCams[i]->TextureTarget = portalRTs[i];
		destinationCams[i]->bUseCustomProjectionMatrix = true;
	}
}
AActor* UPortalComponent::GetDestinationPortal()
{
	return destinationPortal;
}

void UPortalComponent::SetPlaneOffset(float offset)
{
	planeOffset = offset;
}

FTransform UPortalComponent::PortalTransform(FTransform initTM)
{
	FTransform t = GetOwner()->GetActorTransform();
	// new location = player cam relative to this portal, w/ X and Y negated
	FVector loc = t.InverseTransformPosition(initTM.GetLocation()) * FVector(-1, -1, 1);
	// new rotation = player cam relative to this portal, rotated 180 deg around Z
	FQuat rot = FQuat(0, 0, 1, 0) * t.InverseTransformRotation(initTM.GetRotation());
	// return the result, moved back into world space
	return FTransform(rot, loc, initTM.GetScale3D()) * destinationPortal->GetTransform();
}

FVector UPortalComponent::PortalTransformPoint(FVector initPt)
{
	return destinationPortal->GetTransform().TransformPosition(GetOwner()->GetActorTransform().InverseTransformPosition(initPt) * FVector(-1, -1, 1));
}

FVector UPortalComponent::PortalTransformVector(FVector initVec)
{
	return destinationPortal->GetTransform().TransformVector(GetOwner()->GetActorTransform().InverseTransformVector(initVec) * FVector(-1, -1, 1));
}
FQuat UPortalComponent::PortalTransformRotation(FQuat initRot)
{
	return destinationPortal->GetTransform().TransformRotation(FQuat(0, 0, 1, 0) * GetOwner()->GetActorTransform().InverseTransformRotation(initRot));
}

// Called every frame
void UPortalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector playerCamLoc = playerCam->GetComponentLocation();

	// Check if portal needs to swap canvas sides to "follow" the player
	if (AActor* parent = GetOwner()->GetAttachParentActor())
	{
		bool inFront = parent->GetActorForwardVector().Dot((playerCamLoc - parent->GetActorLocation()).GetSafeNormal()) >= 0.f;
		GetOwner()->SetActorRelativeTransform(FTransform(FQuat(0, 0, !inFront, inFront), FVector(inFront ? planeOffset : -planeOffset, 0, 0)));
		destinationPortal->SetActorRelativeTransform(FTransform(FQuat(0, 0, inFront, !inFront), FVector(inFront ? -planeOffset : planeOffset, 0, 0)));
	}

	FTransform t = GetOwner()->GetActorTransform();
	FVector loc = t.InverseTransformPosition(playerCamLoc) * FVector(-1, -1, 1);
	FQuat rot = FQuat(0, 0, 1, 0) * t.InverseTransformRotation(playerCam->GetComponentQuat());

	// update the destination portal's scene captures
	UpdateEye(DeltaTime, 0, loc, rot);
#ifdef UE_BUILD_DEBUG
	if (hmd && hmd->IsHMDEnabled() && GEngine->StereoRenderingDevice->IsStereoEnabled())
		UpdateEye(DeltaTime, 1, loc, rot);
#else
	UpdateEye(DeltaTime, 1, loc, rot);
#endif

	// Update portal "warp"
	FVector portalForward = GetOwner()->GetActorForwardVector();
	portalMat->SetVectorParameterValue("OffsetDistance", FVector4(portalForward * warpAmount, 1.f));

	// Check if objects should teleport to the other side of the portal
	TArray<AActor*> actorsToMove;
	actorsToMove.Reserve(overlapMap.Num());
	for (auto& pair : overlapMap)
	{
		// Add actors that should be teleported to actorsToMove. Omit the player's hands specifically
		if (pair.Key->ComponentHasTag("VRHand")) continue;

		FVector toObj = pair.Key->GetComponentLocation() - GetOwner()->GetActorLocation();
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
			destinationPortal->GetComponentByClass<UPortalComponent>()->PrimaryComponentTick.SetTickFunctionEnable(true);
			/*UKismetSystemLibrary::ExecuteConsoleCommand(this, "r.AllowOcclusionQueries 0");
			GetWorld()->GetTimerManager().SetTimer(occlusionTmrHandle, [this]()
				{
					UKismetSystemLibrary::ExecuteConsoleCommand(this, "r.AllowOcclusionQueries 1");
				}, 1.f, false);*/
			OnPortalTraversed.Broadcast(this, destinationPortal->GetComponentByClass<UPortalComponent>());
		}
		else
		{
			// teleport the actor, retaining its physics properly across portals
			UPrimitiveComponent* c = a->GetComponentByClass<UPrimitiveComponent>();
			c->SetPhysicsLinearVelocity(PortalTransformVector(c->GetPhysicsLinearVelocity()));
			c->SetPhysicsAngularVelocityInDegrees(PortalTransformVector(c->GetPhysicsAngularVelocityInDegrees()));
		}
		a->SetActorTransform(PortalTransform(a->GetTransform()), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UPortalComponent::UpdateEye(float dt, int stereoIndex, FVector initCameraLoc, FQuat initCameraRot)
{
	TSharedPtr<IStereoRendering, ESPMode::ThreadSafe> stereo = GEngine->XRSystem->GetStereoRenderingDevice();
	// update the eye's clip plane, in case the portal itself moved
	FVector forward = destinationPortal->GetActorForwardVector();
	destinationCams[stereoIndex]->ClipPlaneBase = destinationPortal->GetActorLocation() + (forward * -3.f);
	destinationCams[stereoIndex]->ClipPlaneNormal = forward;
	// get Eye projection and view matrices
	FMatrix eyeProj = stereo->GetStereoProjectionMatrix(stereoIndex);
	FMinimalViewInfo viewInfo;
	playerCam->GetCameraView(dt, viewInfo);
	stereo->CalculateStereoViewOffset(stereoIndex, viewInfo.Rotation, GetWorld()->GetWorldSettings()->WorldToMeters, viewInfo.Location);
	FMatrix view, junk, junk2;
	UGameplayStatics::GetViewProjectionMatrix(viewInfo, view, junk, junk2);
	FMatrix viewProjT = view * eyeProj;
	
	// Scale matrices based on how much of the plane is in view. If not in view, don't render.
	//bool inView = IsInFrustum(GetOwner(), stereoIndex); //TODO: FIX THIS
	bool inView = true;
	if (inView && GetScaledViewProjection(viewProjT, eyeProj))
	{
		// update transform, view and projection
		destinationCams[stereoIndex]->CustomProjectionMatrix = eyeProj;

		FRotator eyeRot = initCameraRot.Rotator();
		stereo->CalculateStereoViewOffset(stereoIndex, eyeRot, GetWorld()->GetWorldSettings()->WorldToMeters, initCameraLoc);
		destinationCams[stereoIndex]->SetRelativeLocationAndRotation(initCameraLoc, eyeRot.Quaternion());

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

bool UPortalComponent::GetScaledViewProjection(FMatrix& viewProj, FMatrix& proj)
{
	FVector min, max;
	FVector2D minNDC, maxNDC;
	portalPlane->GetLocalBounds(min, max);
	
	FVector LocalPoints[4] = {
		FVector(min.X, min.Y, min.Z),
		FVector(min.X, max.Y, min.Z),
		FVector(max.X, min.Y, min.Z),
		FVector(max.X, max.Y, min.Z)
	};

	bool inView = true;

	FVector4 HomogenousClipPoints[4];
	FMatrix localToProj = portalPlane->GetComponentTransform().ToMatrixWithScale() * viewProj;
	for (uint32 i = 0; i < 4; ++i)
	{
		HomogenousClipPoints[i] = localToProj.TransformPosition(LocalPoints[i]);
	}

	FVector MinPointNDC = FVector(FLT_MAX);
	FVector MaxPointNDC = FVector(-FLT_MAX);

	uint32 BehindNearPlaneCount = 0;
	for (uint32 i = 0; i < 4; ++i)
	{
		FVector NDCPoint = FVector(HomogenousClipPoints[i].X, HomogenousClipPoints[i].Y, HomogenousClipPoints[i].Z) / HomogenousClipPoints[i].W;

		if (NDCPoint.Z < 0)
		{
			BehindNearPlaneCount += 1;
			continue;
		}

		if (NDCPoint.X < MinPointNDC.X)
			MinPointNDC.X = NDCPoint.X;
		if (NDCPoint.Y < MinPointNDC.Y)
			MinPointNDC.Y = NDCPoint.Y;
		if (NDCPoint.Z < MinPointNDC.Z)
			MinPointNDC.Z = NDCPoint.Z;

		if (NDCPoint.X > MaxPointNDC.X)
			MaxPointNDC.X = NDCPoint.X;
		if (NDCPoint.Y > MaxPointNDC.Y)
			MaxPointNDC.Y = NDCPoint.Y;
		if (NDCPoint.Z > MaxPointNDC.Z)
			MaxPointNDC.Z = NDCPoint.Z;
	}

	if (BehindNearPlaneCount == 4) return false;

	// Clamp to NDC space.
	if (MinPointNDC.X < -1)
		MinPointNDC.X = -1;
	if (MinPointNDC.Y < -1)
		MinPointNDC.Y = -1;
	if (MaxPointNDC.X > 1)
		MaxPointNDC.X = 1;
	if (MaxPointNDC.Y > 1)
		MaxPointNDC.Y = 1;

	if (BehindNearPlaneCount > 0)
	{
		MinPointNDC = FVector(-1, -1, 0);
		MaxPointNDC = FVector(1, 1, 1);
	}
	else if (MinPointNDC.X >= 1 || MinPointNDC.Y >= 1 || MinPointNDC.Z >= 1 || MaxPointNDC.X <= -1 || MaxPointNDC.Y <= -1 || MaxPointNDC.Z <= -1)
	{
		inView = false;
	}
	minNDC = FVector2D(MinPointNDC.X, MinPointNDC.Y);
	maxNDC = FVector2D(MaxPointNDC.X, MaxPointNDC.Y);

	if (inView)
	{
		FVector2D scale = FVector2D(2, 2) / (maxNDC - minNDC);
		FVector2D offset = FVector2D(-1, -1) - (scale * minNDC);
		FMatrix scaledMat = FTransform(FQuat::Identity, FVector(offset.X, offset.Y, 0), FVector(scale.X, scale.Y, 1)).ToMatrixWithScale();

		viewProj *= scaledMat;
		proj *= scaledMat;
	}
	return inView;
}

void UPortalComponent::OnBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (otherActor == GetOwner()) return;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, OtherComp->GetName());
	overlapMap.Add(OtherComp, false);
}
void UPortalComponent::OnEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (otherActor == GetOwner()) return;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, OtherComp->GetName());
	overlapMap.Remove(OtherComp);
}