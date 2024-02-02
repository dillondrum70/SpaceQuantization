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


bool AQuantizer::ComputePath(FVector Source, FVector Destination)
{
	FQuantizedSpace q = Quantize(Source);
	DrawDebugSphere(GetWorld(), FVector(q.Location.X, q.Location.Y, q.Height), 50, 12, FColor::Cyan, true);
	q = Quantize(Destination);
	DrawDebugSphere(GetWorld(), FVector(q.Location.X, q.Location.Y, q.Height), 50, 12, FColor::Emerald, true);
	return false;
}