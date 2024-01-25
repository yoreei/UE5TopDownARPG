#pragma once

#include "CrowdPF.h"
#include <cstdint>
#include <map>

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
	static const FIntVector2 W{ -1,  0 };
	static const FIntVector2 NW{ -1,  1 };
	static const FIntVector2 N{ 0,  1 };
	static const FIntVector2 NE{ 1,  1 };
	static const FIntVector2 E{ 1,  0 };
	static const FIntVector2 SE{ 1, -1 };
	static const FIntVector2 S{ 0, -1 };

	static const bool IsDiagonal(EDirection Dir) { return Dir % 2 == 0; }
	static const FIntVector2 Next(EDirection Dir) { return DIRS.at(EDirection((Dir + 1) % 8)); }
	static const FIntVector2 Prev(EDirection Dir) { return DIRS.at(EDirection((Dir + 7) % 8)); }
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

class FCrowdPFModule::Impl {
public:
	void DoFlowTiles();
	void Init(UWorld* _World);
private:

	/* Parameters from Main class*/
	const float CELL_SIZE = 100.f;
	const float PLANE_HEIGHT = 51.f;
	const float bDRAW_COSTS = false;
	const float bDRAW_INTEGRATION = true;
	const FVector COST_TRACE_Y_START = { 0.f, 0.f, 250.f };
	const FVector COST_TRACE_DIRECTION{ 0.f, 0.f, -100 };
	const FIntVector2 GRIDSIZE{ 30, 30 };
	const float SIGNIFICANT_COST_REDUCTION = 0.75f; // 25% reduction

	/* Constants */
	UWorld* pWorld;
	const float SQRT_OF_2 = FMath::Sqrt(2.f);
	const float H_CELL_SIZE = CELL_SIZE / 2;
	const float Q_CELL_SIZE = CELL_SIZE / 4;
	const float CELL_DIAG = CELL_SIZE * SQRT_OF_2;
	const float H_CELL_DIAG = CELL_SIZE * SQRT_OF_2;

	void BresenhamsRay2D(const TArray<uint8_t>& CostFields, const FIntVector2& Goal, FIntVector2 Origin, OUT std::deque<FIntVector2>& LOS);
	void GetLos(const TArray<uint8_t>& CostFields, const FIntVector2& Cell, const FIntVector2& Goal, std::deque<FIntVector2>& Los);
	void VisitCell(const TArray<uint8_t>& CostFields, std::deque<FIntVector2>& WaveFront, const FIntVector2& Cell, float CurrentCost, const FIntVector2& Goal, OUT TArray<IntegrationField>& IntegrationFields, OUT std::deque<FIntVector2>& SecondWaveFront, bool bLosPass);
	void PropagateWave(const TArray<uint8_t>& CostFields, TArray<IntegrationField>& IntegrationFields, std::deque<FIntVector2>& WaveFront, bool bLosPass, const FIntVector2& Goal, OUT std::deque<FIntVector2>& SecondWaveFront);
	void CalculateFlowFields(TArray<IntegrationField>& IntegrationFields, OUT std::queue<int>& Sources, OUT TArray<FlowField>& FlowFields);
	void CalculateCostFields(UWorld* World, OUT TArray<uint8_t>& CostFields);

	/* Debugging Tools */

	const float LOS_FLAG_HEIGHT = PLANE_HEIGHT + 1.f;
	const float TEXT_HEIGHT = PLANE_HEIGHT + 2.f;
	const float FLOW_ARROW_HEIGHT = PLANE_HEIGHT + 50.f;

	void DrawCosts(const TArray<uint8_t>& CostFields);
	void DrawIntegration(const TArray<IntegrationField>& IntegrationFields);
	void DrawFlows(const TArray<FlowField>& FlowFields);
	void DrawLOS(FVector2D NormDir, FVector2D Origin, int Scale);
	void DrawBox(FIntVector2 At, FColor Color = FColor::Orange);
	void DrawBox(int At, FColor Color = FColor::Orange);
	void DrawCoords();
};