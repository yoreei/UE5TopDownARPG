// Fill out your copyright notice in the Description page of Project Settings.


#include "UE5TopDownARPGAIController.h"
#include "../UE5TopDownARPGCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"

// hack
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "../UE5TopDownARPG.h"
#include "Chaos/Vector.h"

// FCrowdPFModule* AUE5TopDownARPGAIController::CrowdPFModule = nullptr; // TODO remove

AUE5TopDownARPGAIController::AUE5TopDownARPGAIController()
{
  BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
  BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
}

void AUE5TopDownARPGAIController::BeginPlay()
{
    Super::BeginPlay();
    //if (CrowdPFModule != nullptr) // static already set
    //{
    //    return;
    //}
}

FPathFindingResult CustomFindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
    FPathFindingResult Res = FPathFindingResult();
    return Res;
}

void AUE5TopDownARPGAIController::OnPossess(APawn* InPawn)
{
  Super::OnPossess(InPawn);

  AUE5TopDownARPGCharacter* PossesedCharacter = Cast<AUE5TopDownARPGCharacter>(InPawn);
  if (IsValid(PossesedCharacter))
  {
    UBehaviorTree* Tree = PossesedCharacter->GetBehaviorTree();
    if (IsValid(Tree))
    {
      BlackboardComponent->InitializeBlackboard(*Tree->GetBlackboardAsset());
      BehaviorTreeComponent->StartTree(*Tree);
    }
  }

  UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
  if (IsValid(NavSys) == false)
  {
      UE_LOG(LogUE5TopDownARPG, Log, TEXT("No navsys"));
  }

  // # Construct FPathfindingQuery with my own NavData and pass that
  //

  // # Can't call RegisterNavData like this because it's protected
  //ANavigationData = ACrowdNavData
  //NavSys->RegisterNavData(ANavigationData * NavData);

  // # probe result: navData was not set at execution, but was set afterwards:
  // ANavigationData* navData1 = NavSys->GetDefaultNavDataInstance(FNavigationSystem::ECreateIfMissing::DontCreate);
  // INavigationDataInterface* navData2 = NavSys->GetMainNavData();
  //ANavigationData& navData3 = NavSys->GetMainNavDataChecked();

  // todo CustomFindPath
  UE_LOG(LogUE5TopDownARPG, Log, TEXT("No navsys"));

}

void AUE5TopDownARPGAIController::OnUnPossess()
{
  Super::OnUnPossess();

  BehaviorTreeComponent->StopTree();
}



void AUE5TopDownARPGAIController::FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const
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
            CrowdPFModule->DoFlowTiles(MoveRequest.GetGoalActor(), OutPath);
            
                //extras
            //OutPath->SetNavigationDataUsed(Query.NavData.GetEvenIfUnreachable());
            //OutPath->SetGoalActorObservation(*MoveRequest.GetGoalActor(), 1000.f);
            //OutPath->SetQuerier(this);
            //OutPath->SetTimeStamp(GetWorld()->GetTimeSeconds());
            //OutPath->SetGoalActorObservation(*MoveRequest.GetGoalActor(), 100.0f);
        }
        else
        {
            UE_LOG(LogUE5TopDownARPG, Error, TEXT("No CrowdPFModule"));
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
        UE_LOG(LogUE5TopDownARPG, Log, TEXT("gotcha drew location %s"), loc2);
    }
}
