// Fill out your copyright notice in the Description page of Project Settings.


#include "Quantizer.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"

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

	UE_LOG(LogTemp, Display, TEXT("Landscape Dimensions: (%f, %f) \nGrid Dimensions: (%i, %i)"), 
		LandscapeDimensions.X, LandscapeDimensions.Y, GridDimensions.X, GridDimensions.Y);

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

	Location.X = ((int)Location.X / Resolution) * Resolution;	//Rounds location to the location of the grid point at the corner of this cell
	Location.Y = ((int)Location.Y / Resolution) * Resolution;	//Rounds location to the location of the grid point at the corner of this cell

	FIntVector2 Coord = FIntVector2(Location.X, Location.Y);

	Result = CachedHeightmap[Coord];

	return Result;
}


FAStarNode AQuantizer::PopLowestCostNode()
{
	FAStarNode element = Frontier[0];

	for (int i = 1; i < Frontier.Num(); i++)
	{
		if (Frontier[i].Cost < element.Cost)
		{
			element = Frontier[i];
		}
	}

	Frontier.Remove(element);

	return element;
}


bool AQuantizer::ComputePath(FVector Source, FVector Destination)
{
	//Quantize positions in terms of grid points
	QuantizedSource = Quantize(Source);
	QuantizedDestination = Quantize(Destination);

	FAStarNode StartNode(0, 0, QuantizedSource.Location);	//Starting node is on the source position, 0 cost

	//Unexplored spaces, clear frontier
	Frontier.Empty();

	Frontier.Add(StartNode);	//Initialize frontier with starting node

	//Keys lead to the parent node as a value of the key, clear Parents
	//Parents.Empty();
	//Nodes that have already been visited, clear Closed
	Closed.Empty();

	//Loop until goal is found
	while (!Frontier.IsEmpty())
	{
		FAStarNode CurrentNode = PopLowestCostNode();

		GenerateSuccessors(SampleMask, CurrentNode, QuantizedDestination);

		Closed.Add(CurrentNode);
	}

	///// TEST
	//Draw line between source and destination
	//UE_LOG(LogTemp, Display, TEXT("Cost: %f"), CostFunction(FIntVector2(QuantizedSource.Location.X, QuantizedSource.Location.Y), FIntVector2(QuantizedDestination.Location.X, QuantizedDestination.Location.Y)));
	//DrawDebugLine(GetWorld(), FVector(QuantizedSource.Location.X, QuantizedSource.Location.Y, QuantizedSource.Height),
	//	FVector(QuantizedDestination.Location.X, QuantizedDestination.Location.Y, QuantizedDestination.Height), FColor::Cyan, true);

	////Get lowest cost and draw samples
	//FQuantizedSpace New = FindLowestCost(SampleMask, QuantizedSource.Location);
	//DrawDebugSphere(GetWorld(), FVector(New.Location.X, New.Location.Y, New.Height), 25, 12, FColor::Magenta, true);

	return false;
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

	UE_LOG(LogTemp, Display, TEXT("Angle: %f"), angle);

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

			return;
		}

		//Get cost
		float CurrentCost = CostFunction(Current, NextNode);

		//Find node in open list if it exists
		int ExistingIndex = Frontier.Find(NextNode);

		//Ignore this node if one exists already with a lower cost
		if (ExistingIndex >= 0 && Frontier[ExistingIndex].Cost <= NextNode.Cost)
		{
			continue;
		}

		//Find node in open list if it exists
		ExistingIndex = Closed.Find(NextNode);

		//Ignore this node if one exists already with a lower cost
		if (ExistingIndex >= 0 && Closed[ExistingIndex].Cost <= NextNode.Cost)
		{
			continue;
		}

		Frontier.Add(NextNode);
		
		//Draw sample line between current and sample point
		/*DrawDebugLine(GetWorld(), FVector(Current.Location.X, Current.Location.Y, CachedHeightmap[Current.Location].Height),
			FVector(NextNode.Location.X, NextNode.Location.Y, CachedHeightmap[NextNode.Location].Height), FColor::White, true);*/
	}
}