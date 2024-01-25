// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavigationData.h"
#include "Engine/EngineTypes.h"
#include "Math/Color.h"
//#include "AI/Navigation/NavigationData.h"
#include "CrowdNavData.generated.h"

namespace EEndPlayReason {
    enum Type;
}

/**
 * 
 */
UCLASS(BLUEPRINTABLE)
class UE5TOPDOWNARPG_API ACrowdNavData : public ANavigationData
{
	GENERATED_BODY()
	
class FYourNavDataGenerator; // Forward declaration of your custom nav data generator
class FNavigationDirtyArea;
class FPathFindingQueryData;

public:
    // Constructor
    ACrowdNavData(const FObjectInitializer& ObjectInitializer);

    // Destructor
    virtual ~ACrowdNavData();

    //----------------------------------------------------------------------//
    // Life cycle
    //----------------------------------------------------------------------//

    virtual void PostInitProperties() override;
    virtual void PostInitializeComponents() override;
    virtual void PostLoad() override;
#if WITH_EDITOR
    virtual void RerunConstructionScripts() override;
    virtual void PostEditUndo() override;
#endif // WITH_EDITOR
    virtual void Destroyed() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void CleanUp() override;
    virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

public:
    virtual void CleanUpAndMarkPendingKill() override;
    virtual void OnRegistered() override;

    virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

    virtual bool NeedsRebuild() const override;
    virtual bool SupportsRuntimeGeneration() const override;
    virtual bool SupportsStreaming() const override;
    virtual void OnNavigationBoundsChanged() override;

    //----------------------------------------------------------------------//
    // Generation & data access
    //----------------------------------------------------------------------//

    virtual void SetConfig(const FNavDataConfig& Src) override;
    virtual bool DoesSupportAgent(const FNavAgentProperties& AgentProps) const override;
    virtual void FillNavigationDataChunkActor(const FBox& QueryBounds, class ANavigationDataChunkActor& DataChunkActor, FBox& OutTilesBounds) const override;
    virtual void ConditionalConstructGenerator() override;
    virtual void RebuildAll() override;
    virtual void EnsureBuildCompletion() override;
    virtual void CancelBuild() override;
    virtual void TickAsyncBuild(float DeltaSeconds) override;
    virtual void SetRebuildingSuspended(const bool bNewSuspend) override;

    //----------------------------------------------------------------------//
    // Querying
    //----------------------------------------------------------------------//

    virtual bool ProjectPoint(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, FSharedConstNavQueryFilter Filter = NULL, const UObject* Querier = NULL) const override;

    //----------------------------------------------------------------------//
    // Debug
    //----------------------------------------------------------------------//

    virtual uint32 LogMemUsed() const override;

    //----------------------------------------------------------------------//
    // Custom navigation data generator
    //----------------------------------------------------------------------//

protected:
    // Your custom nav data generator
    TSharedPtr<FYourNavDataGenerator, ESPMode::ThreadSafe> YourNavDataGenerator;


};

