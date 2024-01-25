// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FindGoal.h"

#include "../UE5TopDownARPGCharacter.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "NavigationPath.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

EBTNodeResult::Type UBTTask_FindGoal::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = Cast<AAIController>(OwnerComp.GetOwner());
    if (IsValid(AIController) == false)
    {
        return EBTNodeResult::Failed;
    }

    APawn* PossesedPawn = AIController->GetPawn();
    if (IsValid(PossesedPawn) == false)
    {
        return EBTNodeResult::Failed;
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (IsValid(NavSys) == false)
    {
        return EBTNodeResult::Failed;
    }

    TArray<AActor*> FoundActors;
    //UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUE5TopDownARPGCharacter::StaticClass(), FoundActors);

    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Goal", FoundActors);
    check(FoundActors.Num() == 1 && IsValid(FoundActors[0]));
    AActor* GoalActor = FoundActors[0];
    FVector GoalVector = GoalActor->GetActorLocation();

    UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(GetWorld(), PossesedPawn->GetActorLocation(), GoalActor->GetActorLocation());
    if (Path->IsValid() && Path->IsPartial() == false)
    {
        UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
        if (IsValid(BlackboardComponent) == false)
        {
            return EBTNodeResult::Failed;
        }

        BlackboardComponent->SetValueAsObject(FName("Goal"), GoalActor);
        return EBTNodeResult::Succeeded;
    }
    return EBTNodeResult::Failed;
}