// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <memory>
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCrowdPF, Log, All);

const float CELL_SIZE = 100.f;
const float PLANE_HEIGHT = 51.f;
const float bDRAW_COSTS = false;
const float bDRAW_INTEGRATION = true;
const FVector COST_TRACE_Y_START = { 0.f, 0.f, 250.f };
const FVector COST_TRACE_DIRECTION{ 0.f, 0.f, -100 };
const FIntVector2 GRIDSIZE{ 30, 30 };
const float SIGNIFICANT_COST_REDUCTION = 0.75f; // 25% reduction

class CROWDPF_API FCrowdPFModule : public IModuleInterface
{
public:
    FCrowdPFModule();
    ~FCrowdPFModule();
    FCrowdPFModule(FCrowdPFModule&&);
    FCrowdPFModule(const FCrowdPFModule&) = delete;
    FCrowdPFModule& operator=(FCrowdPFModule&&);
    FCrowdPFModule& operator=(const FCrowdPFModule&) = delete;

    /** Public Interface of FCrowdPFModule */
    void Init(UWorld* _World); // TODO refactor into own class (factory)
    void DoFlowTiles(const AActor* GoalActor, FNavPathSharedPtr& OutPath);

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
    class Impl;
    TUniquePtr<Impl> ModuleImplementation = TUniquePtr<Impl>();
};
