// Fill out your copyright notice in the Description page of Project Settings.

#include "ACrowdPFAIController.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "CrowdPF/Public/CrowdPF.h"
#include "UE5TopDownARPGGameMode.h"

ACrowdPFAIController::ACrowdPFAIController()
{
}

void ACrowdPFAIController::BeginPlay()
{
    Super::BeginPlay();

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Goal", FoundActors);
    check(FoundActors.Num() == 1 && IsValid(FoundActors[0]));
    AActor* GoalActor = FoundActors[0];

    EPathFollowingRequestResult::Type Result = MoveToActor(GoalActor, 100.f, false, /*bUsePathFinding*/ true, false, NULL, /* bAllowPartialPaths */ true);
}

/*
    * Profiling procedure
    * In console, write:
    trace.enable counters,cpu,stats
    stat NamedEvents
    trace.start
    <run benchmark>
    trace.stop
*/
void ACrowdPFAIController::FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const
{
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("STAT_CrowdPF_FindPathForMoveRequest"), STAT_CrowdPF_FindPathForMoveRequest, STATGROUP_CrowdPF);

    AUE5TopDownARPGGameMode* GameMode = Cast<AUE5TopDownARPGGameMode>(GetWorld()->GetAuthGameMode());
    if (!ensure(GameMode)) { return; }

    if(!GameMode->UseCrowdPf)
    {
        Super::FindPathForMoveRequest(MoveRequest, Query, OutPath);
    }
    else
    {
        FCrowdPFModule* CrowdPFModule = FModuleManager::LoadModulePtr<FCrowdPFModule>("CrowdPF");
        if (!ensure(GameMode)) { return; }

        CrowdPFModule->DoFlowTiles(GetPawn()->GetActorLocation(), MoveRequest.GetGoalActor()->GetActorLocation(), OutPath);

    }

    if (!GameMode->DrawDebugPath)
    {
        return;
    }

    UWorld* pWorld = GetWorld();
    ensure(pWorld);
    const TArray<FNavPathPoint>& PathPoints = OutPath->GetPathPoints();
    for (const FNavPathPoint& Point : PathPoints)
    {
        Chaos::TVector<double, 3> Loc = Point.Location;
        FVector loc2 = Point.Location;
        DrawDebugBox(pWorld, loc2, {20.f, 20.f, 20.f}, FColor::Black, true, -1.f, 0, 5.f);
    }
}
