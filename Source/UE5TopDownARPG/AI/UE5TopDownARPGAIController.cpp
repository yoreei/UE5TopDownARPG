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
