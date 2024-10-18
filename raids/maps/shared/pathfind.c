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
#define CLOSEST_NODES_COLL_CHECK_SIZE       (3)

#if GATE
void gateSetCollision(int collActive);
#endif

int mapPathCanBeSkippedForTarget(struct PathGraph* path, Moby* moby);

//--------------------------------------------------------------------------
struct PathGraph* pathGetMobyPathGraph(Moby* moby)
{
  VECTOR mobyToStart, mobyToNext;
  if (!moby || !moby->PVar)
    return 0;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (pvars->MobVars.MoveVars.PathGraphIdx >= 0 && pvars->MobVars.MoveVars.PathGraphIdx < PathsCount)
    return &Paths[pvars->MobVars.MoveVars.PathGraphIdx];

  return NULL;
}

//--------------------------------------------------------------------------
u8* pathGetPathAt(struct PathGraph* path, int fromNodeIdx, int toNodeIdx)
{
  int pathIdx = (fromNodeIdx * path->NumNodes) + toNodeIdx;
  u32 addr = (u32)path->Paths;

  return (u8*)(addr + pathIdx * path->MaxPathNodeCount);
}

//--------------------------------------------------------------------------
void pathGetSegment(struct PathGraph* path, u8* currentEdge, VECTOR fromNodePos, VECTOR toNodePos)
{
  if (!currentEdge || !path) return;

  vector_copy(fromNodePos, path->Nodes[currentEdge[0]]);
  vector_copy(toNodePos, path->Nodes[currentEdge[1]]);
  fromNodePos[3] = 0;
  toNodePos[3] = 0;
}

//--------------------------------------------------------------------------
float pathGetSegmentAlpha(struct PathGraph* path, Moby* moby, u8* currentEdge)
{
  VECTOR startNodeToEndNode, startNodeToMoby;

  if (!moby || !currentEdge || !path) return 0;

  // get vectors from start node to end node and moby
  vector_subtract(startNodeToEndNode, path->Nodes[currentEdge[0]], path->Nodes[currentEdge[1]]);
  startNodeToEndNode[3] = 0;
  vector_subtract(startNodeToMoby, path->Nodes[currentEdge[0]], moby->Position);
  startNodeToMoby[3] = 0;

  // project startToMoby on startToEnd
  float edgeLen = vector_length(startNodeToEndNode);
  float distOnEdge = vector_length(startNodeToMoby) * vector_innerproduct(startNodeToEndNode, startNodeToMoby);
  return clamp(distOnEdge / edgeLen, 0, 1);
}

//--------------------------------------------------------------------------
int pathCanStartNodeBeSkipped(struct PathGraph* path, Moby* moby)
{
  VECTOR mobyToStart, mobyToNext;
  if (!moby || !moby->PVar || !path)
    return 0;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  int startEdgeIdx = pvars->MobVars.MoveVars.CurrentPath[0];
  u8* startEdge = path->Edges[startEdgeIdx];

  // if the start node to next node is a jump
  // AND we're not after that jump
  // then circle back, we failed the jump
  // otherwise allow skipping
  float alpha = pathGetSegmentAlpha(path, moby, startEdge);
  float jumpAt = path->EdgesJumpAt[startEdgeIdx] / 255.0;
  if (path->EdgesJumpSpeed[startEdgeIdx] > 0 && alpha >= jumpAt)
    return 0;

  // if segment is required and we haven't completed the required section then circle back
  float requiredAt = path->EdgesRequired[startEdgeIdx] / 255.0;
  if (requiredAt > 0 && alpha <= requiredAt)
    return 0;

  // skip if start is in opposite direction to next target
  vector_subtract(mobyToStart, path->Nodes[startEdge[0]], moby->Position);
  vector_subtract(mobyToNext, path->Nodes[startEdge[1]], moby->Position);
  return vector_innerproduct_unscaled(mobyToNext, mobyToStart) < 0;
}

//--------------------------------------------------------------------------
int pathSegmentCanBeSkipped(struct PathGraph* path, Moby* moby, int segmentStartEdgeIdx, int segmentCount, float segmentStartAlpha)
{
  int i, edge;
  if (!moby || !moby->PVar || !path)
    return 0;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // no path
  if (!pvars->MobVars.MoveVars.PathEdgeCount) {
    return 0;
  }

  // segment not in path
  if (segmentStartEdgeIdx >= pvars->MobVars.MoveVars.PathEdgeCount) {
    return 0;
  }

  // check if current edge is required or there is a jump we haven't reached
  edge = pvars->MobVars.MoveVars.CurrentPath[segmentStartEdgeIdx];
  float jumpAt = path->EdgesJumpAt[edge] / 255.0;
  float requiredAt = path->EdgesRequired[edge] / 255.0;
  if ((requiredAt > 0 && segmentStartAlpha <= requiredAt) || (path->EdgesJumpSpeed[edge] > 0 && segmentStartAlpha <= jumpAt))
    return 0;

  for (i = 1; i < segmentCount && (i+segmentStartEdgeIdx) < pvars->MobVars.MoveVars.PathEdgeCount; ++i) {
    edge = pvars->MobVars.MoveVars.CurrentPath[i + segmentStartEdgeIdx];
    if (PATH_EDGE_IS_EMPTY(edge))
      break;

    if (path->EdgesRequired[edge] > 0 || path->EdgesJumpSpeed[edge] > 0)
      return 0;
  }

  return 1;
}

//--------------------------------------------------------------------------
int pathCanBeSkippedForTarget(struct PathGraph* path, Moby* moby)
{
  int i, edge;
  if (!moby || !moby->PVar || !path)
    return 1;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // pass to map
  // let map decide if path can't be skipped
  if (!mapPathCanBeSkippedForTarget(path, moby))
    return 0;

  // no path
  if (!pvars->MobVars.MoveVars.PathEdgeCount) {
    return 1;
  }

  // wait for grounding
  if (!pvars->MobVars.MoveVars.Grounded) {
    return 0;
  }

  // stuck
  if (pvars->MobVars.MoveVars.StuckCounter) {
    return 0;
  }

  // check if current edge is required or there is a jump we haven't reached
  edge = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent];
  float jumpAt = path->EdgesJumpAt[edge] / 255.0;
  float requiredAt = path->EdgesRequired[edge] / 255.0;
  if ((requiredAt > 0 && pvars->MobVars.MoveVars.PathEdgeAlpha <= requiredAt) || (path->EdgesJumpSpeed[edge] > 0 && pvars->MobVars.MoveVars.LastPathEdgeAlphaForJump <= jumpAt))
    return 0;

  for (i = pvars->MobVars.MoveVars.PathEdgeCurrent+1; i < pvars->MobVars.MoveVars.PathEdgeCount; ++i) {
    edge = pvars->MobVars.MoveVars.CurrentPath[i];
    if (PATH_EDGE_IS_EMPTY(edge))
      break;

    if (path->EdgesRequired[edge] > 0 || path->EdgesJumpSpeed[edge] > 0) {
#if DEBUGPATH
      DPRINTF("CANNOT SKIP PATH WITH JUMP %d=>%d\n", path->Edges[edge][0], path->Edges[edge][1]);
#endif
      return 0;
    }
  }

  return 1;
}

//--------------------------------------------------------------------------
int pathGetClosestNode(struct PathGraph* path, Moby* moby)
{
  int i;
  VECTOR position = {0,0,1,0};
  VECTOR delta;
  int closestNodeIdx = 0;
  float closestNodeDist = 10000000;

  // use center of moby
  vector_add(position, position, moby->Position);

  for (i = 0; i < path->NumNodes; ++i) {

    vector_subtract(delta, path->Nodes[i], position);
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
int pathGetClosestNodeInSight(struct PathGraph* path, Moby* moby, int * foundInSight)
{
  int i,j;
  VECTOR position = {0,0,1,0};
  VECTOR from;
  VECTOR delta;
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  float collRadius = 0.5;
  if (pvars)
    collRadius = pvars->MobVars.Config.CollRadius;

  char orderedNodesByDist[CLOSEST_NODES_COLL_CHECK_SIZE];
  float nodeDists[PATHGRAPH_MAX_NUM_NODES];

  // init
  memset(orderedNodesByDist, -1, sizeof(orderedNodesByDist));
  memset(nodeDists, 0, sizeof(nodeDists));

  // use center of moby
  vector_add(position, position, moby->Position);

  //DPRINTF("get closest node in sight %08X %08X %04X\n", guberGetUID(moby), (u32)moby, moby->OClass);

  // find n closest nodes to moby
  for (i = 0; i < path->NumNodes && i < PATHGRAPH_MAX_NUM_NODES; ++i) {

    vector_subtract(delta, path->Nodes[i], position);
    float radius = delta[3];
    delta[3] = 0;
    float dist = nodeDists[i] = maxf(0, vector_length(delta) - radius);

    // if obstructed then increase distance by factor
    //if (CollLine_Fix(position, path->Nodes[i], COLLISION_FLAG_IGNORE_DYNAMIC, moby, NULL))
    //  dist *= 1000;
    
    for (j = 0; j < CLOSEST_NODES_COLL_CHECK_SIZE; ++j) {
      if (orderedNodesByDist[j] < 0 || dist < nodeDists[(u8)orderedNodesByDist[j]]) {
        int leftover = (CLOSEST_NODES_COLL_CHECK_SIZE - 1) - j;
        if (leftover > 0)
          memmove(&orderedNodesByDist[j+1], &orderedNodesByDist[j], sizeof(char) * leftover);
        
        orderedNodesByDist[j] = i;
        break;
      }
    }
  }

  // if more than 1 node found
  // then find the closest of those that is in sight
  if (orderedNodesByDist[1] >= 0) {
#if GATE
    gateSetCollision(0);
#endif
    
    for (i = 0; i < CLOSEST_NODES_COLL_CHECK_SIZE; ++i) {
      VECTOR nodePos;
      vector_copy(nodePos, path->Nodes[(u8)orderedNodesByDist[i]]);
      float radius = nodePos[3];

      // moby to node
      // compute closest point on node to moby (flat)
      vector_subtract(delta, nodePos, position);
      delta[3] = 0; delta[2] = 0;
      vector_normalize(delta, delta);
      vector_scale(delta, delta, radius);
      vector_subtract(nodePos, nodePos, delta);

      // check if closest point is obstructed
      if (orderedNodesByDist[i] >= 0 && !CollLine_Fix(position, nodePos, COLLISION_FLAG_IGNORE_DYNAMIC, moby, NULL)) {
        if (foundInSight)
          *foundInSight = 1;

        return orderedNodesByDist[i];
      }
    }
  }

  // didn't find in sight
  if (foundInSight)
    *foundInSight = 0;
    
  return orderedNodesByDist[0];
}

//--------------------------------------------------------------------------
int pathRegisterTarget(struct PathGraph* path, Moby* moby)
{
  int i;

  if (!path || !moby) return -1;

  for (i = 0; i < TARGETS_CACHE_COUNT; ++i) {
    if (!path->TargetsCache[i].Target) {
      path->TargetsCache[i].Target = moby;
      return path->TargetsCache[i].ClosestNodeIdx = pathGetClosestNodeInSight(path, moby, NULL);
    }
  }

  return -1;
}

//--------------------------------------------------------------------------
int pathGetClosestNodeIdx(struct PathGraph* path, VECTOR pos)
{
  int i;
  VECTOR dt;
  int bestSqrDist = 1000000;
  int bestIdx = 0;

  if (!path || !pos) return -1;

  for (i = 0; i < path->NumNodes; ++i) {
    vector_subtract(dt, pos, path->Nodes[i]);
    dt[3] = 0;

    float sqrDist = vector_sqrmag(dt);
    if (sqrDist < bestSqrDist) {
      bestSqrDist = sqrDist;
      bestIdx = i;
    }
  }

  return bestIdx;
}

//--------------------------------------------------------------------------
int pathTargetCacheGetClosestNodeIdx(struct PathGraph* path, Moby* moby)
{
  int i;

  if (!path || !moby) return -1;

  for (i = 0; i < TARGETS_CACHE_COUNT; ++i) {
    if (path->TargetsCache[i].Target == moby) {
      return path->TargetsCache[i].ClosestNodeIdx;
    }
  }

  return pathRegisterTarget(path, moby);
}

//--------------------------------------------------------------------------
int pathShouldFindNewPath(struct PathGraph* path, Moby* moby)
{
  if (!moby || !moby->PVar || !path)
    return 0;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars->MobVars.MoveVars.PathEdgeCount) {
    return 1;
  }

  if (pvars->MobVars.MoveVars.IsStuck && pvars->MobVars.MoveVars.StuckCounter > MOB_MAX_STUCK_COUNTER_FOR_NEW_PATH) {
    return 1;
  }

  int lastEdge = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCount-1];
  if (PATH_EDGE_IS_EMPTY(lastEdge)) {
    return 1;
  }

  int closestNodeIdxToTarget = 0;
  if (pvars->MobVars.Target) {
    closestNodeIdxToTarget = pathTargetCacheGetClosestNodeIdx(path, pvars->MobVars.Target);
  } else {
    closestNodeIdxToTarget = pathGetClosestNodeIdx(path, pvars->MobVars.TargetPosition);
  }

  if (path->Edges[lastEdge][1] != closestNodeIdxToTarget) {
    return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
void pathGetPath(struct PathGraph* path, Moby* moby)
{
  int i;
  int inSight = 0;
  if (!moby || !moby->PVar || !path)
    return;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // target closest node should be calculated and cached per frame in pathTick
  int closestNodeIdxToTarget = 0;
  if (pvars->MobVars.Target) {
    closestNodeIdxToTarget = pathTargetCacheGetClosestNodeIdx(path, pvars->MobVars.Target);
  } else {
    closestNodeIdxToTarget = pathGetClosestNodeIdx(path, pvars->MobVars.TargetPosition);
  }

  // we should reuse both the last node we were at
  // and the node we're currently going towards for this depending on the final path
  int closestNodeIdxToMob = pathGetClosestNodeInSight(path, moby, &inSight);
  //DPRINTF("closest node to mob is %d (insight: %d)\n", closestNodeIdxToMob, inSight);

  int lastEdgeIdx = 255;
  if (pvars->MobVars.MoveVars.PathEdgeCount) {
    lastEdgeIdx = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent];
    if (PATH_EDGE_IS_EMPTY(lastEdgeIdx))
      lastEdgeIdx = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent - 1];
  }

  memcpy(pvars->MobVars.MoveVars.CurrentPath, pathGetPathAt(path, closestNodeIdxToMob, closestNodeIdxToTarget), sizeof(u8) * path->MaxPathNodeCount);
  pvars->MobVars.MoveVars.PathEdgeCurrent = 0;
  pvars->MobVars.MoveVars.PathEdgeAlpha = 0;
  pvars->MobVars.MoveVars.PathHasReachedStart = 0;
  pvars->MobVars.MoveVars.PathHasReachedEnd = 0;
  pvars->MobVars.MoveVars.PathStartEndNodes[0] = closestNodeIdxToTarget;
  pvars->MobVars.MoveVars.PathStartEndNodes[1] = closestNodeIdxToMob;
  
  // count path length
  for (i = 0; i < path->MaxPathNodeCount; ++i) {
    if (PATH_EDGE_IS_EMPTY(pvars->MobVars.MoveVars.CurrentPath[i]))
      break;
  }
  pvars->MobVars.MoveVars.PathEdgeCount = i;

  // check if we're on same segment as last
  int isOnSameSegment = 0;
  if (!PATH_EDGE_IS_EMPTY(lastEdgeIdx)) {
    isOnSameSegment = lastEdgeIdx == pvars->MobVars.MoveVars.CurrentPath[0];
  }

  // skip start if its backwards along path
  // and the segment can be skipped
  // or if we're already on this segment from the last path
  //if (i > 0 && (pathSegmentCanBeSkipped(moby, 0, 1, alpha) || isOnSameSegment)) {
  int canBeSkipped = pathCanStartNodeBeSkipped(path, moby);
  if (i > 0 && (isOnSameSegment || canBeSkipped)) {
    pvars->MobVars.MoveVars.PathHasReachedStart = 1;
  }

  // mark mob dirty to send path to others
  if (pvars->MobVars.Owner == gameGetMyClientId()) {
    pvars->MobVars.Dirty = 1;
  }

#if DEBUGPATH
  DPRINTF("NEW PATH GENERATED: (%d)\n", gameGetTime());
  DPRINTF("\tFROM NODE %d (skip:%d,%d,%d)\n", closestNodeIdxToMob, pvars->MobVars.MoveVars.PathHasReachedStart, canBeSkipped, isOnSameSegment);
  DPRINTF("\tTO NODE %d\n", closestNodeIdxToTarget);
  DPRINTF("\tNODES: ");
  
  // count path length
  for (i = 0; i < pvars->MobVars.MoveVars.PathEdgeCount; ++i) {
    int edgeIdx = pvars->MobVars.MoveVars.CurrentPath[i];
    u8 * edge = MOB_PATHFINDING_EDGES[edgeIdx];
    DPRINTF("%d->%d, ", edge[0], edge[1]);
  }
  DPRINTF("\n");
#endif
}

//--------------------------------------------------------------------------
void pathSetPath(struct PathGraph* path, Moby* moby, int fromNodeIdx, int toNodeIdx, int currentOnPath, int hasReachedStart, int hasReachedEnd)
{
  int i;
  if (!moby || !moby->PVar || !path)
    return;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  memcpy(pvars->MobVars.MoveVars.CurrentPath, pathGetPathAt(path, fromNodeIdx, toNodeIdx), sizeof(u8) * path->MaxPathNodeCount);
  if (pvars->MobVars.MoveVars.PathEdgeCurrent != currentOnPath) {
    pvars->MobVars.MoveVars.PathEdgeAlpha = 0;
  }
  pvars->MobVars.MoveVars.PathEdgeCurrent = currentOnPath;
  pvars->MobVars.MoveVars.PathHasReachedStart = hasReachedStart;
  pvars->MobVars.MoveVars.PathHasReachedEnd = hasReachedEnd;
  pvars->MobVars.MoveVars.PathStartEndNodes[0] = toNodeIdx;
  pvars->MobVars.MoveVars.PathStartEndNodes[1] = fromNodeIdx;
  
  // count path length
  for (i = 0; i < path->MaxPathNodeCount; ++i) {
    if (PATH_EDGE_IS_EMPTY(pvars->MobVars.MoveVars.CurrentPath[i]))
      break;
  }
  pvars->MobVars.MoveVars.PathEdgeCount = i;
}

//--------------------------------------------------------------------------
u8* pathGetCurrentEdge(struct PathGraph* path, Moby* moby)
{
  if (!moby || !moby->PVar || !path)
    return NULL;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  
  int edgeIdx = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent];
  if (PATH_EDGE_IS_EMPTY(edgeIdx))
    return NULL;

  return (u8*)path->Edges[edgeIdx];
}

//--------------------------------------------------------------------------
int pathGetTargetNodeIdx(struct PathGraph* path, Moby* moby)
{
  if (!moby || !moby->PVar || !path)
    return -1;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  
  int edgeIdx = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent];
  if (PATH_EDGE_IS_EMPTY(edgeIdx))
    return -1;

  if (pvars->MobVars.MoveVars.PathEdgeCurrent == (pvars->MobVars.MoveVars.PathEdgeCount-1) && pvars->MobVars.MoveVars.PathHasReachedEnd)
    return -1;

  if (pvars->MobVars.MoveVars.PathEdgeCurrent == 0 && !pvars->MobVars.MoveVars.PathHasReachedStart) {
    return path->Edges[edgeIdx][0];
  }

  return path->Edges[edgeIdx][1];
}

//--------------------------------------------------------------------------
void pathGetClosestPointOnNode(struct PathGraph* path, VECTOR output, VECTOR from, VECTOR target, int currentNodeIdx, int nextEdgeIdx, float collRadius)
{
  VECTOR fit;
  VECTOR fromToCurrentNodeCenter;
  VECTOR currentNodeCenterToNextNode;

  float currentNodeRadius = maxf(0, path->Nodes[currentNodeIdx][3] - collRadius);
  float currentNodeCornering = path->Cornering[currentNodeIdx] / 255.0;
  vector_subtract(fromToCurrentNodeCenter, path->Nodes[currentNodeIdx], from);
  
  if (!PATH_EDGE_IS_EMPTY(nextEdgeIdx)) {

    int nextNodeIdx = path->Edges[nextEdgeIdx][1];
    float pathFit = path->EdgesPathFit[nextEdgeIdx] / 255.0;
    vector_lerp(fit, from, path->Nodes[nextNodeIdx], pathFit);
    vector_subtract(currentNodeCenterToNextNode, path->Nodes[nextNodeIdx], path->Nodes[currentNodeIdx]);
  } else {

    vector_copy(fit, from);
    vector_subtract(currentNodeCenterToNextNode, target, path->Nodes[currentNodeIdx]);
  }

  // compute cornering factor
  // the sharper the corner, the tighter the radius
  currentNodeCenterToNextNode[3] = 0;
  fromToCurrentNodeCenter[3] = 0;
  float dot = fabsf(vector_innerproduct(fromToCurrentNodeCenter, currentNodeCenterToNextNode));
  float cornerRadius = lerpf(currentNodeCornering * currentNodeRadius, currentNodeRadius, powf(dot, 2.0));
  
  // compute point in node radius to target
  vector_subtract(output, path->Nodes[currentNodeIdx], fit);
  vector_projectonhorizontal(output, output);
  output[3] = 0;
  float r = vector_length(output);
  if (r > cornerRadius)
    vector_scale(output, output, cornerRadius / r);
  
  // float dist = vector_length(output);
  vector_subtract(output, path->Nodes[currentNodeIdx], output);
  output[3] = 0;

  //DPRINTF("edgeEmpty:%d dot:%f corner:%f cornerRadius:%f r:%f finalDist:%f\n", PATH_EDGE_IS_EMPTY(nextEdgeIdx), dot, currentNodeCornering, cornerRadius, r, dist);
}

//--------------------------------------------------------------------------
void pathGetTargetPos(struct PathGraph* path, VECTOR output, Moby* moby)
{
  VECTOR up = {0,0,1,0};
  VECTOR targetNodePos, delta;
  VECTOR from, to, edgeDir;
  if (!moby || !moby->PVar || !path)
    return;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  /*
  // disable pathfinding
  if (pvars->MobVars.Target)
  {
    vector_copy(output, pvars->MobVars.Target->Position);
    vector_copy(pvars->MobVars.MoveVars.LastTargetPos, output);
    return;
  }
  */

  // reuse last calculated
  if (pvars->MobVars.MoveVars.PathTicks) {
    vector_copy(output, pvars->MobVars.MoveVars.LastTargetPos);
    return;
  }

  // delay next getTargetPos until next tick
  pvars->MobVars.MoveVars.PathTicks = 1;
  pvars->MobVars.MoveVars.PathEdgeAlpha = pathGetSegmentAlpha(path, moby, pathGetCurrentEdge(path, moby));

  // new path
  if (pvars->MobVars.MoveVars.PathNewTicks == 0 && pathShouldFindNewPath(path, moby)) {
    pathGetPath(path, moby);
    pvars->MobVars.MoveVars.PathNewTicks = 255;
  }

  // set default output
  if (pvars->MobVars.Target) {
    vector_copy(pvars->MobVars.TargetPosition, pvars->MobVars.Target->Position);
  }
  
  vector_copy(output, pvars->MobVars.TargetPosition);

  // no path
  if (!pvars->MobVars.MoveVars.PathEdgeCount) {
    vector_copy(pvars->MobVars.MoveVars.LastTargetPos, output);
    return;
  }

  // check if we can just go straight to the target
  if (!pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks && !pvars->MobVars.MoveVars.PathHasReachedEnd) {
  
    int lockOntoPlayer = 0;

    // if target is near and in sight
    // skip rest of path and go straight towards target
    vector_subtract(delta, pvars->MobVars.TargetPosition, moby->Position);
    vector_add(from, moby->Position, up);
    vector_add(to, pvars->MobVars.TargetPosition, up);

    // near and can see
    if (vector_sqrmag(delta) < (MOB_TARGET_DIST_IN_SIGHT_IGNORE_PATH*MOB_TARGET_DIST_IN_SIGHT_IGNORE_PATH) && !CollLine_Fix(from, to, COLLISION_FLAG_IGNORE_DYNAMIC, moby, NULL)) {

      // not in opposite direction of current edge
      u8* currentEdge = pathGetCurrentEdge(path, moby);
      if (currentEdge) {
        vector_subtract(edgeDir, path->Nodes[currentEdge[1]], path->Nodes[currentEdge[0]]);
        edgeDir[3] = 0;
        vector_normalize(edgeDir, edgeDir);
        vector_normalize(delta, delta);

        if (vector_innerproduct(edgeDir, delta) > 0.3) {
          lockOntoPlayer = 1;
        }
      }
      
      if (lockOntoPlayer && pathCanBeSkippedForTarget(path, moby)) {
        pvars->MobVars.MoveVars.PathEdgeCurrent = pvars->MobVars.MoveVars.PathEdgeCount;
      }
    } else if (pvars->MobVars.MoveVars.PathEdgeCurrent && pvars->MobVars.MoveVars.PathEdgeCurrent == pvars->MobVars.MoveVars.PathEdgeCount) {
      pathGetPath(path, moby);
    }

    pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks = TPS;
  }

  // check if we've reached the current node
  int targetNodeIdx = pathGetTargetNodeIdx(path, moby);
  if (targetNodeIdx >= 0) {
    vector_copy(targetNodePos, path->Nodes[targetNodeIdx]);
    targetNodePos[3] = 0;
    float radius = path->Nodes[targetNodeIdx][3];
    
    vector_subtract(delta, targetNodePos, moby->Position);
    vector_projectonhorizontal(delta, delta);
    float hDist = vector_length(delta);

    vector_subtract(delta, pvars->MobVars.MoveVars.LastTargetPos, moby->Position);
    vector_projectonhorizontal(delta, delta);
    float tDist = vector_length(delta);

    // reached target node
    //DPRINTF("r:%f dist:%f 3:%f\n", radius, hDist, delta[3]);
    if (hDist < (radius + 0.5) && tDist < (0.5 + pvars->MobVars.Config.CollRadius)) {
      if (pvars->MobVars.MoveVars.PathEdgeCurrent == 0 && !pvars->MobVars.MoveVars.PathHasReachedStart) {
        pvars->MobVars.MoveVars.PathHasReachedStart = 1;
      //} else if (pvars->MobVars.MoveVars.PathEdgeCurrent == (pvars->MobVars.MoveVars.PathEdgeCount-1) && !pvars->MobVars.MoveVars.PathHasReachedEnd) {
      //  pvars->MobVars.MoveVars.PathHasReachedEnd = 1;
      } else {
        pvars->MobVars.MoveVars.PathEdgeCurrent++;
        pvars->MobVars.MoveVars.PathEdgeAlpha = 0;
        //DPRINTF("hit target nodeIdx %d, new edgeIdx %d\n", targetNodeIdx, pvars->MobVars.MoveVars.PathEdgeCurrent);
      }
    }
  }
  
  // skip end if its backwards along path
  // and we can see the target
  if (!pvars->MobVars.MoveVars.PathCheckSkipEndTicks && pvars->MobVars.MoveVars.PathEdgeCurrent == (pvars->MobVars.MoveVars.PathEdgeCount-1)) {
    u8* lastEdge = pathGetCurrentEdge(path, moby);
    if (lastEdge && pathCanBeSkippedForTarget(path, moby)) {
      VECTOR targetToStart, targetToNext;
      vector_subtract(targetToStart, path->Nodes[lastEdge[0]], pvars->MobVars.TargetPosition);
      vector_subtract(targetToNext, path->Nodes[lastEdge[1]], pvars->MobVars.TargetPosition);
      if (vector_innerproduct(targetToNext, targetToStart) < 0) {
        VECTOR up = {0,0,1,0};
        VECTOR from, to;
        vector_add(from, up, moby->Position);
        vector_add(to, up, path->Nodes[lastEdge[1]]);
        if (!CollLine_Fix(from, to, COLLISION_FLAG_IGNORE_DYNAMIC, moby, NULL)) {
          pvars->MobVars.MoveVars.PathHasReachedEnd = 1;
        }
      }
    }

    pvars->MobVars.MoveVars.PathCheckSkipEndTicks = TPS;
  }

  targetNodeIdx = pathGetTargetNodeIdx(path, moby);
  if (targetNodeIdx < 0) {
    vector_copy(pvars->MobVars.MoveVars.LastTargetPos, output);
    return;
  }

  // get point
  pathGetClosestPointOnNode(path, output, moby->Position, pvars->MobVars.TargetPosition, targetNodeIdx, pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent+1], pvars->MobVars.Config.CollRadius);
  vector_copy(pvars->MobVars.MoveVars.LastTargetPos, output);
}

//--------------------------------------------------------------------------
void pathTick(struct PathGraph* path)
{
  int i,j;
  int hasAlreadyCheckedANode = 0;

  if (path->LastTargetUpdatedIdx < 0)
  {
    memset(path->TargetsCache, 0, sizeof(path->TargetsCache));
    path->LastTargetUpdatedIdx = 0;
  }

  // update target cache
  // since pathGetClosestNodeInSight uses collision to check if a node is in sight
  // and collision testing is very expensive
  // we only do one update per tick
  // and we put a cooldown
  for (i = 0; i < TARGETS_CACHE_COUNT; ++i) {
    int idx = (path->LastTargetUpdatedIdx + i) % TARGETS_CACHE_COUNT;
    struct TargetCache *cache = &path->TargetsCache[idx];

    if (!cache->Target)
      continue;

    if (mobyIsDestroyed(cache->Target)) {
      cache->Target = NULL;
    } else if (cache->DelayNextCheckTicks <= 0 && !hasAlreadyCheckedANode) {
      cache->ClosestNodeIdx = pathGetClosestNodeInSight(path, cache->Target, NULL);
      cache->DelayNextCheckTicks = TPS * 0.2;
      hasAlreadyCheckedANode = 1;
      path->LastTargetUpdatedIdx = idx + 1;
    } else if (cache->DelayNextCheckTicks) {
      cache->DelayNextCheckTicks--;
    }
  }
}

//--------------------------------------------------------------------------
float pathGetJumpSpeed(struct PathGraph* path, Moby* moby)
{
  if (!moby || !moby->PVar || !path)
    return 0;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // no path
  if (!pvars->MobVars.MoveVars.PathEdgeCount) {
    return 0;
  }

  // get and check current edge exists
  int edgeIdx = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent];
  if (PATH_EDGE_IS_EMPTY(edgeIdx)) {
    return 0;
  }

  return path->EdgesJumpSpeed[edgeIdx];
}

//--------------------------------------------------------------------------
int pathShouldJump(struct PathGraph* path, Moby* moby)
{
  if (!moby || !moby->PVar || !path)
    return 0;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  
  // no path
  if (!pvars->MobVars.MoveVars.PathEdgeCount) {
    return 0;
  }

  // reached end
  if (pvars->MobVars.MoveVars.PathHasReachedEnd) {
    return 0;
  }

  u8* currentEdge = pathGetCurrentEdge(path, moby);
  if (currentEdge) {

    // check if edge has jump
    int edgeIdx = pvars->MobVars.MoveVars.CurrentPath[pvars->MobVars.MoveVars.PathEdgeCurrent];
    float jumpSpeed = path->EdgesJumpSpeed[edgeIdx];
    float jumpAt = path->EdgesJumpAt[edgeIdx] / 255.0;
    float lastDistOnEdge = pvars->MobVars.MoveVars.LastPathEdgeAlphaForJump;
    
    // get segment alpha if we haven't refreshed the path this tick
    if (!pvars->MobVars.MoveVars.PathTicks)
      pvars->MobVars.MoveVars.PathEdgeAlpha = pathGetSegmentAlpha(path, moby, currentEdge);

    // update
    pvars->MobVars.MoveVars.LastPathEdgeAlphaForJump = pvars->MobVars.MoveVars.PathEdgeAlpha;

    // we've stepped over threshold for when to jump in the last frame
    if (jumpSpeed > 0 && lastDistOnEdge <= jumpAt && pvars->MobVars.MoveVars.PathEdgeAlpha > jumpAt) {
#if DEBUGPATH
      DPRINTF("jump %f (%f) speed:%f\n", pvars->MobVars.MoveVars.PathEdgeAlpha, jumpAt, jumpSpeed);
#endif
      return 1;
    }
  }

  return 0;
}
