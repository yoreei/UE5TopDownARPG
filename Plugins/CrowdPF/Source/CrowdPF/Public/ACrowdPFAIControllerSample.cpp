// Fill out your copyright notice in the Description page of Project Settings.


#include "ACrowdPFAIControllerSample.h"

// hack
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
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
    // UE_LOG(LogCrowdPF, Log, TEXT("MoveToActor Result %s"), Result);
}

FPathFindingResult CustomFindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
    FPathFindingResult Res = FPathFindingResult();
    return Res;
}

void ACrowdPFAIControllerSample::OnPossess(APawn* InPawn)
{
  Super::OnPossess(InPawn);


}

void ACrowdPFAIControllerSample::OnUnPossess()
{
  Super::OnUnPossess();
}



void ACrowdPFAIControllerSample::FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const
{
    if(!UseCrowdPf)
    {
            Super::FindPathForMoveRequest(MoveRequest, Query, OutPath);
    }
    else
    {
        FCrowdPFModule* CrowdPFModule = FModuleManager::LoadModulePtr<FCrowdPFModule>("CrowdPF");
        if (CrowdPFModule)
        {
            UWorld* pWorld = GetWorld();
            ensure(pWorld);
            CrowdPFModule->Init(pWorld);
            CrowdPFModule->DoFlowTiles(GetPawn()->GetActorLocation(), MoveRequest.GetGoalActor()->GetActorLocation(), OutPath);
            //TArray<FVector> Points{ {1940.f,350.f,60.f},
        }
        else
        {
            UE_LOG(LogCrowdPF, Error, TEXT("No CrowdPFModule"));
            return;
        }
    }

    if (!DrawDebugPath)
    {
        return;
    }

    auto DrawBox = [&](FIntVector2 At, FColor Color) // TODO find better place
        {
            UWorld* pWorld = GetWorld();
            ensure(pWorld);
            FVector Center{
                At.X * 100.f + 50.f,
                At.Y * 100.f + 50.f,
                60.f
            };
            FVector Extent = FVector(50.f, 50.f, 50.f);
            DrawDebugBox(pWorld, Center, Extent, Color, true);
            //UE_LOG(LogCrowdPF, Log, TEXT("Draw Box: Center = %s; Extent = %s"), *Center.ToString(), *Extent.ToString());
        };
    UWorld* pWorld = GetWorld();
    ensure(pWorld);
    const TArray<FNavPathPoint>& PathPoints = OutPath->GetPathPoints();
    for (const FNavPathPoint& Point : PathPoints)
    {
        Chaos::TVector<double, 3> Loc = Point.Location;
        FVector loc2 = Point.Location;
        DrawDebugBox(pWorld, loc2, {50.f, 50.f, 50.f}, FColor::Black, true, -1.f, 0, 10.f);
        UE_LOG(LogCrowdPF, Log, TEXT("gotcha drew location %s"), loc2);
    }
}
