/***************************************************
 * FILENAME :		pathfind.c
 * 
 * DESCRIPTION :
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <libdl/collision.h>
#include "include/pathfind.h"

#define PATH_EDGE_IS_EMPTY(x)               (x == 255)
#define TARGETS_CACHE_COUNT                 (16)

struct TargetCache
{
  Moby* Target;
  int ClosestNodeIdx;
  int DelayNextCheckTicks;
};

struct TargetCache TargetsCache[TARGETS_CACHE_COUNT];

//--------------------------------------------------------------------------
int pathGetClosestNode(Moby* moby)
{
  int i;
  VECTOR position = {0,0,1,0};
  VECTOR delta;
  int closestNodeIdx = 0;
  float closestNodeDist = 10000000;

  // use center of moby
  vector_add(position, position, moby->Position);

  for (i = 0; i < MOB_PATHFINDING_NODES_COUNT; ++i) {

    vector_subtract(delta, MOB_PATHFINDING_NODES[i], position);
    delta[3] = 0;
    float dist = vector_length(delta);

    if (dist < closestNodeDist) {
      closestNodeDist = dist;
      closestNodeIdx = i;
    }
  }

  return closestNodeIdx;
}

//--------------------------------------------------------------------------
int pathGetClosestNodeInSight(Moby* moby)
{
  int i,j;
  VECTOR position = {0,0,1,0};
  VECTOR delta;
  int closestNodeIdx = 0;
  float closestNodeDist = 10000000;

  char orderedNodesByDist[3];
  float nodeDists[MOB_PATHFINDING_NODES_COUNT];

  // init
  memset(orderedNodesByDist, -1, sizeof(orderedNodesByDist));
  memset(nodeDists, 0, sizeof(nodeDists));

  // use center of moby
  vector_add(position, position, moby->Position);

  // find 3 closest nodes to moby
  for (i = 0; i < MOB_PATHFINDING_NODES_COUNT; ++i) {

    vector_subtract(delta, MOB_PATHFINDING_NODES[i], position);
    delta[3] = 0;
    float dist = nodeDists[i] = vector_length(delta);

    // if obstructed then increase distance by factor
    //if (CollLine_Fix(position, MOB_PATHFINDING_NODES[i], 2, moby, 0))
    //  dist *= 1000;
    
    for (j = 0; j < 3; ++j) {
      if (orderedNodesByDist[j] < 0 || dist < nodeDists[orderedNodesByDist[j]]) {
        int leftover = 2 - j;
        if (leftover > 0)
          memmove(&orderedNodesByDist[j+1], &orderedNodesByDist[j], sizeof(char) * leftover);
        
        orderedNodesByDist[j] = i;
        break;
      }
    }
  }

  // if more than 1 node found
  // then find the closest of those 3 that is in sight
  if (orderedNodesByDist[1] >= 0) {
    for (i = 0; i < 3; ++i) {
      if (!CollLine_Fix(position, MOB_PATHFINDING_NODES[orderedNodesByDist[i]], 2, moby, 0))
        return orderedNodesByDist[i];
    }
  }

  return orderedNodesByDist[0];
}

//--------------------------------------------------------------------------
int pathRegisterTarget(Moby* moby)
{
  int i;

  for (i = 0; i < TARGETS_CACHE_COUNT; ++i) {
    if (!TargetsCache[i].Target) {
      TargetsCache[i].Target = moby;
      return TargetsCache[i].ClosestNodeIdx = pathGetClosestNodeInSight(moby);
    }
  }

  return -1;
}

//--------------------------------------------------------------------------
int pathTargetCacheGetClosestNodeIdx(Moby* moby)
{
  int i;

  for (i = 0; i < TARGETS_CACHE_COUNT; ++i) {
    if (TargetsCache[i].Target == moby) {
      return TargetsCache[i].ClosestNodeIdx;
    }
  }

  return pathRegisterTarget(moby);
}

//--------------------------------------------------------------------------
int pathShouldFindNewPath(Moby* moby)
{
  if (!moby || !moby->PVar)
    return 0;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars->MobVars.Target)
    return 0;

  if (!pvars->MobVars.MoveVars.PathEdgeCount)
    return 1;

  int closestNodeIdxToTarget = pathTargetCacheGetClosestNodeIdx(pvars->MobVars.Target);
  int lastEdge = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCount-1];
  if (PATH_EDGE_IS_EMPTY(lastEdge))
    return 1;

  if (MOB_PATHFINDING_EDGES[lastEdge][1] != closestNodeIdxToTarget)
    return 1;

  return 0;
}

//--------------------------------------------------------------------------
void pathGetPath(Moby* moby)
{
  int i;
  if (!moby || !moby->PVar)
    return;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars->MobVars.Target)
    return;

  // target closest node should be calculated and cached per frame in pathTick
  int closestNodeIdxToTarget = pathTargetCacheGetClosestNodeIdx(pvars->MobVars.Target);

  // we should reuse both the last node we were at
  // and the node we're currently going towards for this depending on the final path
  int closestNodeIdxToMob = pathGetClosestNodeInSight(moby);

  int pathIdx = (closestNodeIdxToMob * MOB_PATHFINDING_NODES_COUNT) + closestNodeIdxToTarget;
  memcpy(pvars->MobVars.MoveVars.CurrentPath, MOB_PATHFINDING_PATHS[pathIdx], sizeof(u8) * MOB_PATHFINDING_PATHS_MAX_PATH_LENGTH);
  pvars->MobVars.MoveVars.PathEdgeCurrent = 0;
  pvars->MobVars.MoveVars.PathHasReachedStart = 0;
  pvars->MobVars.MoveVars.PathHasReachedEnd = 0;
  pvars->MobVars.MoveVars.PathStartEndNodes[0] = closestNodeIdxToTarget;
  pvars->MobVars.MoveVars.PathStartEndNodes[1] = closestNodeIdxToMob;
  
  // count path length
  for (i = 0; i < MOB_PATHFINDING_PATHS_MAX_PATH_LENGTH; ++i) {
    if (PATH_EDGE_IS_EMPTY(pvars->MobVars.MoveVars.CurrentPath[i]))
      break;
  }
  pvars->MobVars.MoveVars.PathEdgeCount = i;

  // skip start if its backwards along path
  if (i > 0) {
    int startEdgeIdx = pvars->MobVars.MoveVars.CurrentPath[0];
    char* startEdge = MOB_PATHFINDING_EDGES[startEdgeIdx];
    
    VECTOR mobyToStart, mobyToNext;
    vector_subtract(mobyToStart, MOB_PATHFINDING_NODES[startEdge[0]], moby->Position);
    vector_subtract(mobyToNext, MOB_PATHFINDING_NODES[startEdge[1]], moby->Position);

    if (vector_innerproduct(mobyToNext, mobyToStart) < 0)
      pvars->MobVars.MoveVars.PathHasReachedStart = 1;
  }

  DPRINTF("NEW PATH GENERATED: (%d)\n", gameGetTime());
  DPRINTF("\tFROM NODE %d\n", closestNodeIdxToMob);
  DPRINTF("\tTO NODE %d\n", closestNodeIdxToTarget);
  DPRINTF("\tNODES: ");
  
  // count path length
  for (i = 0; i < pvars->MobVars.MoveVars.PathEdgeCount; ++i) {
    int edgeIdx = pvars->MobVars.MoveVars.CurrentPath[i];
    char * edge = MOB_PATHFINDING_EDGES[edgeIdx];
    DPRINTF("%d->%d, ", edge[0], edge[1]);
  }
  DPRINTF("\n");
}

//--------------------------------------------------------------------------
void pathSetPath(Moby* moby, int fromNodeIdx, int toNodeIdx, int currentOnPath, int hasReachedStart, int hasReachedEnd)
{
  int i;
  if (!moby || !moby->PVar)
    return;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars->MobVars.Target)
    return;

  int pathIdx = (fromNodeIdx * MOB_PATHFINDING_NODES_COUNT) + toNodeIdx;
  memcpy(pvars->MobVars.MoveVars.CurrentPath, MOB_PATHFINDING_PATHS[pathIdx], sizeof(u8) * MOB_PATHFINDING_PATHS_MAX_PATH_LENGTH);
  pvars->MobVars.MoveVars.PathEdgeCurrent = currentOnPath;
  pvars->MobVars.MoveVars.PathHasReachedStart = hasReachedStart;
  pvars->MobVars.MoveVars.PathHasReachedEnd = hasReachedEnd;
  pvars->MobVars.MoveVars.PathStartEndNodes[0] = toNodeIdx;
  pvars->MobVars.MoveVars.PathStartEndNodes[1] = fromNodeIdx;
  
  // count path length
  for (i = 0; i < MOB_PATHFINDING_PATHS_MAX_PATH_LENGTH; ++i) {
    if (PATH_EDGE_IS_EMPTY(pvars->MobVars.MoveVars.CurrentPath[i]))
      break;
  }
  pvars->MobVars.MoveVars.PathEdgeCount = i;
}

//--------------------------------------------------------------------------
char* pathGetCurrentEdge(Moby* moby)
{
  int i;
  if (!moby || !moby->PVar)
    return NULL;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  
  int edgeIdx = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent];
  if (PATH_EDGE_IS_EMPTY(edgeIdx))
    return NULL;

  return (char*)MOB_PATHFINDING_EDGES[edgeIdx];
}

//--------------------------------------------------------------------------
int pathGetTargetNodeIdx(Moby* moby)
{
  int i;
  if (!moby || !moby->PVar)
    return -1;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  
  int edgeIdx = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent];
  if (PATH_EDGE_IS_EMPTY(edgeIdx))
    return -1;

  if (pvars->MobVars.MoveVars.PathEdgeCurrent == (pvars->MobVars.MoveVars.PathEdgeCount-1) && pvars->MobVars.MoveVars.PathHasReachedEnd)
    return -1;

  if (pvars->MobVars.MoveVars.PathEdgeCurrent == 0 && !pvars->MobVars.MoveVars.PathHasReachedStart)
    return MOB_PATHFINDING_EDGES[edgeIdx][0];

  return MOB_PATHFINDING_EDGES[edgeIdx][1];
}

//--------------------------------------------------------------------------
void pathGetClosestPointOnNode(VECTOR output, VECTOR from, VECTOR target, int currentNodeIdx, int nextEdgeIdx)
{
  VECTOR fit;
  VECTOR fromToCurrentNodeCenter;
  VECTOR currentNodeCenterToNextNode;

  float currentNodeRadius = MOB_PATHFINDING_NODES[currentNodeIdx][3];
  float currentNodeCornering = MOB_PATHFINDING_NODES_CORNERING[currentNodeIdx];
  vector_subtract(fromToCurrentNodeCenter, MOB_PATHFINDING_NODES[currentNodeIdx], from);
  
  if (!PATH_EDGE_IS_EMPTY(nextEdgeIdx)) {

    int nextNodeIdx = MOB_PATHFINDING_EDGES[nextEdgeIdx][1];
    vector_lerp(fit, from, MOB_PATHFINDING_NODES[currentNodeIdx], MOB_PATHFINDING_EDGES_PATHFIT[nextEdgeIdx]);
    vector_subtract(currentNodeCenterToNextNode, MOB_PATHFINDING_NODES[nextNodeIdx], MOB_PATHFINDING_NODES[currentNodeIdx]);
  } else {

    vector_copy(fit, from);
    vector_subtract(currentNodeCenterToNextNode, target, MOB_PATHFINDING_NODES[currentNodeIdx]);
  }

  // compute cornering factor
  // the sharper the corner, the tighter the radius
  currentNodeCenterToNextNode[3] = 0;
  fromToCurrentNodeCenter[3] = 0;
  float dot = fabsf(vector_innerproduct(fromToCurrentNodeCenter, currentNodeCenterToNextNode));
  float cornerRadius = lerpf(currentNodeCornering, currentNodeRadius, powf(dot, 2.0));
  
  // compute point in node radius to target
  vector_subtract(output, MOB_PATHFINDING_NODES[currentNodeIdx], fit);
  vector_projectonhorizontal(output, output);
  output[3] = 0;
  float r = vector_length(output);
  if (r > cornerRadius)
    vector_scale(output, output, cornerRadius / r);
  
  vector_subtract(output, MOB_PATHFINDING_NODES[currentNodeIdx], output);
}

//--------------------------------------------------------------------------
void pathGetTargetPos(VECTOR output, Moby* moby)
{
  int i;
  VECTOR up = {0,0,1,0};
  VECTOR targetNodePos, delta;
  VECTOR from, to, edgeDir;
  if (!moby || !moby->PVar)
    return;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // reuse last calculated
  if (pvars->MobVars.MoveVars.PathTicks) {
    vector_copy(output, pvars->MobVars.MoveVars.LastTargetPos);
    return;
  }

  // delay next getTargetPos until next tick
  pvars->MobVars.MoveVars.PathTicks = 1;

  // new path
  if (pathShouldFindNewPath(moby))
    pathGetPath(moby);

  // set default output
  if (pvars->MobVars.Target)
    vector_copy(output, pvars->MobVars.Target->Position);
  else
    vector_copy(output, moby->Position);

  // no path
  if (!pvars->MobVars.MoveVars.PathEdgeCount || !pvars->MobVars.Target) {
    vector_copy(pvars->MobVars.MoveVars.LastTargetPos, output);
    return;
  }

  // check if we can just go straight to the target
  if (!pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks && pvars->MobVars.Target && !pvars->MobVars.MoveVars.PathHasReachedEnd) {
    
    int lockOntoPlayer = 0;

    // if target is near and in sight
    // skip rest of path and go straight towards target
    vector_subtract(delta, pvars->MobVars.Target->Position, moby->Position);
    vector_add(from, moby->Position, up);
    vector_add(to, pvars->MobVars.Target->Position, up);

    // near and can see
    if (vector_sqrmag(delta) < (MOB_TARGET_DIST_IN_SIGHT_IGNORE_PATH*MOB_TARGET_DIST_IN_SIGHT_IGNORE_PATH) && !CollLine_Fix(from, to, 2, moby, 0)) {

      // not in opposite direction of current edge
      char * currentEdge = pathGetCurrentEdge(moby);
      if (currentEdge) {
        vector_subtract(edgeDir, MOB_PATHFINDING_NODES[currentEdge[1]], MOB_PATHFINDING_NODES[currentEdge[0]]);
        edgeDir[3] = 0;
        vector_normalize(edgeDir, edgeDir);
        vector_normalize(delta, delta);


        if (vector_innerproduct(edgeDir, delta) > 0.3) {
          lockOntoPlayer = 1;
        }
      }
      
      if (lockOntoPlayer) {
        pvars->MobVars.MoveVars.PathEdgeCurrent = pvars->MobVars.MoveVars.PathEdgeCount;
      }
    }
    else if (pvars->MobVars.MoveVars.PathEdgeCurrent && pvars->MobVars.MoveVars.PathEdgeCurrent == pvars->MobVars.MoveVars.PathEdgeCount) {
      pathGetPath(moby);
    }

    pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks = TPS;
  }

  // check if we've reached the current node
  int targetNodeIdx = pathGetTargetNodeIdx(moby);
  if (targetNodeIdx >= 0) {
    vector_copy(targetNodePos, MOB_PATHFINDING_NODES[targetNodeIdx]);
    targetNodePos[3] = 0;
    float radius = MOB_PATHFINDING_NODES[targetNodeIdx][3];
    
    vector_subtract(delta, targetNodePos, moby->Position);
    vector_projectonhorizontal(delta, delta);
    float hDist = vector_length(delta);

    // reached target node
    //DPRINTF("r:%f dist:%f 3:%f\n", radius, hDist, delta[3]);
    if (hDist < (radius + 0.5)) {
      if (pvars->MobVars.MoveVars.PathEdgeCurrent == 0 && !pvars->MobVars.MoveVars.PathHasReachedStart) {
        pvars->MobVars.MoveVars.PathHasReachedStart = 1;
      //} else if (pvars->MobVars.MoveVars.PathEdgeCurrent == (pvars->MobVars.MoveVars.PathEdgeCount-1) && !pvars->MobVars.MoveVars.PathHasReachedEnd) {
      //  pvars->MobVars.MoveVars.PathHasReachedEnd = 1;
      } else {
        pvars->MobVars.MoveVars.PathEdgeCurrent++;
      }
    }
  }
  
  // skip end if its backwards along path
  if (pvars->MobVars.MoveVars.PathEdgeCurrent == (pvars->MobVars.MoveVars.PathEdgeCount-1)) {
    char* lastEdge = pathGetCurrentEdge(moby);
    if (lastEdge) {
      VECTOR targetToStart, targetToNext;
      vector_subtract(targetToStart, MOB_PATHFINDING_NODES[lastEdge[0]], pvars->MobVars.Target->Position);
      vector_subtract(targetToNext, MOB_PATHFINDING_NODES[lastEdge[1]], pvars->MobVars.Target->Position);
      if (vector_innerproduct(targetToNext, targetToStart) < 0)
        pvars->MobVars.MoveVars.PathHasReachedEnd = 1;
    }
  }

  targetNodeIdx = pathGetTargetNodeIdx(moby);
  if (targetNodeIdx < 0) {
    vector_copy(pvars->MobVars.MoveVars.LastTargetPos, output);
    return;
  }

  // get point
  pathGetClosestPointOnNode(output, moby->Position, pvars->MobVars.Target->Position, targetNodeIdx, pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent+1]);
  vector_copy(pvars->MobVars.MoveVars.LastTargetPos, output);
}

//--------------------------------------------------------------------------
void pathTick(void)
{
  static int initialized = 0;
  static int lastTargetUpdatedIdx = 0;
  int i;
  int hasAlreadyCheckedANode = 0;

  if (!initialized)
  {
    memset(TargetsCache, 0, sizeof(TargetsCache));
    initialized = 1;
  }

  // update target cache
  // since pathGetClosestNodeInSight uses collision to check if a node is in sight
  // and collision testing is very expensive
  // we only do one update per tick
  // and we put a 
  for (i = 0; i < TARGETS_CACHE_COUNT; ++i) {
    int idx = (lastTargetUpdatedIdx + i) % TARGETS_CACHE_COUNT;
    struct TargetCache *cache = &TargetsCache[idx];

    if (!cache->Target)
      continue;

    if (mobyIsDestroyed(cache->Target)) {
      cache->Target = NULL;
    } else if (cache->DelayNextCheckTicks <= 0 && !hasAlreadyCheckedANode) {
      cache->ClosestNodeIdx = pathGetClosestNodeInSight(cache->Target);
      cache->DelayNextCheckTicks = TPS * 0.2;
      hasAlreadyCheckedANode = 1;
      lastTargetUpdatedIdx = idx + 1;
    } else if (cache->DelayNextCheckTicks) {
      cache->DelayNextCheckTicks--;
    }
  }
}
