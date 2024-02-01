// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Quantizer.generated.h"

USTRUCT(BlueprintType)
struct FQuantizedSpace
{
	GENERATED_BODY()

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

	FIntVector2 Dimensions;

	UPROPERTY(EditAnywhere)
	AActor* LandscapeActor;

	float SampleMaxHeight = 10000;

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

	UFUNCTION(BlueprintCallable)
	FQuantizedSpace Quantize(FVector Location);
	
};
