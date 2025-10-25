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
#include <libdl/math.h>
#include <libdl/player.h>

#include "game.h"
#include "orxon.h"
#include "maputils.h"

int isPlayerInGasArea(Player* player);
int isMobyInGasArea(Moby* moby);
Player* wraithTargetOverride = NULL;

extern int aaa;

SoundDef WraithSoundDef =
{
	0.0,	// MinRange
	50.0,	// MaxRange
	100,		// MinVolume
	1000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	158,    // 298, 158
	3 		  // Bank
};


//--------------------------------------------------------------------------
void wraithSetPath(Moby* moby, int currentNode, int nextNode)
{
  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
  if (!pvars)
    return;

	GuberEvent* event = guberCreateEvent(moby, WRAITH_EVENT_PATH_UPDATE);
	if (event) {
		guberEventWrite(event, moby->Position, 12);
		guberEventWrite(event, &currentNode, sizeof(currentNode));
		guberEventWrite(event, &nextNode, sizeof(nextNode));
	}
}

//--------------------------------------------------------------------------
void wraithSetTarget(Moby* moby, Player* target)
{
  int playerId = -1;
  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
  if (!pvars)
    return;

  if (target) {
    playerId = target->PlayerId;
  }
  
	GuberEvent* event = guberCreateEvent(moby, WRAITH_EVENT_TARGET_UPDATE);
	if (event) {
		guberEventWrite(event, moby->Position, 12);
		guberEventWrite(event, &playerId, sizeof(playerId));
	}
}

//--------------------------------------------------------------------------
void wraithCheckRemotePosition(Moby* moby, VECTOR remotePosition)
{
  VECTOR delta;

  vector_subtract(delta, moby->Position, remotePosition);
  if (vector_sqrmag(delta) > (WRAITH_MAX_REMOTE_DIST_TO_TP*WRAITH_MAX_REMOTE_DIST_TO_TP)) {
    vector_copy(moby->Position, remotePosition);
  }
}

//--------------------------------------------------------------------------
void wraithDrawGasParticle(VECTOR position, WraithParticle_t* particle, int texId, float wraithScale)
{
	struct QuadDef quad;
  float scale = (particle->Scale + (sinf(particle->ScaleBreatheSpeed * (gameGetTime() / 1000.0) + particle->Random) * particle->ScaleBreathe)) * wraithScale;
	MATRIX m2, mScale, mTexRot;
	VECTOR t;
  VECTOR vScale = {scale,scale,scale,0};
	VECTOR pTL = {0.5,0,0.5,1};
	VECTOR pTR = {-0.5,0,0.5,1};
	VECTOR pBL = {0.5,0,-0.5,1};
	VECTOR pBR = {-0.5,0,-0.5,1};
  
	// set draw args
	matrix_unit(m2);

  // create scale matrix
	matrix_unit(mScale);
  matrix_scale(mScale, mScale, vScale);

  // create texture rotation
  matrix_unit(mTexRot);
  matrix_rotate_y(mTexRot, mTexRot, particle->RTheta);

	// init
  gfxResetQuad(&quad);

	// color of each corner?
	vector_apply(quad.VertexPositions[0], pTL, mTexRot);
	vector_apply(quad.VertexPositions[1], pTR, mTexRot);
	vector_apply(quad.VertexPositions[2], pBL, mTexRot);
	vector_apply(quad.VertexPositions[3], pBR, mTexRot);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = particle->Color;
	quad.VertexUVs[0] = (struct UV){0,0};
	quad.VertexUVs[1] = (struct UV){1,0};
	quad.VertexUVs[2] = (struct UV){0,1};
	quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 0x0000000100000001;
	quad.Tex0 = gfxGetEffectTex(texId, 1);
	quad.Tex1 = 0;
	quad.Alpha = 0x8000000048;

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

  // get camera aligned offset
  t[0] = cosf(particle->PTheta);
  t[1] = 0;
  t[2] = sinf(particle->PTheta);
  vector_scale(t, t, particle->RRadius);
  vector_add(t, t, particle->Offset);
  t[2] += particle->Height;
  vector_apply(t, t, m2);
  vector_scale(t, t, wraithScale);

  // apply scale
  matrix_multiply(m2, mScale, m2);

  // position
	vector_add(&m2[12], position, t);
  
	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

//--------------------------------------------------------------------------
void wraithPostDraw(Moby* moby)
{
  int i;
  float dt = MATH_DT;
  VECTOR position = {0,0,1,0};
  WraithParticle_t eyeParticle;
	struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
	if (!pvars)
		return;

  int isAggro = pvars->Target || wraithTargetOverride;

  // compute scale
  float targetScale = isMobyInGasArea(moby) ? 1 : WRAITH_OUTSIDE_GAS_SCALE;
  pvars->Scale = lerpf(pvars->Scale, targetScale, 0.1 * MATH_DT);

  if (isAggro) {
    dt *= 10;
    //targetScale *= 1 + randRange(-0.5, 0.5);
  }

  // 
  vector_add(position, moby->Position, position);

  for (i = 0; i < WRAITH_MAX_PARTICLES; ++i) {
    WraithParticle_t* particle = &pvars->Particles[i];
    if (particle->Scale <= 0)
      continue;

    particle->PTheta = clampAngle(particle->PTheta + particle->PSpeed*dt);
    particle->RTheta = clampAngle(particle->RTheta + particle->RSpeed*dt);
    
    if (1) {
      if (particle->Randomness[3] == 0) {
        particle->Randomness[0] = ((rand(2)*2)-1);
        particle->Randomness[1] = ((rand(2)*2)-1);
        particle->Randomness[2] = ((rand(2)*2)-1);
        particle->Randomness[3] = randRangeInt(5, 60);
      } else {
        particle->Randomness[3] -= 1;
      }
      particle->Offset[0] = clamp(particle->Offset[0] + (particle->Randomness[0] * particle->PRandomess[0] * dt), -particle->PRadius, particle->PRadius);
      particle->Offset[1] = clamp(particle->Offset[1] + (particle->Randomness[1] * particle->PRandomess[1] * dt), -particle->PRadius, particle->PRadius);
      particle->Offset[2] = clamp(particle->Offset[2] + (particle->Randomness[2] * particle->PRandomess[2] * dt), -particle->PRadius, particle->PRadius);
    }

    particle->ScaleBreatheSpeed = 60 * dt;
    wraithDrawGasParticle(position, particle, particle->TexId, pvars->Scale);
  }

  // draw eyes
  memcpy(&eyeParticle, &pvars->Particles[5], sizeof(WraithParticle_t));
  eyeParticle.Scale = 0.4;
  eyeParticle.Color = isAggro ? 0x400000C0 : 0x40303030;
  eyeParticle.RRadius = 0;
  eyeParticle.RTheta = 0;
  eyeParticle.Height += 0.6;
  eyeParticle.Offset[0] += WRAITH_EYE_SPACING;
  wraithDrawGasParticle(position, &eyeParticle, 12, pvars->Scale);
  eyeParticle.Offset[0] -= WRAITH_EYE_SPACING * 2;
  wraithDrawGasParticle(position, &eyeParticle, 12, pvars->Scale);
}

//--------------------------------------------------------------------------
void wraithScanPlayers(Moby* moby)
{
  int i;
  VECTOR delta;
  MobyColDamageIn in;
  float hitRadius;

  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
  if (!pvars)
    return;

  // scale hit radius
  hitRadius = WRAITH_HIT_RADIUS * pvars->Scale;

  // damage players in cloud
  Player** players = playerGetAll();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->SkinMoby || !player->PlayerMoby || playerIsDead(player))
      continue;

    // check if in aggro range
    // if in range, target player
    vector_subtract(delta, player->PlayerPosition, moby->Position);
    if (gameAmIHost() && !pvars->Target && isPlayerInGasArea(player) && player->SkinMoby->Opacity >= 0x80) {
      if (vector_sqrmag(delta) < (WRAITH_AGGRO_RANGE*WRAITH_AGGRO_RANGE)) {
        wraithSetTarget(moby, player);
      }
    }

    // have local player's manage their own damage
    if (!player->IsLocal)
      continue;

    // can't hit players with shield
    if (player->timers.armorLevelTimer > 0)
      continue;

    // cylindrical collision check
    // check caps first, then radius
    if (delta[2] < -hitRadius)
      continue;
    if ((delta[2] - WRAITH_HEIGHT) > hitRadius)
      continue;

    delta[2] = 0;
    if (vector_sqrmag(delta) > (hitRadius*hitRadius))
      continue;

    // damage
    vector_write(in.Momentum, 0);
    in.Damager = moby;
    in.DamageFlags = 0x00081881; // acid
    in.DamageClass = 0;
    in.DamageStrength = 1;
    in.DamageIndex = moby->OClass;
    in.Flags = 1;
    in.DamageHp = WRAITH_DAMAGE;
    
    // do less damage outside gas
    if (!isMobyInGasArea(moby)) {
      in.DamageHp = WRAITH_OUTSIDE_GAS_DAMAGE;
    }

    mobyCollDamageDirect(player->PlayerMoby, &in);
  }
}

//--------------------------------------------------------------------------
// Picks a random node to go to next from the list of connected nodes.
// It tries to avoid picking the same node that the wraith came from.
void wraithGetNextNode(Moby* moby)
{
  int nextNodeIdx = 0;
  int i;
  int possibleNextNodesCount = 0;
  char possibleNextNodes[sizeof(((GasPathNode_t *)0)->ConnectedNodes)];

  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
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

  wraithSetPath(moby, pvars->NextNodeIdx, nextNodeIdx);
}

//--------------------------------------------------------------------------
void wraithMove(Moby* moby)
{
  int i;
  VECTOR delta;
  float speed;

  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
  if (!pvars)
    return;

  speed = pvars->Speed;

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
    wraithGetNextNode(moby);
  }

  GasPathNode_t* to = &GAS_PATHFINDING_NODES[pvars->NextNodeIdx];

  // if we have a target then move towards them
  // otherwise move towards next node
  if (wraithTargetOverride) {
    vector_subtract(delta, wraithTargetOverride->PlayerPosition, moby->Position);
    vector_normalize(delta, delta);
    speed = WRAITH_AGGRO_SPEED;
  } else if (pvars->Target) {
    vector_subtract(delta, pvars->Target->PlayerPosition, moby->Position);
    vector_normalize(delta, delta);
  } else {
    vector_subtract(delta, to->Position, moby->Position);
    float distanceToTarget = vector_length(delta);
    vector_scale(delta, delta, 1 / distanceToTarget);
    if (distanceToTarget < 1 && gameAmIHost()) {
      wraithGetNextNode(moby);
    }
  }

  if (!isMobyInGasArea(moby))
    speed = WRAITH_OUTSIDE_GAS_SPEED;

  // set speed
  vector_scale(delta, delta, (MATH_DT * speed));

  // move
  vector_add(moby->Position, moby->Position, delta);
}

//--------------------------------------------------------------------------
void wraithUpdate(Moby* moby)
{
  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
  if (!pvars)
    return;

  // set draw dist
  moby->DrawDist = isMobyInGasArea(moby) ? 50 : 128;

  // draw
  if (isInDrawDist(moby)) {
	  gfxRegisterDrawFunction((void**)0x0022251C, (gfxDrawFuncDef*)&wraithPostDraw, moby);
  }
  
  // play sound
  int ambientSoundTicks = decTimerU8(&pvars->AmbientSoundTicks);
  if (!ambientSoundTicks) {
    
    // play sound
    soundPlay(&WraithSoundDef, 0, moby, 0, 0x400);

    pvars->AmbientSoundTicks = randRangeInt(WRAITH_AMBIENT_SOUND_COOLDOWN_MIN, WRAITH_AMBIENT_SOUND_COOLDOWN_MAX);
  }

  // ignore target if they leave the gas
  // or if they die
  // or if they use a cloak
  if (pvars->Target && pvars->Target->IsLocal && (!isPlayerInGasArea(pvars->Target) || playerIsDead(pvars->Target) || pvars->Target->SkinMoby->Opacity < 0x80)) {
    wraithSetTarget(moby, NULL);
  }

  // damage players in cloud
  // and find new targets
  wraithScanPlayers(moby);

  // move
#if !WRAITH_IDLE
  wraithMove(moby);
#endif
}

//--------------------------------------------------------------------------
int wraithHandleEvent_Spawned(Moby* moby, GuberEvent* event)
{
	int i;
  
  DPRINTF("wraith spawned: %08X\n", (u32)moby);
  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, moby->Position, 12);

	// set update
	moby->PUpdate = &wraithUpdate;
  moby->DrawDist = 50;
  moby->Bolts = -1; // tells gamemode to allow this to damage players

  // init pvars
  pvars->Speed = WRAITH_WALK_SPEED;
  pvars->Scale = 1;

  // init particles
  for (i = 0; i < WRAITH_MAX_PARTICLES; ++i) {
    WraithParticle_t* particle = &pvars->Particles[i];
    float t = (i % WRAITH_BASE_PARTICLE_COUNT) / (float)(WRAITH_BASE_PARTICLE_COUNT-1);
    int type = i / WRAITH_BASE_PARTICLE_COUNT;

    vector_write(particle->Offset, 0);
    vector_write(particle->PRandomess, 0);
    particle->Random = randRange(-1, 1);
    particle->PSpeed = randRange(-1, 1);
    particle->RSpeed = randRange(-1, 1);
    particle->Color = 0x20003000;
    particle->Scale = (t + 1) * 3;
    particle->ScaleBreatheSpeed = 1;
    particle->ScaleBreathe = 0;
    particle->Height = t * WRAITH_HEIGHT;
    particle->RRadius = t;
    particle->TexId = 12; // 42, 71, 72, 

    // 100 = many tiny particles
    // 59 = soft gaussian particles
    // 57 = soft spiky particles
    // 46 = noisy soft particles
    // 16 = 

    switch (type)
    {
      case 0: // skeleton
      {
        particle->PSpeed = 0;
        particle->PRandomess[2] = 0.3 * t;
        particle->RSpeed = 0;
        particle->Color = 0x40004000;
        particle->PRadius = 0.5 * t;
        particle->ScaleBreathe = 2 * t;
        particle->RRadius = 0;
        particle->TexId = 19;
        if (i == WRAITH_HEAD_PARTICLE_IDX) {
          particle->TexId = 12;
          particle->ScaleBreathe = 0;
          particle->PRadius = 0.25 * t;
        }
        else if (i == 4) {
          particle->Scale = 0;
        }
        break;
      }
      case 1: // 
      {
        float randomness = 0.5 * t;
        particle->PRandomess[0] = randomness;
        particle->PRandomess[1] = randomness;
        particle->PRandomess[2] = randomness;
        particle->RRadius = powf(t, 0.5) * 0.5;
        particle->PRadius = t;
        particle->Scale = (t + 1) * 1;
        particle->TexId = 100;
        break;
      }
      default: // 
      {
        float randomness = 0.5 * t;
        particle->PRandomess[0] = randomness;
        particle->PRandomess[1] = randomness;
        particle->PRandomess[2] = randomness;
        particle->RRadius = powf(t, 1) * 0.75;
        particle->PRadius = t * 0.5;
        particle->TexId = 16;
        break;
      }
    }
  }

  // set default state
	mobySetState(moby, 0, -1);
  return 0;
}

//--------------------------------------------------------------------------
int wraithHandleEvent_TargetUpdate(Moby* moby, GuberEvent* event)
{
  int targetPlayerId = -1;
  VECTOR position;

  //DPRINTF("wraith target update: %08X\n", (u32)moby);
  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
  if (!pvars)
    return 0;
  
	// read event
	guberEventRead(event, position, 12);
	guberEventRead(event, &targetPlayerId, 4);

  // set target
  if (targetPlayerId < 0) {
    pvars->Target = NULL;
    pvars->HasPath = 0;
    pvars->Speed = WRAITH_WALK_SPEED;
  } else {
    pvars->Target = playerGetAll()[targetPlayerId];
    pvars->Speed = WRAITH_AGGRO_SPEED;
  }

  wraithCheckRemotePosition(moby, position);
  return 0;
}

//--------------------------------------------------------------------------
int wraithHandleEvent_PathUpdate(Moby* moby, GuberEvent* event)
{
  int currentNodeIdx, nextNodeIdx;
  VECTOR position;

  //DPRINTF("wraith path update: %08X\n", (u32)moby);
  struct WraithPVar* pvars = (struct WraithPVar*)moby->PVar;
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

  wraithCheckRemotePosition(moby, position);
  return 0;
}

//--------------------------------------------------------------------------
struct GuberMoby* wraithGetGuber(Moby* moby)
{
	if (moby->OClass == WRAITH_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int wraithHandleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event)
		return 0;

	if (isInGame() && !mobyIsDestroyed(moby) && moby->OClass == WRAITH_MOBY_OCLASS && moby->PVar) {
		u32 eventId = event->NetEvent.EventID;
		switch (eventId)
		{
			case WRAITH_EVENT_SPAWN: return wraithHandleEvent_Spawned(moby, event);
			case WRAITH_EVENT_TARGET_UPDATE: return wraithHandleEvent_TargetUpdate(moby, event);
			case WRAITH_EVENT_PATH_UPDATE: return wraithHandleEvent_PathUpdate(moby, event);
			default:
			{
				DPRINTF("unhandle wraith event %d\n", eventId);
				break;
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------
int wraithCreate(VECTOR position)
{
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(WRAITH_MOBY_OCLASS, sizeof(struct WraithPVar), &guberEvent, NULL);
	if (guberEvent)
	{
    position[2] += 1; // spawn slightly above point
		guberEventWrite(guberEvent, position, 12);
	}
	else
	{
		DPRINTF("failed to guberevent wraith\n");
	}
  
  return guberEvent != NULL;
}

//--------------------------------------------------------------------------
void wraithSpawn(void)
{
  static int spawned = 0;
  
  if (spawned)
    return;

  // spawn
  if (gameAmIHost()) {
#if WRAITH_IDLE
    VECTOR p = { 366.6999, 562.2997, 428.0787, 0 };
    wraithCreate(p);
#else
    wraithCreate(GAS_PATHFINDING_NODES[rand(GAS_PATHFINDING_NODES_COUNT)].Position);
#endif
  }

  spawned = 1;
}

//--------------------------------------------------------------------------
void wraithInit(void)
{
  Moby* temp = mobySpawn(WRAITH_MOBY_OCLASS, 0);
  if (!temp)
    return;

  // set vtable callbacks
  u32 mobyFunctionsPtr = (u32)mobyGetFunctions(temp);
  if (mobyFunctionsPtr) {
    *(u32*)(mobyFunctionsPtr + 0x04) = (u32)&wraithGetGuber;
    *(u32*)(mobyFunctionsPtr + 0x14) = (u32)&wraithHandleEvent;
    DPRINTF("WRAITH oClass:%04X mClass:%02X func:%08X getGuber:%08X handleEvent:%08X\n", temp->OClass, temp->MClass, mobyFunctionsPtr, *(u32*)(mobyFunctionsPtr + 0x04), *(u32*)(mobyFunctionsPtr + 0x14));
  }
  mobyDestroy(temp);
}
