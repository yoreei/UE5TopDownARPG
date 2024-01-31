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

#include <map>
#include <queue>
#include <climits>
#include <cmath>
#include <NavigationData.h>

#define LOCTEXT_NAMESPACE "FCrowdPFModule"

DEFINE_LOG_CATEGORY(LogCrowdPF)

void FCrowdPFModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FCrowdPFModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}
/* Public Functions */

FCrowdPFModule::FCrowdPFModule() { ModuleImplementation = MakeUnique<FCrowdPFModule::Impl>(); };
FCrowdPFModule::FCrowdPFModule(FCrowdPFModule&&) = default;
FCrowdPFModule::~FCrowdPFModule() = default;
FCrowdPFModule& FCrowdPFModule::operator=(FCrowdPFModule&&) = default;
void FCrowdPFModule::DoFlowTiles(const AActor* GoalActor, FNavPathSharedPtr& OutPath) { ModuleImplementation->DoFlowTiles(GoalActor, OutPath); }
void FCrowdPFModule::Init(UWorld* _World) { ModuleImplementation->Init(_World); } // TODO better handling of World?

/* Begin Eikonal */
/*
Ray starts from Origin and extends in opposite direction of Goal
*/
void FCrowdPFModule::Impl::BresenhamsRay2D(const TArray<uint8_t>& CostFields, const FIntVector2& Goal, FIntVector2 Origin, OUT std::deque<FIntVector2>& LOS)
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
void FCrowdPFModule::Impl::GetLos(const TArray<uint8_t>& CostFields, const FIntVector2& Cell, const FIntVector2& Goal, std::deque<FIntVector2>& Los)
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

void FCrowdPFModule::Impl::VisitCell(const TArray<uint8_t>& CostFields, std::deque<FIntVector2>& WaveFront, const FIntVector2& Cell, float CurrentCost, const FIntVector2& Goal, OUT TArray<IntegrationField>& IntegrationFields, OUT std::deque<FIntVector2>& SecondWaveFront, bool bLosPass) {
	UE_LOG(LogCrowdPF, Log, TEXT("VisitCell Processing [%d, %d]"), Cell.X, Cell.Y);

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

void FCrowdPFModule::Impl::PropagateWave(const TArray<uint8_t>& CostFields, TArray<IntegrationField>& IntegrationFields, std::deque<FIntVector2>& WaveFront, bool bLosPass, const FIntVector2& Goal, OUT std::deque<FIntVector2>& SecondWaveFront)
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
/* End Eikonal*/

void FCrowdPFModule::Impl::CalculateFlowFields(TArray<IntegrationField>& IntegrationFields, OUT std::queue<int>& Sources, OUT TArray<FlowField>& FlowFields)
{

	auto DiagonalReachable = [this, &IntegrationFields](const int CurIdx, const Dirs::EDirection NewDir) {
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
		Dirs::EDirection BestDir = Dirs::EDirection::North; // Begin somewhere
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

		//UE_LOG(LogCrowdPF, Log, TEXT("CalculateFlowFields [%d].Dir = %d"), CurIdx, FlowFields[CurIdx].Dir);
	}
}

void FCrowdPFModule::Impl::CalculateCostFields(UWorld* World, OUT TArray<uint8_t>& CostFields)
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
	DrawCosts(CostFields);
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

void FCrowdPFModule::Impl::DoFlowTiles(const AActor* GoalActor, FNavPathSharedPtr& OutPath)
{
	TArray<FVector> Points{ {0.f,0.f,0.f} };
	OutPath = MakeShared<FNavigationPath, ESPMode::ThreadSafe>(Points); ;

	ensure(pWorld);
	TArray<uint8_t> CostFields;
	CostFields.Init(1, GRIDSIZE.X * GRIDSIZE.Y);
	CalculateCostFields(pWorld, CostFields);

	DrawCoords();

	FIntVector2 Goal;
	GetGoal(pWorld, "Goal", Goal);
	


	DrawBox(Goal);
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


	PropagateWave(CostFields, IntegrationFields, WaveFront, /*bLosPass =*/ true, Goal, SecondWaveFront);
	PropagateWave(CostFields, IntegrationFields, SecondWaveFront, /*bLosPass =*/ false, Goal, DummyWaveFront); // todo templating
	DrawIntegration(IntegrationFields);

	TArray<FlowField> FlowFields;
	FlowFields.Init({ Dirs::EDirection(), false, 0 }, GRIDSIZE.X * GRIDSIZE.Y); // TODO optimize: can we omit constructing these?
	std::queue<int> Crowd; // TODO optimize
	GetCrowd(pWorld, "Crowd", Crowd);
	check(Crowd.size() > 0);

	CalculateFlowFields(IntegrationFields, Crowd, FlowFields);

	DrawFlows(FlowFields);
}

void FCrowdPFModule::Impl::Init(UWorld* _World)
{
	pWorld = _World;
}

void FCrowdPFModule::Impl::DrawCosts(const TArray<uint8_t>& CostFields)
{
	if (!bDRAW_COSTS) { return; }
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

void FCrowdPFModule::Impl::DrawIntegration(const TArray<IntegrationField>& IntegrationFields)
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

				auto TextActor = pWorld->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), TextStart, FRotator(90, 0, 180), SpawnParameters);
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

void FCrowdPFModule::Impl::DrawFlows(const TArray<FlowField>& FlowFields)
{
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
	FVector2D Direction = NormDir * Scale;
	FVector2D End = Direction + Origin;

	FVector Origin3D = addZ(Origin, 60.f);
	FVector End3D = addZ(End, 60.f);
	DrawDebugDirectionalArrow(pWorld, Origin3D, End3D, 3.f, FColor::Emerald, true, 9999999.f, 0, 2.f);
	//UE_LOG(LogCrowdPF, Log, TEXT("Draw LOS: Origin3D = %s; End3D = %s"), *Origin3D.ToString(), *End3D.ToString());
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