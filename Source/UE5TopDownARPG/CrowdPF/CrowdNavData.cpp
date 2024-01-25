#include "CrowdNavData.h"
//#include "YourNavDataGenerator.h"

ACrowdNavData::ACrowdNavData(const FObjectInitializer& ObjectInitializer)
{
}

ACrowdNavData::~ACrowdNavData()
{
    // Clean up your navigation data here
}

void ACrowdNavData::PostInitProperties()
{
    Super::PostInitProperties();
    // Additional post-init logic here
}

void ACrowdNavData::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    // Additional post-initialize components logic here
}

void ACrowdNavData::PostLoad()
{
    Super::PostLoad();
    // Additional post-load logic here
}

#if WITH_EDITOR
void ACrowdNavData::RerunConstructionScripts()
{
    Super::RerunConstructionScripts();
    // Additional editor-time logic here
}

void ACrowdNavData::PostEditUndo()
{
    Super::PostEditUndo();
    // Additional post-edit undo logic here
}
#endif // WITH_EDITOR

void ACrowdNavData::Destroyed()
{
    Super::Destroyed();
    // Additional destruction logic here
}

void ACrowdNavData::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    // Additional end play logic here
}

void ACrowdNavData::CleanUp()
{
    Super::CleanUp();
    // Additional clean up logic here
}

void ACrowdNavData::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
    Super::ApplyWorldOffset(InOffset, bWorldShift);
    // Additional world offset logic here
}

void ACrowdNavData::CleanUpAndMarkPendingKill()
{
    Super::CleanUpAndMarkPendingKill();
    // Additional clean up and mark pending kill logic here
}

void ACrowdNavData::OnRegistered()
{
    Super::OnRegistered();
    // Additional on registered logic here
}

void ACrowdNavData::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
    Super::TickActor(DeltaTime, TickType, ThisTickFunction);
    // Additional tick actor logic here
}

bool ACrowdNavData::NeedsRebuild() const
{
    // Implement your logic to determine if a rebuild is needed
    return false;
}

bool ACrowdNavData::SupportsRuntimeGeneration() const
{
    // Implement your logic for supporting runtime generation
    return false;
}

bool ACrowdNavData::SupportsStreaming() const
{
    // Implement your logic for supporting streaming
    return false;
}

void ACrowdNavData::OnNavigationBoundsChanged()
{
    Super::OnNavigationBoundsChanged();
    // Additional logic for when navigation bounds change
}

void ACrowdNavData::ConditionalConstructGenerator()
{
    Super::ConditionalConstructGenerator();
    // Additional logic for conditionally constructing the generator
}

void ACrowdNavData::RebuildAll()
{
    Super::RebuildAll();
    // Additional logic for rebuilding all navigation data
}

void ACrowdNavData::EnsureBuildCompletion()
{
    Super::EnsureBuildCompletion();
    // Additional logic to ensure build completion
}

void ACrowdNavData::CancelBuild()
{
    Super::CancelBuild();
    // Additional logic to cancel build
}

void ACrowdNavData::TickAsyncBuild(float DeltaSeconds)
{
    Super::TickAsyncBuild(DeltaSeconds);
    // Additional logic for async build tick
}

void ACrowdNavData::SetRebuildingSuspended(const bool bNewSuspend)
{
    Super::SetRebuildingSuspended(bNewSuspend);
    // Additional logic for setting rebuilding suspended
}

bool ACrowdNavData::ProjectPoint(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, FSharedConstNavQueryFilter Filter, const UObject* Querier) const
{
    // Implement your point projection logic
    return false;
}

uint32 ACrowdNavData::LogMemUsed() const
{
    // Implement your memory usage logging logic
    return 0;
}

void ACrowdNavData::SetConfig(const FNavDataConfig& Src)
{

}

bool ACrowdNavData::DoesSupportAgent(const FNavAgentProperties& AgentProps) const
{
    return true;
}

void ACrowdNavData::FillNavigationDataChunkActor(const FBox& QueryBounds, class ANavigationDataChunkActor& DataChunkActor, FBox& OutTilesBounds) const
{

}