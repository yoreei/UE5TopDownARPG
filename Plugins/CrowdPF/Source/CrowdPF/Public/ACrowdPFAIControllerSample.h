// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <AIController.h>
#include "CrowdPF/Public/CrowdPF.h"
#include "ACrowdPFAIControllerSample.generated.h"

DECLARE_STATS_GROUP(TEXT("CrowdPF"), STATGROUP_CrowdPF, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("DoFlowTiles"), STAT_DoFlowTiles, STATGROUP_CrowdPF);
DECLARE_DWORD_COUNTER_STAT(TEXT("CrowdPF Pathfinding Invocations"), STAT_DoFlowTiles_DWORD, STATGROUP_CrowdPF);
/**
 * 
 */
UCLASS()
class CROWDPF_API ACrowdPFAIControllerSample : public AAIController
{
	GENERATED_BODY()

public:
	ACrowdPFAIControllerSample();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const override;

	UPROPERTY()
	class UBlackboardComponent* BlackboardComponent;

	UPROPERTY()
	class UBehaviorTreeComponent* BehaviorTreeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
	bool UseCrowdPf;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
	bool DrawDebugPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yoreei's Crowd Pathfinder")
	FCrowdPFOptions Options;
};
