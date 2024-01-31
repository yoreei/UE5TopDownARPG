//#include "NavigationSystem.h"
//
//class UCrowdNavigationSystem : public UNavigationSystemV1
//{
//    GENERATED_BODY()
//
//public:
//    UCrowdNavigationSystem(const FObjectInitializer& ObjectInitializer);

//	//----------------------------------------------------------------------//
////~ Begin Public querying Interface                                                                
////----------------------------------------------------------------------//
///**
// *	Synchronously looks for a path from @fLocation to @EndLocation for agent with properties @AgentProperties. NavData actor appropriate for specified
// *	FNavAgentProperties will be found automatically
// *	@param ResultPath results are put here
// *	@param NavData optional navigation data that will be used instead of the one that would be deducted from AgentProperties
// *  @param Mode switch between normal and hierarchical path finding algorithms
// */
//	FPathFindingResult FindPathSync(const FNavAgentProperties& AgentProperties, FPathFindingQuery Query, EPathFindingMode::Type Mode = EPathFindingMode::Regular) override;
//
//	/**
//	 *	Does a simple path finding from @StartLocation to @EndLocation on specified NavData. If none passed MainNavData will be used
//	 *	Result gets placed in ResultPath
//	 *	@param NavData optional navigation data that will be used instead main navigation data
//	 *  @param Mode switch between normal and hierarchical path finding algorithms
//	 */
//	FPathFindingResult FindPathSync(FPathFindingQuery Query, EPathFindingMode::Type Mode = EPathFindingMode::Regular) override;
//
//	/**
//	 *	Asynchronously looks for a path from @StartLocation to @EndLocation for agent with properties @AgentProperties. NavData actor appropriate for specified
//	 *	FNavAgentProperties will be found automatically
//	 *	@param ResultDelegate delegate that will be called once query has been processed and finished. Will be called even if query fails - in such case see comments for delegate's params
//	 *	@param NavData optional navigation data that will be used instead of the one that would be deducted from AgentProperties
//	 *	@param PathToFill if points to an actual navigation path instance than this instance will be filled with resulting path. Otherwise a new instance will be created and
//	 *		used in call to ResultDelegate
//	 *  @param Mode switch between normal and hierarchical path finding algorithms
//	 *	@return request ID
//	 */
//	uint32 FindPathAsync(const FNavAgentProperties& AgentProperties, FPathFindingQuery Query, const FNavPathQueryDelegate& ResultDelegate, EPathFindingMode::Type Mode = EPathFindingMode::Regular) override;
//
//};