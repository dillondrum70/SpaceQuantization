// Fill out your copyright notice in the Description page of Project Settings.


#include "Quantizer.h"

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
	
}

// Called every frame
void AQuantizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


FQuantizedSpace AQuantizer::Quantize(FVector Location)
{
	FQuantizedSpace Result;

	Result.CellIndex.X = Location.X / Resolution;
	Result.CellIndex.Y = Location.Y / Resolution;
	Result.CellIndex.Z = Location.Z / Resolution;

	return Result;
}