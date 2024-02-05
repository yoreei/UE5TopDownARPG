#  Crowd Pathfinding Using Flow Field Tiles
> A Project Proposal for Game Engine Architecture With Unreal Engine 5

## Intro

![Alt text](image-14.png)

## Core Goals

Implement Flow Field pathfinding as described in [Emerson, Elijah. "Crowd pathfinding and steering using flow field tiles." Game AI Pro 360: Guide to Movement and Pathfinding. CRC Press, 2019. 67-76.](https://www.gameaipro.com/GameAIPro/GameAIPro_Chapter23_Crowd_Pathfinding_and_Steering_Using_Flow_Field_Tiles.pdf). This pathfinding algorithm should provide better performance for games with hundreds to thousands of units, which are often ordered to travel to a specific location, e.g. armies in RTS games.

## Assumptions
- Map is 2D from navigation point of view
- Single-section: Will not use multiple sections with portals as described in Emerson's publication
- Characters try to go to a location defined by an object placable from the Editor. 
- Navigation Grid Creation Method: Use vertical line traces to probe the map for impassable terrain.

### Pseudocode:

```
struct NavMap {
    2d aray
};

class CrowdPF{
    map<UWorld, NavMap>
};

void CrowdPF::BuildNavMap(unsigned int TileSize, vector<?>WalkableMeshes, UWorld*)
{
    int n = MapWidth / TileSize;
    int m = MapHeight / TileSize;
    // create n*m NavMap for this world;

    for 0 to n
    {
        for 0 to m
        {
            HitResult = Probe map with **UWorld::LineTraceSingleByChannel** using vector perpendicular to the ground
            if (HitResult in WalkableMeshes)
            {
                // set NavMap at [n,m] as walkable
            }
            else
            {
                // set NavMap at [n,m] as blocked
            }
        }
    }
    

    store navmap;
}

void CrowdPF::MoveActors(list of actors)
{
    // create FlowTiles from NavMap
    // create spline from FlowTiles for each actor and set them to follow the spline
}
```

## Implementation Choices
- Worst-case scenario LOS
- Flow Tiles computed only for Agent position, not for whole map
- Agents prefer LOS if contributes to Goal

## Stretch Goals
[x] Visualize cost & direction
[] Package as a UE Plugin
[] Profile & Benchmark against UE5's single-track pathfinding solution
[] Implement Automation Tests
[] Paralelize using SIMD | GPU
[] Utilize **Morton order** for NavMap``
[] Query Recast's NavMesh instead of using LineTrace for NavMap creation
[] Dynamically add obstacles / change costs

## Nice to Have
[] Recognize Duplicate Queries (i.e. player spams move command)
[] Object Pooling for FlowTiles
[] Serialize / Deserialize NavMesh
[] Automatically Detect Map Sectors.
[] Greedy Eikonal: Directed Wave Propagation.
[] Improve LOS by taking into account Goal's quadrant inside its tile

![Alt text](image-13.png)




