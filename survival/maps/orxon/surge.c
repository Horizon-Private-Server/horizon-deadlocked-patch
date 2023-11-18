#include <libdl/stdio.h>
#include <libdl/string.h>
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
#include "orxon.h"
#include "maputils.h"

int powerIsMobyOn(Moby* moby);
void powerOnSurgeMoby(Moby* moby);
int powerMobyHasTempPowerOn(Moby* moby);
int isPlayerInGasArea(Player* player);
int isMobyInGasArea(Moby* moby);

#if MAP_WRAITH
extern Player* wraithTargetOverride;
#endif

extern int aaa;


struct CubicLineEndPoint surgeLines[SURGE_LINE_COUNT] = {
	{
		.iCoreRGBA = 0,
		.iGlowRGBA = 0,
		.bDisabled = 0,
		.bFadeEnd = 0,
		.iNumSkipPoints = 0,
		.numEndPoints = 2,
		.style = 0,
		.vPos = { 0,0,0,0 },
		.vTangent = { 0,0,1,0 },
		.vTangentOccQuat = { 0,0,0,1 }
	},
	{
		.iCoreRGBA = 0,
		.iGlowRGBA = 0,
		.bDisabled = 0,
		.bFadeEnd = 0,
		.iNumSkipPoints = 0,
		.numEndPoints = 2,
		.style = 0,
		.vPos = { 0,0,0,0 },
		.vTangent = { 0,0,1,0 },
		.vTangentOccQuat = { 0,0,0,1 }
	}
};

SoundDef SurgeExplosionSoundDef =
{
	0.0,	// MinRange
	50.0,	// MaxRange
	100,		// MinVolume
	1000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	0x106,  // 0x123, 0x171, 
	3			  // Bank
};

SoundDef SurgeArcSoundDef =
{
	0.0,	// MinRange
	20.0,	// MaxRange
	100,		// MinVolume
	1000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	95,    // 98, 95, 
	3			  // Bank
};


//--------------------------------------------------------------------------
void surgeSetPath(Moby* moby, int currentNode, int nextNode)
{
  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return;

	GuberEvent* event = guberCreateEvent(moby, SURGE_EVENT_PATH_UPDATE);
	if (event) {
		guberEventWrite(event, moby->Position, 12);
		guberEventWrite(event, &currentNode, sizeof(currentNode));
		guberEventWrite(event, &nextNode, sizeof(nextNode));
	}
}

//--------------------------------------------------------------------------
void surgeSetAttached(Moby* moby, Player* player)
{
  int playerId = -1;
  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return;

  if (player) {
    playerId = player->PlayerId;
  }

	GuberEvent* event = guberCreateEvent(moby, SURGE_EVENT_ATTACH_UPDATE);
	if (event) {
		guberEventWrite(event, moby->Position, 12);
		guberEventWrite(event, &playerId, 4);
	}
}

//--------------------------------------------------------------------------
void surgeCheckRemotePosition(Moby* moby, VECTOR remotePosition)
{
  VECTOR delta;

	struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
	if (!pvars)
		return;

  vector_subtract(delta, moby->Position, remotePosition);
  if (vector_sqrmag(delta) > (SURGE_MAX_REMOTE_DIST_TO_TP*SURGE_MAX_REMOTE_DIST_TO_TP)) {
    vector_copy(pvars->Position, remotePosition);
    vector_copy(moby->Position, remotePosition);
  }
}

//--------------------------------------------------------------------------
void surgeArcDamageMoby(Moby* moby, Moby* damageMoby)
{
  MobyColDamageIn in;
  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return;

  // damage
  vector_write(in.Momentum, 0);
  in.Damager = moby;
  in.DamageFlags = 0x00081841; // shock
  in.DamageClass = 0;
  in.DamageStrength = 1;
  in.DamageIndex = moby->OClass;
  in.Flags = 1;
  in.DamageHp = 50;

  mobyCollDamageDirect(damageMoby, &in);
}

//--------------------------------------------------------------------------
void surgeDrawQuad(VECTOR position, float scale, u32 color, int texId)
{
	struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0.5,0,0.5,1};
	VECTOR pTR = {-0.5,0,0.5,1};
	VECTOR pBL = {0.5,0,-0.5,1};
	VECTOR pBR = {-0.5,0,-0.5,1};
  
	// set draw args
	matrix_unit(m2);

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	vector_copy(quad.VertexPositions[0], pTL);
	vector_copy(quad.VertexPositions[1], pTR);
	vector_copy(quad.VertexPositions[2], pBL);
	vector_copy(quad.VertexPositions[3], pBR);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
	quad.VertexUVs[0] = (struct UV){0,0};
	quad.VertexUVs[1] = (struct UV){1,0};
	quad.VertexUVs[2] = (struct UV){0,1};
	quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 1;
	quad.Tex0 = gfxGetEffectTex(texId, 1);
	quad.Tex1 = 0;
	quad.Alpha = 0x8000000058;

	GameCamera* camera = cameraGetGameCamera(0);
	if (!camera)
		return;

	// get forward vector
	vector_subtract(t, camera->pos, position);
	t[2] = 0;
	vector_normalize(&m2[4], t);

	// up vector
	m2[8 + 2] = 1;

	// right vector
	vector_outerproduct(&m2[0], &m2[4], &m2[8]);

  // position
	memcpy(&m2[12], position, sizeof(VECTOR));

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

//--------------------------------------------------------------------------
void surgePostDraw(Moby* moby)
{
  asm (".set noreorder;");
  int i,j;
  u32 color = 0;
  VECTOR delta, lastPosition;
  VECTOR velocity;
  VECTOR gravity = {0,0,-10 * MATH_DT,0};
  
	struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
	if (!pvars)
		return;

  //surgeDrawQuad(moby->Position, 0.1, 0x80800000, 16);

  for (i = 0; i < SURGE_MAX_PARTICLES; ++i) {
    
    if (pvars->Particles[i].Ticker < SURGE_ARC_RENDER_FOR) {
      
      // play sound on arc
      if (pvars->Particles[i].Ticker == (SURGE_ARC_RENDER_FOR-1)) {
        soundPlay(&SurgeArcSoundDef, 0, moby, 0, 0x400);
      }

      vector_copy(velocity, pvars->Particles[i].StartVelocity);
      vector_copy(lastPosition, moby->Position);

      for (j = 0; j < SURGE_LINE_COUNT; ++j) {

        float t = j / (float)(SURGE_LINE_COUNT - 1);

        if (pvars->AttachedTo) {
          color = colorLerp(0x800E56EA, 0x80781AE1, t*t);
        } else if (pvars->StunnedTicks) {
          color = 0x80781AE1;
        } else {
          color = colorLerp(0x80E18000, 0x80781AE1, t*t);
        }

        surgeLines[j].iCoreRGBA = surgeLines[j].iGlowRGBA = color;

        vector_write(surgeLines[j].vTangent, 0);

        vector_scale(delta, gravity, 2);
        vector_add(velocity, velocity, delta);

        vector_scale(delta, velocity, 1);
        vector_copy(surgeLines[j].vPos, lastPosition);
        vector_add(lastPosition, surgeLines[j].vPos, velocity);
      }

      if (!pvars->Particles[i].Ticker) {
        
        // set number of ticks before next arc depending on state
        if (pvars->Particles[i].SurgeArc) {
          pvars->Particles[i].Ticker = randRangeInt(SURGE_SURGE_ARC_TICKER_MIN, SURGE_SURGE_ARC_TICKER_MAX);
        } else if (pvars->AttachedTo) {
          pvars->Particles[i].Ticker = randRangeInt(SURGE_ARC_TICKER_MIN, SURGE_ARC_TICKER_MAX);
        } else if (pvars->StunnedTicks) {
          pvars->Particles[i].Ticker = randRangeInt(SURGE_STUNNED_ARC_TICKER_MIN, SURGE_STUNNED_ARC_TICKER_MAX);
        } else {
          pvars->Particles[i].Ticker = randRangeInt(SURGE_ARC_TICKER_MIN, SURGE_ARC_TICKER_MAX);
        }

        vector_write(pvars->Particles[i].StartVelocity, randVectorRange(-1, 1));
        pvars->Particles[i].StartVelocity[2] = .5;
        vector_scale(pvars->Particles[i].StartVelocity, pvars->Particles[i].StartVelocity, randRange(0.5, 1.0));
      } else {
        pvars->Particles[i].Ticker -= 1;
      }

      gfxDrawCubicLine((void*)0x2225a8, surgeLines, SURGE_LINE_COUNT, (void*)0x383a68, randRange(0.3, 0.6));
    } else {
      pvars->Particles[i].Ticker -= 1;

      // wait for full cooldown before reseting surge arc
      if (pvars->Particles[i].Ticker == SURGE_ARC_RENDER_FOR) {
        pvars->Particles[i].SurgeArc = NULL;
      }
    }

  }
}

//--------------------------------------------------------------------------
// Picks a random node to go to next from the list of connected nodes.
// It tries to avoid picking the same node that the surge came from.
void surgeGetNextNode(Moby* moby)
{
  int nextNodeIdx = 0;
  int i;
  int possibleNextNodesCount = 0;
  char possibleNextNodes[sizeof(((GasPathNode_t *)0)->ConnectedNodes)];

  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return;

  GasPathNode_t* nodeAt = &GAS_PATHFINDING_NODES[pvars->NextNodeIdx];

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

  surgeSetPath(moby, pvars->NextNodeIdx, nextNodeIdx);
}

//--------------------------------------------------------------------------
int surgePlayerOnPath(Moby* moby)
{
  int i;
  VECTOR mobyToPlayer, mobyToNode;
  Player** players = playerGetAll();

  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars) return 0;

  vector_subtract(mobyToNode, GAS_PATHFINDING_NODES[pvars->NextNodeIdx].Position, moby->Position);
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->SkinMoby || playerIsDead(player) || player->SkinMoby->Opacity < 0x80)
      continue;

    vector_subtract(mobyToPlayer, player->PlayerPosition, moby->Position);
    if (vector_sqrmag(mobyToPlayer) < (SURGE_AVOID_PLAYER_RADIUS*SURGE_AVOID_PLAYER_RADIUS)) {
      if (vector_innerproduct_unscaled(mobyToPlayer, mobyToNode) > 0) {
        return 1;
      }
    }
  }

  return 0;
}

//--------------------------------------------------------------------------
void surgeMove(Moby* moby)
{
  int i;
  VECTOR delta;
  VECTOR from, to;
  VECTOR up = {0,0,2,0};
  VECTOR hitoff = {0,0,0.3,0};

  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return;

  // if we don't have a "path"
  // then find nearest node
  if (!pvars->HasPath && gameAmIHost()) {
    float bestSqrDist = 100000;
    int bestNodeIdx = 0;    

    for (i = 0; i < GAS_PATHFINDING_NODES_COUNT; ++i) {
      vector_subtract(delta, GAS_PATHFINDING_NODES[i].Position, moby->Position);
      float sqrDist = vector_sqrmag(delta);
      if (sqrDist < bestSqrDist) {
        bestSqrDist = sqrDist;
        bestNodeIdx = i;
      }
    }

    pvars->CurrentNodeIdx = pvars->NextNodeIdx = bestNodeIdx;
    surgeGetNextNode(moby);
  }

  GasPathNode_t* nextNode = &GAS_PATHFINDING_NODES[pvars->NextNodeIdx];

  // move towards next node
  vector_subtract(delta, nextNode->Position, pvars->Position);
  float distanceToTarget = vector_length(delta);
  vector_scale(delta, delta, 1 / distanceToTarget);
  if (distanceToTarget < 1 && gameAmIHost()) {
    surgeGetNextNode(moby);
  }

  // check that no players are near target node
  // if there are any, then turn around
  if (gameAmIHost() && surgePlayerOnPath(moby) && !pvars->StuckOnPathTicks) {
    surgeSetPath(moby, pvars->NextNodeIdx, pvars->CurrentNodeIdx);
    pvars->StuckOnPathTicks = TPS;
  } else if (pvars->StuckOnPathTicks) {
    pvars->StuckOnPathTicks -= 1;
  }

  // set speed
  vector_scale(delta, delta, (MATH_DT * pvars->Speed));

  // determine angular rotation
  moby->Rotation[0] = 0;
  moby->Rotation[2] = atan2f(delta[1], delta[0]);
  moby->Rotation[1] = clampAngle(moby->Rotation[1] + 5*MATH_PI*MATH_DT);

  // move
  vector_add(pvars->Position, pvars->Position, delta);

  // snap to ground
  vector_add(from, pvars->Position, up);
  vector_subtract(to, pvars->Position, up);
  if (CollLine_Fix(from, to, COLLISION_FLAG_IGNORE_DYNAMIC, moby, NULL)) {
    vector_add(moby->Position, CollLine_Fix_GetHitPosition(), hitoff);
  } else {
    vector_copy(moby->Position, pvars->Position);
  }
}

//--------------------------------------------------------------------------
int surgeSetArc(Moby* moby, Moby* arcTowards, int arcCount)
{
  int i;
  int count = 0;
  VECTOR target;

  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars) return 0;

  for (i = 0; i < SURGE_MAX_PARTICLES; ++i) {
    if (pvars->Particles[i].SurgeArc == arcTowards) {
      --arcCount;
    }
  }

  for (i = 0; i < SURGE_MAX_PARTICLES && arcCount; ++i) {
    if (pvars->Particles[i].Ticker < SURGE_ARC_RENDER_FOR)
      continue;

    pvars->Particles[i].SurgeArc = arcTowards;
    pvars->Particles[i].Ticker = SURGE_ARC_RENDER_FOR;
    vector_write(target, randVectorRange(-arcTowards->Scale*3, arcTowards->Scale*3));
    vector_add(target, target, arcTowards->Position);
    target[2] = arcTowards->Position[2];
    vector_subtract(pvars->Particles[i].StartVelocity, target, moby->Position);
    vector_scale(pvars->Particles[i].StartVelocity, pvars->Particles[i].StartVelocity, 0.5);
    pvars->Particles[i].StartVelocity[2] = 0;
    --arcCount;
    ++count;
  }

  return count;
}

//--------------------------------------------------------------------------
void surgeSpawnSlamExplosion(Moby* moby)
{
  int i;
  u128 vPos = vector_read(moby->Position);

  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return;

  float damage = 120;
  if (pvars->AttachedToPlayer && MapConfig.State) {
    damage *= 1 + (PLAYER_UPGRADE_DAMAGE_FACTOR * MapConfig.State->PlayerStates[pvars->AttachedToPlayer->PlayerId].State.Upgrades[UPGRADE_DAMAGE]);
    damage *= pvars->AttachedToPlayer->DamageMultiplier;
  }

  // spawn explosion
  mobySpawnExplosion
        (vPos, 1, 0x0, 0x0, 0x0, 0x10, 0x10, 0x0, 0, 0, 0, 0,
        1, 0, 0x80080800, 0, 0x80E18000, 0x80E18000, 0x80E18000, 0x80E18000, 0x80E18000, 0x80E18000, 0x80E18000, 0x80E18000,
        0x80E18000, 1, 0, 0, vPos, 3, 0, damage, 15);
  
  // play explosion sound
	soundPlay(&SurgeExplosionSoundDef, 0, moby, 0, 0x400);

  // trigger surge arcs
  for (i = 0; i < SURGE_MAX_PARTICLES; ++i) {
    SurgeParticle_t* particle = &pvars->Particles[i];
    particle->Ticker = SURGE_ARC_RENDER_FOR;
    particle->SurgeArc = NULL;
    
    vector_fromyaw(particle->StartVelocity, MATH_TAU * (i / (float)SURGE_MAX_PARTICLES));
    particle->StartVelocity[2] = 0.5;
    vector_scale(particle->StartVelocity, particle->StartVelocity, 2);
  }
}

//--------------------------------------------------------------------------
void surgeAttached(Moby* moby)
{
  MATRIX jointMtx, offset;
  VECTOR delta;
  int count = 0;
  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return;

  // unattach if not attached to wrench anymore
  if (mobyIsDestroyed(pvars->AttachedTo) || playerIsDead(pvars->AttachedToPlayer) || pvars->AttachedTo->OClass != MOBY_ID_WRENCH) {
    surgeSetAttached(moby, NULL);
    return;
  }

  // player settings
  if (pvars->AttachedToPlayer) {
    
    // set surge boost timers
    pvars->AttachedToPlayer->timers.clankRedEye = 10;

    // hyper strike slam
    if (pvars->AttachedToPlayer->PlayerState == PLAYER_STATE_JUMP_ATTACK && pvars->AttachedToPlayer->PlayerMoby->AnimSeqId == 43) {
      if (pvars->HasSlammed == 1 && pvars->AttachedToPlayer->PlayerMoby->AnimSeqT > 12) {
            
        // spawn explosion to push zombies back
        surgeSpawnSlamExplosion(moby);

        pvars->HasSlammed = 2;
      }
    } else {
      pvars->HasSlammed = 1;
    }
  }
  
  // set position to end of wrench
  mobyGetJointMatrix(pvars->AttachedTo, 2, jointMtx);
  vector_scale(offset, &jointMtx[8], 0.2);
  vector_add(moby->Position, &jointMtx[12], offset);
  vector_copy(moby->Rotation, pvars->AttachedTo->Rotation);
  vector_copy(pvars->Position, moby->Position);

  // activate nearby mobies
  // iterate the list of mobies n times per tick to reduce CPU usage each tick
  // place in list we left off at is pvars->MobyListAt
  Moby* mStart = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();
  Moby* mAt = pvars->MobyListAt;
  if (!mAt || mAt >= mEnd || mAt <= mStart) {
    mAt = mStart;
  } 

  while (mAt < mEnd) {

    if (!mobyIsDestroyed(mAt)) {

      vector_subtract(delta, moby->Position, mAt->Position);
      if (vector_sqrmag(delta) < (SURGE_ATTACHED_POWER_UP_RADIUS*SURGE_ATTACHED_POWER_UP_RADIUS)) {

        switch (mAt->OClass)
        {
          case MOBY_ID_BOLT_CRANK_MP:
          {
            if (!powerIsMobyOn(mAt) && surgeSetArc(moby, mAt, 5)) {
              powerOnSurgeMoby(mAt);
            }
            break;
          }
          case MOBY_ID_JUMP_PAD:
          {
            // activate jumppad
            if (!powerIsMobyOn(mAt) && surgeSetArc(moby, mAt, 1)) {
              powerOnSurgeMoby(mAt);
            }
            break;
          }
          case MOBY_ID_PICKUP_PAD:
          {
            // activate pickup pad
            if (!powerMobyHasTempPowerOn(mAt) && surgeSetArc(moby, mAt, 1)) {
              powerOnSurgeMoby(mAt);
            }
            break;
          }
        }

        // hit mobs
        if (mobyIsMob(mAt))
        {
          if (surgeSetArc(moby, mAt, 1)) {
            surgeArcDamageMoby(moby, mAt);
          }
        }
      }
    }

    ++count;
    ++mAt;

    if (count >= SURGE_ATTACHED_MOBY_LIST_ITERATE_PER_FRAME_COUNT)
      break;
  }

  pvars->MobyListAt = mAt;
}

//--------------------------------------------------------------------------
void surgeUpdate(Moby* moby)
{
  VECTOR delta;
  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return;

  // hidden/active logic
  if (MapConfig.State) {

    // if attached to someone then only hide after round ends
    if (pvars->AttachedTo) {
      //if (MapConfig.State->RoundCompleteTime > 0) {
      //  mobySetState(moby, SURGE_STATE_HIDDEN, -1);
      //  if (gameAmIHost()) {
      //    surgeSetAttached(moby, NULL);
      //  }
      //}
    }
    // spawn only after 50 mobs have spawned this round
    // and we still have at least another 100 to kill
    else if (MapConfig.State->RoundMobSpawnedCount > 50 && (MapConfig.State->RoundMaxMobCount - (MapConfig.State->RoundMobSpawnedCount - MapConfig.State->RoundMobCount)) > 100) {
      mobySetState(moby, SURGE_STATE_ACTIVE, -1);
    }
    else if (isMobyInGasArea(moby)) {
      mobySetState(moby, SURGE_STATE_HIDDEN, -1);
    }
  }
  // no survival mode so just always have surge active
  else {
    mobySetState(moby, SURGE_STATE_ACTIVE, -1);
  }

  // no active so don't do rest
  if (moby->State != SURGE_STATE_ACTIVE) {
    moby->DrawDist = 0;
    return;
  }

  moby->DrawDist = 50;

#if DEBUG || 1

  if (!pvars->AttachedTo) {
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
  }

#endif

  // draw
  if (moby->Drawn) {
	  gfxRegisterDrawFunction((void**)0x0022251C, (gfxDrawFuncDef*)&surgePostDraw, moby);
  }

  // attached
  if (pvars->AttachedTo) {
    surgeAttached(moby);
  }

  // move
  if (!pvars->AttachedTo) {
    if (!pvars->StunnedTicks) {
      surgeMove(moby);
    } else {
      pvars->StunnedTicks -= 1;
      moby->Rotation[0] = clampAngle(moby->Rotation[0] + MATH_DT*MATH_PI*3);
      moby->Rotation[1] = clampAngle(moby->Rotation[1] + MATH_DT*MATH_PI*3);
    }
  }

  // damage
  if (moby->CollDamage >= 0) {
    
    MobyColDamage* colDamage = mobyGetDamage(moby, 0x400C00, 0);
    if (colDamage && colDamage->Damager ) {
      if (colDamage->Damager->OClass == MOBY_ID_WRENCH) {
        Player * damager = guberMobyGetPlayerDamager(colDamage->Damager);
        if (damager && damager->IsLocal) {
          surgeSetAttached(moby, damager);
        }
      }
      
      // stun if hit near
      // by B6 or Arb
      if (colDamage->Damager->OClass == MOBY_ID_B6_BOMB_EXPLOSION || colDamage->Damager->OClass == MOBY_ID_ARBITER_ROCKET0) {
        vector_subtract(delta, colDamage->Damager->Position, moby->Position);
        if (vector_sqrmag(delta) < (3*3)) {
          pvars->StunnedTicks = SURGE_STUN_DURATION_TICKS;
        }
      }
    }

    moby->CollDamage = -1;
  }
}

//--------------------------------------------------------------------------
int surgeHandleEvent_Spawned(Moby* moby, GuberEvent* event)
{
	int i;
  
  DPRINTF("surge spawned: %08X\n", (u32)moby);
  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, moby->Position, 12);

	// set update
	moby->PUpdate = &surgeUpdate;
  moby->DrawDist = 50;
  moby->ModeBits |= 0x4000; // hitable by direct shots

  // init pvars
  pvars->Speed = SURGE_RUN_SPEED;
#if SURGE_IDLE
  pvars->Speed = 0;
#endif

  for (i = 0; i < SURGE_MAX_PARTICLES; ++i) {
    //pvars->Particles[i].Ticker = randRangeInt(TPS, 25 * TPS);
  }

  // set default state
	mobySetState(moby, SURGE_STATE_HIDDEN, -1);
  return 0;
}

//--------------------------------------------------------------------------
int surgeHandleEvent_PathUpdate(Moby* moby, GuberEvent* event)
{
  int currentNodeIdx, nextNodeIdx;
  VECTOR position;

  DPRINTF("surge path update: %08X\n", (u32)moby);
  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, position, 12);
	guberEventRead(event, &currentNodeIdx, 4);
	guberEventRead(event, &nextNodeIdx, 4);

  // set path
  pvars->CurrentNodeIdx = currentNodeIdx;
  pvars->NextNodeIdx = nextNodeIdx;
  pvars->HasPath = 1;

  surgeCheckRemotePosition(moby, position);
  return 0;
}

//--------------------------------------------------------------------------
int surgeHandleEvent_AttachUpdate(Moby* moby, GuberEvent* event)
{
  int playerId;
  VECTOR position;
  Player* player;
  Player** players = playerGetAll();

  DPRINTF("surge path attach: %08X\n", (u32)moby);
  struct SurgePVar* pvars = (struct SurgePVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, position, 12);
	guberEventRead(event, &playerId, 4);

#if MAP_WRAITH
  wraithTargetOverride = NULL;
#endif

  pvars->AttachedTo = NULL;
  pvars->AttachedToPlayer = NULL;
  pvars->HasSlammed = 0;
  pvars->StunnedTicks = 0;
  moby->CollActive = 0;
  if (playerId >= 0) {
    player = players[playerId];
    if (player) {
      pvars->AttachedTo = player->Gadgets[0].pMoby;
      pvars->AttachedToPlayer = player;
      moby->CollActive = -1;

      if (player->IsLocal) {
        pushSnack(player->LocalPlayerIndex, "Surge acquired!", 60);
      }
      
#if MAP_WRAITH
      wraithTargetOverride = player;
#endif
    }
  }

  surgeCheckRemotePosition(moby, position);
  return 0;
}

//--------------------------------------------------------------------------
struct GuberMoby* surgeGetGuber(Moby* moby)
{
	if (moby->OClass == SURGE_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int surgeHandleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event)
		return 0;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == SURGE_MOBY_OCLASS && moby->PVar) {
		u32 eventId = event->NetEvent.EventID;
		switch (eventId)
		{
			case SURGE_EVENT_SPAWN: return surgeHandleEvent_Spawned(moby, event);
			case SURGE_EVENT_PATH_UPDATE: return surgeHandleEvent_PathUpdate(moby, event);
			case SURGE_EVENT_ATTACH_UPDATE: return surgeHandleEvent_AttachUpdate(moby, event);
			default:
			{
				DPRINTF("unhandle surge event %d\n", eventId);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
int surgeCreate(VECTOR position)
{
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(SURGE_MOBY_OCLASS, sizeof(struct SurgePVar), &guberEvent, NULL);
	if (guberEvent)
	{
    position[2] += 1; // spawn slightly above point
		guberEventWrite(guberEvent, position, 12);
	}
	else
	{
		DPRINTF("failed to guberevent surge\n");
	}
  
  return guberEvent != NULL;
}

//--------------------------------------------------------------------------
void surgeSpawn(void)
{
  static int spawned = 0;
  if (spawned)
    return;

  // spawn
  if (gameAmIHost()) {
#if SURGE_IDLE
    VECTOR p = { 366.6999, 562.2997, 428.0787, 0 };
    surgeCreate(p);
#else
    surgeCreate(GAS_PATHFINDING_NODES[rand(GAS_PATHFINDING_NODES_COUNT)].Position);
#endif
  }

  spawned = 1;
}

//--------------------------------------------------------------------------
void surgeInit(void)
{
  Moby* temp = mobySpawn(SURGE_MOBY_OCLASS, 0);
  if (!temp)
    return;

  // set vtable callbacks
  u32 mobyFunctionsPtr = (u32)mobyGetFunctions(temp);
  if (mobyFunctionsPtr) {
    *(u32*)(mobyFunctionsPtr + 0x04) = (u32)&surgeGetGuber;
    *(u32*)(mobyFunctionsPtr + 0x14) = (u32)&surgeHandleEvent;
    DPRINTF("SURGE oClass:%04X mClass:%02X func:%08X getGuber:%08X handleEvent:%08X\n", temp->OClass, temp->MClass, mobyFunctionsPtr, *(u32*)(mobyFunctionsPtr + 0x04), *(u32*)(mobyFunctionsPtr + 0x14));
  }
  mobyDestroy(temp);
}
