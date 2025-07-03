#include "PortalCanvas.h"
#include "PortalComponent.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"

void APortalCanvas::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(APortalCanvas, drawPts);
	DOREPLIFETIME(APortalCanvas, newPts);
	DOREPLIFETIME(APortalCanvas, cutPts);
	DOREPLIFETIME(APortalCanvas, portalable);
}

// Sets default values
APortalCanvas::APortalCanvas()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void APortalCanvas::OnConstruction(const FTransform& Transform)
{
	// set portal canvas planes' materials
	pmesh = GetComponentByClass<UProceduralMeshComponent>();
	if (pmesh)
	{
		TArray<FVector2D> uvs{ { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } };
		pmesh->CreateMeshSection(0,
			{ { planeOffset, -50, -50 }, { planeOffset, 50, -50 }, { planeOffset, 50, 50 }, { planeOffset, -50, 50 } },
			{ 0, 2, 1, 0, 3, 2 },
			{ { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 } }, uvs, {}, {}, true);

		pmesh->CreateMeshSection(1,
			{ { -planeOffset, -50, -50 }, { -planeOffset, 50, -50 }, { -planeOffset, 50, 50 }, { -planeOffset, -50, 50 } },
			{ 0, 1, 2, 0, 2, 3 },
			{ { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 } }, uvs, {}, {}, true);

		if (!destinationCanvas)
			portalable = false;

		OnRep_Portalable();
	}
}

// Called when the game starts or when spawned
void APortalCanvas::BeginPlay()
{
	AActor::BeginPlay();

	// set portal canvas planes' materials
	pmesh = GetComponentByClass<UProceduralMeshComponent>();

	// Check if this canvas is in the same world as its destination, which should not happen.
	if (destinationCanvas && destinationCanvas->isUnderworldCanvas == isUnderworldCanvas)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("ERROR: PORTAL CANVAS " + 
			UKismetSystemLibrary::GetDisplayName(this) + " IS CONNECTED TO A PORTAL CANVAS OF " +
			"THE SAME WORLD! \nPlease set either this or its Destination Canvas to a different " +
			"world by changing its 'Is Underworld Canvas' bool."));

		portalable = false;
		OnRep_Portalable();
	}
}

// Called every frame
void APortalCanvas::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);

	if (taggingSources.IsEmpty())
		SetActorTickEnabled(false);

	prevTaggingSources = taggingSources;
	taggingSources.Empty(2); // We expect at least 2 tag sources per frame frequently

	//DebugDrawPoints();
}

bool Intersects(FVector2D v0Start, FVector2D v0End, FVector2D v1Start, FVector2D v1End, FVector2D& result)
{
	FVector2D v0Delta = v0End - v0Start;
	FVector2D v1Delta = v1End - v1Start;

	float s, t;
	s = (-v0Delta.Y * (v0Start.X - v1Start.X) + v0Delta.X * (v0Start.Y - v1Start.Y)) / (-v1Delta.X * v0Delta.Y + v0Delta.X * v1Delta.Y);
	t = (v1Delta.X * (v0Start.Y - v1Start.Y) - v1Delta.Y * (v0Start.X - v1Start.X)) / (-v1Delta.X * v0Delta.Y + v0Delta.X * v1Delta.Y);

	if (s >= FLT_EPSILON && s <= 1.f - FLT_EPSILON && t >= FLT_EPSILON && t <= 1.f - FLT_EPSILON)
	{
		// Intersect detected
		result = v0Start + (t * v0Delta);
		return true;
	}

	return false; // No intersect
}

bool APortalCanvas::TraceLoop(float minArea, TArray<bool>& visited, TArray<int>& visitedIndices, 
	int startIndex, int thisIndex, int previousIndex, TArray<FVector2D>& outLoopPts)
{
	visited[thisIndex] = true;

	for (int i : drawPts[thisIndex].adjIndices)
	{
		if (!visited[i])
		{
			if (TraceLoop(minArea, visited, visitedIndices, startIndex, i, thisIndex, outLoopPts))
			{
				outLoopPts.Add(drawPts[i].pos);
				visitedIndices.Add(i);
				if (thisIndex == startIndex)
				{
					// Back at the top of this recursion, check if the polygon is large enough to be considered "openworthy"
					// Area calculation source: https://www.geeksforgeeks.org/area-of-a-polygon-with-given-n-ordered-vertices/
					float area = 0;
					int k = outLoopPts.Num() - 1;
					FVector2D scale(pmesh->GetComponentScale().Y, pmesh->GetComponentScale().Z);
					for (int j = 0; j < outLoopPts.Num(); j++)
					{
						FVector2D scaledPt0 = outLoopPts[j] * scale;
						FVector2D scaledPt1 = outLoopPts[k] * scale;
						area += (scaledPt1.X + scaledPt0.X) * (scaledPt1.Y - scaledPt0.Y);
						k = j;
					}
					if (FMath::Abs(area / 2.f) >= minArea)
						return true;
					else
					{
						// reset all visited indices to unvisited to check for another loop
						for (int v : visitedIndices)
							visited[v] = false;
						visitedIndices.Reset();
						outLoopPts.Reset();
					}
				}
				else return true;
			}
		}
		else if (i != previousIndex && i == startIndex) // loop detected
		{
			outLoopPts.Add(drawPts[i].pos);
			return true;
		}
	}
	return false;
}

void APortalCanvas::OnTagged_Implementation(FHitResult tagHit, AActor* tagSource)
{
	// do nothing if the player isn't or shouldn't be drawing on the plane
	if (!(portalable && tagHit.Component == pmesh)) return;

	taggingSources.AddUnique(tagSource);

	// first bring the impact pt into local space and make it 2D
	tagHit.ImpactPoint = pmesh->GetComponentTransform().InverseTransformPosition(tagHit.ImpactPoint);
	FPortalPoint impactPt({ tagHit.ImpactPoint.Y, tagHit.ImpactPoint.Z });
	int initSize = drawPts.Num();

	if (prevTaggingSources.Contains(tagSource))
	{
		int offsetFromEnd = pointOffsets[tagSource] + 1;

		// Step 1: Add any intersection points between the last point and impact point
		for (int i = 0; i < initSize - 2; i++)
		{
			for (int j : drawPts[i].adjIndices)
			{
				if (j < i) continue; // already checked this edge

				FPortalPoint intersection;
				int startIndex = drawPts.Num() - offsetFromEnd;
				if (Intersects(drawPts[i].pos, drawPts[j].pos, drawPts[startIndex].pos, impactPt.pos, intersection.pos))
				{
					intersection.adjIndices = { i, j, startIndex };
					drawPts[i].adjIndices[drawPts[i].adjIndices.Find(j)] = drawPts.Num();
					drawPts[j].adjIndices[drawPts[j].adjIndices.Find(i)] = drawPts.Num();
					drawPts[startIndex].adjIndices.Add(drawPts.Num());

					drawPts.Add(intersection);

					// Check if an intersection resulted in a closed loop
					TArray<FVector2D> loopPts;
					TArray<bool> visited;
					TArray<int> visitedIndices;
					visited.AddZeroed(drawPts.Num());
					if (TraceLoop(minimumArea, visited, visitedIndices, drawPts.Num() - 1, drawPts.Num() - 1, -1, loopPts))
					{
						// Set the portal loop vertices, procedurally triangulating the new mesh
						cutPts = loopPts;
						OnRep_CutPts();
						goto notifyEvent; // AAHAAHAHAHAHAHAHAHAHAHAHAHAAHAHAAHA
					}
				}
			}
		}

		{ // Step 3: Add the impact point itself
			int prevIndex = drawPts.Num() - (initSize == drawPts.Num() ? offsetFromEnd : 1);
			if ((FVector2D(pmesh->GetComponentScale().Y, pmesh->GetComponentScale().Z) *
				(impactPt.pos - drawPts[prevIndex].pos)).SquaredLength() >= FMath::Square(drawVertexOffset))
			{
				impactPt.adjIndices = { prevIndex };
				drawPts[prevIndex].adjIndices.Add(drawPts.Num());
				drawPts.Add(impactPt);
			}
		}

	notifyEvent:

		int diff = drawPts.Num() - initSize;
		if (diff > 0)
		{
			// Update offset map
			pointOffsets[tagSource] = 0;
			for (auto& a : pointOffsets)
			{
				if (a.Key != tagSource) a.Value += diff;
			}

			// Notify new points
			newPts.source = tagSource;
			newPts.points.Empty(diff);
			for (int i = initSize; i < drawPts.Num(); i++)
				newPts.points.Add(drawPts[i].pos);
			newPts.previousPoint = drawPts[initSize - offsetFromEnd].pos;
			OnRep_NewPts();
		}
	}
	else
	{
		int index;
		bool exists = pointOffsets.Find(tagSource) != nullptr;
		if (exists && drawPts[index = drawPts.Num() - 1 - pointOffsets[tagSource]].adjIndices.IsEmpty())
		{
			drawPts[index] = impactPt;
		}
		else
		{
			if (exists) pointOffsets[tagSource] = 0;
			else pointOffsets.Add(tagSource, 0);

			for (auto& a : pointOffsets)
			{
				if (a.Key != tagSource) a.Value++;
			}
			drawPts.Add(impactPt);
		}

		// Notify new point
		newPts.source = tagSource;
		newPts.points = { impactPt.pos };
		newPts.previousPoint = { FLT_MAX, FLT_MAX };
		OnRep_NewPts();
	}

	SetActorTickEnabled(true);
}

void APortalCanvas::SetPortalVertices(TArray<FVector2D> vertices)
{
	// OnRep needs to be explicitely called in c++ for it to happen 
	// on the server, but it's called automatically for the client.
	if (HasAuthority())
	{
		for (int i = 0; i < vertices.Num(); i++)
		{
			FPortalPoint p(vertices[i]);
			p.adjIndices.Add((i == 0 ? vertices.Num() : i) - 1);
			p.adjIndices.Add((i + 1) % vertices.Num());
			drawPts.Add(p);
		}

		cutPts = vertices;
		OnRep_CutPts();
	}
}

void APortalCanvas::OnRep_CutPts()
{
	if (cutPts.IsEmpty()) return;

	// Set portal plane stuff
	AActor* thisPortal = isUnderworldCanvas ? underPortal : overPortal;
	AActor* destPortal = isUnderworldCanvas ? overPortal : underPortal;
	thisPortal->SetActorTransform(pmesh->GetComponentTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	thisPortal->AttachToActor(this, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
	destPortal->SetActorTransform(destinationCanvas->pmesh->GetComponentTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	destPortal->AttachToActor(destinationCanvas, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));

	UPortalComponent* pc = thisPortal->GetComponentByClass<UPortalComponent>();
	pc->SetPlaneOffset(planeOffset - 0.01f); // slight bias so the canvas plane lies in front
	pc->warpAmount = portalWarp;
	pc->traversable = traversable;

	pc = pc->GetDestinationPortal();
	pc->SetPlaneOffset(destinationCanvas->planeOffset - 0.01f); // slight bias so the canvas plane lies in front
	pc->warpAmount = destinationCanvas->portalWarp;
	pc->traversable = destinationCanvas->traversable;

	// Set pmesh verts and normals
	TArray<FVector> verts, normals;
	normals.Add({ 1, 0, 0 });
	verts.Reserve(cutPts.Num());
	verts.Add({ planeOffset, cutPts[0].X, cutPts[0].Y });

	float inAngle = 0, outAngle = 0;
	for (int i = 1; i < cutPts.Num(); i++)
	{
		FVector thisPt(planeOffset, cutPts[i].X, cutPts[i].Y);
		FVector lastPt(planeOffset, cutPts[i - 1].X, cutPts[i - 1].Y);
		FVector nextPt(planeOffset, cutPts[(i + 1) % cutPts.Num()].X, cutPts[(i + 1) % cutPts.Num()].Y);

		FVector lastVec = (lastPt - thisPt).GetSafeNormal();
		FVector nextVec = (nextPt - thisPt).GetSafeNormal();
		FVector cross = lastVec.Cross(nextVec);

		float angle = FMath::Acos(lastVec.Dot(nextVec));
		inAngle += angle - (cross.X > 0 ? 0 : PI * 2.f);
		outAngle += (cross.X > 0 ? PI * 2.f : 0) - angle;

		verts.Add(thisPt);
		normals.Add({ 1, 0, 0 });
	}

	pmesh->CreateMeshSection(0, {}, {}, {}, {}, {}, {}, true);
	destinationCanvas->pmesh->CreateMeshSection(0, {}, {}, {}, {}, {}, {}, true);

	for (int i = 0; i < verts.Num(); i++)
	{
		verts[i].X *= -1;
		normals[i].X *= -1;
	}
	pmesh->CreateMeshSection(1, {}, {}, {}, {}, {}, {}, true);
	destinationCanvas->pmesh->CreateMeshSection(1, {}, {}, {}, {}, {}, {}, true);

	FNavigationSystem::UpdateComponentData(*pmesh);
	FNavigationSystem::UpdateComponentData(*destinationCanvas->pmesh);

	// Notify event
	OnPortalDrawn.Broadcast(this);
}

void APortalCanvas::OnRep_NewPts()
{
	if (!newPts.points.IsEmpty())
		OnDrawPointsAdded.Broadcast(newPts.source, newPts.points, newPts.previousPoint);
}

void APortalCanvas::SetPortalable(bool value)
{
	if (!(destinationCanvas && HasAuthority())) return;
	portalable = value;
	OnRep_Portalable();
}

void APortalCanvas::OnRep_Portalable()
{
	UMaterialInstanceDynamic* mid = Cast<UMaterialInstanceDynamic>(pmesh->GetMaterial(0));
	mid->SetScalarParameterValueByInfo(FMaterialParameterInfo("IsTaggable", LayerParameter, 0), portalable ? 1.f : 0.f);
}

void APortalCanvas::DebugDrawPoints_Implementation()
{
	for (auto p : drawPts)
	{
		for (auto i : p.adjIndices)
		{
			UKismetSystemLibrary::DrawDebugLine(this,
				pmesh->GetComponentTransform().TransformPosition({ planeOffset, p.pos.X, p.pos.Y }),
				pmesh->GetComponentTransform().TransformPosition({ planeOffset, drawPts[i].pos.X, drawPts[i].pos.Y }),
				FColor::Yellow, 0.5f, 1);
			UKismetSystemLibrary::DrawDebugLine(this,
				pmesh->GetComponentTransform().TransformPosition({ -planeOffset, p.pos.X, p.pos.Y }),
				pmesh->GetComponentTransform().TransformPosition({ -planeOffset, drawPts[i].pos.X, drawPts[i].pos.Y }),
				FColor::Yellow, 0.5f, 1);
		}
	}
}

TArray<FVector2D> APortalCanvas::GetDrawPoints()
{
	TArray<FVector2D> pts;
	pts.Reserve(drawPts.Num());
	for (auto& portalPt : drawPts)
		pts.Add(portalPt.pos);

	return pts;
}

void APortalCanvas::ResetCanvasPlanes()
{
	drawPts.Empty();
	cutPts.Empty();
	pointOffsets.Empty();

	TArray<FVector2D> uvs{ { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } };

	pmesh->CreateMeshSection(0,
		{ { planeOffset, -50, -50 }, { planeOffset, 50, -50 }, { planeOffset, 50, 50 }, { planeOffset, -50, 50 } },
		{ 0, 2, 1, 0, 3, 2 },
		{ { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 } },
		uvs, {}, {}, true);

	pmesh->CreateMeshSection(1,
		{ { -planeOffset, -50, -50 }, { -planeOffset, 50, -50 }, { -planeOffset, 50, 50 }, { -planeOffset, -50, 50 } },
		{ 0, 1, 2, 0, 2, 3 },
		{ { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 } },
		uvs, {}, {}, true);

	FNavigationSystem::UpdateComponentData(*pmesh);

	// Add some buffer time between portal activations so it
	// can't be spammed, which would be terribly flashy
	if (portalable)
	{
		portalable = false;
		OnRep_Portalable();
		GetWorldTimerManager().SetTimer(portalableTmrHandle,
			[this] { portalable = true; OnRep_Portalable(); }, 0.5f, false);
	}

	OnPortalClosed.Broadcast(this);
}