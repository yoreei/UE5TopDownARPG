# Yoreei's Tile-Flow Crowd Pathfinder


## Intro

![Alt text](image-14.png)

## Core Goals

Implement Flow Field pathfinding as described in [Emerson, Elijah. "Crowd pathfinding and steering using flow field tiles." Game AI Pro 360: Guide to Movement and Pathfinding. CRC Press, 2019. 67-76.](https://www.gameaipro.com/GameAIPro/GameAIPro_Chapter23_Crowd_Pathfinding_and_Steering_Using_Flow_Field_Tiles.pdf). This pathfinding algorithm should provide better performance for games with hundreds to thousands of units, which are often ordered to travel to a specific location, e.g. armies in RTS games.

## Assumptions
- 2D Square-tiled map

## Motivation
Flow field pathfinding should excel at many-units-single-goal scenarios, as well as maze-style maps which require a lot of turns. These are worst-case scenarios for the stock navmesh pathfinding in Unreal Engine 5, which prefers open fields and lower number of units per pathfinding request. Our objective is not to beat A* at its game, but to provide a solution to the above worst-case scenarios.

## Implementation

### Setup

![CostFields](CostFields.png)

**CalculateCostFields**

### Pathfinding

For each new goal, we perform 4 main steps:
		
**UpdateCostFields** // Check if Cost Fields are dirty & updates them

**PropagateWave**(/*bLosPass =*/ true); 

**PropagateWave**(/*bLosPass =*/ false);

**CalculateFlowFields** // The output of our algorithm

### Adaptors

Since Unreal Engine's stock PathFollowingComponent does not understand Flow Fields, we need to translate the flow fields to a Path (an array of points):

**ConvertFlowTilesToPath**

## Evaluation


## Stretch Goals
[x] Profile & Benchmark against UE5's single-track pathfinding solution
[x] Implement Automation Tests
[x] Visualize cost & direction
[x] Package as a UE Plugin

## Further Work
[] Paralelize using SIMD | GPU
[] Utilize **Morton order** for NavMap``
[] Query Recast's NavMesh instead of using LineTrace for NavMap creation
[] Dynamically add obstacles / change costs
[] Combine with Any-angle pathfinding, e.g. Theta*, Anya

## Things to Experiment With

[] Hexagons/Octagons instead of squares
[] Object Pooling for FlowTiles
[] Automatically Detect Map Sectors.
[] Greedy Eikonal: Directed Wave Propagation.

![Alt text](image-13.png)

