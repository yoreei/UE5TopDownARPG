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
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "Internationalization/Text.h"
#include <map>
#include <queue>
#include <climits>

const int CELL_SIZE = 100;
const int COST_TRACE_Y_START = 300;
const int COST_TRACE_Y_END = 100;

struct IntegrationField {
	float IntegratedCost = FLT_MAX;
	uint8_t ActiveWaveFront : 4;
	uint8_t LOS : 4;
};

enum class EDirection : uint8_t {
	Unset = 0,
	North,
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

void DebugDraw(UWorld* World, const FIntVector2& GridSize, const TArray<uint8_t>& CostFields)
{
	FVector RayStart;
	FVector RayEnd;
	for (int i = 0; i < GridSize.Y; ++i)
	{
		for (int j = 0; j < GridSize.X; ++j)
		{
			RayStart = { i * 100.f + 50, j * 100.f + 50, COST_TRACE_Y_START };
			RayEnd = { i * 100.f + 50, j * 100.f + 50, COST_TRACE_Y_END };
			UE_LOG(LogUE5TopDownARPG, Log, TEXT("CostFields [%d][%d] = [%d]"),i, j, CostFields[i * GridSize.X + j]);
			DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.0f, CostFields[i * GridSize.X + j] == UINT8_MAX ? FColor::Red : FColor::Green, true, -1.f, 0, 2.f);
		}
	}
}

void DebugDraw(UWorld* World, const FIntVector2& GridSize, const TArray<IntegrationField>& IntegrationFields)
{
	FlushPersistentDebugLines(World);
	FVector TextStart;
	for (int i = 0; i < GridSize.Y; ++i)
	{
		for (int j = 0; j < GridSize.X; ++j)
		{
			TextStart = { i * CELL_SIZE + CELL_SIZE/2.f, j * CELL_SIZE + CELL_SIZE/2.f, 60.f };
			auto IntegratedCost = FString::FromInt(IntegrationFields[i * GridSize.X + j].IntegratedCost);
			if (IntegrationFields[i * GridSize.X + j].IntegratedCost == FLT_MAX)
			{
				IntegratedCost = "MAX";
			}

			UE_LOG(LogUE5TopDownARPG, Log, TEXT("IntegrationFields [%d][%d] = [%s]"), i, j, *IntegratedCost);

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			auto TextActor = World->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90,0,180), SpawnParameters);
			TextActor->GetTextRender()->SetText(FText::FromString(IntegratedCost));
		}
	}
}

void DebugDraw(UWorld* World, const FIntVector2& GridSize, const TArray<FlowField>& FlowFields)
{
	FlushPersistentDebugLines(World);
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

	for (int i = 0; i < GridSize.Y; ++i)
	{
		for (int j = 0; j < GridSize.X; ++j)
		{
			if (FlowFields[i * GridSize.X + j].DirectionLookup == EDirection::Unset)
			{
				FVector DonutLoc{ i * 100.f + 50 , j * 100.f + 50 , 50.f };
				FMatrix TransformMatrix = FMatrix::Identity;
				TransformMatrix.SetOrigin(DonutLoc);
				DrawDebug2DDonut(World, TransformMatrix, 5.f, 10.f, 16, FColor::Blue, true);
			}
			else
			{
				offset = directionOffsets[FlowFields[i * GridSize.X + j].DirectionLookup];
				RayStart = { i * 100.f + 50 - offset.X, j * 100.f + 50 - offset.Y, 50.f };
				RayEnd = { i * 100.f + 50 + offset.X, j * 100.f + 50 + offset.Y, 50.f };
				DrawDebugDirectionalArrow(World, RayStart, RayEnd, 3.f, FColor::Green, true, 9999999.f, 0, 2.f);
			}

		}
	}
}

namespace Eikonal {
	const float SIGNIFICANT_COST_REDUCTION = 0.75f; // 25% reduction

	bool isValidCell(const TArray<uint8_t>& CostFields, const FIntVector2& GridSize, const FIntVector2& cell) { // TODO can we reuse?
		return cell.X >= 0 && cell.X < GridSize.X && cell.Y >= 0 && cell.Y < GridSize.Y && CostFields[cell.Y * GridSize.X + cell.X] != UINT8_MAX;
	}

	void updateCell(const TArray<uint8_t>& CostFields, TArray<IntegrationField>& IntegrationFields, const FIntVector2& GridSize, std::queue<FIntVector2>& WaveFront, const FIntVector2& cell, float currentCost) {
		if (isValidCell(CostFields, GridSize, cell)) {
			float newCost = currentCost + CostFields[cell.Y * GridSize.X + cell.X];
			float oldCost = IntegrationFields[cell.Y * GridSize.X + cell.X].IntegratedCost;

			if (newCost < oldCost * SIGNIFICANT_COST_REDUCTION) {
				IntegrationFields[cell.Y * GridSize.X + cell.X].IntegratedCost = newCost;
				WaveFront.push(cell);
			}
		}
	}

	void propagateWave(const TArray<uint8_t>& CostFields, TArray<IntegrationField>& IntegrationFields, const FIntVector2& GridSize, std::queue<FIntVector2>& WaveFront) {

		while (!WaveFront.empty()) {
			FIntVector2 current = WaveFront.front();
			WaveFront.pop();

			float currentCost = IntegrationFields[current.Y * GridSize.X + current.X].IntegratedCost;

			// Check adjacent cells: up, down, left, right
			updateCell(CostFields, IntegrationFields, GridSize, WaveFront, FIntVector2(current.X + 1, current.Y), currentCost);
			updateCell(CostFields, IntegrationFields, GridSize, WaveFront, FIntVector2(current.X - 1, current.Y), currentCost);
			updateCell(CostFields, IntegrationFields, GridSize, WaveFront, FIntVector2(current.X, current.Y + 1), currentCost);
			updateCell(CostFields, IntegrationFields, GridSize, WaveFront, FIntVector2(current.X, current.Y - 1), currentCost);
		}
	}
}

/*
Direction is normalized
*/
void BresenhamsRay2D(int MapHeight, int MapWidth, const TArray<uint8_t>& CostFields, const FVector2D& Direction, const FIntVector2& Pos, TArray<IntegrationField>& IntegrationFields)
{   // TODO Test
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
	// TODO Implement
}

bool IsValidIdx(const FIntVector2& GridSize, int Idx)
{
	return Idx >= 0 && Idx < GridSize.X * GridSize.Y;
}

void CalculateFlowFields(const FIntVector2& GridSize, TArray<IntegrationField>& IntegrationFields, OUT std::queue<int>& Sources, OUT TArray<FlowField>& FlowFields)
{
	auto UpdateIfBetter = [&GridSize, &IntegrationFields](const int NextIdx, const EDirection NextDir, OUT float& BestCost, OUT EDirection& BestDir) {
		if (NextIdx >= 0 && NextIdx < GridSize.X * GridSize.Y)
		{
			if (BestCost > IntegrationFields[NextIdx].IntegratedCost)
			{
				BestDir = NextDir;
				BestCost = IntegrationFields[NextIdx].IntegratedCost;
			}
		}
	};

	while (Sources.empty() == false)
	{
		const int CurIdx = Sources.front();
		Sources.pop();
		check(IsValidIdx(GridSize, CurIdx)); // TODO optimize: executed twice for each Idx

		if (FlowFields[CurIdx].DirectionLookup != EDirection::Unset)
		{
			continue;
		}

		float BestCost = IntegrationFields[CurIdx].IntegratedCost;
		EDirection BestDir = EDirection::Unset;
		const int NorthIdx = CurIdx + GridSize.X; // TODO optimize
		const int EastIdx = CurIdx + 1;
		const int SouthIdx = CurIdx - GridSize.X;
		const int WestIdx = CurIdx - 1;

		UpdateIfBetter(NorthIdx, EDirection::North, OUT BestCost, OUT BestDir);
		UpdateIfBetter(EastIdx, EDirection::East, OUT BestCost, OUT BestDir);
		UpdateIfBetter(SouthIdx, EDirection::South, OUT BestCost, OUT BestDir);
		UpdateIfBetter(WestIdx, EDirection::West, OUT BestCost, OUT BestDir);

		FlowFields[CurIdx].DirectionLookup = BestDir;
		// TODO diagonal
	}

}

bool CalculateCostFields(UWorld* World, const FIntVector2& GridSize, OUT TArray<uint8_t>& CostFields)
{
	ensure(World);

	FVector HitLocation;
	FVector RayStart;
	FVector RayEnd;
	FHitResult OutHit;
	for (int i = 0; i < GridSize.Y; ++i)
	{
		for (int j = 0; j < GridSize.X; ++j)
		{
			RayStart = FVector(i * CELL_SIZE + CELL_SIZE/2, j * CELL_SIZE + CELL_SIZE/2, COST_TRACE_Y_START);
			RayEnd = FVector(i * CELL_SIZE + CELL_SIZE/2, j * CELL_SIZE + CELL_SIZE/2, COST_TRACE_Y_END);
			FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), true);
			bool bLineTraceObstructed = World->LineTraceSingleByChannel(OutHit, RayStart, RayEnd, ECollisionChannel::ECC_WorldStatic /* , = FCollisionQueryParams::DefaultQueryParam, = FCollisionResponseParams::DefaultResponseParam */);

			CostFields[i * GridSize.X + j] = bLineTraceObstructed ? UINT8_MAX : 1;
		}
	}
	return true;
}

void DoFlowTiles(UWorld* World)
{
	ensure(World);

	FIntVector2 GridSize{ 30, 30 };
	TArray<uint8_t> CostFields;
	CostFields.Init(1, GridSize.X * GridSize.Y);
	CalculateCostFields(World, GridSize, CostFields);

//	DebugDraw(World, GridSize, CostFields);

	std::queue<FIntVector2> WaveFront;
	FIntVector2 Goal{ 27, 27 };
	WaveFront.push(Goal);
	TArray<IntegrationField> IntegrationFields;
	IntegrationFields.Init(IntegrationField(), GridSize.X * GridSize.Y);
	// runLos()
	// remove after runLos() is implemented
	IntegrationFields[Goal.Y * GridSize.X + Goal.X].IntegratedCost = 0;

	Eikonal::propagateWave(CostFields, IntegrationFields, GridSize, WaveFront);

	DebugDraw(World, GridSize, IntegrationFields);

	TArray<FlowField> FlowFields;
	FlowFields.Init({ EDirection::Unset, 0, 0 }, GridSize.X * GridSize.Y); // TODO optimize: can we omit constructing these?
	std::queue<int> Sources; // TODO optimize
	Sources.push(2 * GridSize.X + 2);
	Sources.push(27 * GridSize.X + 2);
	Sources.push(2 * GridSize.X + 27);
	Sources.push(10 * GridSize.X + 20);
	CalculateFlowFields(GridSize, IntegrationFields, Sources, FlowFields);

	DebugDraw(World, GridSize, FlowFields);
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
	ensure(World);

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
