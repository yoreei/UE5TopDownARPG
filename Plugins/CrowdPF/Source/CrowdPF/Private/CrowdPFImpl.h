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
	uint8_t WaveFrontBlocked : 1; // TODO use bitset?
	uint8_t LOS : 1;
};

class FCrowdPFModule::Impl {
public:
	void DoFlowTiles(const FVector& WorldOrigin, const FVector& WorldGoal, OUT FNavPathSharedPtr& OutPath);
	void SetDebugDraw(bool _bDebugDraw);
	void Init(UWorld* _World, FCrowdPFOptions _Options);
private:

	UWorld* pWorld;
	const float SQRT_OF_2 = FMath::Sqrt(2.f);
	FCrowdPFOptions Options;

	bool IsWall(const FIntVector2& Cell) const;

	void BresenhamsRay2D(const FIntVector2& Goal, FIntVector2 Origin, OUT std::deque<FIntVector2>& LOS);
	void GetLos(const FIntVector2& Cell, const FIntVector2& Goal, std::deque<FIntVector2>& Los);
	void VisitCell(std::deque<FIntVector2>& WaveFront, const FIntVector2& Cell, float CurrentCost, const FIntVector2& Goal, OUT std::deque<FIntVector2>& SecondWaveFront, bool bLosPass);
	void PropagateWave(std::deque<FIntVector2>& WaveFront, bool bLosPass, const FIntVector2& Goal, OUT std::deque<FIntVector2>& SecondWaveFront);
	void ConvertFlowTilesToPath(const FVector& WorldOrigin, const FVector& WorldGoal, FNavPathSharedPtr& OutPath);
	void CalculateFlowFields();
	void CalculateCostFields();
	void SetNeedToRecalculate(const bool bValue);
	bool GetNeedToRecalculate(const FIntVector2& Goal);

	/* Data */
	TArray<FlowField> FlowFields;
	TArray<uint8_t> CostFields;
	TArray<IntegrationField> IntegrationFields;
	bool bNeedToRecalculate = true;

	void DrawCosts();
	void DrawIntegration();
	void DrawFlows();
	void DrawLOS(FVector2D NormDir, FVector2D Origin, int Scale);
	void DrawBox(FIntVector2 At, FColor Color = FColor::Orange);
	void DrawBox(int At, FColor Color = FColor::Orange);
	void DrawCoords();
	const int ApplyDir(const int Idx, const FIntVector2& Dir);
	bool IsInGrid(int X, int Y);
	bool IsInGrid(const FIntVector2& Cell);
	bool IsInGrid(int Idx);
	FIntVector2 ToFIntVector2(int LinearIdx);
	FIntVector2 WorldVectToGridVect(const FVector& Vect);
	FVector GridVectToWorldVect(const FIntVector2& Vect);
	FIntVector2 ToFIntVector2(FVector Vect);
	int ToLinearIdx(FIntVector2 IntVector2);
	FVector2D ToVector2D(const FIntVector2& IntVector2);
	bool IsValidIdx(int Idx);
	FVector addZ(FVector2D Vect, float Z);
};
