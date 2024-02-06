// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <AIController.h>
#include "CrowdPF/Public/CrowdPF.h"
#include "ACrowdPFAIController.generated.h"

/**
 * 
 */
UCLASS()
class ACrowdPFAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACrowdPFAIController();

protected:

	virtual void BeginPlay() override;
	virtual void FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const override;
};
