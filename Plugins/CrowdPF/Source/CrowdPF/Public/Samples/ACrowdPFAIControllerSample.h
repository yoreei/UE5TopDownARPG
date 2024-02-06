// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <AIController.h>
#include "CrowdPF/Public/CrowdPF.h"
#include "ACrowdPFAIControllerSample.generated.h"

/**
 * This is a sample file, meant to be copied to your game files if you plan to use the Yoreei's Flow Tile Crowd Pathfinder
 */
UCLASS()
class ACrowdPFAIControllerSample : public AAIController
{
	GENERATED_BODY()

public:
	ACrowdPFAIControllerSample();

protected:

	virtual void BeginPlay() override;
	virtual void FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const override;
};
