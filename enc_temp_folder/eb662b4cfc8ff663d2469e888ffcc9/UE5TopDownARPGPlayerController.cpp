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
#include <queue>
#include <climits>

const int CELL_SIZE = 100;

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

void DebugDraw(UWorld* World, const FIntVector2& gridSize, const TArray<uint8_t>& CostFields)
{
	FVector RayStart;
	FVector RayEnd;
	for (int i = 0; i < gridSize.Y; ++i)
	{
		for (int j = 0; j < gridSize.X; ++j)
		{
			RayStart = { i * 100.f + 50, j * 100.f + 50, 300.f };
			RayEnd = { i * 100.f + 50, j * 100.f + 50, -300.f };
			UE_LOG(LogUE5TopDownARPG, Log, TEXT("CostFields [%d][%d] = [%d]"),i, j, CostFields[i * gridSize.X + j]);
			DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.0f, CostFields[i * gridSize.X + j] == 255 ? FColor::Red : FColor::Green, true, 9999999.f, 0, 2.f);
		}
	}
}

void DebugDraw(UWorld* World, const FIntVector2& gridSize, const TArray<IntegrationField>& IntegrationFields)
{
	FVector RayStart;
	FVector RayEnd;
	for (int i = 0; i < gridSize.Y; ++i)
	{
		for (int j = 0; j < gridSize.X; ++j)
		{
			RayStart = { i * 100.f + 50, j * 100.f + 50, 300.f };
			RayEnd = { i * 100.f + 50, j * 100.f + 50, 100.f };
			UE_LOG(LogUE5TopDownARPG, Log, TEXT("IntegrationFields [%d][%d] = [%d]"), i, j, (int)IntegrationFields[i * gridSize.X + j].LOS);
			DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.0f, IntegrationFields[i * gridSize.X + j].LOS == false ? FColor::Red : FColor::Green, true, 9999999.f, 0, 2.f);
		}
	}
}

void DebugDraw(UWorld* World, const FIntVector2& gridSize, const TArray<FlowField>& FlowFields)
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

	for (int i = 0; i < gridSize.Y; ++i)
	{
		for (int j = 0; j < gridSize.X; ++j)
		{
			offset = directionOffsets[FlowFields[i * gridSize.X + j].DirectionLookup];
			RayStart = { i * 100.f + 50 - offset.X * 25, j * 100.f + 50 - offset.Y * 25, 50.f };
			RayEnd = { i * 100.f + 50 + offset.X * 25, j * 100.f + 50 + offset.Y * 25, 50.f };

			DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.f, FColor::Green, true, 9999999.f, 0, 2.f);
		}
	}
}

namespace Eikonal {
	const float SIGNIFICANT_COST_REDUCTION = 0.75f; // 25% reduction

	bool isValidCell(const TArray<uint8_t>& CostFields, const FIntVector2& gridSize, const FIntVector2& cell) {
		return cell.X >= 0 && cell.X < gridSize.X && cell.Y >= 0 && cell.Y < gridSize.Y && CostFields[cell.Y * gridSize.X + cell.X] != UINT8_MAX;
	}

	void updateCell(const TArray<uint8_t>& CostFields, TArray<IntegrationField>& IntegrationFields, const FIntVector2& gridSize, std::queue<FIntVector2>& wavefront, const FIntVector2& cell, float currentCost) {
		if (isValidCell(CostFields, gridSize, cell)) {
			float newCost = currentCost + CostFields[cell.Y * gridSize.X + cell.X];
			float oldCost = IntegrationFields[cell.Y * gridSize.X + cell.X].IntegratedCost;

			if (newCost < oldCost * SIGNIFICANT_COST_REDUCTION) {
				IntegrationFields[cell.Y * gridSize.X + cell.X].IntegratedCost = newCost;
				wavefront.push(cell);
			}
		}
	}

	void propagateWave(const TArray<uint8_t>& CostFields, TArray<IntegrationField>& IntegrationFields, const FIntVector2& gridSize, std::queue<FIntVector2>& wavefront) {
		while (!wavefront.empty()) {
			FIntVector2 current = wavefront.front();
			wavefront.pop();

			float currentCost = IntegrationFields[current.Y * gridSize.X + current.X].IntegratedCost;

			// Check adjacent cells: up, down, left, right
			updateCell(CostFields, IntegrationFields, gridSize, wavefront, FIntVector2(current.X + 1, current.Y), currentCost);
			updateCell(CostFields, IntegrationFields, gridSize, wavefront, FIntVector2(current.X - 1, current.Y), currentCost);
			updateCell(CostFields, IntegrationFields, gridSize, wavefront, FIntVector2(current.X, current.Y + 1), currentCost);
			updateCell(CostFields, IntegrationFields, gridSize, wavefront, FIntVector2(current.X, current.Y - 1), currentCost);
		}
	}
}

/*
Direction is normalized
*/
void BresenhamsRay2D(int MapHeight, int MapWidth, const TArray<uint8_t>& CostFields, const FVector2D& Direction, const FIntVector2& Pos, TArray<IntegrationField>& IntegrationFields)
{
	int x = Pos.X;
	int y = Pos.Y;
	int endX = std::floor(Pos.X + Direction.X * MapWidth);
	int endY = std::floor(Pos.Y + Direction.Y * MapHeight);

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

void LineOfSightPass(int MapHeight, int MapWidth, const FIntVector2& Goal, OUT TArray<IntegrationField>& IntegrationFields)
{

}

void CalculateFlowFields(const FIntVector2& gridSize, const FIntVector2& EndPos, OUT TArray<FlowField> FlowFields)
{


}

bool CalculateCostFields(UWorld* World, const FIntVector2& gridSize, OUT TArray<uint8_t>& CostFields)
{
	if (IsValid(World))
	{
		return false;
	}

	FVector HitLocation;
	FVector RayStart;
	FVector RayEnd;
	FHitResult OutHit;
	for (int i = 0; i < gridSize.Y; ++i)
	{
		for (int j = 0; j < gridSize.X; ++j)
		{
			RayStart = FVector(i * CELL_SIZE + CELL_SIZE/2, j * CELL_SIZE + CELL_SIZE/2, 300.f);
			RayEnd = FVector(i * CELL_SIZE + CELL_SIZE/2, j * CELL_SIZE + CELL_SIZE/2, -300.f);
			FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), true);
			bool bLineTraceObstructed = World->LineTraceSingleByChannel(OutHit, RayStart, RayEnd, ECollisionChannel::ECC_WorldStatic /* , = FCollisionQueryParams::DefaultQueryParam, = FCollisionResponseParams::DefaultResponseParam */);

			CostFields[i * gridSize.X + j] = bLineTraceObstructed ? 1 : 255;
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

	FIntVector2 gridSize{ 30, 30 };
	TArray<uint8_t> CostFields;
	CostFields.Init(1, gridSize.X * gridSize.Y);
	CalculateCostFields(World, gridSize, CostFields);

	DebugDraw(World, gridSize, CostFields);

	std::queue<FIntVector2> wavefront;
	FIntVector2 Goal{ 27, 27 };
	wavefront.push(Goal);
	TArray<IntegrationField> IntegrationFields;
	IntegrationFields.Init({ 1, 0, 0 }, gridSize.X * gridSize.Y);
	Eikonal::propagateWave(CostFields, IntegrationFields, gridSize, wavefront);

	// DebugDraw(World, gridSize, IntegrationFields);

	TArray<FlowField> FlowFields;
	FlowFields.Init({ EDirection::North, 0, 0 }, gridSize.X * gridSize.Y);
	CalculateFlowFields(gridSize, Goal, FlowFields);

	// DebugDraw(World, gridSize, FlowFields);
}

// TESTS

bool Test_BresenhamsRay2D(UWorld* World)
{
	return false;
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

	DoFlowTiles(World);
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
