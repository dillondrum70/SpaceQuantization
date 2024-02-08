// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Components/SplineMeshComponent.h"

#include "Quantizer.generated.h"

class USplineComponent;
class UStaticMesh;

/// <summary>
/// Nodes used in A* algorithm
/// </summary>
USTRUCT()
struct FAStarNode
{
	GENERATED_BODY()

	float Cost;	//Cost heuristic
	float DistanceFromStart;	//Distance (euclidean) from start node
	FIntVector2 Location;

	FIntVector2 Parent;

	FAStarNode()
		: Cost(0), DistanceFromStart(0), Location(0, 0)
	{}

	FAStarNode(float _Cost, float _DistanceFromStart, FIntVector2 _Location, FIntVector2 _Parent)
	: Cost(_Cost), DistanceFromStart(_DistanceFromStart), Location(_Location), Parent(_Parent)
	{}

	FAStarNode(const FAStarNode& Other)
		: Cost(Other.Cost), DistanceFromStart(Other.DistanceFromStart), Location(Other.Location), Parent(Other.Parent)
	{}

	bool operator==(const FAStarNode& Other) const
	{
		return Equals(Other);
	}

	bool operator<(const FAStarNode& Other) const
	{
		return Cost < Other.Cost;
	}

	bool Equals(const FAStarNode& Other) const
	{
		return Location == Other.Location;
	}

	static bool CompareLess(const FAStarNode& l, const FAStarNode& r) 
	{ 
		return l.Cost > r.Cost; 
	}
};

/// <summary>
/// Allows FAStarNode to be used as a key in a TMap
/// </summary>
/// <param name="Thing"></param>
/// <returns></returns>
FORCEINLINE uint32 GetTypeHash(const FAStarNode& Thing)
{
	uint32 Hash = FCrc::MemCrc32(&Thing, sizeof(FAStarNode));
	return Hash;
}

/// <summary>
/// A list of points that defines a mask of points whose heights are sampled relative to a center point
/// </summary>
USTRUCT(BlueprintType)
struct FGridMask
{
	GENERATED_BODY()

	FGridMask();

	UPROPERTY(EditAnywhere)
	TArray<FIntVector2> MaskPoints;
};

/// <summary>
/// A point on the terrain that has been discretized
/// </summary>
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

	//A* Data Structures
	TArray<FAStarNode> Frontier;	//Unexplored nodes
	TMap<FIntVector2, FIntVector2> Parents;	//Map of parent nodes, key = child, value = parent
	TArray<FAStarNode> Closed;		//Explored nodes

	//Finished path
	TArray<FVector> Path;

	//Cached source and destination points
	FVector Source;
	FVector Destination;

	//How many units large each cell is
	UPROPERTY(EditAnywhere)
	int Resolution = 1000;

	//Weights of different types of costs in A* calculation
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

	//The terrain we are sampling from
	UPROPERTY(EditAnywhere)
	AActor* LandscapeActor;

	UPROPERTY(EditAnywhere)
	float SampleMaxHeight = 10000;	//Max height of terrain above 0

	UPROPERTY(EditAnywhere)
	float SampleMaxDepth = 5000;	//Max depth of terrain below 0

	//Mask used for sampling points
	UPROPERTY(EditAnywhere)
	FGridMask SampleMask;

	//Actors that show the positions of the source and destination 
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	AActor* SourceMarker;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	AActor* DestinationMarker;

	//Store these at the beginning of the algorithm for use later
	FQuantizedSpace QuantizedSource;
	FQuantizedSpace QuantizedDestination;

	//Used to visualize path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	USplineComponent* SplineComp;

	//Mesh used for path
	UPROPERTY(EditAnywhere, Category = "Spline")
	UStaticMesh* SplineMesh;

	//Material used for SplineMesh
	UPROPERTY(EditAnywhere, Category = "Spline")
	class UMaterialInterface* SplineMat;

	//Forward axis of mesh
	UPROPERTY(EditDefaultsOnly, Category = "Spline")
	TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis;

	// Sets default values for this actor's properties
	AQuantizer();

protected:

	TMap<FIntVector2, FQuantizedSpace> CachedHeightmap;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/// <summary>
	/// Normally run once in BeginPlay, samples heights and populates CachedHeightmap
	/// </summary>
	void GenerateHeightmap();

	/// <summary>
	/// Sample terrain at location and store quantized result in OutResult
	/// </summary>
	/// <param name="StartLocation"></param>
	/// <param name="OutResult"></param>
	/// <returns></returns>
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
	/// Pops the lowest cost node off the Frontier
	/// </summary>
	/// <returns></returns>
	FAStarNode PopLowestCostNode();

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
	float CostFunction(const FAStarNode& Current, FAStarNode& Next) const;

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
	/// Use grid mask to generate A* successors
	/// </summary>
	/// <param name="GridMask"></param>
	/// <param name="Current"></param>
	void GenerateSuccessors(const FGridMask& GridMask, const FAStarNode& Current, const FQuantizedSpace& Goal);

	/// <summary>
	/// Trace back path from the current node to the start
	/// </summary>
	/// <param name="LastNode"></param>
	/// <returns></returns>
	void TraceBackPath(const FAStarNode& LastNode, TArray<FVector>& Path);

	/// <summary>
	/// Draws path with spline
	/// </summary>
	void DrawPath();
};
