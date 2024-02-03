// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdPFImpl.h"
#include "CrowdPF.h"
#include "Utils.h"

#include "GameFramework/Pawn.h"
#include "Engine/World.h"
// #include "NavigationSystem.h" eh?
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "Internationalization/Text.h"
#include "Kismet/GameplayStatics.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"

#include <map>
#include <queue>
#include <climits>
#include <cmath>
#include <NavigationData.h>

#define LOCTEXT_NAMESPACE "FCrowdPFModule"

DEFINE_LOG_CATEGORY(LogCrowdPF)

static const int GridSizeX{ 30 }; // TODO unhardcode
static const int GridSizeY{ 30 };

void FCrowdPFModule::StartupModule()
{
	//FEditorDelegates::BeginPIE.AddRaw(this, &FCrowdPFModule::OnBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FCrowdPFModule::OnEndPIE);
}

void FCrowdPFModule::ShutdownModule()
{
	//FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
}

//void FCrowdPFModule::OnBeginPIE(bool bIsSimulating)
//{
//	// Reset state here before PIE starts
//}

void FCrowdPFModule::OnEndPIE(bool bIsSimulating)
{
	ModuleImplementation.Reset();
	ModuleImplementation = MakeUnique<Impl>();
}


/* Public Functions */

FCrowdPFModule::FCrowdPFModule() { ModuleImplementation = MakeUnique<FCrowdPFModule::Impl>(); };
FCrowdPFModule::FCrowdPFModule(FCrowdPFModule&&) = default;
FCrowdPFModule::~FCrowdPFModule() = default;
FCrowdPFModule& FCrowdPFModule::operator=(FCrowdPFModule&&) = default;
void FCrowdPFModule::DoFlowTiles(const FVector& WorldOrigin, const FVector& WorldGoal, OUT FNavPathSharedPtr& OutPath) { ModuleImplementation->DoFlowTiles(WorldOrigin, WorldGoal, OutPath); }
void FCrowdPFModule::Init(UWorld* _World, FCrowdPFOptions Options) { ModuleImplementation->Init(_World, Options); } // TODO better handling of World?


bool FCrowdPFModule::Impl::IsWall(const FIntVector2& Cell) const
{
	return CostFields[Cell.Y * GridSizeX + Cell.X] == UINT8_MAX;
}

/* Begin Eikonal */
/*
Ray starts from Origin and extends in opposite direction of Goal
*/
void FCrowdPFModule::Impl::BresenhamsRay2D(const FIntVector2& Goal, FIntVector2 Origin, OUT std::deque<FIntVector2>& LOS)
{
	// Calculate end point - large enough to ensure it goes "off-grid"
	FIntVector2 Direction = (Origin - Goal) * 1000;

	// Bresenham's Algorithm in 2D
	int dx = FMath::Abs(Direction.X - Origin.X), sx = Origin.X < Direction.X ? 1 : -1;
	int dy = -FMath::Abs(Direction.Y - Origin.Y), sy = Origin.Y < Direction.Y ? 1 : -1;
	int err = dx + dy, e2;

	while (IsWall(Origin) == false) {
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
void FCrowdPFModule::Impl::GetLos(const FIntVector2& Cell, const FIntVector2& Goal, std::deque<FIntVector2>& Los)
{
	FVector2D GoalToCell = ToVector2D(Cell - Goal);
	bool isDiagonalOrientation = GoalToCell.X * GoalToCell.Y > 0;

	if (isDiagonalOrientation)
	{
		FIntVector2 sideNW = Cell + Dirs::NW;
		if (IsInGrid(sideNW)
			&& !IsWall(sideNW)
			&& !IsWall(Cell + Dirs::W)
			&& !IsWall(Cell + Dirs::N))
		{
			BresenhamsRay2D(Goal + Dirs::S, GoalToCell.Y > 0 ? Cell + Dirs::N : Cell + Dirs::W, OUT Los);
		}

		FIntVector2 sideSE = Cell + Dirs::SE;
		if (IsInGrid(sideSE)
			&& !IsWall(sideSE)
			&& !IsWall(Cell + Dirs::S)
			&& !IsWall(Cell + Dirs::E))
		{
			BresenhamsRay2D(Goal + Dirs::N, GoalToCell.Y > 0 ? Cell + Dirs::E : Cell + Dirs::S, OUT Los);
		}
	}
	else {
		FIntVector2 sideNE = Cell + Dirs::NE;
		if (IsInGrid(sideNE)
			&& !IsWall(sideNE)
			&& !IsWall(Cell + Dirs::N)
			&& !IsWall(Cell + Dirs::E))
		{
			BresenhamsRay2D(Goal + Dirs::S, GoalToCell.Y > 0 ? Cell + Dirs::N : Cell + Dirs::E, OUT Los);
		}

		FIntVector2 sideSW = Cell + Dirs::SW;
		if (IsInGrid(sideSW)
			&& !IsWall(sideSW)
			&& !IsWall(Cell + Dirs::S)
			&& !IsWall(Cell + Dirs::W))
		{
			BresenhamsRay2D(Goal + Dirs::N, GoalToCell.Y > 0 ? Cell + Dirs::W : Cell + Dirs::S, OUT Los);
		}
	}
}

void FCrowdPFModule::Impl::VisitCell(std::deque<FIntVector2>& WaveFront, const FIntVector2& Cell, float CurrentCost, const FIntVector2& Goal, OUT std::deque<FIntVector2>& SecondWaveFront, bool bLosPass) {
	UE_LOG(LogCrowdPF, Log, TEXT("VisitCell Processing [%d, %d]"), Cell.X, Cell.Y);

	if (IsInGrid(Cell) == false || IntegrationFields[Cell.Y * GridSizeX + Cell.X].LOS)
	{
		return;
	}
	//if (Cell.X == 22 && Cell.Y == 25) { __debugbreak(); }

	if (!bLosPass && IsWall(Cell)) // used to be !bLosPass &&
	{
		return;
	}

	if (bLosPass && IsWall(Cell))
	{
		std::deque<FIntVector2> Los;
		GetLos(Cell, Goal, OUT Los);
		for (auto& LosCell : Los)
		{
			int LosCellIdx = ToLinearIdx(LosCell);
			IntegrationFields[LosCellIdx].WaveFrontBlocked = true;
			IntegrationFields[LosCellIdx].LOS = false; // false is default, but let's make sure
		}
		SecondWaveFront.insert(SecondWaveFront.end(), Los.begin(), Los.end());

		return;
	}

	int Idx = ToLinearIdx(Cell);
	float NewCost = CurrentCost + CostFields[Idx];
	float OldCost = IntegrationFields[Idx].IntegratedCost;

	if (NewCost < OldCost * Options.SignificantCostReduction) {
		IntegrationFields[Idx].IntegratedCost = NewCost;
		if (IntegrationFields[Idx].WaveFrontBlocked == false)
		{
			WaveFront.push_back(Cell); // calculate WaveFrontBlocked but don't propagate
			IntegrationFields[Idx].LOS = bLosPass;
			UE_LOG(LogCrowdPF, Log, TEXT("VisitCell: Pushing [%d, %d]"), Cell.X, Cell.Y);
		}

	}
}

void FCrowdPFModule::Impl::PropagateWave(std::deque<FIntVector2>& WaveFront, bool bLosPass, const FIntVector2& Goal, OUT std::deque<FIntVector2>& SecondWaveFront)
{

	while (!WaveFront.empty()) {
		FIntVector2 Current = WaveFront.front();
		WaveFront.pop_front();

		float CurrentCost = IntegrationFields[Current.Y * GridSizeX + Current.X].IntegratedCost;

		VisitCell(WaveFront, FIntVector2(Current.X + 1, Current.Y), CurrentCost, Goal, SecondWaveFront, bLosPass);
		VisitCell(WaveFront, FIntVector2(Current.X - 1, Current.Y), CurrentCost, Goal, SecondWaveFront, bLosPass);
		VisitCell(WaveFront, FIntVector2(Current.X, Current.Y + 1), CurrentCost, Goal, SecondWaveFront, bLosPass);
		VisitCell(WaveFront, FIntVector2(Current.X, Current.Y - 1), CurrentCost, Goal, SecondWaveFront, bLosPass);
	}
}
/* End Eikonal*/

void FCrowdPFModule::Impl::ConvertFlowTilesToPath(const FVector& WorldOrigin, const FVector& WorldGoal, FNavPathSharedPtr& OutPath)
{
	FIntVector2 Origin = WorldVectToGridVect(WorldOrigin);
	FIntVector2 Goal = WorldVectToGridVect(WorldGoal);

	TArray<FVector> Points;
	Points.Add(WorldOrigin);

	bool bReachedLOS = false;
	Dirs::EDirection LastDirection = static_cast<Dirs::EDirection>(-1);

	while (!bReachedLOS) {
		int idx = ToLinearIdx(Origin);
		if (idx < 0 || idx >= FlowFields.Num()) {
			break;
		}

		const FlowField& CurrentField = FlowFields[idx];
		if (CurrentField.LOS) {
			FVector CurrentWorldPos = GridVectToWorldVect(Origin);
			Points.Add(CurrentWorldPos);
			bReachedLOS = true;
			break;
		}

		if (LastDirection != CurrentField.Dir) {
			FVector CurrentWorldPos = GridVectToWorldVect(Origin);
			Points.Add(CurrentWorldPos);
			LastDirection = CurrentField.Dir;
		}

		FIntVector2 DirectionOffset = Dirs::DIRS.at(CurrentField.Dir);
		FIntVector2 NextGridPos = Origin + DirectionOffset;

		Origin = NextGridPos;
	}
	Points.Add(WorldGoal);
	OutPath = MakeShared<FNavigationPath, ESPMode::ThreadSafe>(Points);

	OutPath->GetPathPoints()[0].Flags = 81665; // Beginning node
	for (int i = 1; i < OutPath->GetPathPoints().Num() - 1; ++i) {
		OutPath->GetPathPoints()[i].Flags = 81664; // Intermediate nodes
	}
	OutPath->GetPathPoints().Last().Flags = 81666; // End node
}


void FCrowdPFModule::Impl::CalculateFlowFields()
{
	auto DiagonalReachable = [this](const int CurIdx, const Dirs::EDirection NewDir) {
		return IntegrationFields[ApplyDir(CurIdx, Next(NewDir))].IntegratedCost != FLT_MAX && IntegrationFields[ApplyDir(CurIdx, Prev(NewDir))].IntegratedCost != FLT_MAX; // TODO change to binary comparison?
		};

	for (int i = 0; i < FlowFields.Num(); ++i)
	{
		FlowFields[i].LOS = IntegrationFields[i].LOS;

		if (FlowFields[i].LOS)
		{
			continue;
		}

		float CurrentCost = IntegrationFields[i].IntegratedCost;
		float BestCost = FLT_MAX;
		Dirs::EDirection BestDir = Dirs::EDirection::North; // Begin somewhere
		int BestIdx;

		for (const auto& pair : Dirs::DIRS) {
			Dirs::EDirection NewDir = pair.first;
			int NewIdx = ApplyDir(i, pair.second);

			if (IsValidIdx(NewIdx) && IntegrationFields[NewIdx].IntegratedCost != FLT_MAX) { // TODO change to binary comparison?

				if (Dirs::IsDiagonal(NewDir) && DiagonalReachable(i, NewDir) == false) continue;


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
		FlowFields[i].Dir = BestDir;
		FlowFields[i].Completed = true;
	}
}

void FCrowdPFModule::Impl::CalculateCostFields()
{
	CostFields.Init(1, GridSizeX * GridSizeY);
	FVector HitLocation;
	FHitResult OutHit;
	for (int y = 0; y < GridSizeY; ++y)
	{
		for (int x = 0; x < GridSizeX; ++x)
		{
			FVector RayStart = Options.CostTraceYStart + FVector(x * Options.CellSize + Options.CellSize / 2, y * Options.CellSize + Options.CellSize / 2, 0.f);
			FVector RayEnd = RayStart + Options.CostTraceDirection;
			FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), true);
			bool bLineTraceObstructed = pWorld->LineTraceSingleByChannel(OutHit, RayStart, RayEnd, ECollisionChannel::ECC_WorldStatic /* , = FCollisionQueryParams::DefaultQueryParam, = FCollisionResponseParams::DefaultResponseParam */);

			CostFields[y * GridSizeX + x] = bLineTraceObstructed ? UINT8_MAX : 1;
		}
	}
	DrawCosts();
}

bool FCrowdPFModule::Impl::GetNeedToRecalculate(const FIntVector2& Goal)
{
	return bNeedToRecalculate;
}

void FCrowdPFModule::Impl::SetNeedToRecalculate(const bool bValue)
{
	bNeedToRecalculate = bValue;
}

void FCrowdPFModule::Impl::DoFlowTiles(const FVector& WorldOrigin, const FVector& WorldGoal, OUT FNavPathSharedPtr& OutPath)
{
	ensure(pWorld);

	FIntVector2 Goal = WorldVectToGridVect(WorldGoal);
	FIntVector2 Origin = WorldVectToGridVect(WorldOrigin);

	if (GetNeedToRecalculate(Goal))
	{
		//Cost Fields
		CalculateCostFields();

		// Eikonal
		std::deque<FIntVector2> WaveFront;
		WaveFront.push_back(Goal);
		std::deque<FIntVector2> SecondWaveFront;
		std::deque<FIntVector2> DummyWaveFront; // TODO refactor
		IntegrationFields.Init({ FLT_MAX, false, false }, GridSizeX * GridSizeY);
		IntegrationFields[Goal.Y * GridSizeX + Goal.X].IntegratedCost = 0;
		IntegrationFields[Goal.Y * GridSizeX + Goal.X].LOS = true;
		PropagateWave(WaveFront, /*bLosPass =*/ true, Goal, SecondWaveFront);
		PropagateWave(SecondWaveFront, /*bLosPass =*/ false, Goal, DummyWaveFront); // todo templating

		// Flow Fields
		FlowFields.Init({ Dirs::EDirection(), false, 0 }, GridSizeX * GridSizeY); // TODO optimize: can we omit constructing these?
		CalculateFlowFields();

		SetNeedToRecalculate(false);

		// Debug
		if (Options.bDebugDraw)
		{
			DrawCoords();
			DrawIntegration();
			DrawFlows();
		}
	}

	ConvertFlowTilesToPath(WorldOrigin, WorldGoal, OutPath);
}

void FCrowdPFModule::Impl::Init(UWorld* _World, FCrowdPFOptions _Options)
{
	pWorld = _World;
	Options = _Options;
}

void FCrowdPFModule::Impl::DrawCosts()
{
	if (!Options.bDebugDraw || !Options.bDrawCosts)
	{
		return;
	}
	ensure(pWorld);
	for (int y = 0; y < GridSizeY; ++y)
	{
		for (int x = 0; x < GridSizeX; ++x)
		{
			FVector RayStart = Options.CostTraceYStart + FVector(x * Options.CellSize + Options.CellSize / 2, y * Options.CellSize + Options.CellSize / 2, 0.f);
			FVector RayEnd = RayStart + Options.CostTraceDirection;
			// UE_LOG(LogCrowdPF, Log, TEXT("CostFields [%d][%d] = [%d]"),i, j, CostFields[i * GridSizeX + j]);
			DrawDebugDirectionalArrow(pWorld, RayStart, RayEnd, 3.0f, CostFields[y * GridSizeX + x] == UINT8_MAX ? FColor::Red : FColor::Green, true, -1.f, 0, 2.f);
		}
	}
}

void FCrowdPFModule::Impl::DrawIntegration()
{
	if (!Options.bDrawIntegration)
	{
		return;
	}
	ensure(pWorld);
	FlushPersistentDebugLines(pWorld);

	for (int y = 0; y < GridSizeY; ++y)
	{
		for (int x = 0; x < GridSizeX; ++x)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			{
				FVector TextStart = { x * Options.CellSize + Options.HCellSize(), y * Options.CellSize + Options.HCellSize() , Options.TextHeight()}; // Top corner of cell
				auto IntegratedCost = FString::FromInt(IntegrationFields[y * GridSizeX + x].IntegratedCost);
				if (IntegrationFields[y * GridSizeX + x].IntegratedCost == FLT_MAX)
				{
					IntegratedCost = "MAX";
				}

				auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90, 0, 180), SpawnParameters);
				TextActor->GetTextRender()->SetText(FText::FromString(IntegratedCost));
				TextActor->GetTextRender()->SetTextRenderColor(FColor::Magenta);
			}

			{
				FVector TextStart = { x * Options.CellSize + Options.HCellSize(), y * Options.CellSize + Options.QCellSize(), Options.TextHeight()}; // Top corner of cell
				FString WaveFrontBlocked = IntegrationFields[y * GridSizeX + x].WaveFrontBlocked ? "|" : "";
				auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90, 0, 180), SpawnParameters);
				TextActor->GetTextRender()->SetText(FText::FromString(WaveFrontBlocked));
				TextActor->GetTextRender()->SetTextRenderColor(FColor::Black);
			}

			{
				FVector RayStart = Options.CostTraceYStart + FVector(x * Options.CellSize + Options.CellSize / 2, y * Options.CellSize + Options.CellSize / 2, 0.f);
				FVector RayEnd = RayStart + Options.CostTraceDirection;
				// UE_LOG(LogCrowdPF, Log, TEXT("CostFields [%d][%d] = [%d]"),i, j, CostFields[i * GridSizeX + j]);
				FVector Points[]{
					{5.f, 5.f, 0.f},
					{Options.CellSize - 5.f, Options.CellSize - 5.f, 0.f}
				};
				FBox Box{ Points, 2 };
				FTransform Transform{ FVector(x * Options.CellSize, y * Options.CellSize, Options.LosFlagHeight()) };
				FColor Color = IntegrationFields[y * GridSizeX + x].LOS ? FColor::White : FColor::Blue;
				if (IntegrationFields[y * GridSizeX + x].LOS == false)
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

void FCrowdPFModule::Impl::DrawFlows()
{
	if (!Options.bDrawFlows)
	{
		return;
	}

	ensure(pWorld);
	FVector Ray{ 0.f, 0.f, Options.FlowArrowHeight()};

	for (int y = 0; y < GridSizeY; ++y)
	{
		for (int x = 0; x < GridSizeX; ++x)
		{
			if (FlowFields[y * GridSizeX + x].Completed == true)
			{
				Dirs::EDirection Dir = FlowFields[y * GridSizeX + x].Dir;
				FIntVector2 IntOffset = Dirs::DIRS.at(Dir) * 25;
				FVector Offset{ IntOffset.X * 1.f, IntOffset.Y * 1.f, 0.f };
				Ray.Y = y * 100.f + 50;
				Ray.X = x * 100.f + 50;

				FVector Start = Ray - Offset;
				FVector End = Ray + Offset;

				DrawDebugDirectionalArrow(pWorld, Start, End, 3.f, FColor::Green, true, 9999999.f, 0,  Options.QCellSize() / 2.f);

				//UE_LOG(LogCrowdPF, Log, TEXT("FlowFields[%d][%d].Dir = [%d]"), y, x, (int)FlowFields[y * GridSizeX + x].Dir);
				//UE_LOG(LogCrowdPF, Log, TEXT("Start = %s"), *Start.ToString());
				//UE_LOG(LogCrowdPF, Log, TEXT("End = %s"), *End.ToString());
			}
		}
	}
}

/*
Origin in World Coordinates
*/
void FCrowdPFModule::Impl::DrawLOS(FVector2D NormDir, FVector2D Origin = { 0.f,0.f }, int Scale = 1000000)
{
	if (!Options.bDebugDraw || !Options.bDrawLos)
	{
		return;
	}

	FVector2D Direction = NormDir * Scale;
	FVector2D End = Direction + Origin;

	FVector Origin3D = addZ(Origin, 60.f);
	FVector End3D = addZ(End, 60.f);
	DrawDebugDirectionalArrow(pWorld, Origin3D, End3D, 3.f, FColor::Emerald, true, 9999999.f, 0, 2.f);

}

void FCrowdPFModule::Impl::DrawBox(FIntVector2 At, FColor Color)
{
	ensure(pWorld);
	FVector Center{
		At.X * Options.CellSize + Options.HCellSize(),
		At.Y * Options.CellSize + Options.HCellSize(),
		60.f
	};
	FVector Extent = FVector( Options.QCellSize(),  Options.QCellSize(),  Options.QCellSize());
	DrawDebugBox(pWorld, Center, Extent, Color, true);
	//UE_LOG(LogCrowdPF, Log, TEXT("Draw Box: Center = %s; Extent = %s"), *Center.ToString(), *Extent.ToString());
}

void FCrowdPFModule::Impl::DrawBox(int At, FColor Color)
{
	return DrawBox(ToFIntVector2(At), Color);
}

void FCrowdPFModule::Impl::DrawCoords()
{
	if (!Options.bDrawCoords)
	{
		return;
	}
	ensure(pWorld);
	DrawDebugCoordinateSystem(pWorld, { 0.f, 0.f, 0.f }, FRotator(0.f), Options.CellSize, true);

	for (int y = 0; y < GridSizeY; ++y)
	{
		for (int x = 0; x < GridSizeX; ++x)
		{
			FVector TextStart = { x * Options.CellSize +  Options.QCellSize(), y * Options.CellSize, Options.TextHeight()}; // Bottom part of cell // TODO why x,y reversed?

			// UE_LOG(LogCrowdPF, Log, TEXT("IntegrationFields [%d][%d] = [%s]"), i, j, *IntegratedCost
			FText Coords = FText::FromString(" [" + FString::FromInt(x) + ", " + FString::FromInt(y) + "]");
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90, 0, 180), SpawnParameters);
			TextActor->GetTextRender()->SetText(Coords);
			TextActor->GetTextRender()->SetTextRenderColor(FColor::Turquoise);
		}
	}
}

#define BREAK_IF_EQUAL(current, at) if ((current) == (at)) { __debugbreak(); }

const int FCrowdPFModule::Impl::ApplyDir(const int Idx, const FIntVector2& Dir) {
	int Result = Idx + Dir.Y * GridSizeX + Dir.X;
	return Result;
}

bool FCrowdPFModule::Impl::IsInGrid(int X, int Y)
{
	return X >= 0 && X < GridSizeX && Y >= 0 && Y < GridSizeY;
}

bool FCrowdPFModule::Impl::IsInGrid(const FIntVector2& Cell) {
	return IsInGrid(Cell.X, Cell.Y);
}

bool FCrowdPFModule::Impl::IsInGrid(int Idx)
{
	int X = Idx % GridSizeX;
	int Y = Idx / GridSizeX;
	return IsInGrid(X, Y);
}

FIntVector2 FCrowdPFModule::Impl::ToFIntVector2(int LinearIdx)
{
	return FIntVector2(
		LinearIdx % GridSizeX,
		LinearIdx / GridSizeX
	);
}

FIntVector2 FCrowdPFModule::Impl::WorldVectToGridVect(const FVector& Vect)
{
	return FIntVector2(
		Vect.X / Options.CellSize,
		Vect.Y / Options.CellSize
	);
}

FVector FCrowdPFModule::Impl::GridVectToWorldVect(const FIntVector2& Vect)
{
	return FVector(
		Vect.X * Options.CellSize + Options.HCellSize(),
		Vect.Y * Options.CellSize + Options.HCellSize(),
		Options.PlaneHeight
	);
}


FIntVector2 FCrowdPFModule::Impl::ToFIntVector2(FVector Vect)
{
	return FIntVector2(
		static_cast<int>(Vect.X),
		static_cast<int>(Vect.Y)
	);
}

int FCrowdPFModule::Impl::ToLinearIdx(FIntVector2 IntVector2)
{
	return IntVector2.Y * GridSizeX + IntVector2.X;
}

FVector2D FCrowdPFModule::Impl::ToVector2D(const FIntVector2& IntVector2)
{
	FVector2D Result;
	Result.X = static_cast<float>(IntVector2.X);
	Result.Y = static_cast<float>(IntVector2.Y);
	return Result;
};

bool FCrowdPFModule::Impl::IsValidIdx(int Idx)
{
	return Idx >= 0 && Idx < GridSizeX * GridSizeY;
}

FVector FCrowdPFModule::Impl::addZ(FVector2D Vect, float Z)
{
	return FVector(Vect.X, Vect.Y, Z);
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCrowdPFModule, CrowdPF)