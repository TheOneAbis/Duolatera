// Fill out your copyright notice in the Description page of Project Settings.


#include "ExtraBlueprintFunctionLibrary.h"

#include "Components/SceneComponent.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "PortalComponent.h"

void UExtraBlueprintFunctionLibrary::UpdateNavComponentData(USceneComponent* component)
{
	if (component && component->IsRegistered())
	{
		if (component->GetWorld() != nullptr)
		{
			FNavigationSystem::UpdateComponentData(*component);
		}
	}
}

bool UExtraBlueprintFunctionLibrary::PortalLineTrace(
	const UObject* WorldContextObject, 
	const FVector Start, 
	const FVector Direction, 
	const float Distance,
	ETraceTypeQuery TraceChannel, 
	bool bTraceComplex, 
	const TArray<AActor*>& ActorsToIgnore, 
	EDrawDebugTrace::Type DrawDebugType,
	FHitResult& OutHit, 
	FHitResult& OutPortalHit,
	bool& tracedThroughPortal,
	bool bIgnoreSelf, 
	FLinearColor TraceColor, 
	FLinearColor TraceHitColor, 
	float DrawTime)
{
	FVector normalDir = Direction.GetSafeNormal();

	// start with the usual line trace
	bool hit = UKismetSystemLibrary::LineTraceSingle(WorldContextObject, Start, Start + (normalDir * Distance), TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
	if (!hit) return false;

	// if we hit a portal plane, trace through it
	AActor* hitActor = OutHit.GetActor();
	tracedThroughPortal = hitActor && hitActor->ActorHasTag("Portal");
	if (tracedThroughPortal)
	{
		OutPortalHit = OutHit;
		UPortalPlane* hitPlane = (UPortalPlane*)OutHit.GetComponent();

		// ignore the exit portal so it's not accidentally blocking
		TArray<AActor*> newActorsToIgnore = ActorsToIgnore;
		if (auto* ptr = OutHit.GetActor()->GetComponentByClass<UPortalComponent>()->GetDestinationPortal())
			newActorsToIgnore.Add(ptr->GetOwner());

		// calculate new start and end pts to trace on the other side of the portal
		FVector newStart = UPortalComponent::PortalTransformPoint(OutHit.ImpactPoint, hitPlane);
		FVector newDir = UPortalComponent::PortalTransformVector(normalDir, hitPlane) * (Distance - OutHit.Distance);
		hit = UKismetSystemLibrary::LineTraceSingle(WorldContextObject, newStart, newStart + newDir, TraceChannel, bTraceComplex,
			newActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
	}
	return hit;
}

bool UExtraBlueprintFunctionLibrary::PortalPredictProjectilePath(
	const UObject* WorldContextObject, 
	FHitResult& OutHit,
	FHitResult& OutPortalHit,
	TArray<FVector>& OutPathPositions, 
	FVector& OutLastTraceDestination,
	int& portalStartIndex, 
	FVector StartPos, 
	FVector LaunchVelocity, 
	bool bTracePath,
	float ProjectileRadius, 
	const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, 
	bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore, 
	EDrawDebugTrace::Type DrawDebugType, 
	float DrawDebugTime, 
	float SimFrequency,
	float MaxSimTime, 
	float OverrideGravityZ)
{
	FPredictProjectilePathParams Params = FPredictProjectilePathParams(ProjectileRadius, StartPos, LaunchVelocity, MaxSimTime);
	Params.bTraceWithCollision = bTracePath;
	Params.bTraceComplex = bTraceComplex;
	Params.ActorsToIgnore = ActorsToIgnore;
	Params.DrawDebugType = DrawDebugType;
	Params.DrawDebugTime = DrawDebugTime;
	Params.SimFrequency = SimFrequency;
	Params.OverrideGravityZ = OverrideGravityZ;
	Params.ObjectTypes = ObjectTypes; // Object trace
	Params.bTraceWithChannel = false;

	// Start with normal prediction
	FPredictProjectilePathResult Result;
	bool hit = UGameplayStatics::PredictProjectilePath(WorldContextObject, Params, Result);

	// fill results
	OutPathPositions.Empty(Result.PathData.Num());
	for (auto& datapt : Result.PathData)
	{
		OutPathPositions.Add(datapt.Location);
	}
	portalStartIndex = OutPathPositions.Num();
	if (!hit) return false;

	// if hit portal plane, do another prediction on the other side
	AActor* hitActor = Result.HitResult.GetActor();
	if (hitActor && hitActor->ActorHasTag("Portal"))
	{
		if (hitActor->GetComponentByClass<UPortalComponent>()->traversable)
		{
			OutPortalHit = Result.HitResult;
			UPortalPlane* hitPlane = (UPortalPlane*)Result.HitResult.GetComponent();

			// ignore the exit portal so it's not accidentally blocking
			if (auto* ptr = hitActor->GetComponentByClass<UPortalComponent>()->GetDestinationPortal())
				Params.ActorsToIgnore.Add(ptr->GetOwner());

			// calculate new start location and velocity to predict on the other side of the portal
			Params.StartLocation = UPortalComponent::PortalTransformPoint(Result.HitResult.ImpactPoint, hitPlane);
			FVector interpVel = FMath::Lerp(Result.PathData.Last(1).Velocity, Result.LastTraceDestination.Velocity, Result.HitResult.Time);
			Params.LaunchVelocity = UPortalComponent::PortalTransformVector(interpVel, hitPlane);
			hit = UGameplayStatics::PredictProjectilePath(WorldContextObject, Params, Result);
			if (hit)
			{
				for (auto& datapt : Result.PathData)
				{
					OutPathPositions.Add(datapt.Location);
				}
			}
		}
	}

	OutLastTraceDestination = Result.LastTraceDestination.Location;
	OutHit = Result.HitResult;
	return hit;
}

FVector UExtraBlueprintFunctionLibrary::CalculateHolsterRelativeLocation(const FTransform cameraRelativeTrans, 
	const FVector VROriginWorldLoc, const float playerHeight)
{	
	FVector forwardHorizontal = cameraRelativeTrans.GetRotation().GetForwardVector();
	forwardHorizontal.Z = 0;

	FVector holsterRelativeLoc = cameraRelativeTrans.GetLocation() //start from camera's location
		+ forwardHorizontal * playerHeight * 0.1 //forward
		+ FVector(0,0,-1) * playerHeight * 0.3; //downward
	return holsterRelativeLoc;
}

float UExtraBlueprintFunctionLibrary::CalculateTorsoInclinationAngle(const TArray<float>& socketHeights, const float playerHeight, const float cameraRelativeHeight)
{
	//socket heights: eye, hip, knee, ankel, in order
	if (socketHeights.Num() != 4) {
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Warning: CalculateTorsoInclination: socketHeights contains incorrect number of element"));
		return 0.0f;
	}
	
	float
		Lu = (socketHeights[0] - socketHeights[1]) * playerHeight / 180.0, //upper body
		Lth = (socketHeights[1] - socketHeights[2]) * playerHeight / 180.0, //thigh
		Lc = (socketHeights[2] - socketHeights[3]) * playerHeight / 180.0; //calf
	float H = cameraRelativeHeight - socketHeights[3] * playerHeight / 180.0;
	float L = Lu + Lc;

	float cosA = (H * H + L * L - Lth * Lth) / (2 * H * L);
	if (abs(cosA) < 1) { //eye's low enough and needs leaning
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("arc cos A: %f PI"), acos(cosA)/3.1415926));
		return acos(cosA);
	}
	else return 0.0f;
}

int64 UExtraBlueprintFunctionLibrary::GetObjectUniqueID(const UObject* obj)
{
	uint32 id = obj->GetUniqueID();
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("uint32: %i PI"), id));
	int64 bpID = id;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("uint64: %i PI"), bpID));
	return bpID;
}

FVector UExtraBlueprintFunctionLibrary::ClampElbowTargetLocation(const FTransform handTransform, const FTransform headTransform, bool leftHand)
{
	FVector elbowDir = handTransform.GetRotation().GetForwardVector() * -1.0f;
	FVector headSideDir = headTransform.GetRotation().GetRightVector() * (leftHand ? -1.0f : 1.0f);
	//return handTransform.GetLocation() + headSideDir * 20;

	if (FVector::DotProduct(elbowDir, headSideDir) < 0) return handTransform.GetLocation() + FVector(0, 0, -1) * 20;
	//else if (FVector::DotProduct(elbowDir, FVector(0, 0, -1)) < 0) return handTransform.GetLocation() + headSideDir * 20;
	else return handTransform.GetLocation() + elbowDir * 20;
}

//This snapping function works roughly in a range of 150 - 50 cm
void UExtraBlueprintFunctionLibrary::AdjustSplinePoint(
	const UObject* WorldContextObject, 
	const AActor* selfRef, 
	USplineComponent* spline,
	const float detectRadius,
	const float expandDistance,
	const float snappingMargin,
	const bool bShowDebugDrawing)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	
	//iterate through every spline point
	for (int i = 0; i < spline->GetNumberOfSplinePoints(); i++) {
		//make sure spline points' type are all linear
		spline->SetSplinePointType(i, ESplinePointType::Linear, false);
		FSplinePoint originalPoint = spline->GetSplinePointAt(i, ESplineCoordinateSpace::World);
		FVector segmentPoint = selfRef->GetActorLocation() + originalPoint.Position;

		//sphere cast to get hit points
		TArray<FHitResult> outResults;
		FCollisionShape sphereCol = FCollisionShape::MakeSphere(40.0f);
		if (bShowDebugDrawing) DrawDebugSphere(World, segmentPoint, detectRadius, 16, FColor::Green);
		World->SweepMultiByChannel(outResults, segmentPoint, segmentPoint, FQuat::Identity, ECollisionChannel::ECC_Visibility, sphereCol);

		if (outResults.Num() > 2 || outResults.Num() == 0) {//more than 2 hit points is too complex, simply leave as is
			if (bShowDebugDrawing) DrawDebugSphere(World, segmentPoint, detectRadius, 16, FColor::Black);
			continue;
		}
		if (bShowDebugDrawing) {
			for (auto hit = outResults.CreateConstIterator(); hit; ++hit) {
				DrawDebugSphere(World, hit->ImpactPoint, detectRadius / 20, 8, FColor::White);
			}
		}
		

		//flat surface or outside corner
		if (outResults.Num() == 1) {
			FVector arriveTangent = originalPoint.ArriveTangent.GetSafeNormal();
			FVector leaveTangent = originalPoint.LeaveTangent.GetSafeNormal();

			//Then set arrive and leave tangent
			FVector arriveTraceOrigin = segmentPoint - arriveTangent * expandDistance;
			FVector leaveTraceOrigin = segmentPoint + leaveTangent * expandDistance;
			
			//prepare for line trace
			FHitResult arriveHit;
			FHitResult leaveHit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(selfRef); // Ignore the actor performing the trace

			//cast 2 more line trace from both sides of the spline point, following the tangents
			bool bArriveHit = World->LineTraceSingleByChannel(arriveHit, arriveTraceOrigin,
				arriveTraceOrigin - outResults[0].ImpactNormal * expandDistance * 10, ECC_Visibility, QueryParams);
			bool bLeaveHit = World->LineTraceSingleByChannel(leaveHit, leaveTraceOrigin,
				leaveTraceOrigin - outResults[0].ImpactNormal * expandDistance * 10, ECC_Visibility, QueryParams);
			
			if (bArriveHit && bLeaveHit) { //only try to modify arrive & leave tangent when both hit
				//first, check if two hit normals are aligned, which means it's a flat surface
				if (FVector::DotProduct(arriveHit.ImpactNormal.GetUnsafeNormal(), leaveHit.ImpactNormal.GetUnsafeNormal()) > 0.95) {
					spline->SetLocationAtSplinePoint(i, outResults[0].ImpactPoint + outResults[0].ImpactNormal * snappingMargin,
						ESplineCoordinateSpace::World, false);
					continue;
				}
				else {
					FVector n1 = arriveHit.ImpactNormal.GetSafeNormal();
					FVector n2 = leaveHit.ImpactNormal.GetSafeNormal();
					
					//if the spline doesn't go along with the curve, leave as is
					if (abs(FVector::DotProduct(
						FVector::CrossProduct(n1, n2),
						FVector::CrossProduct(originalPoint.ArriveTangent, originalPoint.LeaveTangent)
					)) < 0.95) {
						if (bShowDebugDrawing) DrawDebugSphere(World, segmentPoint, detectRadius, 16, FColor::Red);
						continue;
					}

					float offsetDistnace = snappingMargin / sqrt((1 + FVector::DotProduct(n1, n2)) / 2.f);
					spline->SetLocationAtSplinePoint(i, outResults[0].ImpactPoint + offsetDistnace * (n1 + n2).GetSafeNormal(),
						ESplineCoordinateSpace::World, false);
				}
			}
			else continue; //if any side doesn't hit anything, leave tangents as is
		}

		// outResults.Num() == 2, inside corner
		else {
			if (FVector::DotProduct(outResults[0].ImpactPoint - segmentPoint, outResults[1].ImpactPoint - segmentPoint) == 1) {
				if (bShowDebugDrawing) DrawDebugSphere(World, segmentPoint, detectRadius, 16, FColor::Red);
				continue;
			}
			FVector crossVector = FVector::CrossProduct(outResults[0].ImpactPoint - segmentPoint, outResults[1].ImpactPoint - segmentPoint);

			//cast two more line parallel to the walls, pointing to the inside corner, with a snappingMargin distance to the walls
			FVector parallelDirs[2];
			for (int j = 0; j < 2; j++) parallelDirs[j] = FVector::CrossProduct(outResults[j].ImpactNormal, crossVector);

			FVector hitsMiddlePoint = (outResults[0].ImpactPoint + outResults[1].ImpactPoint) / 2.f;
			//line trace along parallel directions
			FHitResult parallelHits[2];
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(selfRef);
			bool bothHit = true;
			for (int j = 0; j < 2; j++) {
				//make sure parallel direction is facing the middle point
				if (FVector::DotProduct(parallelDirs[j], hitsMiddlePoint - outResults[j].ImpactPoint) < 0) 
					parallelDirs[j] = -parallelDirs[j];

				if (!World->LineTraceSingleByChannel(parallelHits[j], outResults[j].ImpactPoint + outResults[j].ImpactNormal * snappingMargin,
					outResults[j].ImpactPoint + outResults[j].ImpactNormal * snappingMargin + parallelDirs[j] * expandDistance * 10, ECC_Visibility, QueryParams)) {
					bothHit = false;
					break;
				}
			}
			// somehow line trace on parallel direction failed, leave as is
			if (!bothHit) { 
				if (bShowDebugDrawing) DrawDebugSphere(World, segmentPoint, detectRadius, 16, FColor::Red);
				continue;
			}
			//solve equation to get final snapping point
			FVector parallelDirCross = FVector::CrossProduct(parallelDirs[0], parallelDirs[1]);
			float t = FVector::DotProduct(
				FVector::CrossProduct(parallelDirs[0], parallelHits[0].ImpactPoint - parallelHits[1].ImpactPoint),
				parallelDirCross)
				/
				FVector::DotProduct(parallelDirCross, parallelDirCross);
			spline->SetLocationAtSplinePoint(i, parallelHits[0].ImpactPoint + parallelDirs[0] * t,
				ESplineCoordinateSpace::World, false);
		}

		if (bShowDebugDrawing) DrawDebugSphere(World, segmentPoint, detectRadius, 16, FColor::Green);
	}
	spline->UpdateSpline();
	
	return;
}


#include "Developer/MessageLog/Public/MessageLogModule.h"
#include "Developer/MessageLog/Public/IMessageLogListing.h"
FString UExtraBlueprintFunctionLibrary::GetImportFbxWarnignMessages()
{
	const TCHAR* LogTitle = TEXT("FBXImport");
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	TSharedPtr<class IMessageLogListing> LogListing = MessageLogModule.GetLogListing(LogTitle);
	LogListing->SetLabel(FText::FromString("FBX Import"));
	if (!LogListing.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("LogListing for %s is not valid!"), LogTitle);
	}
	FString result;
	result = LogListing->GetAllMessagesAsString();

	return result;
}
















