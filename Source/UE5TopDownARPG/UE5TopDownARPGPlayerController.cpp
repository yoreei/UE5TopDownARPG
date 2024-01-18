// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5TopDownARPGPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "UE5TopDownARPGCharacter.h"
#include "UE5TopDownARPGHUD.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "UE5TopDownARPG.h"
#include "NavigationSystem.h"
#include <map>

struct IntegrationField {
	int MapWidth;
	int MapHeight;

	float IntegratedCost;
	uint8_t ActiveWaveFront : 4;
	uint8_t LOS : 4;


};

enum class EDirection : uint8_t {
	North = 0,
	NorthEast,
	East,
	SouthEast,
	South,
	SouthWest,
	West,
	NorthWest
};

struct FlowField {
	EDirection DirectionLookup : 4;
	uint8_t Pathable : 2;
	uint8_t LOS : 2;
};

void DebugDraw(UWorld* World, int MapWidth, int MapHeight, const TArray<uint8_t>& CostFields)
{
	FVector RayStart;
	FVector RayEnd;
	for (int i = 0; i <= MapHeight; ++i)
	{
		for (int j = 0; j <= MapWidth; ++j)
		{
			RayStart = { i * 100.f + 50, j * 100.f + 50, 300.f };
			RayEnd = { i * 100.f + 50, j * 100.f + 50, 100.f };
			DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.0f, CostFields[i * MapWidth + j] == 255 ? FColor::Red : FColor::Green, true, 9999999.f, 0, 2.f);
		}
	}
}

void DebugDraw(UWorld* World, int MapWidth, int MapHeight, const TArray<IntegrationField> IntegrationFields)
{
	FVector RayStart;
	FVector RayEnd;
	for (int i = 0; i <= MapHeight; ++i)
	{
		for (int j = 0; j <= MapWidth; ++j)
		{
			RayStart = { i * 100.f + 50, j * 100.f + 50, 300.f };
			RayEnd = { i * 100.f + 50, j * 100.f + 50, 100.f };
			DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.0f, IntegrationFields[i * MapWidth + j].LOS == false ? FColor::Red : FColor::Green, true, 9999999.f, 0, 2.f);
		}
	}
}

void DebugDraw(UWorld* World, int MapWidth, int MapHeight, const TArray<FlowField>& FlowFields)
{
	FVector RayStart;
	FVector RayEnd;

	std::map<EDirection, FVector2D> directionOffsets =
	{
		{EDirection::North, FVector2D(0, 25)},
		{EDirection::NorthEast, FVector2D(25, 25)},
		{EDirection::East, FVector2D(25, 0)},
		{EDirection::SouthEast, FVector2D(25, -25)},
		{EDirection::South, FVector2D(0, -25)},
		{EDirection::SouthWest, FVector2D(-25, -25)},
		{EDirection::West, FVector2D(-25, 0)},
		{EDirection::NorthWest, FVector2D(-25, -25)},
	};
	FVector2D offset;

	for (int i = 0; i <= MapHeight; ++i)
	{
		for (int j = 0; j <= MapWidth; ++j)
		{
			offset = directionOffsets[FlowFields[i * MapWidth + j].DirectionLookup];
			RayStart = { i * 100.f + 50 - offset.X * 25, j * 100.f + 50 - offset.Y * 25, 50.f };
			RayEnd = { i * 100.f + 50 + offset.X * 25, j * 100.f + 50 + offset.Y * 25, 50.f };

			DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.f, FColor::Green, true, 9999999.f, 0, 2.f);
		}
	}
}

/*
Direction is normalized
*/
void BresenhamsRay2D(int MapHeight, int MapWidth, const TArray<uint8_t>& CostFields, const FVector2D& Direction, int PosX, int PosY, TArray<IntegrationField>& IntegrationFields)
{
	int x = PosX;
	int y = PosY;
	int endX = std::floor(PosX + Direction.X * MapWidth);
	int endY = std::floor(PosY + Direction.Y * MapHeight);

	int dx = std::abs(endX - x);
	int dy = -std::abs(endY - y);
	int sx = x < endX ? 1 : -1;
	int sy = y < endY ? 1 : -1;
	int err = dx + dy, e2;

	while (true) {
		if (x >= 0 && x < MapWidth && y >= 0 && y < MapHeight) {
			if (CostFields[y * MapWidth + x] == 255) {
				IntegrationFields[y * MapWidth + x].LOS = false;
				break;
			}
			else {
				IntegrationFields[y * MapWidth + x].LOS = true;
			}
		}
		else {
			// Break if we are out of bounds
			break;
		}

		if (x == endX && y == endY) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x += sx; }
		if (e2 <= dx) { err += dx; y += sy; }
	}
}

void LineOfSightPass(int MapHeight, int MapWidth, const FVector& Goal, OUT TArray<IntegrationField>& IntegrationFields)
{

}

void CalculateIntegrationFields(int MapHeight, int MapWidth, const FVector& Goal, OUT TArray<IntegrationField>& IntegrationFields)
{
	LineOfSightPass();

}

void CalculateFlowFields(int MapHeight, int MapWidth, const FVector& EndPos, OUT TArray<FlowField> FlowFields)
{


}

bool CalculateCostFields(UWorld* World, int MapHeight, int MapWidth, OUT TArray<uint8_t>& CostFields)
{
	if (IsValid(World))
	{
		return false;
	}

	FVector HitLocation;
	FVector RayStart;
	FVector RayEnd;
	FHitResult OutHit;
	for (int i = 0; i <= MapHeight; ++i)
	{
		for (int j = 0; j <= MapWidth; ++j)
		{
			RayStart = FVector(i * 100 + 50, j * 100 + 50, 300);
			RayEnd = FVector(i * 100 + 50, j * 100 + 50, 100);
			FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), true);
			bool bLineTraceObstructed = World->LineTraceSingleByChannel(OutHit, RayStart, RayEnd, ECollisionChannel::ECC_WorldStatic /* , = FCollisionQueryParams::DefaultQueryParam, = FCollisionResponseParams::DefaultResponseParam */);

			CostFields[i * MapHeight + j] = bLineTraceObstructed ? 1 : 255;
			UE_LOG(LogUE5TopDownARPG, Log, TEXT("Raycast at %s, %s: %d"), *(RayStart.ToString()), *(RayEnd.ToString()), (int)bLineTraceObstructed);
			DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.0f, bLineTraceObstructed ? FColor::Red : FColor::Green, true, 9999999.f, 0, 2.f);
		}
	}
	return true;
}

void DoFlowTiles(UWorld* World)
{
	if (IsValid(World) == false)
	{
		return;
	}

	int MapHeight = 30;
	int MapWidth = 30;
	TArray<uint8_t> CostFields;
	CostFields.Init(1, MapHeight * MapWidth);
	CalculateCostFields(World, MapHeight, MapWidth, CostFields);

	FVector Goal{ 27.f, 27.f, 1.f };
	TArray<IntegrationField> IntegrationFields;
	IntegrationFields.Init({ 1, 0, 0 }, MapHeight * MapWidth);
	CalculateIntegrationFields(MapHeight, MapWidth, Goal, IntegrationFields);

	TArray<FlowField> FlowFields;
	FlowFields.Init({ EDirection::North, 0, 0 }, MapHeight * MapWidth);
	CalculateFlowFields(MapHeight, MapWidth, Goal, FlowFields);
}

// TESTS

bool Test_BresenhamsRay2D(UWorld* World)
{
	return false;
}

void MainFlowTiles(UWorld* World)
{
	// DoFlowTiles(World);
	Test_BresenhamsRay2D(World);
}


AUE5TopDownARPGPlayerController::AUE5TopDownARPGPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}

void AUE5TopDownARPGPlayerController::OnPlayerDied()
{
	AUE5TopDownARPGHUD* HUD = Cast<AUE5TopDownARPGHUD>(GetHUD());
	if (IsValid(HUD))
	{
		HUD->ShowEndGameScreen();
	}
}

void AUE5TopDownARPGPlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	MainFlowTiles(World);
}

void AUE5TopDownARPGPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &AUE5TopDownARPGPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &AUE5TopDownARPGPlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &AUE5TopDownARPGPlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &AUE5TopDownARPGPlayerController::OnSetDestinationReleased);

		EnhancedInputComponent->BindAction(ActivateAbilityAction, ETriggerEvent::Started, this, &AUE5TopDownARPGPlayerController::OnActivateAbilityStarted);

		// Setup touch input events
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &AUE5TopDownARPGPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &AUE5TopDownARPGPlayerController::OnTouchTriggered);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &AUE5TopDownARPGPlayerController::OnTouchReleased);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &AUE5TopDownARPGPlayerController::OnTouchReleased);
	}
}

void AUE5TopDownARPGPlayerController::OnInputStarted()
{
	StopMovement();
}

// Triggered every frame when the input is held down
void AUE5TopDownARPGPlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();
	
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	if (bIsTouch)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else
	{
		bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
	
	// Move towards mouse pointer or touch
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void AUE5TopDownARPGPlayerController::OnSetDestinationReleased()
{
	// If it was a short press
	if (FollowTime <= ShortPressThreshold)
	{
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

// Triggered every frame when the input is held down
void AUE5TopDownARPGPlayerController::OnTouchTriggered()
{
	bIsTouch = true;
	OnSetDestinationTriggered();
}

void AUE5TopDownARPGPlayerController::OnTouchReleased()
{
	bIsTouch = false;
	OnSetDestinationReleased();
}

void AUE5TopDownARPGPlayerController::OnActivateAbilityStarted()
{
	UE_LOG(LogUE5TopDownARPG, Log, TEXT("OnActivateAbilityStarted"));

	AUE5TopDownARPGCharacter* ARPGCharacter = Cast<AUE5TopDownARPGCharacter>(GetPawn());
	if (IsValid(ARPGCharacter))
	{
		FHitResult Hit;
		bool bHitSuccessful = false;
		if (bIsTouch)
		{
			bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
		}
		else
		{
			bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
		}

		// If we hit a surface, cache the location
		if (bHitSuccessful)
		{
			ARPGCharacter->ActivateAbility(Hit.Location);
		}
	}
}
