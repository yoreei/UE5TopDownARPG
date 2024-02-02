// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdPFImpl.h"
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
void FCrowdPFModule::Init(UWorld* _World) { ModuleImplementation->Init(_World); } // TODO better handling of World?

void FCrowdPFModule::SetDebugDraw(bool _bDebugDraw) { ModuleImplementation->SetDebugDraw(_bDebugDraw); }


bool FCrowdPFModule::Impl::IsWall(const FIntVector2& Cell) const
{
	return CostFields[Cell.Y * GRIDSIZE.X + Cell.X] == UINT8_MAX;
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

	if (IsInGrid(Cell) == false || IntegrationFields[Cell.Y * GRIDSIZE.X + Cell.X].LOS)
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
			DrawBox(LosCellIdx, FColor::Cyan);
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
			UE_LOG(LogCrowdPF, Log, TEXT("VisitCell: Pushing [%d, %d]"), Cell.X, Cell.Y);
		}

	}
}

void FCrowdPFModule::Impl::PropagateWave(std::deque<FIntVector2>& WaveFront, bool bLosPass, const FIntVector2& Goal, OUT std::deque<FIntVector2>& SecondWaveFront)
{

	while (!WaveFront.empty()) {
		FIntVector2 Current = WaveFront.front();
		WaveFront.pop_front();

		float CurrentCost = IntegrationFields[Current.Y * GRIDSIZE.X + Current.X].IntegratedCost;

		VisitCell(WaveFront, FIntVector2(Current.X + 1, Current.Y), CurrentCost, Goal, SecondWaveFront, bLosPass);
		VisitCell(WaveFront, FIntVector2(Current.X - 1, Current.Y), CurrentCost, Goal, SecondWaveFront, bLosPass);
		VisitCell(WaveFront, FIntVector2(Current.X, Current.Y + 1), CurrentCost, Goal, SecondWaveFront, bLosPass);
		VisitCell(WaveFront, FIntVector2(Current.X, Current.Y - 1), CurrentCost, Goal, SecondWaveFront, bLosPass);
	}
}
/* End Eikonal*/

void FCrowdPFModule::Impl::ConvertFlowTilesToPath(const FVector& WorldOrigin, const FVector& WorldGoal, FNavPathSharedPtr& OutPath)
{
	// TODO test
	// Assuming these helper functions exist for coordinate conversion and indexing
	FIntVector2 Origin = WorldVectToGridVect(WorldOrigin);
	FIntVector2 Goal = WorldVectToGridVect(WorldGoal);

	// Prepare the path points, starting with the origin
	TArray<FVector> Points;
	Points.Add(WorldOrigin);

	bool bReachedLOS = false;
	Dirs::EDirection LastDirection = static_cast<Dirs::EDirection>(-1); // Initialize with an invalid direction

	while (!bReachedLOS) {
		int idx = ToLinearIdx(Origin);

		// Check if index is out of bounds
		if (idx < 0 || idx >= FlowFields.Num()) {
			break;
		}

		const FlowField& CurrentField = FlowFields[idx];
		if (CurrentField.LOS) {
			// If LOS flag is true, break the loop to add the goal directly
			bReachedLOS = true;
			break;
		}

		// If the direction changes, add the current position to the path
		if (LastDirection != CurrentField.Dir) {
			FVector CurrentWorldPos = GridVectToWorldVect(Origin);
			Points.Add(CurrentWorldPos);
			LastDirection = CurrentField.Dir;
		}

		// Calculate the next grid position based on the flow direction
		FIntVector2 DirectionOffset = Dirs::DIRS.at(CurrentField.Dir);
		FIntVector2 NextGridPos = Origin + DirectionOffset;

		// Update the origin for the next iteration
		Origin = NextGridPos;
	}

	// Add the WorldGoal to the path directly if LOS was found or as the final step
	Points.Add(WorldGoal);

	// Create the navigation path object
	OutPath = MakeShared<FNavigationPath, ESPMode::ThreadSafe>(Points);

	// Assuming this loop correctly assigns flags based on your specifications
	if (OutPath->GetPathPoints().Num() > 0) {
		OutPath->GetPathPoints()[0].Flags = 81665; // Beginning node
		for (int i = 1; i < OutPath->GetPathPoints().Num() - 1; ++i) {
			OutPath->GetPathPoints()[i].Flags = 81664; // Intermediate nodes
		}
		OutPath->GetPathPoints().Last().Flags = 81666; // End node
	}
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
	CostFields.Init(1, GRIDSIZE.X * GRIDSIZE.Y);
	FVector HitLocation;
	FHitResult OutHit;
	for (int y = 0; y < GRIDSIZE.Y; ++y)
	{
		for (int x = 0; x < GRIDSIZE.X; ++x)
		{
			FVector RayStart = COST_TRACE_Y_START + FVector(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 0.f);
			FVector RayEnd = RayStart + COST_TRACE_DIRECTION;
			FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), true);
			bool bLineTraceObstructed = pWorld->LineTraceSingleByChannel(OutHit, RayStart, RayEnd, ECollisionChannel::ECC_WorldStatic /* , = FCollisionQueryParams::DefaultQueryParam, = FCollisionResponseParams::DefaultResponseParam */);

			CostFields[y * GRIDSIZE.X + x] = bLineTraceObstructed ? UINT8_MAX : 1;
		}
	}
	DrawCosts();
}

void GetGoal(UWorld* pWorld, FName Tag, FIntVector2& Goal)
{
	TArray<AActor*> FoundGoals;
	UGameplayStatics::GetAllActorsWithTag(pWorld, Tag, FoundGoals);
	check(FoundGoals.Num() == 1 && IsValid(FoundGoals[0]));
	AActor* GoalActor = FoundGoals[0];
	FVector GoalVector = GoalActor->GetActorLocation();
	Goal.X = (int)(GoalVector.X) / CELL_SIZE; // TODO use round?
	Goal.Y = (int)(GoalVector.Y) / CELL_SIZE;
}

void GetCrowd(UWorld* pWorld, FName Tag, std::queue<int>& Crowd)
{
	TArray<AActor*> Members;
	UGameplayStatics::GetAllActorsWithTag(pWorld, Tag, Members);
	for (const AActor* Member : Members)
	{
		FVector MemberVect = Member->GetActorLocation();
		MemberVect.X = (int)(MemberVect.X) / CELL_SIZE; // TODO use round?
		MemberVect.Y = (int)(MemberVect.Y) / CELL_SIZE;
		FIntVector2 IntMemberVect = ToFIntVector2(MemberVect);
		Crowd.push(ToLinearIdx(IntMemberVect));
	}

	//Crowd.push(2 * GRIDSIZE.X + 2);
	//Crowd.push(27 * GRIDSIZE.X + 2);
	//Crowd.push(2 * GRIDSIZE.X + 27);
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
		IntegrationFields.Init({ FLT_MAX, false, false }, GRIDSIZE.X * GRIDSIZE.Y);
		IntegrationFields[Goal.Y * GRIDSIZE.X + Goal.X].IntegratedCost = 0;
		IntegrationFields[Goal.Y * GRIDSIZE.X + Goal.X].LOS = true;
		PropagateWave(WaveFront, /*bLosPass =*/ true, Goal, SecondWaveFront);
		PropagateWave(SecondWaveFront, /*bLosPass =*/ false, Goal, DummyWaveFront); // todo templating

		// Flow Fields
		FlowFields.Init({ Dirs::EDirection(), false, 0 }, GRIDSIZE.X * GRIDSIZE.Y); // TODO optimize: can we omit constructing these?
		CalculateFlowFields();

		SetNeedToRecalculate(false);

		// Debug
		DrawCoords();
		DrawBox(Goal);
		DrawIntegration();
		DrawFlows();
	}
	ConvertFlowTilesToPath(WorldOrigin, WorldGoal, OutPath);

}

void FCrowdPFModule::Impl::SetDebugDraw(bool _bDebugDraw)
{
	bDebugDraw = _bDebugDraw;
}

void FCrowdPFModule::Impl::Init(UWorld* _World)
{
	pWorld = _World;
}

void FCrowdPFModule::Impl::DrawCosts()
{
	if (!bDebugDraw || !bDRAW_COSTS)
	{
		return;
	}
	ensure(pWorld);
	for (int y = 0; y < GRIDSIZE.Y; ++y)
	{
		for (int x = 0; x < GRIDSIZE.X; ++x)
		{
			FVector RayStart = COST_TRACE_Y_START + FVector(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 0.f);
			FVector RayEnd = RayStart + COST_TRACE_DIRECTION;
			// UE_LOG(LogCrowdPF, Log, TEXT("CostFields [%d][%d] = [%d]"),i, j, CostFields[i * GRIDSIZE.X + j]);
			DrawDebugDirectionalArrow(pWorld, RayStart, RayEnd, 3.0f, CostFields[y * GRIDSIZE.X + x] == UINT8_MAX ? FColor::Red : FColor::Green, true, -1.f, 0, 2.f);
		}
	}
}

void FCrowdPFModule::Impl::DrawIntegration()
{
	if (!bDebugDraw || !bDRAW_INTEGRATION)
	{
		return;
	}
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

				auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90, 0, 180), SpawnParameters);
				TextActor->GetTextRender()->SetText(FText::FromString(IntegratedCost));
				TextActor->GetTextRender()->SetTextRenderColor(FColor::Magenta);
			}

			{
				FVector TextStart = { x * CELL_SIZE + H_CELL_SIZE, y * CELL_SIZE + Q_CELL_SIZE, TEXT_HEIGHT }; // Top corner of cell
				FString WaveFrontBlocked = IntegrationFields[y * GRIDSIZE.X + x].WaveFrontBlocked ? "|" : "";
				auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90, 0, 180), SpawnParameters);
				TextActor->GetTextRender()->SetText(FText::FromString(WaveFrontBlocked));
				TextActor->GetTextRender()->SetTextRenderColor(FColor::Black);
			}

			{
				FVector RayStart = COST_TRACE_Y_START + FVector(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 0.f);
				FVector RayEnd = RayStart + COST_TRACE_DIRECTION;
				// UE_LOG(LogCrowdPF, Log, TEXT("CostFields [%d][%d] = [%d]"),i, j, CostFields[i * GRIDSIZE.X + j]);
				FVector Points[]{
					{5.f, 5.f, 0.f},
					{CELL_SIZE - 5.f, CELL_SIZE - 5.f, 0.f}
				};
				FBox Box{ Points, 2 };
				FTransform Transform{ FVector(x * CELL_SIZE, y * CELL_SIZE, LOS_FLAG_HEIGHT) };
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

void FCrowdPFModule::Impl::DrawFlows()
{
	if (!bDebugDraw || !bDRAW_FLOWS)
	{
		return;
	}

	ensure(pWorld);
	FVector Ray{ 0.f, 0.f, FLOW_ARROW_HEIGHT };

	for (int y = 0; y < GRIDSIZE.Y; ++y)
	{
		for (int x = 0; x < GRIDSIZE.X; ++x)
		{
			if (FlowFields[y * GRIDSIZE.X + x].Completed == true)
			{
				Dirs::EDirection Dir = FlowFields[y * GRIDSIZE.X + x].Dir;
				FIntVector2 IntOffset = Dirs::DIRS.at(Dir) * 25;
				FVector Offset{ IntOffset.X * 1.f, IntOffset.Y * 1.f, 0.f };
				Ray.Y = y * 100.f + 50;
				Ray.X = x * 100.f + 50;

				FVector Start = Ray - Offset;
				FVector End = Ray + Offset;

				DrawDebugDirectionalArrow(pWorld, Start, End, 3.f, FColor::Green, true, 9999999.f, 0, Q_CELL_SIZE / 2.f);

				//UE_LOG(LogCrowdPF, Log, TEXT("FlowFields[%d][%d].Dir = [%d]"), y, x, (int)FlowFields[y * GRIDSIZE.X + x].Dir);
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
	if (!bDebugDraw || !bDRAW_LOS)
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
		At.X * CELL_SIZE + H_CELL_SIZE,
		At.Y * CELL_SIZE + H_CELL_SIZE,
		60.f
	};
	FVector Extent = FVector(Q_CELL_SIZE, Q_CELL_SIZE, Q_CELL_SIZE);
	DrawDebugBox(pWorld, Center, Extent, Color, true);
	//UE_LOG(LogCrowdPF, Log, TEXT("Draw Box: Center = %s; Extent = %s"), *Center.ToString(), *Extent.ToString());
}

void FCrowdPFModule::Impl::DrawBox(int At, FColor Color)
{
	return DrawBox(ToFIntVector2(At), Color);
}

void FCrowdPFModule::Impl::DrawCoords()
{
	if (!bDebugDraw || !bDRAW_COORDS)
	{
		return;
	}
	ensure(pWorld);
	DrawDebugCoordinateSystem(pWorld, { 0.f, 0.f, 0.f }, FRotator(0.f), CELL_SIZE, true);

	for (int y = 0; y < GRIDSIZE.Y; ++y)
	{
		for (int x = 0; x < GRIDSIZE.X; ++x)
		{
			FVector TextStart = { x * CELL_SIZE + Q_CELL_SIZE, y * CELL_SIZE, TEXT_HEIGHT }; // Bottom part of cell // TODO why x,y reversed?

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

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCrowdPFModule, CrowdPF)