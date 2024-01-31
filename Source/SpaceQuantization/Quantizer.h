// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Quantizer.generated.h"

USTRUCT(BlueprintType)
struct FQuantizedSpace
{
	GENERATED_BODY()

	FIntVector CellIndex;
};

UCLASS()
class SPACEQUANTIZATION_API AQuantizer : public AActor
{
	GENERATED_BODY()
	
public:	

	//How many units large each cell is
	UPROPERTY(EditDefaultsOnly)
	int Resolution = 100;

	//UPROPERTY(BlueprintReadWrite);
	//TMap<QuantizedSpace, FVector> Grid;

	// Sets default values for this actor's properties
	AQuantizer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	FQuantizedSpace Quantize(FVector Location);
	
};
