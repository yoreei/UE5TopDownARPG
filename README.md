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

![PropagateWave](LosPass.png)
**PropagateWave**(/*bLosPass =*/ true); 

![PropagateWave](PropagateWaveFront.png)
**PropagateWave**(/*bLosPass =*/ false);

![FlowFields](FlowFields.png)
**CalculateFlowFields** // The output of our algorithm

### Adaptors

Since Unreal Engine's stock PathFollowingComponent does not understand Flow Fields, we need to translate the flow fields to a Path (an array of points):

**ConvertFlowTilesToPath**

## Evaluation - Pathfinder

All evaluations were run 5 times to ensure statistical stability on the following configuration:

```
Processor	13th Gen Intel(R) Core(TM) i7-13700H, 2400 Mhz, 14 Core(s), 20 Logical Processor(s)

Total Physical Memory	63,6 GB

Microsoft Visual Studio Community 2022 (64-bit) Version 17.8.3

Unreal Engine 5.3.2
```


### 1 Unit, Thick Maze Map

| Yoreei's Flow Tile Crowd Pathfinder | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 5 | **1.1 ms** | 85.8 µs
STAT_CrowdPF_CalculateFlowFields | 5 | 633.8 µs | 633.8 µs
STAT_CrowdPF_PropagateWave | 10 | 373 µs | 373 µs
STAT_CrowdPF_ConvertFlowTilesToPath | 5 | 42.4 µs | 42.4 µs

| Stock Unreal Pathfinder  | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 5 | **325.4 µs** | 6.6 µs



### 50 Units, Thick maze Map

| Yoreei's Flow Tile Crowd Pathfinder | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 250 | **1.7 ms** | 160.3 µs
STAT_CrowdPF_ConvertFlowTilesToPath | 250 | 662.4 µs | 662.4 µs
STAT_CrowdPF_CalculateFlowFields | 5 | 523 µs | 523 µs
STAT_CrowdPF_PropagateWave | 10 | 325.9 µs | 325.9 µs


| Stock Unreal Pathfinder  | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 250 | **2.9 ms** | 48.1 µs


### 200 Units, Thick Maze Map

| Yoreei's Flow Tile Crowd Pathfinder | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 1,576 | **6 ms** | 754.6 µs
STAT_CrowdPF_ConvertFlowTilesToPath | 1,576 | 4.1 ms | 4.1 ms
STAT_CrowdPF_CalculateFlowFields | 8 | 723.3 µs | 723.3 µs
STAT_CrowdPF_PropagateWave | 16 | 400.4 µs | 400.4 µs


| Stock Unreal Pathfinder  | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_CalculateCostFields | 5 | **5.1 ms** | 282.2 µs

### 1 Unit, Simple Obstacle Map

| Yoreei's Flow Tile Crowd Pathfinder | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 5 | **991.6 µs** | 78.8 µs
STAT_CrowdPF_PropagateWave | 10 | 488.7 µs | 488.7 µs
STAT_CrowdPF_CalculateFlowFields | 5 | 396.4 µs | 396.4 µs
STAT_CrowdPF_ConvertFlowTilesToPath | 5 | 27.7 µs | 27.7 µs


| Stock Unreal Pathfinder  | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 5 | **342.2 µs** | 8.3 µs

As expected, the stock pathfinder is significantly faster for simple maps with small amounts of units

### 50 Units, Simple Obstacle Map

| Yoreei's Flow Tile Crowd Pathfinder | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 245 | **982.5 µs** | 145.6 µs
STAT_CrowdPF_PropagateWave | 10 | 315 µs | 315 µs
STAT_CrowdPF_CalculateFlowFields | 5 | 283.2 µs | 283.2 µs
STAT_CrowdPF_ConvertFlowTilesToPath | 245 | 238.7 µs | 238.7 µs


| Stock Unreal Pathfinder  | Count | Incl |  Excl | 
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 245 | **2.1 ms** | 40 µs

At 50 units, the flow tile pathfinder is already 2x faster than the stock pathfinder. Notice that the functions **PropagateWave** and **CalculateFlowFields**, which are our main computational costs, do not scale with unit volume. Only our adapter function **ConvertFlowTilesToPath** scales, which can be eliminated in future work (see below).

### 200 Units, Simple Obstacle Map

| Yoreei's Flow Tile Crowd Pathfinder                                 | Count | Incl     |  Excl      |
|--------------------------------------|-------|----------|----------|
STAT_CrowdPF_FindPathForMoveRequest | 995 | **1.6 ms** | 355.9 µs
STAT_CrowdPF_ConvertFlowTilesToPath | 995 | 759.5 µs | 759.5 µs
STAT_CrowdPF_PropagateWave | 10 | 254.3 µs | 254.3 µs
STAT_CrowdPF_CalculateFlowFields | 5 | 240.6 µs | 240.6 µs


| Stock Unreal Pathfinder                                 | Count | Incl     | Excl     |
|--------------------------------------|-------|----------|----------|
| STAT_CrowdPF_FindPathForMoveRequest  | 995   | **6.3 ms** | 129.6 µs   |



## Evaluation - CostFields

| STAT_CrowdPF_CalculateCostFields  | Count | Incl     |  Excl      |
Thick Maze Map | 5 | 8.4 ms | 446 µs
Simple Obstacle Map | 5 | 4.7 ms | 301.7 µs

## Stretch Goals
- [x] Profile & Benchmark against UE5's single-track pathfinding solution
- [x] Implement Automation Tests
- [x] Visualize cost & direction
- [x] Package as a UE Plugin

## Further Work
- [ ] Paralelize using SIMD | GPU
- [ ] Utilize **Morton order** for NavMap``
- [ ] Query Recast's NavMesh instead of using LineTrace for NavMap creation
- [ ] Dynamically add obstacles / change costs
- [ ] Combine with Any-angle pathfinding, e.g. Theta*, Anya

## Things to Experiment With

- [ ] Hexagons/Octagons instead of squares
- [ ] Object Pooling for FlowTiles
- [ ] Automatically Detect Map Sectors.
- [ ] Greedy Eikonal: Directed Wave Propagation.
