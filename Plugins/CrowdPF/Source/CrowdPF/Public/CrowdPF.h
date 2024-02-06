// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <memory>
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/ObjectMacros.h"
#include "Math/Vector.h"
#include "CrowdPF.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogCrowdPF, Log, All);
DECLARE_STATS_GROUP(TEXT("CrowdPF"), STATGROUP_CrowdPF, STATCAT_Advanced);


const float SQRT_OF_2 = FMath::Sqrt(2.f);

USTRUCT(BlueprintType)
struct CROWDPF_API FCrowdPFOptions {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    float CellSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    float PlaneHeight = 51.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    FVector CostTraceYStart { 0.f, 0.f, 250.f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    FVector CostTraceDirection { 0.f, 0.f, -100 };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    float SignificantCostReduction = 0.75f; // 25% reduction

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    bool bDebugDraw = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    bool bDrawCoords = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    bool bDrawCosts = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    bool bDrawIntegration = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
    bool bDrawFlows = true;

    float LosFlagHeight() { return PlaneHeight + 1.f; };
    float TextHeight() { return PlaneHeight + 2.f; };
    float FlowArrowHeight() { return PlaneHeight + 50.f; };
    float HCellSize() { return CellSize / 2; };
    float QCellSize() { return CellSize / 4; };
    float CellDiag() { return CellSize * SQRT_OF_2; };
    float HCellDiag() { return CellSize * SQRT_OF_2; };
};

class CROWDPF_API FCrowdPFModule : public IModuleInterface
{
public:
    void OnEndPIE(bool bIsSimulating);
    FCrowdPFModule();
    ~FCrowdPFModule();
    FCrowdPFModule(FCrowdPFModule&&);
    FCrowdPFModule(const FCrowdPFModule&) = delete;
    FCrowdPFModule& operator=(FCrowdPFModule&&);
    FCrowdPFModule& operator=(const FCrowdPFModule&) = delete;

    /** Public Interface of FCrowdPFModule */
    void Init(UWorld* _World, FCrowdPFOptions Options); // TODO refactor into own class (factory)
    void DoFlowTiles(const FVector& WorldOrigin, const FVector& WorldGoal, OUT FNavPathSharedPtr& OutPath);

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;


private:
    class Impl;
    TUniquePtr<Impl> ModuleImplementation = TUniquePtr<Impl>(); // TODO optimize: Use MakeUnique
};
