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
#include "Kismet/GameplayStatics.h"
#include <map>
#include <queue>
#include <climits>
#include <cmath>

const float SQRT_OF_2 = FMath::Sqrt(2.f);
const float CELL_SIZE = 100.f;
const float H_CELL_SIZE = CELL_SIZE / 2;
const float Q_CELL_SIZE = CELL_SIZE / 4;
const float CELL_DIAG = CELL_SIZE * SQRT_OF_2;
const float H_CELL_DIAG = CELL_SIZE * SQRT_OF_2;
const float PLANE_HEIGHT = 51.f;

const float bDRAW_COSTS = false;
const float bDRAW_INTEGRATION = true;

const FVector COST_TRACE_Y_START = { 0.f, 0.f, 250.f };
const FVector COST_TRACE_DIRECTION {0.f, 0.f, -100};
const FIntVector2 GRIDSIZE{ 30, 30 };

namespace Dirs
{
	enum EDirection : uint8_t {
		SouthWest = 0,
		West,
		NorthWest,
		North,
		NorthEast,
		East,
		SouthEast,
		South
	};

	static const std::map<EDirection, FIntVector2> DIRS = { //TODO OPTIMIZE FLAGS
	{ EDirection::SouthWest,  {-1, -1} },
	{ EDirection::West,       {-1, 0 } },
	{ EDirection::NorthWest,  {-1, 1 } },
	{ EDirection::North,      { 0, 1 } },
	{ EDirection::NorthEast,  { 1, 1 } },
	{ EDirection::East,       { 1, 0 } },
	{ EDirection::SouthEast,  { 1, -1} },
	{ EDirection::South,      { 0, -1} }
	};

	static const FIntVector2 SW{ -1, -1 };
	static const FIntVector2 W { -1,  0 };
	static const FIntVector2 NW{ -1,  1 };
	static const FIntVector2 N {  0,  1 };
	static const FIntVector2 NE{  1,  1 };
	static const FIntVector2 E {  1,  0 };
	static const FIntVector2 SE{  1, -1 };
	static const FIntVector2 S {  0, -1 };

	static const bool IsDiagonal(EDirection Dir) { return Dir % 2 == 0; }
	static const FIntVector2 Next(EDirection Dir) { return DIRS.at( EDirection((Dir + 1) % 8) ); }
	static const FIntVector2 Prev(EDirection Dir) { return DIRS.at( EDirection((Dir + 7) % 8) ); }
}

static const int ApplyDir(const int Idx, const FIntVector2& Dir) {
	int Result = Idx + Dir.Y * GRIDSIZE.X + Dir.X;
	return Result;
}

struct FlowField {
	Dirs::EDirection Dir : 4;
	uint8_t Completed : 1; // TODO Change this to Pathable in next version
	uint8_t LOS : 1;
};

struct IntegrationField {
	float IntegratedCost = FLT_MAX;
	uint8_t WaveFrontBlocked : 1;
	uint8_t LOS : 1;
};

bool IsInGrid(int X, int Y)
{
	return X >= 0 && X < GRIDSIZE.X && Y >= 0 && Y < GRIDSIZE.Y;
}

bool IsInGrid(const FIntVector2& Cell) {
	return IsInGrid(Cell.X ,Cell.Y);
}

bool IsInGrid(int Idx)
{
	int X = Idx % GRIDSIZE.X;
	int Y = Idx / GRIDSIZE.X;
	return IsInGrid(X, Y);
}



bool IsWall(const TArray<uint8_t>& CostFields, const FIntVector2& Cell)
{
	return CostFields[Cell.Y * GRIDSIZE.X + Cell.X] == UINT8_MAX;
}

FIntVector2 ToFIntVector2(int LinearIdx)
{
	return FIntVector2(
		LinearIdx % GRIDSIZE.X,
		LinearIdx / GRIDSIZE.X
	);
}

int ToLinearIdx(FIntVector2 IntVector2)
{
	return IntVector2.Y * GRIDSIZE.X + IntVector2.X;
}

FVector2D ToVector2D(const FIntVector2& IntVector2)
{
	FVector2D Result;
	Result.X = static_cast<float>(IntVector2.X);
	Result.Y = static_cast<float>(IntVector2.Y);
	return Result;
};

namespace Debug
{
	#define BREAK_IF_EQUAL(current, at) if ((current) == (at)) { __debugbreak(); }

	const float LOS_FLAG_HEIGHT = PLANE_HEIGHT + 1.f;
	const float TEXT_HEIGHT = PLANE_HEIGHT + 2.f;
	const float FLOW_ARROW_HEIGHT = PLANE_HEIGHT + 50.f;
	UWorld* pWorld;

	void DrawCosts(const TArray<uint8_t>& CostFields)
	{
		if (!bDRAW_COSTS) { return;  }
		ensure(pWorld);
		for (int y = 0; y < GRIDSIZE.Y; ++y)
		{
			for (int x = 0; x < GRIDSIZE.X; ++x)
			{
				FVector RayStart = COST_TRACE_Y_START + FVector(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 0.f);
				FVector RayEnd = RayStart + COST_TRACE_DIRECTION;
				// UE_LOG(LogUE5TopDownARPG, Log, TEXT("CostFields [%d][%d] = [%d]"),i, j, CostFields[i * GRIDSIZE.X + j]);
				DrawDebugDirectionalArrow(pWorld, RayStart, RayEnd, 3.0f, CostFields[y * GRIDSIZE.X + x] == UINT8_MAX ? FColor::Red : FColor::Green, true, -1.f, 0, 2.f);
			}
		}
	}

	void DrawIntegration(const TArray<IntegrationField>& IntegrationFields)
	{
		if (!bDRAW_INTEGRATION) { return; }
		ensure(pWorld);
		FlushPersistentDebugLines(pWorld);

		for (int y = 0; y < GRIDSIZE.Y; ++y)
		{
			for (int x = 0; x < GRIDSIZE.X; ++x)
			{
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				{
					FVector TextStart = { x * CELL_SIZE + H_CELL_SIZE, y * CELL_SIZE + H_CELL_SIZE , TEXT_HEIGHT }; // Top corner of cell
					auto IntegratedCost = FString::FromInt(IntegrationFields[y * GRIDSIZE.X + x].IntegratedCost);
					if (IntegrationFields[y * GRIDSIZE.X + x].IntegratedCost == FLT_MAX)
					{
						IntegratedCost = "MAX";
					}

					auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90,0,180), SpawnParameters);
					TextActor->GetTextRender()->SetText(FText::FromString(IntegratedCost));
					TextActor->GetTextRender()->SetTextRenderColor(FColor::Magenta);
				}

				{
					FVector TextStart = { x * CELL_SIZE + H_CELL_SIZE, y * CELL_SIZE + Q_CELL_SIZE, TEXT_HEIGHT }; // Top corner of cell
					FString WaveFrontBlocked = IntegrationFields[y * GRIDSIZE.X + x].WaveFrontBlocked ? "+" : "-";
					auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90, 0, 180), SpawnParameters);
					TextActor->GetTextRender()->SetText(FText::FromString(WaveFrontBlocked));
					TextActor->GetTextRender()->SetTextRenderColor(FColor::Purple);
				}

				{
					FVector RayStart = COST_TRACE_Y_START + FVector(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 0.f);
					FVector RayEnd = RayStart + COST_TRACE_DIRECTION;
					// UE_LOG(LogUE5TopDownARPG, Log, TEXT("CostFields [%d][%d] = [%d]"),i, j, CostFields[i * GRIDSIZE.X + j]);
					FVector Points[]{
						{5.f, 5.f, 0.f},
						{CELL_SIZE - 5.f, CELL_SIZE -5.f, 0.f}
					};
					FBox Box{ Points, 2 };
					FTransform Transform { FVector(x * CELL_SIZE, y * CELL_SIZE, LOS_FLAG_HEIGHT)};
					FColor Color = IntegrationFields[y * GRIDSIZE.X + x].LOS ? FColor::White : FColor::Blue;
					if (IntegrationFields[y * GRIDSIZE.X + x].LOS == false)
					{
						int a = 5;
					}
					else
					{
						int b = 6;
					}
					DrawDebugSolidBox(pWorld, Box, Color, Transform, true);
				}
			}
		}
	}

	void Draw(UWorld* World, const TArray<FlowField>& FlowFields)
	{
		ensure(World);
		FVector Ray{ 0.f, 0.f, FLOW_ARROW_HEIGHT };

		for (int y = 0; y < GRIDSIZE.Y; ++y)
		{
			for (int x = 0; x < GRIDSIZE.X; ++x)
			{
				if (FlowFields[y * GRIDSIZE.X + x].Completed == false)
				{
					FMatrix Transformation = FMatrix::Identity;
					Transformation.SetOrigin({ x * 100.f + 50 , y * 100.f + 50 , 50.f });
					FMatrix Rotation = FRotationMatrix({ 90.f, 0.f, 180.f });

					FMatrix DonutMatrix = Rotation * Transformation;
					DrawDebug2DDonut(World, DonutMatrix, 5.f, 10.f, 16, FColor::Blue, true);
				}
				else
				{
					Dirs::EDirection Dir = FlowFields[y * GRIDSIZE.X + x].Dir;
					FIntVector2 IntOffset = Dirs::DIRS.at(Dir) * 25;
					FVector Offset{ IntOffset.X * 1.f, IntOffset.Y * 1.f, 0.f };
					Ray.Y = y * 100.f + 50;
					Ray.X = x * 100.f + 50;

					FVector Start = Ray - Offset;
					FVector End = Ray + Offset;

					DrawDebugDirectionalArrow(World, Start, End, 3.f, FColor::Green, true, 9999999.f, 0, Q_CELL_SIZE / 2.f);

					//UE_LOG(LogUE5TopDownARPG, Log, TEXT("FlowFields[%d][%d].Dir = [%d]"), y, x, (int)FlowFields[y * GRIDSIZE.X + x].Dir);
					//UE_LOG(LogUE5TopDownARPG, Log, TEXT("Start = %s"), *Start.ToString());
					//UE_LOG(LogUE5TopDownARPG, Log, TEXT("End = %s"), *End.ToString());
				}
			}
		}
	}

	FVector addZ(FVector2D Vect, float Z)
	{
		return FVector(Vect.X, Vect.Y, Z);
	}
	/*
	Origin in World Coordinates
	*/
	void DrawLOS(FVector2D NormDir, FVector2D Origin = {0.f,0.f}, int Scale = 1000000)
	{
		FVector2D Direction = NormDir * Scale;
		FVector2D End = Direction + Origin;

		FVector Origin3D = addZ(Origin, 60.f);
		FVector End3D = addZ(End, 60.f);
		DrawDebugDirectionalArrow(pWorld, Origin3D, End3D, 3.f, FColor::Emerald, true, 9999999.f, 0, 2.f);
		//UE_LOG(LogUE5TopDownARPG, Log, TEXT("Draw LOS: Origin3D = %s; End3D = %s"), *Origin3D.ToString(), *End3D.ToString());
	}

	void DrawBox(FIntVector2 At, FColor Color = FColor::Orange)
	{
		ensure(pWorld);
		FVector Center{
			At.X * CELL_SIZE + H_CELL_SIZE,
			At.Y * CELL_SIZE + H_CELL_SIZE,
			60.f
		};
		FVector Extent = FVector(Q_CELL_SIZE, Q_CELL_SIZE, Q_CELL_SIZE);
		DrawDebugBox(pWorld, Center, Extent, Color, true);
		//UE_LOG(LogUE5TopDownARPG, Log, TEXT("Draw Box: Center = %s; Extent = %s"), *Center.ToString(), *Extent.ToString());
	}

	void DrawBox(int At, FColor Color = FColor::Orange)
	{
		return DrawBox(ToFIntVector2(At), Color);
	}

	void DrawCoords()
	{
		ensure(pWorld);
		DrawDebugCoordinateSystem(pWorld, { 0.f, 0.f, 0.f }, FRotator(0.f), CELL_SIZE, true);

		for (int y = 0; y < GRIDSIZE.Y; ++y)
		{
			for (int x = 0; x < GRIDSIZE.X; ++x)
			{
				FVector TextStart = { x * CELL_SIZE + Q_CELL_SIZE, y * CELL_SIZE, TEXT_HEIGHT}; // Bottom part of cell // TODO why x,y reversed?
					
				// UE_LOG(LogUE5TopDownARPG, Log, TEXT("IntegrationFields [%d][%d] = [%s]"), i, j, *IntegratedCost
				FText Coords = FText::FromString(" [" + FString::FromInt(x) + ", " + FString::FromInt(y) + "]");
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90, 0, 180), SpawnParameters);
				TextActor->GetTextRender()->SetText(Coords);
				TextActor->GetTextRender()->SetTextRenderColor(FColor::Turquoise);
			}
		}
	}

}

namespace Eikonal {
	const float SIGNIFICANT_COST_REDUCTION = 0.75f; // 25% reduction

	/*
Ray starts from Origin and extends in opposite direction of Goal
*/
	void BresenhamsRay2D(const TArray<uint8_t>& CostFields, const FIntVector2& Goal, FIntVector2 Origin, OUT std::deque<FIntVector2>& LOS)
	{
		// Calculate end point - large enough to ensure it goes "off-grid"
		FIntVector2 Direction = (Origin - Goal) * 1000;

		// Bresenham's Algorithm in 2D
		int dx = FMath::Abs(Direction.X - Origin.X), sx = Origin.X < Direction.X ? 1 : -1;
		int dy = -FMath::Abs(Direction.Y - Origin.Y), sy = Origin.Y < Direction.Y ? 1 : -1;
		int err = dx + dy, e2;

		while (IsWall(CostFields, Origin) == false) {
			// Mark the current cell as blocked
			if (IsInGrid(Origin))
			{	
				LOS.push_back(Origin);
			}

			if (Origin.X == Direction.X && Origin.Y == Direction.Y) break;
			e2 = 2 * err;
			if (e2 >= dy) { err += dy; Origin.X += sx; }
			if (e2 <= dx) { err += dx; Origin.Y += sy; }
		}
	}

	/*
	Cell & Goal: Grid Coordinates
	*/
	void GetLos(const TArray<uint8_t>& CostFields, const FIntVector2& Cell, const FIntVector2& Goal, std::deque<FIntVector2>& Los)
	{
		FVector2D GoalToCell = ToVector2D(Cell - Goal);
		bool isDiagonalOrientation = GoalToCell.X * GoalToCell.Y > 0;

		if (isDiagonalOrientation)
		{
			FIntVector2 sideNW = Cell + Dirs::NW;
			if (IsInGrid(sideNW)
				&& !IsWall(CostFields, sideNW)
				&& !IsWall(CostFields, Cell + Dirs::W)
				&& !IsWall(CostFields, Cell + Dirs::N))
			{
				BresenhamsRay2D(CostFields, Goal + Dirs::S, GoalToCell.Y > 0 ? Cell + Dirs::N : Cell + Dirs::W, OUT Los);
			}

			FIntVector2 sideSE = Cell + Dirs::SE;
			if (IsInGrid(sideSE)
				&& !IsWall(CostFields, sideSE)
				&& !IsWall(CostFields, Cell + Dirs::S)
				&& !IsWall(CostFields, Cell + Dirs::E))
			{
				BresenhamsRay2D(CostFields, Goal + Dirs::N, GoalToCell.Y > 0 ? Cell + Dirs::E : Cell + Dirs::S, OUT Los);
			}
		}
		else {
			FIntVector2 sideNE = Cell + Dirs::NE;
			if (IsInGrid(sideNE)
				&& !IsWall(CostFields, sideNE)
				&& !IsWall(CostFields, Cell + Dirs::N)
				&& !IsWall(CostFields, Cell + Dirs::E))
			{
				BresenhamsRay2D(CostFields, Goal + Dirs::S, GoalToCell.Y > 0 ? Cell + Dirs::N : Cell + Dirs::E, OUT Los);
			}

			FIntVector2 sideSW = Cell + Dirs::SW;
			if (IsInGrid(sideSW)
				&& !IsWall(CostFields, sideSW)
				&& !IsWall(CostFields, Cell + Dirs::S)
				&& !IsWall(CostFields, Cell + Dirs::W))
			{
				BresenhamsRay2D(CostFields, Goal + Dirs::N, GoalToCell.Y > 0 ? Cell + Dirs::W : Cell + Dirs::S, OUT Los);
			}
		}
	}

	void VisitCell(const TArray<uint8_t>& CostFields, std::deque<FIntVector2>& WaveFront, const FIntVector2& Cell, float CurrentCost, const FIntVector2& Goal, OUT TArray<IntegrationField>& IntegrationFields, OUT std::deque<FIntVector2>& SecondWaveFront, bool bLosPass) {
		UE_LOG(LogUE5TopDownARPG, Log, TEXT("VisitCell Processing [%d, %d]"), Cell.X, Cell.Y);
		
		if (IsInGrid(Cell) == false || IntegrationFields[Cell.Y * GRIDSIZE.X + Cell.X].LOS)
		{
			return;
		}
		//if (Cell.X == 22 && Cell.Y == 25) { __debugbreak(); }

		if (!bLosPass && IsWall(CostFields, Cell)) // used to be !bLosPass &&
		{
			return;
		}

		if (bLosPass && IsWall(CostFields, Cell))
		{
			std::deque<FIntVector2> Los;
			GetLos(CostFields, Cell, Goal, OUT Los);
			for (auto& LosCell : Los)
			{
				int LosCellIdx = ToLinearIdx(LosCell);
				IntegrationFields[LosCellIdx].WaveFrontBlocked = true;
				IntegrationFields[LosCellIdx].LOS = false; // false is default, but let's make sure
				Debug::DrawBox(LosCellIdx, FColor::Cyan);
			}
			SecondWaveFront.insert(SecondWaveFront.end(), Los.begin(), Los.end());

			return;
		}

		int Idx = ToLinearIdx(Cell);
		float NewCost = CurrentCost + CostFields[Idx];
		float OldCost = IntegrationFields[Idx].IntegratedCost;

		if (NewCost < OldCost * SIGNIFICANT_COST_REDUCTION) {
			IntegrationFields[Idx].IntegratedCost = NewCost;
			if (IntegrationFields[Idx].WaveFrontBlocked == false)
			{
				WaveFront.push_back(Cell); // calculate WaveFrontBlocked but don't propagate
				IntegrationFields[Idx].LOS = bLosPass;
				UE_LOG(LogUE5TopDownARPG, Log, TEXT("VisitCell: Pushing [%d, %d]"), Cell.X, Cell.Y);
			}

		}
	}

	void PropagateWave(const TArray<uint8_t>& CostFields, TArray<IntegrationField>& IntegrationFields, std::deque<FIntVector2>& WaveFront, bool bLosPass, const FIntVector2& Goal, OUT std::deque<FIntVector2>& SecondWaveFront)
	{

		while (!WaveFront.empty()) {
			FIntVector2 Current = WaveFront.front();
			WaveFront.pop_front();

			float CurrentCost = IntegrationFields[Current.Y * GRIDSIZE.X + Current.X].IntegratedCost;

			VisitCell(CostFields, WaveFront, FIntVector2(Current.X + 1, Current.Y), CurrentCost, Goal, OUT IntegrationFields, SecondWaveFront, bLosPass);
			VisitCell(CostFields, WaveFront, FIntVector2(Current.X - 1, Current.Y), CurrentCost, Goal, OUT IntegrationFields, SecondWaveFront, bLosPass);
			VisitCell(CostFields, WaveFront, FIntVector2(Current.X, Current.Y + 1), CurrentCost, Goal, OUT IntegrationFields, SecondWaveFront, bLosPass);
			VisitCell(CostFields, WaveFront, FIntVector2(Current.X, Current.Y - 1), CurrentCost, Goal, OUT IntegrationFields, SecondWaveFront, bLosPass);
		}
	}
}

void LineOfSightPass(int MapHeight, int MapWidth, const FIntVector2& Goal, OUT TArray<IntegrationField>& IntegrationFields)
{
	// TODO Implement
}

bool IsValidIdx(int Idx)
{
	return Idx >= 0 && Idx < GRIDSIZE.X * GRIDSIZE.Y;
}

void CalculateFlowFields(TArray<IntegrationField>& IntegrationFields, OUT std::queue<int>& Sources, OUT TArray<FlowField>& FlowFields)
{

	auto DiagonalReachable = [&IntegrationFields](const int CurIdx, const Dirs::EDirection NewDir) {
		return IntegrationFields[ApplyDir(CurIdx, Next(NewDir))].IntegratedCost != FLT_MAX && IntegrationFields[ApplyDir(CurIdx, Prev(NewDir))].IntegratedCost != FLT_MAX; // TODO change to binary comparison?
	};

	for (int i = 0; i < FlowFields.Num(); ++i)
	{
		FlowFields[i].LOS = IntegrationFields[i].LOS;
	}

	while (Sources.empty() == false)
	{
		const int CurIdx = Sources.front();
		Sources.pop();
		check(IsValidIdx(CurIdx)); // TODO optimize: executed twice for each Idx. Do we even need?

		if (FlowFields[CurIdx].Completed || FlowFields[CurIdx].LOS) // Reached other solution or goal
		{
			continue;
		}

		float CurrentCost = IntegrationFields[CurIdx].IntegratedCost;
		float BestCost = FLT_MAX;
		Dirs::EDirection BestDir; // Begin somewhere
		int BestIdx;

		for (const auto& pair : Dirs::DIRS) {
			Dirs::EDirection NewDir = pair.first;
			int NewIdx = ApplyDir(CurIdx, pair.second);

			if (IsValidIdx(NewIdx) && IntegrationFields[NewIdx].IntegratedCost != FLT_MAX) { // TODO change to binary comparison?

				if (Dirs::IsDiagonal(NewDir) && DiagonalReachable(CurIdx, NewDir) == false) continue;


				float LosBonus = (IntegrationFields[NewIdx].IntegratedCost - CurrentCost) * IntegrationFields[NewIdx].LOS * 10.f; // Prefer LOS only if contributes to goal
				float NewCost = IntegrationFields[NewIdx].IntegratedCost + LosBonus;
				//if (CurIdx == 62) { __debugbreak(); }
				if (NewCost < BestCost) 
				{
					BestIdx = NewIdx;
					BestCost = NewCost;
					BestDir = NewDir;
				}
			}
		}
		FlowFields[CurIdx].Dir = BestDir;
		FlowFields[CurIdx].Completed = true;
		Sources.push(BestIdx);
		
		//UE_LOG(LogUE5TopDownARPG, Log, TEXT("CalculateFlowFields [%d].Dir = %d"), CurIdx, FlowFields[CurIdx].Dir);
	}
}

void CalculateCostFields(UWorld* World, OUT TArray<uint8_t>& CostFields)
{
	ensure(World);

	FVector HitLocation;
	FHitResult OutHit;
	for (int y = 0; y < GRIDSIZE.Y; ++y)
	{
		for (int x = 0; x < GRIDSIZE.X; ++x)
		{
			FVector RayStart = COST_TRACE_Y_START + FVector(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 0.f);
			FVector RayEnd = RayStart + COST_TRACE_DIRECTION;
			FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), true);
			bool bLineTraceObstructed = World->LineTraceSingleByChannel(OutHit, RayStart, RayEnd, ECollisionChannel::ECC_WorldStatic /* , = FCollisionQueryParams::DefaultQueryParam, = FCollisionResponseParams::DefaultResponseParam */);

			CostFields[y * GRIDSIZE.X + x] = bLineTraceObstructed ? UINT8_MAX : 1;
		}
	}
	Debug::DrawCosts(CostFields);
}

void DoFlowTiles(UWorld* World)
{
	ensure(World);
	Debug::pWorld = World;
	TArray<uint8_t> CostFields;
	CostFields.Init(1, GRIDSIZE.X * GRIDSIZE.Y);
	CalculateCostFields(World, CostFields);

	Debug::DrawCoords();

	FIntVector2 Goal;
	{
		TArray<AActor*> FoundGoals;
		UGameplayStatics::GetAllActorsWithTag(World, "Goal", FoundGoals);
		check(FoundGoals.Num() == 1 && IsValid(FoundGoals[0]));
		AActor* GoalActor = FoundGoals[0];
		FVector GoalVector = GoalActor->GetActorLocation();
		Goal.X = (int)(GoalVector.X) / CELL_SIZE; // TODO use round?
		Goal.Y = (int)(GoalVector.Y) / CELL_SIZE;
		//UE_LOG(LogUE5TopDownARPG, Log, TEXT("Found Goal at [%d, %d]"), Goal.X, Goal.Y);
	}

	Debug::DrawBox(Goal);
	std::deque<FIntVector2> WaveFront;
	WaveFront.push_back(Goal);
	std::deque<FIntVector2> SecondWaveFront;
	std::deque<FIntVector2> DummyWaveFront; // TODO refactor
	TArray<IntegrationField> IntegrationFields;
	IntegrationFields.Init({ FLT_MAX, false, false }, GRIDSIZE.X * GRIDSIZE.Y);
	// runLos()
	// remove after runLos() is implemented
	IntegrationFields[Goal.Y * GRIDSIZE.X + Goal.X].IntegratedCost = 0;
	IntegrationFields[Goal.Y * GRIDSIZE.X + Goal.X].LOS = true;


	Eikonal::PropagateWave(CostFields, IntegrationFields, WaveFront, /*bLosPass =*/ true ,Goal, SecondWaveFront);
	Eikonal::PropagateWave(CostFields, IntegrationFields, SecondWaveFront, /*bLosPass =*/ false, Goal, DummyWaveFront);
	Debug::DrawIntegration(IntegrationFields);

	TArray<FlowField> FlowFields;
	FlowFields.Init({Dirs::EDirection(), false, 0}, GRIDSIZE.X * GRIDSIZE.Y); // TODO optimize: can we omit constructing these?
	std::queue<int> Sources; // TODO optimize
	//Sources.push(25 * GRIDSIZE.X + 27);
	Sources.push(2 * GRIDSIZE.X + 2);
	Sources.push(27 * GRIDSIZE.X + 2);
	Sources.push(2 * GRIDSIZE.X + 27);
	//Sources.push(10 * GRIDSIZE.X + 20);
	CalculateFlowFields(IntegrationFields, Sources, FlowFields);

	Debug::Draw(World, FlowFields);
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
