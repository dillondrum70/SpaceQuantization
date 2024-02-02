// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Quantizer.generated.h"

USTRUCT(BlueprintType)
struct FGridMask
{
	GENERATED_BODY()

	FGridMask();

	UPROPERTY(EditAnywhere)
	TArray<FIntVector2> MaskPoints;
};

USTRUCT(BlueprintType)
struct FQuantizedSpace
{
	GENERATED_BODY()

	FQuantizedSpace();

	FIntVector2 Location;	//2D location in the XY axes where this point lies, integer to avoid floating point errors and use as map key

	float Height;	//Height in Z axis of this point
};

UCLASS()
class SPACEQUANTIZATION_API AQuantizer : public AActor
{
	GENERATED_BODY()
	
public:	

	//How many units large each cell is
	UPROPERTY(EditAnywhere)
	int Resolution = 1000;

	/*
	*	Weights of different types of costs in A* calculation
	*/
	UPROPERTY(EditAnywhere)
	float LengthCostWeight = 1;
	UPROPERTY(EditAnywhere)
	float AngleCostWeight = 1;

	//Max angle of path
	UPROPERTY(EditAnywhere)
	float MaxAngleThreshold = 15.f;

	//Dimensions of the discretized grid
	FIntVector2 GridDimensions;

	//Dimensions of the array in Unreal units
	FVector2D LandscapeDimensions;

	UPROPERTY(EditAnywhere)
	AActor* LandscapeActor;

	UPROPERTY(EditAnywhere)
	float SampleMaxHeight = 10000;	//Max height of terrain above 0

	UPROPERTY(EditAnywhere)
	float SampleMaxDepth = 5000;	//Max depth of terrain below 0

	UPROPERTY(EditAnywhere)
	FGridMask SampleMask;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	AActor* SourceMarker;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	AActor* DestinationMarker;

	//Store these at the beginning of the algorithm for use later
	FQuantizedSpace QuantizedSource;
	FQuantizedSpace QuantizedDestination;

	// Sets default values for this actor's properties
	AQuantizer();

protected:

	TMap<FIntVector2, FQuantizedSpace> CachedHeightmap;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void GenerateHeightmap();

	bool SampleTerrainHeight(FIntVector StartLocation, FQuantizedSpace& OutResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/// <summary>
	/// Evaluates a given point in terms of heightmap grid points, rounds to the point that corresponds to the cell the Location resides within
	/// </summary>
	/// <param name="Location"></param>
	/// <returns></returns>
	UFUNCTION(BlueprintCallable)
	FQuantizedSpace Quantize(FVector Location);

	/// <summary>
	/// Compute the path between source and destination vectors
	/// </summary>
	/// <param name="Source"></param>
	/// <param name="Destination"></param>
	/// <returns>Success</returns>
	UFUNCTION(BlueprintCallable)
	bool ComputePath(FVector Source, FVector Destination);

	/// <summary>
	/// Get the cost of moving to a given cell
	/// </summary>
	/// <param name="Current"></param>
	/// <param name="Next"></param>
	/// <returns></returns>
	float CostFunction(FIntVector2 Current, FIntVector2 Next) const;

	/// <summary>
	/// Get the distance between current cell and goal, uses euclidean distance (as the crow flies)
	/// </summary>
	/// <param name="Current"></param>
	/// <returns></returns>
	float GoalFunction(FIntVector2 Current) const;

	/// <summary>
	/// Whether or not the passed grid point is in range
	/// </summary>
	/// <param name="GridPoint"></param>
	/// <returns></returns>
	bool IsGridPointValid(FIntVector2 GridPoint) const;
	
	/// <summary>
	/// Return the lowest cost next grid point between all the adjacent points in the GridMask
	/// </summary>
	/// <param name="GridMask"></param>
	/// <param name="CurrentLocation"></param>
	/// <returns></returns>
	FQuantizedSpace FindLowestCost(const FGridMask& GridMask, FIntVector2 CurrentLocation);
};
