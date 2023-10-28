#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/radar.h>
#include <libdl/camera.h>
#include <libdl/guber.h>
#include <libdl/color.h>
#include <libdl/math3d.h>
#include <libdl/math.h>
#include <libdl/player.h>

#include "game.h"
#include "upgrade.h"
#include "mpass.h"
#include "maputils.h"
#include "bigal.h"

extern int aaa;

//--------------------------------------------------------------------------
void bigalSetPath(Moby* moby, int currentNode, int nextNode, u16 stuckOnPathTicks, char stuckForPlayerIdx)
{
  struct BigAlPVar* pvars = (struct BigAlPVar*)moby->PVar;
  if (!pvars)
    return;

	GuberEvent* event = guberCreateEvent(moby, BIGAL_EVENT_PATH_UPDATE);
	if (event) {
		guberEventWrite(event, moby->Position, 12);
		guberEventWrite(event, &currentNode, sizeof(currentNode));
		guberEventWrite(event, &nextNode, sizeof(nextNode));
		guberEventWrite(event, &stuckOnPathTicks, sizeof(stuckOnPathTicks));
		guberEventWrite(event, &stuckForPlayerIdx, sizeof(stuckForPlayerIdx));
	}
}

//--------------------------------------------------------------------------
void bigalCheckRemotePosition(Moby* moby, VECTOR remotePosition)
{
  VECTOR delta;

	struct BigAlPVar* pvars = (struct BigAlPVar*)moby->PVar;
	if (!pvars)
		return;

  vector_subtract(delta, moby->Position, remotePosition);
  if (vector_sqrmag(delta) > (BIGAL_MAX_REMOTE_DIST_TO_TP*BIGAL_MAX_REMOTE_DIST_TO_TP)) {
    vector_copy(pvars->Position, remotePosition);
    vector_copy(moby->Position, remotePosition);
  }
}

//--------------------------------------------------------------------------
// Picks a random node to go to next from the list of connected nodes.
// It tries to avoid picking the same node that the bigal came from.
void bigalGetNextNode(Moby* moby)
{
  int nextNodeIdx = 0;
  int i;
  int possibleNextNodesCount = 0;
  char possibleNextNodes[sizeof(((BigAlPathNode_t *)0)->ConnectedNodes)];

  struct BigAlPVar* pvars = (struct BigAlPVar*)moby->PVar;
  if (!pvars)
    return;

  BigAlPathNode_t* nodeAt = &BIGAL_PATHFINDING_NODES[pvars->NextNodeIdx];

  // find all possible node to go to next
  for (i = 0; i < sizeof(nodeAt->ConnectedNodes); ++i) {
    int nodeIdx = nodeAt->ConnectedNodes[i];
    if (nodeIdx < 0)
      break;
    
    // prevent going backwards
    if (nodeIdx == pvars->CurrentNodeIdx)
      continue;
    
    possibleNextNodes[possibleNextNodesCount++] = nodeIdx;
  }

  // if none, return back to current
  // otherwise randomly select next node
  if (!possibleNextNodesCount) {
    nextNodeIdx = pvars->CurrentNodeIdx;
  } else {
    nextNodeIdx = possibleNextNodes[rand(possibleNextNodesCount)];
  }

  bigalSetPath(moby, pvars->NextNodeIdx, nextNodeIdx, pvars->StuckOnPathTicks, pvars->StuckForPlayerIdx);
}

//--------------------------------------------------------------------------
int bigalPlayerOnPath(Moby* moby)
{
  int i;
  VECTOR mobyToPlayer, mobyToNode;
  Player** players = playerGetAll();

  struct BigAlPVar* pvars = (struct BigAlPVar*)moby->PVar;
  if (!pvars)
    return;

  // reset
  pvars->StuckForPlayerIdx = -1;

  vector_subtract(mobyToNode, BIGAL_PATHFINDING_NODES[pvars->NextNodeIdx].Position, moby->Position);
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->SkinMoby || playerIsDead(player) || player->SkinMoby->Opacity < 0x80)
      continue;

    vector_subtract(mobyToPlayer, player->PlayerPosition, moby->Position);
    if (vector_sqrmag(mobyToPlayer) < (BIGAL_AVOID_PLAYER_RADIUS*BIGAL_AVOID_PLAYER_RADIUS)) {
      pvars->StuckForPlayerIdx = i;
      return 1;
    }
  }
}

//--------------------------------------------------------------------------
void bigalMove(Moby* moby)
{
  int i;
  VECTOR delta, bitangent;
  VECTOR from, to;
  VECTOR up = {0,0,2,0};
  VECTOR down = {0,0,-4,0};
  VECTOR right = {1,0,0,0};
  VECTOR hitoff = {0,0,0.01,0};
  Player** players = playerGetAll();
  float targetYaw = 0;
  int targetAnim = BIGAL_ANIM_IDLE;

  struct BigAlPVar* pvars = (struct BigAlPVar*)moby->PVar;
  if (!pvars)
    return;

  // if we don't have a "path"
  // then find nearest node
  if (!pvars->HasPath && gameAmIHost()) {
    float bestSqrDist = 100000;
    int bestNodeIdx = 0;    

    for (i = 0; i < BIGAL_PATHFINDING_NODES_COUNT; ++i) {
      vector_subtract(delta, BIGAL_PATHFINDING_NODES[i].Position, moby->Position);
      float sqrDist = vector_sqrmag(delta);
      if (sqrDist < bestSqrDist) {
        bestSqrDist = sqrDist;
        bestNodeIdx = i;
      }
    }

    pvars->CurrentNodeIdx = pvars->NextNodeIdx = bestNodeIdx;
    bigalGetNextNode(moby);
  }

  BigAlPathNode_t* nextNode = &BIGAL_PATHFINDING_NODES[pvars->NextNodeIdx];

  // check that no players are near target node
  // if there are any, then stun
  if (!pvars->StuckOnPathTicks && gameAmIHost() && bigalPlayerOnPath(moby)) {
    //bigalSetPath(moby, pvars->NextNodeIdx, pvars->CurrentNodeIdx);
    pvars->StuckOnPathTicks = TPS;
    
    // broadcast
    bigalSetPath(moby, pvars->CurrentNodeIdx, pvars->NextNodeIdx, pvars->StuckOnPathTicks, pvars->StuckForPlayerIdx);
  }
    
  // if stuck, look at player
  if (pvars->StuckOnPathTicks) {
    
    // decrement
    // when ticker hits 0
    // rebroadcast path state
    if (gameAmIHost()) {
      pvars->StuckOnPathTicks -= 1;
      if (!pvars->StuckOnPathTicks) {
        bigalSetPath(moby, pvars->CurrentNodeIdx, pvars->NextNodeIdx, pvars->StuckOnPathTicks, pvars->StuckForPlayerIdx);
      }
    }

    // determine angular rotation towards player
    if (pvars->StuckForPlayerIdx >= 0) {
      Player* targetPlayer = players[pvars->StuckForPlayerIdx];
      if (targetPlayer) {
        vector_subtract(delta, targetPlayer->PlayerPosition, moby->Position);
        targetYaw = atan2f(delta[1], delta[0]);
      }
    }

    // set anim
    targetAnim = BIGAL_ANIM_SCHEMING;
  } else {

    // move towards next node
    vector_subtract(delta, nextNode->Position, pvars->Position);
    float distanceToTarget = vector_length(delta);
    vector_scale(delta, delta, 1 / distanceToTarget);
    if (distanceToTarget < 1 && gameAmIHost()) {
      bigalGetNextNode(moby);
    }
    
    // set speed
    vector_scale(delta, delta, (MATH_DT * pvars->Speed));

    // move
    vector_add(pvars->Position, pvars->Position, delta);

    // determine target yaw
    targetYaw = atan2f(delta[1], delta[0]);

    // set anim
    targetAnim = BIGAL_ANIM_WALKING;
  }

  // determine angular rotation
  moby->Rotation[0] = 0;
  moby->Rotation[1] = 0;
  moby->Rotation[2] = lerpfAngle(moby->Rotation[2], targetYaw, 0.1);

  // snap to ground
  vector_add(from, pvars->Position, up);
  vector_add(to, pvars->Position, down);
  if (CollLine_Fix(from, to, 2, moby, 0)) {
    vector_add(moby->Position, CollLine_Fix_GetHitPosition(), hitoff);
  } else {
    vector_copy(moby->Position, pvars->Position);
  }

  // set anim
  if (moby->AnimSeqId != targetAnim) {
    mobyAnimTransition(moby, targetAnim, 10, 0);
  }
}

//--------------------------------------------------------------------------
void bigalUpdate(Moby* moby)
{
  VECTOR delta;
  struct BigAlPVar* pvars = (struct BigAlPVar*)moby->PVar;
  if (!pvars)
    return;

  // no active so don't do rest
  if (moby->State != BIGAL_STATE_ACTIVE) {
    moby->DrawDist = 0;
    return;
  }

  // add blip
  int blipId = radarGetBlipIndex(moby);
  if (blipId >= 0)
  {
    RadarBlip * blip = radarGetBlips() + blipId;
    blip->X = moby->Position[0];
    blip->Y = moby->Position[1];
    blip->Life = 0x1F;
    blip->Type = 4;
    blip->Team = 2;
  }

  // move
  bigalMove(moby);
}

//--------------------------------------------------------------------------
int bigalHandleEvent_Spawned(Moby* moby, GuberEvent* event)
{
	int i,j;
  
  DPRINTF("bigal spawned: %08X\n", (u32)moby);
  struct BigAlPVar* pvars = (struct BigAlPVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, moby->Position, 12);

	// set update
	moby->PUpdate = &bigalUpdate;
  //moby->DrawDist = 50;
  //moby->ModeBits |= 0x4000; // hitable by direct shots

  // init pvars
  pvars->Speed = BIGAL_RUN_SPEED;
#if BIGAL_IDLE
  pvars->Speed = 0;
#endif

  // update bigal reference in state
  if (MapConfig.State) {
    MapConfig.State->BigAl = moby;
  }

  // set default state
	mobySetState(moby, BIGAL_STATE_ACTIVE, -1);
  return 0;
}

//--------------------------------------------------------------------------
int bigalHandleEvent_PathUpdate(Moby* moby, GuberEvent* event)
{
	int i;
  int currentNodeIdx, nextNodeIdx;
  char stuckForPlayerIdx;
  u16 stuckForTicks;
  VECTOR position;

  DPRINTF("bigal path update: %08X\n", (u32)moby);
  struct BigAlPVar* pvars = (struct BigAlPVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, position, 12);
	guberEventRead(event, &currentNodeIdx, 4);
	guberEventRead(event, &nextNodeIdx, 4);
	guberEventRead(event, &stuckForTicks, 2);
	guberEventRead(event, &stuckForPlayerIdx, 1);

  // set path
  pvars->CurrentNodeIdx = currentNodeIdx;
  pvars->NextNodeIdx = nextNodeIdx;
  pvars->StuckForPlayerIdx = stuckForPlayerIdx;
  pvars->StuckOnPathTicks = stuckForTicks;
  pvars->HasPath = 1;

  bigalCheckRemotePosition(moby, position);
  return 0;
}

//--------------------------------------------------------------------------
struct GuberMoby* bigalGetGuber(Moby* moby)
{
	if (moby->OClass == BIGAL_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int bigalHandleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event)
		return 0;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == BIGAL_MOBY_OCLASS && moby->PVar) {
		u32 eventId = event->NetEvent.EventID;
		switch (eventId)
		{
			case BIGAL_EVENT_SPAWN: return bigalHandleEvent_Spawned(moby, event);
			case BIGAL_EVENT_PATH_UPDATE: return bigalHandleEvent_PathUpdate(moby, event);
			default:
			{
				DPRINTF("unhandle bigal event %d\n", eventId);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
int bigalCreate(VECTOR position)
{
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(BIGAL_MOBY_OCLASS, sizeof(struct BigAlPVar), &guberEvent, NULL);
	if (guberEvent)
	{
    position[2] += 1; // spawn slightly above point
		guberEventWrite(guberEvent, position, 12);
	}
	else
	{
		DPRINTF("failed to guberevent bigal\n");
	}
  
  return guberEvent != NULL;
}

//--------------------------------------------------------------------------
void bigalSpawn(void)
{
  static int spawned = 0;
  int i;
  
  if (spawned)
    return;

  // spawn
  if (gameAmIHost()) {
#if BIGAL_IDLE
    VECTOR p = {639.44,847.07,499.28,0};
    bigalCreate(p);
#else
    bigalCreate(BIGAL_PATHFINDING_NODES[rand(BIGAL_PATHFINDING_NODES_COUNT)].Position);
#endif
  }

  spawned = 1;
}

//--------------------------------------------------------------------------
void bigalInit(void)
{
  int i;
  Moby* temp = mobySpawn(BIGAL_MOBY_OCLASS, 0);
  if (!temp)
    return;

  // set vtable callbacks
  u32 mobyFunctionsPtr = (u32)mobyGetFunctions(temp);
  if (mobyFunctionsPtr) {
    *(u32*)(mobyFunctionsPtr + 0x04) = (u32)&bigalGetGuber;
    *(u32*)(mobyFunctionsPtr + 0x14) = (u32)&bigalHandleEvent;
    DPRINTF("BIGAL oClass:%04X mClass:%02X func:%08X getGuber:%08X handleEvent:%08X\n", temp->OClass, temp->MClass, mobyFunctionsPtr, *(u32*)(mobyFunctionsPtr + 0x04), *(u32*)(mobyFunctionsPtr + 0x14));
  }
  mobyDestroy(temp);
}
