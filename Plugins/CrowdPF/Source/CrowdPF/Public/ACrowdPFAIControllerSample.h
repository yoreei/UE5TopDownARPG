// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <AIController.h>
#include "CrowdPF/Public/CrowdPF.h"
#include "ACrowdPFAIControllerSample.generated.h"

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
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const override;

	UPROPERTY()
	class UBlackboardComponent* BlackboardComponent;

	UPROPERTY()
	class UBehaviorTreeComponent* BehaviorTreeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CrowdPf)
	bool UseCrowdPf;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CrowdPf)
	bool DrawDebugPath;
};