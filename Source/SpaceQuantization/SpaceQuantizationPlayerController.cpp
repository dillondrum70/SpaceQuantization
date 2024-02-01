// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceQuantizationPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "SpaceQuantizationCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Quantizer.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ASpaceQuantizationPlayerController::ASpaceQuantizationPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedSource = FVector::ZeroVector;
	CachedDestination = FVector::ZeroVector;
}

void ASpaceQuantizationPlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("World is null in ASpaceQuantizationPlayerController::BeginPlay"));
		return;
	}

	Quantizer = nullptr;
	Quantizer = StaticCast<AQuantizer*>(UGameplayStatics::GetActorOfClass(World, AQuantizer::StaticClass()));

	if (Quantizer == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Quantizer is null in ASpaceQuantizationPlayerController::BeginPlay"));
		return;
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Found Quantizer with name %s"), *Quantizer->GetName());
	}
}

void ASpaceQuantizationPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &ASpaceQuantizationPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &ASpaceQuantizationPlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &ASpaceQuantizationPlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &ASpaceQuantizationPlayerController::OnSetDestinationReleased);


		EnhancedInputComponent->BindAction(SetSourceClickAction, ETriggerEvent::Started, this, &ASpaceQuantizationPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetSourceClickAction, ETriggerEvent::Triggered, this, &ASpaceQuantizationPlayerController::OnSetSourceTriggered);
		EnhancedInputComponent->BindAction(SetSourceClickAction, ETriggerEvent::Completed, this, &ASpaceQuantizationPlayerController::OnSetSourceReleased);
		EnhancedInputComponent->BindAction(SetSourceClickAction, ETriggerEvent::Canceled, this, &ASpaceQuantizationPlayerController::OnSetSourceReleased);
	}
}

void ASpaceQuantizationPlayerController::OnInputStarted()
{
	
}

// Triggered every frame when the input is held down
void ASpaceQuantizationPlayerController::OnSetDestinationTriggered()
{
	//Cache position
	bool bHitSuccessful = GetPositionUnderCursor(CachedDestination);

	//Log if failed
	if (!bHitSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cursor hit nothing for destination"));
		return;
	}
	
	//Draw sphere at point
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not get world in ASpaceQuantizationPlayerController::OnSetDestinationTriggered"));
		return;
	}

	DrawDebugSphere(World, CachedDestination, 50, 12, FColor::Red, true);
}

void ASpaceQuantizationPlayerController::OnSetDestinationReleased()
{
	
}

void ASpaceQuantizationPlayerController::OnSetSourceTriggered()
{
	//Cache position
	bool bHitSuccessful = GetPositionUnderCursor(CachedSource);

	//Log if failed
	if (!bHitSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cursor hit nothing for source"));
		return;
	}

	//Draw sphere at point
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not get world in ASpaceQuantizationPlayerController::OnSetSourceTriggered"));
		return;
	}

	DrawDebugSphere(World, CachedSource, 50, 12, FColor::Green, true);
}


void ASpaceQuantizationPlayerController::OnSetSourceReleased()
{

}


bool ASpaceQuantizationPlayerController::GetPositionUnderCursor(FVector& OutPosition)
{
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		OutPosition = Hit.Location;
	}

	return bHitSuccessful;
}


bool ASpaceQuantizationPlayerController::ComputePath()
{
	if (Quantizer == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Quantizer is null in ASpaceQuantizationPlayerController::ComputePath"));
		return false;
	}

	Quantizer->ComputePath(CachedSource, CachedDestination);

	return true;
}
