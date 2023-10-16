#ifndef _SURVIVAL_BIGALPATH_H_
#define _SURVIVAL_BIGALPATH_H_

#include <libdl/math3d.h>

typedef struct BigAlPathNode
{
	VECTOR Position;
	char ConnectedNodes[3];
} BigAlPathNode_t;

extern const int BIGAL_PATHFINDING_NODES_COUNT;
extern BigAlPathNode_t BIGAL_PATHFINDING_NODES[];

#endif // _SURVIVAL_BIGALPATH_H_
