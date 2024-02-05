* Day Tasks
1. feb: POC character movement
2. feb: setup benchmark bed & Char movement from my pathfinder
3. feb: make anki for test & fixes
4. feb: cram for test & benchmark
5. feb: do test & clean the code
6. feb: prepare presentation & present

optional: Test Automation?



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

-----
                  //extras
            //OutPath->SetNavigationDataUsed(Query.NavData.GetEvenIfUnreachable());
            //OutPath->SetGoalActorObservation(*MoveRequest.GetGoalActor(), 1000.f);
            //OutPath->SetQuerier(this);
            //OutPath->SetTimeStamp(GetWorld()->GetTimeSeconds());
            //OutPath->SetGoalActorObservation(*MoveRequest.GetGoalActor(), 100.0f);



23.6 The Integrator
Input: the request’s cost field data as well as the request’s “initial wave front” as input. The initial
wave front is a list of goal locations, each having a predetermined integrated cost value.
The integrator takes the initial wave front and integrates it outward using an Eikonal
equation [Ki Jeong 08]. Visualize the effect of touching still water, creating a rippling wave
moving across the water surface. The Integrator’s active wave front behaves similarly in
how it moves across the pathable surface while setting larger and larger integrated cost
values into the integration field. It repeats this process until the active wave front stops
moving by hitting a wall or the sector’s borders. 

23.6.1	 	Integration Step 1: Reset the Integration Field
The integrator’s first step is to reset its integration field values and apply the initial goal
wave front. If the requested flow field has the final 1 × 1 goal, then its initial goal wave front
is a single 1 × 1 location with a zero integrated cost value. However, if the flow field request
is from a 10 × 1 or 1 × 10 portal, then there will be ten goal locations with ten different
integrated cost goals.
23.6.2	 	Integration Step 2: Line Of Sight Pass
If we are integrating from the actual path goal, then we first run a line of sight (LOS) pass.
We do this to have the highest quality flow directions near the path goal. When an agent is
within the LOS it can ignore the Flow field results altogether and just steer toward the exact
goal position. Without the LOS pass, you can have diamond-shaped flow directions around
your goal due to the integrator only looking at the four up, down, left, and right neighbors.

To integrate LOS you have the initial goal wave front integrate out as you normally
would, but, instead of comparing the cost field neighbor costs to determine the integrated
cost, just increment the wave front cost by one as you move the wave front while flagging
the location as “Has Line of Sight.” Do this until the wave front hits something with any
cost greater than one.

Once we hit something with a cost greater than one, we need to determine if the location is an LOS corner. We do this by looking at the location’s neighbors. If one side has a
cost greater than one while the other side does not, we have an LOS corner.
For all LOS corners we build out a 2D line starting at the grid square’s outer edge position, in a direction away from the goal. Follow this line across the grid using Bresenham’s
line algorithm, flagging each grid location as “Wave Front Blocked” and putting the location in a second active wave front list to be used later, by the cost integration pass. By
marking each location as “Wave Front Blocked” the LOS integration wave front will stop
along the line that marks the edge of what is visible by the goal.

Continue moving the LOS pass wave front outward until it stops moving by hitting a
wall or a location that has the “Wave Front Blocked” flag. Other than the time spent using
Bresenham’s line algorithm, the LOS first pass is very cheap because it does not look at
neighboring cost values. The wave front just sets flags and occasionally detects corners and
iterates over a line.

Figure  23.2 shows the results of a LOS pass. Each clear white grid square has been
flagged as “Has Line Of Sight.” Each LOS corner has a line where each grid square that
overlaps that line is flagged as “Wave Front Blocked.”

23.6.3	 	Integration Step 3: Cost Integration Pass
We are now ready for cost field integration. As with the LOS pass, we start with the active
wave front list. This active wave front comes from the list of “Wave Front Blocked” locations from the previous LOS pass. In this way we only integrate locations that are not
visible from the goal.

We integrate this wave front out until it stops moving by hitting a wall or a sector
border. At each grid location we compute the integrated cost by adding the cheapest cost
field and integrated cost field’s up, down, left, or right neighbors together. Then repeat this
Eikonal equation process again and again, moving the wave front outward toward each
location’s un-integrated, non-walled neighbors.
During integration, look out for overlapping previously integrated results because of small
cost differences. To fix this costly behavior, make sure your wave front stops when it hits

previously integrated results, unless you really have a significant difference in integrated
costs. If you don’t do this, you risk having wave fronts bounce back and forth, eating up results
when it’s not necessary. 

23.6.4	 	Integration Step 4: Flow Field Pass
We are now ready to build a flow field from our newly created integration field. This is
done by iterating over each integrated cost location and either writing out the LOS flag or
comparing all eight NW, N, NE, E, SE, S, SW, W neighbors to determine the “cheapest”
direction we should take for that location.

23.12	 	Steering with Flow Fields
The agent should look for an LOS flag to steer to its goal;
otherwise, it should use the specified flow field direction. 
    
    
    
    # Code
    
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);

	if (IsValid(NavSys) == false)
	{
		return;
	}
	ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();

	if (IsValid(NavData) == false)
	{
		return;
	}
            
            bool bRaycastObstructed = NavData->Raycast(RayStart, RayEnd, HitLocation, FSharedConstNavQueryFilter()); // true if line from RayStart to RayEnd is obstructed