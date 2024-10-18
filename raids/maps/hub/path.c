#include <libdl/math3d.h>
#include "pathfind.h"

extern struct PathGraph;

VECTOR MOB0_PATHFINDING_NODES[] = {
	{ 229.03, 392.91, 126.7187, 1 },
	{ 240.1, 414.7, 126.7187, 1 },
	{ 260.6, 425.4, 126.7187, 1 },
	{ 236.5, 437.9, 126.7187, 1 },
	{ 222, 453.4, 126.7187, 1 },
	{ 289.5, 425.4, 126.7187, 1 },
};

u8 MOB0_PATHFINDING_NODES_CORNERING[] = {
	255,
	255,
	255,
	255,
	255,
	255,
};

u8 MOB0_PATHFINDING_EDGES[][2] = {
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 2 },
	{ 2, 1 },
	{ 1, 3 },
	{ 3, 1 },
	{ 2, 3 },
	{ 3, 2 },
	{ 2, 5 },
	{ 5, 2 },
	{ 3, 4 },
	{ 4, 3 },
};

u8 MOB0_PATHFINDING_EDGES_REQUIRED[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

u8 MOB0_PATHFINDING_EDGES_PATHFIT[] = {
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
	127,
};

u8 MOB0_PATHFINDING_EDGES_JUMPPADSPEED[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

u8 MOB0_PATHFINDING_EDGES_JUMPPADAT[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

u8 MOB0_PATHFINDING_PATHS[][3] = {
	{ 255, 255, 255, },
	{ 0, 255, 255, },
	{ 0, 2, 255, },
	{ 0, 4, 255, },
	{ 0, 4, 10, },
	{ 0, 2, 8, },
	{ 1, 255, 255, },
	{ 255, 255, 255, },
	{ 2, 255, 255, },
	{ 4, 255, 255, },
	{ 4, 10, 255, },
	{ 2, 8, 255, },
	{ 3, 1, 255, },
	{ 3, 255, 255, },
	{ 255, 255, 255, },
	{ 6, 255, 255, },
	{ 6, 10, 255, },
	{ 8, 255, 255, },
	{ 5, 1, 255, },
	{ 5, 255, 255, },
	{ 7, 255, 255, },
	{ 255, 255, 255, },
	{ 10, 255, 255, },
	{ 7, 8, 255, },
	{ 11, 5, 1, },
	{ 11, 5, 255, },
	{ 11, 7, 255, },
	{ 11, 255, 255, },
	{ 255, 255, 255, },
	{ 11, 7, 8, },
	{ 9, 3, 1, },
	{ 9, 3, 255, },
	{ 9, 255, 255, },
	{ 9, 6, 255, },
	{ 9, 6, 10, },
	{ 255, 255, 255, },
};


const int PathsCount = 1;
struct PathGraph Paths[] = {
  {
    .NumNodes = 6,
    .NumEdges = 12,
    .MaxPathNodeCount = 3,
    .Nodes = MOB0_PATHFINDING_NODES,
    .Cornering = MOB0_PATHFINDING_NODES_CORNERING,
    .Edges = MOB0_PATHFINDING_EDGES,
    .EdgesRequired = MOB0_PATHFINDING_EDGES_REQUIRED,
    .EdgesPathFit = MOB0_PATHFINDING_EDGES_PATHFIT,
    .EdgesJumpSpeed = MOB0_PATHFINDING_EDGES_JUMPPADSPEED,
    .EdgesJumpAt = MOB0_PATHFINDING_EDGES_JUMPPADAT,
    .Paths = MOB0_PATHFINDING_PATHS,
    .LastTargetUpdatedIdx = -1
  },
};
