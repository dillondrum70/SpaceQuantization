// Fill out your copyright notice in the Description page of Project Settings.


#include "Quantizer.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"

FGridMask::FGridMask()
{

}


FQuantizedSpace::FQuantizedSpace()
{
	Location = FIntVector2(0, 0);
	Height = 0;
}

// Sets default values
AQuantizer::AQuantizer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SplineComp = CreateDefaultSubobject<USplineComponent>(FName("Spline Component"));
	SplineComp->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AQuantizer::BeginPlay()
{
	Super::BeginPlay();
	
	GenerateHeightmap();
}


void AQuantizer::GenerateHeightmap()
{
	if (LandscapeActor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("LandscapeActor is null in AQuantizer::GenerateHeightmap"));
		return;
	}

	FVector Extents, Origin;

	//Get bounds of landscape to calculate size
	LandscapeActor->GetActorBounds(true, Origin, Extents);

	//Get Dimensions of landscape in Unreal units
	LandscapeDimensions.X = Extents.X * 2;
	LandscapeDimensions.Y = Extents.Y * 2;

	//Get Dimensions in terms of grid points
	GridDimensions.X = (int)LandscapeDimensions.X / Resolution;
	GridDimensions.Y = (int)LandscapeDimensions.Y / Resolution;

	/*UE_LOG(LogTemp, Display, TEXT("Landscape Dimensions: (%f, %f) \nGrid Dimensions: (%i, %i)"), 
		LandscapeDimensions.X, LandscapeDimensions.Y, GridDimensions.X, GridDimensions.Y);*/

	//For every grid point
	for (int x = 0; x < LandscapeDimensions.X; x += Resolution)
	{
		for (int y = 0; y < LandscapeDimensions.Y; y += Resolution)
		{
			FIntVector StartLocation = FIntVector(x, y, (int)SampleMaxHeight);

			FQuantizedSpace NewSpace;

			//Sample terrain
			if (SampleTerrainHeight(StartLocation, NewSpace))
			{
				//Cache returned value
				CachedHeightmap.Add(FIntVector2(x, y), NewSpace);
			}
			else
			{
				//Print if failed
				UE_LOG(LogTemp, Warning, TEXT("Line trace missed terrain at (%i, %i)"), StartLocation.X, StartLocation.Y);
			}
		}
	}
}


bool AQuantizer::SampleTerrainHeight(FIntVector StartLocation, FQuantizedSpace& OutResult)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("AQuantizer::SampleTerrainHeight - How the hell is World null?"))
		return false;
	}

	FVector Start = (FVector)StartLocation;
	FVector End = (FVector)StartLocation + (FVector::DownVector * (SampleMaxHeight + SampleMaxDepth));

	// Raycast down from location to terrain
	FHitResult Hit;
	bool bHitSuccessful = World->LineTraceSingleByChannel(
		Hit, 
		Start, 
		End,
		ECollisionChannel::ECC_Visibility);

	// If we hit a surface, cache the location
	if (!bHitSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("Raycast starting at (%i, %i, %i) did not hit"), StartLocation.X, StartLocation.Y, StartLocation.Z);

		//Draw a red line where it failed
		DrawDebugLine(World, Start, End, FColor::Red, true);

		return false;
	}

	OutResult.Location = FIntVector2(StartLocation.X, StartLocation.Y);
	OutResult.Height = Hit.Location.Z;

	//Draw line check
	//DrawDebugLine(World, Start, FVector(Start.X, Start.Y, OutResult.Height), FColor::Green, true);

	return true;
}


// Called every frame
void AQuantizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}


FQuantizedSpace AQuantizer::Quantize(FVector Location)
{
	FQuantizedSpace Result = FQuantizedSpace();

	//Rounds location to the location of the grid point at the corner of this cell
	Location.X = ((int)Location.X / Resolution) * Resolution;	
	Location.Y = ((int)Location.Y / Resolution) * Resolution;
	
	//UE_LOG(LogTemp, Display, TEXT("Coord: (%f, %f)"), Location.X, Location.Y);

	FIntVector2 Coord = FIntVector2(Location.X, Location.Y);

	//Get and return result
	Result = CachedHeightmap[Coord];

	//UE_LOG(LogTemp, Display, TEXT("Quantized Source: (%i, %i)"), Result.Location.X, Result.Location.Y);

	return Result;
}


FAStarNode AQuantizer::PopLowestCostNode()
{
	FAStarNode element = Frontier[0];

	//Loop through Frontier and find lowest cost node
	for (int i = 1; i < Frontier.Num(); i++)
	{
		if (Frontier[i].Cost < element.Cost)
		{
			element = Frontier[i];
		}
	}

	//Remove lowest cost node from Frontier and return it
	Frontier.Remove(element);

	return element;
}


bool AQuantizer::ComputePath(FVector _Source, FVector _Destination)
{
	//Cache passed values
	Source = _Source;
	Destination = _Destination;

	//Quantize positions in terms of grid points
	QuantizedSource = Quantize(_Source);
	QuantizedDestination = Quantize(_Destination);

	//UE_LOG(LogTemp, Display, TEXT("Source: (%f, %f)"), Source.X, Source.Y);
	//UE_LOG(LogTemp, Display, TEXT("Quantized Source: (%i, %i)"), QuantizedSource.Location.X, QuantizedSource.Location.Y);

	//Delete previous visualization
	SplineComp->ClearSplinePoints();

	FAStarNode StartNode(0, 0, QuantizedSource.Location, QuantizedSource.Location);	//Starting node is on the source position, 0 cost

	//Unexplored spaces, clear frontier
	Frontier.Empty();

	Frontier.Add(StartNode);	//Initialize frontier with starting node

	//Keys lead to the parent node as a value of the key, clear Parents
	Parents.Empty();
	//Nodes that have already been visited, clear Closed
	Closed.Empty();

	//Parent of start node is itself
	Parents.Add(StartNode.Location, StartNode.Location);

	//Loop until goal is found
	while (!Frontier.IsEmpty())
	{
		FAStarNode CurrentNode = PopLowestCostNode();

		GenerateSuccessors(SampleMask, CurrentNode, QuantizedDestination);

		Closed.Add(CurrentNode);
	}

	DrawPath();

	return true;
}


float AQuantizer::CostFunction(const FAStarNode& Current, FAStarNode& Next) const
{
	//Distance calculations using 2D space
	FVector Current3 = FVector(Current.Location.X, Current.Location.Y, 0);
	FVector Next3 = FVector(Next.Location.X, Next.Location.Y, 0);

	//Can probably be cached somewhere
	FVector Goal = FVector(QuantizedDestination.Location.X, QuantizedDestination.Location.Y, 0);
	
	FVector Vec = Next3 - Current3;				//Vector between current point and neighbor
	float CurrentNextDistance = Vec.Length();	//Distance between current point and neibor

	//Distance (in terms of grid units) between next and goal (h)
	float DistanceToGoal = ((Goal - Next3).Length() / Resolution) * LengthCostWeight;

	//Calculate new distance from start (g)
	Next.DistanceFromStart = Current.DistanceFromStart + ((CurrentNextDistance / Resolution) * LengthCostWeight);

	//Calculate new cost (g + h)
	Next.Cost = Next.DistanceFromStart + DistanceToGoal;

	//Add third dimension
	Current3.Z = CachedHeightmap[Current.Location].Height;
	Next3.Z = CachedHeightmap[Next.Location].Height;

	Vec = Next3 - Current3;

	//Angle calculations
	FVector ProjectedVec = (FVector::VectorPlaneProject(Vec, FVector::UpVector)).GetSafeNormal();

	float angle = FMath::RadiansToDegrees(FMath::Acos(Vec.Dot(ProjectedVec) / (Vec.Length() * ProjectedVec.Length())));

	//Do not accept any neighbors above the angle threshold
	if (angle > MaxAngleThreshold)
	{
		Next.Cost = INFINITY;
		return INFINITY;
	}

	//UE_LOG(LogTemp, Display, TEXT("Angle: %f"), angle);

	//return (angle * AngleCostWeight) + ((length / Resolution) * LengthCostWeight);
	return Next.Cost;
}


float AQuantizer::GoalFunction(FIntVector2 Current) const
{
	return (FVector(QuantizedDestination.Location.X, QuantizedDestination.Location.Y, QuantizedDestination.Height) -
		FVector(Current.X, Current.Y, CachedHeightmap[Current].Height)).Length();
}


bool AQuantizer::IsGridPointValid(FIntVector2 GridPoint) const
{
	//Check grid point is not out of range or less than 0
	return CachedHeightmap.Contains(GridPoint);
}


void AQuantizer::GenerateSuccessors(const FGridMask& GridMask, const FAStarNode& Current, const FQuantizedSpace& Goal)
{
	//float LowestCost = INFINITY;

	//Sample all grid mask points
	for (int i = 0; i < GridMask.MaskPoints.Num(); i++)
	{
		FAStarNode NextNode;

		//Calcualte next point
		NextNode.Location = Current.Location;
		NextNode.Location.X += GridMask.MaskPoints[i].X * Resolution;
		NextNode.Location.Y += GridMask.MaskPoints[i].Y * Resolution;

		//Ensure grid point is valid
		if (!IsGridPointValid(NextNode.Location))
		{
			UE_LOG(LogTemp, Display, TEXT("Grid point not valid at (%i, %i)"), NextNode.Location.X, NextNode.Location.Y);
			continue;
		}
		
		//Check if reached goal
		if (NextNode.Location == Goal.Location)
		{
			UE_LOG(LogTemp, Warning, TEXT("Algorithm finished, goal reached with A*"));

			/////////////////// TODO - Implement traceback and finish algorithm

			Path.Empty();
			Path.Add(Destination);
			TraceBackPath(NextNode, Path);
			Path.Add(Source);

			//Finish A*
			Frontier.Empty();

			return;
		}

		//Get cost
		float CurrentCost = CostFunction(Current, NextNode);

		//Find node in open list if it exists
		int ExistingIndex = Frontier.Find(NextNode);

		//Ignore this node if one exists already with a lower cost
		if (ExistingIndex != INDEX_NONE && Frontier[ExistingIndex].Cost <= NextNode.Cost)
		{
			continue;
		}

		//Find node in open list if it exists
		ExistingIndex = Closed.Find(NextNode);

		//Ignore this node if one exists already with a lower cost
		if (ExistingIndex != INDEX_NONE && Closed[ExistingIndex].Cost <= NextNode.Cost)
		{
			continue;
		}

		//Set parent
		NextNode.Parent = Current.Location;

		//Overwrite if in map, add new entry otherwise
		if (Parents.Contains(NextNode.Location))
		{
			Parents[NextNode.Location] = Current.Location;
		}
		else
		{
			Parents.Add(NextNode.Location, Current.Location);
		}

		//Add to frontier
		Frontier.Add(NextNode);
		
		//Draw sample line between current and sample point
		/*DrawDebugLine(GetWorld(), FVector(Current.Location.X, Current.Location.Y, CachedHeightmap[Current.Location].Height),
			FVector(NextNode.Location.X, NextNode.Location.Y, CachedHeightmap[NextNode.Location].Height), FColor::White, true);*/
	}
}


void AQuantizer::TraceBackPath(const FAStarNode& LastNode, TArray<FVector>& PathTrace)
{
	PathTrace.Add(FVector((float)LastNode.Location.X, (float)LastNode.Location.Y, CachedHeightmap[LastNode.Location].Height));

	FIntVector2 CurrentLocation = LastNode.Parent;
	
	//Trace back until you reach the start
	while (CurrentLocation != Parents[CurrentLocation])
	{
		//UE_LOG(LogTemp, Display, TEXT("Current Location: (%f, %f, %f)"), (float)CurrentLocation.X, (float)CurrentLocation.Y, CachedHeightmap[CurrentLocation].Height);

		//Add current location to array
		PathTrace.Add(FVector((float)CurrentLocation.X, (float)CurrentLocation.Y, CachedHeightmap[CurrentLocation].Height));

		//Move to next node
		CurrentLocation = Parents[CurrentLocation];
	}

	PathTrace.Add(FVector((float)CurrentLocation.X, (float)CurrentLocation.Y, CachedHeightmap[CurrentLocation].Height));
}


void AQuantizer::DrawPath()
{
	if (!SplineComp)
	{
		UE_LOG(LogTemp, Error, TEXT("Spline Component is not valid in AQuantizer::GenerateSuccessors"));
		return;
	}

	//UE_LOG(LogTemp, Display, TEXT("Source: (%f, %f, %f)"), Source.X, Source.Y, Source.Z);
	FSplinePoint NewPoint;
	NewPoint.Position = Source;
	NewPoint.InputKey = 0;
	SplineComp->AddPoint(NewPoint);

	//FVector Previous = Source;

	//Print path
	for (int c = Path.Num() - 1; c >= 0; c--)
	{
		//UE_LOG(LogTemp, Display, TEXT("Path Node: (%f, %f, %f)"), Path[c].X, Path[c].Y, Path[c].Z);
		//DrawDebugLine(GetWorld(), Previous, Path[c], FColor::Magenta, true);
		NewPoint.Position = Path[c];
		NewPoint.InputKey = Path.Num() - c;
		SplineComp->AddPoint(NewPoint);
		//Previous = Path[c];
	}
	//UE_LOG(LogTemp, Display, TEXT("Destination: (%f, %f, %f)"), Destination.X, Destination.Y, Destination.Z);
	//DrawDebugLine(GetWorld(), Previous, Destination, FColor::Magenta, true);
	NewPoint.Position = Destination;

	NewPoint.InputKey = Path.Num() + 1;
	SplineComp->AddPoint(NewPoint);

	if (!SplineMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Spline Mesh is null in AQuantizer::DrawPath"));
		return;
	}

	// Source: https://www.youtube.com/watch?v=iD3l44uMd58
	for (int SplineIndex = 0; SplineIndex < SplineComp->GetNumberOfSplinePoints() - 1; SplineIndex++)
	{
		//Create mesh, register with world and attach to spline component
		USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass());
		SplineMeshComponent->SetMobility(EComponentMobility::Movable);
		SplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
		SplineMeshComponent->AttachToComponent(SplineComp, FAttachmentTransformRules::KeepRelativeTransform);

		//Get start and end points
		const FVector StartPoint = SplineComp->GetLocationAtSplinePoint(SplineIndex, ESplineCoordinateSpace::Local);
		const FVector StartTangent = SplineComp->GetTangentAtSplinePoint(SplineIndex, ESplineCoordinateSpace::Local);
		const FVector EndPoint = SplineComp->GetLocationAtSplinePoint(SplineIndex + 1, ESplineCoordinateSpace::Local);
		const FVector EndTangent = SplineComp->GetTangentAtSplinePoint(SplineIndex + 1, ESplineCoordinateSpace::Local);

		//Set position and tangent data
		SplineMeshComponent->SetStartAndEnd(StartPoint, StartTangent, EndPoint, EndTangent, true);

		SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}