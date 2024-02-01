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

ASpaceQuantizationPlayerController::ASpaceQuantizationPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
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


		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &ASpaceQuantizationPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &ASpaceQuantizationPlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &ASpaceQuantizationPlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &ASpaceQuantizationPlayerController::OnSetDestinationReleased);
	}
}

void ASpaceQuantizationPlayerController::OnInputStarted()
{
	
}

// Triggered every frame when the input is held down
void ASpaceQuantizationPlayerController::OnSetDestinationTriggered()
{
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
}

void ASpaceQuantizationPlayerController::OnSetDestinationReleased()
{
	
}

void ASpaceQuantizationPlayerController::OnSetSourceTriggered()
{

}


void ASpaceQuantizationPlayerController::OnSetSourceReleased()
{

}
