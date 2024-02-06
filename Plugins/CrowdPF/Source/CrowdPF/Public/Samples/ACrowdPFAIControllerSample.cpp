// Fill out your copyright notice in the Description page of Project Settings.


#include "ACrowdPFAIControllerSample.h"

// hack
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "CrowdPF/Public/CrowdPF.h"
//#include "Chaos/Vector.h"

// FCrowdPFModule* ACrowdPFAIControllerSample::CrowdPFModule = nullptr; // TODO remove



ACrowdPFAIControllerSample::ACrowdPFAIControllerSample()
{
}

void ACrowdPFAIControllerSample::BeginPlay()
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
void ACrowdPFAIControllerSample::FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const
{
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("STAT_CrowdPF_FindPathForMoveRequest"), STAT_CrowdPF_FindPathForMoveRequest, STATGROUP_CrowdPF);
    // Super::FindPathForMoveRequest(MoveRequest, Query, OutPath); return; // uncomment to enable stock pathfinding

    FCrowdPFModule* CrowdPFModule = FModuleManager::LoadModulePtr<FCrowdPFModule>("CrowdPF");
    if (CrowdPFModule)
    {
        CrowdPFModule->DoFlowTiles(GetPawn()->GetActorLocation(), MoveRequest.GetGoalActor()->GetActorLocation(), OutPath);
    }
    else
    {
        UE_LOG(LogCrowdPF, Error, TEXT("No CrowdPFModule"));
        return;
    }

    // Uncomment to enable debugg drawing
    //UWorld* pWorld = GetWorld();
    //ensure(pWorld);
    //const TArray<FNavPathPoint>& PathPoints = OutPath->GetPathPoints();
    //for (const FNavPathPoint& Point : PathPoints)
    //{
    //    Chaos::TVector<double, 3> Loc = Point.Location;
    //    FVector loc2 = Point.Location;
    //    DrawDebugBox(pWorld, loc2, {20.f, 20.f, 20.f}, FColor::Black, true, -1.f, 0, 5.f);
    //    UE_LOG(LogCrowdPF, Log, TEXT("Drawing Path Box at: %s"), *loc2.ToString());
    //}
}
