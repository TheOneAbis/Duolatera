// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementInterpolation.h"

bool UMovementInterpolation::IterateTimer(float deltaTime, UPARAM(ref) float& currentTimer, float interval)
{
	currentTimer += deltaTime;
	if (currentTimer > interval) {
		currentTimer = 0.0;
		return true;
	}
	return false;
}

 FVector3f UMovementInterpolation::CatmullRomLocationInterploate(UPARAM(ref) TArray<FVector3f>& points, float currentTime, float interval)
{
	 if (points[0] == FVector3f(0, 0, 0)) {
		 return points[3];
	 }

	 if (currentTime > interval) {
	 	UE_LOG(LogTemp, Warning, TEXT(""));
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("current time is greater than interval, this shouldn't happen"));
		}
	 	currentTime = 0.0;
		
	 }
	 float t = currentTime / interval;
	 
	 FVector3f pt3 = (-points[0] + 3 * points[1] - 3 * points[2] + points[3]) / 2.000;
	 FVector3f pt2 = (2 * points[0] - 5 * points[1] + 4 * points[2] - points[3]) / 2.000;
	 FVector3f pt1 = (-points[0] + points[2]) / 2.000;
	 FVector3f pt0 = points[1];
	 
	 return pt3 * pow(t, 3) + pt2 * pow(t, 2) + pt1 * t + pt0;
}

 void UMovementInterpolation::CatmullRomPointsUpdate(FVector3f p, UPARAM(ref) TArray<FVector3f>& points)
 {

	 if (points.GetTypeSize() > 12) {
		 UE_LOG(LogTemp, Warning, TEXT("too many vector passed in, current floats (3 floats to one vector): %d"), points.GetTypeSize());
		 return;
	 }

	 //p[3] is the latest location
	 for (int i = 0; i < 3; i++) {
		 points[i] = points[i + 1];
	 }
	 points[3] = p;

	 return;
 }


