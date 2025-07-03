// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Taggable.h"

#include "PortalCanvas.generated.h"

class UProceduralMeshComponent;
class UNiagaraComponent;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPortalDrawnDelegate, APortalCanvas*, newCanvas);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPortalClosedDelegate, APortalCanvas*, closedCanvas);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDrawPointsAddedDelegate, AActor*, drawSource, const TArray<FVector2D>&, newPoints, FVector2D, lastDrawnPoint);

USTRUCT()
struct FPortalPoint
{
	GENERATED_BODY()

	FPortalPoint() { this->pos = {}; }
	FPortalPoint(FVector2D pos) { this->pos = pos; }

	UPROPERTY()
	FVector2D pos;
	UPROPERTY()
	TArray<int> adjIndices;
};

USTRUCT()
struct FNewPointsList
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* source;
	UPROPERTY()
	TArray<FVector2D> points;
	UPROPERTY()
	FVector2D previousPoint;
};

UCLASS(Abstract)
class DUOLATERA_API APortalCanvas : public AActor, public ITaggable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalCanvas();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Portal Canvas Properties")
	APortalCanvas* destinationCanvas;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Portal Canvas Properties")
	bool isUnderworldCanvas = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Portal Canvas Properties")
	bool traversable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal Canvas Properties")
	float planeOffset = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal Canvas Properties")
	float portalDistortion = 1.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Portal Canvas Properties")
	float portalWarp = -10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal Canvas Properties")
	float drawVertexOffset = 7.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal Canvas Properties")
	float minimumArea = 45.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Portalable, Category = "Portal Canvas Properties")
	bool portalable = true;

protected:
	// Construction script
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnTagged_Implementation(FHitResult tagHit, AActor* tagSource) override;

	// Enter and exit portals are to be set by the BP child of this actor
	UPROPERTY(BlueprintReadWrite, Category = "Portal Canvas Protected Properties")
	AActor* overPortal;
	UPROPERTY(BlueprintReadWrite, Category = "Portal Canvas Protected Properties")
	AActor* underPortal;

	// Portal plane draw points. This is only set when the points are finalized from tempPts.
	UPROPERTY(Replicated)
	TArray<FPortalPoint> drawPts;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = "OnRep_CutPts", meta = (DisplayName = "Cut Points"))
	TArray<FVector2D> cutPts;

	UPROPERTY(ReplicatedUsing = "OnRep_NewPts")
	FNewPointsList newPts;

private:

	UFUNCTION()
	void OnRep_CutPts();
	UFUNCTION()
	void OnRep_NewPts();
	UFUNCTION()
	void OnRep_Portalable();

	UFUNCTION(NetMulticast, Unreliable)
	void DebugDrawPoints();

	bool TraceLoop(float minArea, TArray<bool>& visited, TArray<int>& visitedIndices, 
		int startIndex, int thisIndex, int previousIndex, TArray<FVector2D>& outLoopPts);

	TArray<AActor*> taggingSources;
	TArray<AActor*> prevTaggingSources;
	TMap<AActor*, int> pointOffsets;

	FTimerHandle portalableTmrHandle;
	UProceduralMeshComponent* pmesh;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Portal Canvas")
	void SetPortalVertices(TArray<FVector2D> verts);

	// Resets this canvas's planes back to what they were before the portal was opened.
	UFUNCTION(BlueprintCallable, Category = "Portal Canvas")
	void ResetCanvasPlanes();

	UFUNCTION(BlueprintCallable, Category = "Portal Canvas")
	TArray<FVector2D> GetDrawPoints();

	UFUNCTION(BlueprintCallable, Category = "Portal Canvas")
	void SetPortalable(bool value);

	// Notification Events
	UPROPERTY(BlueprintAssignable)
	FPortalDrawnDelegate OnPortalDrawn;

	UPROPERTY(BlueprintAssignable)
	FPortalClosedDelegate OnPortalClosed;

	UPROPERTY(BlueprintAssignable)
	FDrawPointsAddedDelegate OnDrawPointsAdded;

};
