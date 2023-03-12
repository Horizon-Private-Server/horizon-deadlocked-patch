#ifndef _SURVIVAL_GASPATH_H_
#define _SURVIVAL_GASPATH_H_

#include <libdl/math3d.h>

typedef struct GasPathNode
{
	VECTOR Position;
	char ConnectedNodes[3];
} GasPathNode_t;

extern const int GAS_PATHFINDING_NODES_COUNT;
extern GasPathNode_t GAS_PATHFINDING_NODES[];

#endif // _SURVIVAL_GASPATH_H_
