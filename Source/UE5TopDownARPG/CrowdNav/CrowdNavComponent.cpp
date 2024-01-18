// Fill out your copyright notice in the Description page of Project Settings.

#include "CrowdNavComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "Engine/GameEngine.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

// Sets default values for this component's properties
UCrowdNavComponent::UCrowdNavComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UCrowdNavComponent::MoveTo(TArray<AAIController*> AIControllers, const FVector_NetQuantize& Location)
{
	for (AAIController* AIController : AIControllers)
	{
		if (IsValid(AIController) == false)
		{
			continue;
		}

		//EPathFollowingRequestResult::Type Result = AIController->MoveToLocation (
		//	Location,
		//	1.0f,   //acceptanceradius
		//	true,   //stoponoverlap
		//	false,  //bUsePathfinding
		//	true,   //bProjectDestinationToNavigation,
		//	true,   // bCanStrafe,
		//	nullptr,  // TSubclassOf< UNavigationQueryFilter > FilterClass,
		//	true    //bAllowPartialPath
		//);
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIController, Location);

		//if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("path result: %d"), (int)Result); }
		APawn* Pawn = AIController->GetPawn();
		// UE_LOG(LogTemp, Log, TEXT("Pawn: %s, path result: %d"), *(Pawn->GetName()), Result);
	}
}


// Called when the game starts
void UCrowdNavComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCrowdNavComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

