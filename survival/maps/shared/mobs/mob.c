#include <tamtypes.h>
#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/math.h>
#include <libdl/random.h>
#include <libdl/math3d.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/sound.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/collision.h>
#include <libdl/utils.h>
#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../include/gate.h"
#include "../include/maputils.h"
#include "../include/pathfind.h"
#include "messageid.h"
#include "module.h"

void mobForceIntoMapBounds(Moby* moby);

#if GATE
void gateSetCollision(int collActive);
#endif

extern int aaa;

#if MOB_ZOMBIE
#include "zombie.c"
#endif

#if MOB_EXECUTIONER
#include "executioner.c"
#endif

#if MOB_TREMOR
#include "tremor.c"
#endif

//--------------------------------------------------------------------------
int mobAmIOwner(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return gameGetMyClientId() == pvars->MobVars.Owner;
}

//--------------------------------------------------------------------------
int mobIsFrozen(Moby* moby)
{
  if (!moby || !moby->PVar || !MapConfig.State)
    return 0;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  return MapConfig.State->Freeze && pvars->MobVars.Config.MobType != MOB_FREEZE && pvars->MobVars.Config.MobSpecialMutation != MOB_SPECIAL_MUTATION_FREEZE;
}

//--------------------------------------------------------------------------
void mobDoDamageTryHit(Moby* moby, Moby* hitMoby, VECTOR jointPosition, float sqrHitRadius, int damageFlags, float amount)
{
  VECTOR mobToHitMoby, mobToJoint, jointToHitMoby;
	MobyColDamageIn in;

  vector_subtract(mobToHitMoby, hitMoby->Position, moby->Position);
  vector_subtract(mobToJoint, jointPosition, moby->Position);
  vector_subtract(jointToHitMoby, hitMoby->Position, jointPosition);

  // ignore if hit behind
  if (vector_innerproduct(mobToHitMoby, mobToJoint) < 0)
    return;

  // ignore if past attack radius
  if (vector_innerproduct(mobToHitMoby, jointToHitMoby) > 0 && vector_sqrmag(jointToHitMoby) > sqrHitRadius)
    return;

  vector_write(in.Momentum, 0);
  in.Damager = moby;
  in.DamageFlags = damageFlags;
  in.DamageClass = 0;
  in.DamageStrength = 1;
  in.DamageIndex = moby->OClass;
  in.Flags = 1;
  in.DamageHp = amount;

  mobyCollDamageDirect(hitMoby, &in);
}

//--------------------------------------------------------------------------
void mobDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire, int jointId)
{
	VECTOR p, delta;
	MATRIX jointMtx;
  Player** players = playerGetAll();
  int i;
  float sqrRadius = radius * radius;
  float firstPassSqrRadius = powf(5 + radius, 2);

  // get position of right spike joint
  mobyGetJointMatrix(moby, jointId, jointMtx);
  vector_copy(p, &jointMtx[12]);

  // if no friendly fire just check hit on players
  // otherwise check all mobys
  if (!friendlyFire) {
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      Player* player = players[i];
      if (!player || !player->SkinMoby || playerIsDead(player))
        continue;

      vector_subtract(delta, player->PlayerPosition, p);
      if (vector_sqrmag(delta) > firstPassSqrRadius)
        continue;

      mobDoDamageTryHit(moby, player->PlayerMoby, p, sqrRadius, damageFlags, amount);
    }
  }
  else if (CollMobysSphere_Fix(p, 2, moby, 0, 5 + radius) > 0) {
    Moby** hitMobies = CollMobysSphere_Fix_GetHitMobies();
    Moby* hitMoby;
    while ((hitMoby = *hitMobies++)) {
      mobDoDamageTryHit(moby, hitMoby, p, sqrRadius, damageFlags, amount);
    }
  }
}

//--------------------------------------------------------------------------
void mobSetAction(Moby* moby, int action)
{
	struct MobActionUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// don't set if already action
	if (pvars->MobVars.Action == action)
		return;

	GuberEvent* event = guberCreateEvent(moby, MOB_EVENT_STATE_UPDATE);
	if (event) {
		args.Action = action;
		args.ActionId = ++pvars->MobVars.ActionId;
		guberEventWrite(event, &args, sizeof(struct MobActionUpdateEventArgs));
	}
}

//--------------------------------------------------------------------------
void mobTransAnimLerp(Moby* moby, int animId, int lerpFrames, float startOff)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (moby->AnimSeqId != animId) {
		mobyAnimTransition(moby, animId, lerpFrames, startOff);

		pvars->MobVars.AnimationReset = 1;
		pvars->MobVars.AnimationLooped = 0;
	} else {
		
		// get current t
		// if our stored start is uninitialized, then set current t as start
		float t = moby->AnimSeqT;
		float end = *(u8*)((u32)moby->AnimSeq + 0x10) - (float)(lerpFrames * 0.5);
		if (t >= end && pvars->MobVars.AnimationReset) {
			pvars->MobVars.AnimationLooped++;
			pvars->MobVars.AnimationReset = 0;
		} else if (t < end) {
			pvars->MobVars.AnimationReset = 1;
		}
	}
}

//--------------------------------------------------------------------------
void mobTransAnim(Moby* moby, int animId, float startOff)
{
	mobTransAnimLerp(moby, animId, 10, startOff);
}

//--------------------------------------------------------------------------
int mobHasVelocity(struct MobPVar* pvars)
{
	VECTOR t;
	vector_projectonhorizontal(t, pvars->MobVars.MoveVars.Velocity);
	return vector_sqrmag(t) >= 0.0001;
}

//--------------------------------------------------------------------------
void mobStand(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return;

  // remove horizontal velocity
  vector_projectonvertical(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity);
}

//--------------------------------------------------------------------------
int mobMoveCheck(Moby* moby, VECTOR outputPos, VECTOR from, VECTOR to)
{
  VECTOR delta, horizontalDelta, reflectedDelta;
  VECTOR hitTo, hitFrom;
  VECTOR hitToEx, hitNormal;
  VECTOR up = {0,0,0,0};
  float angle;
  float collRadius = 0.5; // pvars->MobVars.Config.CollRadius;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return 0;

  up[2] = collRadius;

  // offset hit scan to center of mob
  //vector_add(hitFrom, from, up);
  //vector_add(hitTo, to, up);
  
  // get horizontal delta between to and from
  vector_subtract(delta, to, from);
  vector_projectonhorizontal(horizontalDelta, delta);

  // offset hit scan to center of mob
  vector_add(hitFrom, from, up);
  vector_add(hitTo, hitFrom, horizontalDelta);
  
  vector_normalize(horizontalDelta, horizontalDelta);

  // move to further out to factor in the radius of the mob
  vector_normalize(hitToEx, delta);
  vector_scale(hitToEx, hitToEx, collRadius);
  vector_add(hitTo, hitTo, hitToEx);

  // check if we hit something
  if (CollLine_Fix(hitFrom, hitTo, 2, moby, NULL)) {

    vector_normalize(hitNormal, CollLine_Fix_GetHitNormal());

    // compute wall slope
    pvars->MobVars.MoveVars.WallSlope = getSignedSlope(horizontalDelta, hitNormal); //atan2f(1, vector_innerproduct(horizontalDelta, hitNormal)) - MATH_PI/2;
    pvars->MobVars.MoveVars.HitWall = 1;

    // reflect velocity on hit normal and add back to starting position
    vector_reflect(reflectedDelta, delta, hitNormal);
    //vectorProjectOnHorizontal(delta, delta);
    if (reflectedDelta[2] > delta[2])
      reflectedDelta[2] = delta[2];

    vector_add(outputPos, to, reflectedDelta);
    return 1;
  }

  vector_copy(outputPos, to);
  return 0;
}

//--------------------------------------------------------------------------
void mobMove(Moby* moby)
{
  VECTOR normalizedTargetDirection, planarTargetDirection;
  VECTOR targetVelocity;
  VECTOR nextPos;
  VECTOR temp;
  VECTOR groundCheckFrom, groundCheckTo;
  int isMovingDown = 0;
  int i;
  const VECTOR up = {0,0,1,0};
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  u8 stuckCheckTicks = decTimerU8(&pvars->MobVars.MoveVars.StuckCheckTicks);
  u8 ungroundedTicks = decTimerU8(&pvars->MobVars.MoveVars.UngroundedTicks);
  u8 moveSkipTicks = decTimerU8(&pvars->MobVars.MoveVars.MoveSkipTicks);

  if (moveSkipTicks == 0) {

#if GATE
    gateSetCollision(0);
#endif

    // move next position to last position
    vector_copy(moby->Position, pvars->MobVars.MoveVars.NextPosition);
    vector_copy(pvars->MobVars.MoveVars.LastPosition, pvars->MobVars.MoveVars.NextPosition);

    // reset state
    pvars->MobVars.MoveVars.Grounded = 0;
    pvars->MobVars.MoveVars.WallSlope = 0;
    pvars->MobVars.MoveVars.HitWall = 0;

    if (1)
    {
      // add additive velocity
      vector_add(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.AddVelocity);

      // compute simulated velocity by multiplying velocity by number of ticks to simulate
      vector_scale(targetVelocity, pvars->MobVars.MoveVars.Velocity, MOB_MOVE_SKIP_TICKS);

      // compute next position
      vector_add(nextPos, moby->Position, targetVelocity);

      // move physics check twice to prevent clipping walls
      if (mobMoveCheck(moby, nextPos, moby->Position, nextPos)) {
        mobMoveCheck(moby, nextPos, moby->Position, nextPos);
      }

      // check ground or ceiling
      isMovingDown = vector_innerproduct(targetVelocity, up) < 0;
      if (isMovingDown) {
        vector_copy(groundCheckFrom, nextPos);
        groundCheckFrom[2] = maxf(moby->Position[2], nextPos[2]) + ZOMBIE_BASE_STEP_HEIGHT;
        vector_copy(groundCheckTo, nextPos);
        groundCheckTo[2] -= 0.5;
        if (CollLine_Fix(groundCheckFrom, groundCheckTo, 2, moby, NULL)) {
          // mark grounded this frame
          pvars->MobVars.MoveVars.Grounded = 1;

          // force position to above ground
          vector_copy(nextPos, CollLine_Fix_GetHitPosition());
          nextPos[2] += 0.01;

          // remove vertical velocity from velocity
          vector_projectonhorizontal(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity);
        }
      } else {
        vector_copy(groundCheckFrom, nextPos);
        groundCheckFrom[2] = moby->Position[2];
        vector_copy(groundCheckTo, nextPos);
        //groundCheckTo[2] += ZOMBIE_BASE_STEP_HEIGHT;
        if (CollLine_Fix(groundCheckFrom, groundCheckTo, 2, moby, NULL)) {
          // force position to below ceiling
          vector_copy(nextPos, CollLine_Fix_GetHitPosition());
          nextPos[2] -= 0.01;

          // remove vertical velocity from velocity
          //vectorProjectOnHorizontal(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity);
        }
      }

      // set position
      vector_copy(pvars->MobVars.MoveVars.NextPosition, nextPos);
    }

    // add gravity to velocity with clamp on downwards speed
    pvars->MobVars.MoveVars.Velocity[2] -= GRAVITY_MAGNITUDE * MATH_DT * MOB_MOVE_SKIP_TICKS;
    if (pvars->MobVars.MoveVars.Velocity[2] < -10 * MATH_DT)
      pvars->MobVars.MoveVars.Velocity[2] = -10 * MATH_DT;

    // check if stuck by seeing if the sum horizontal delta position over the last second
    // is less than 1 in magnitude
    if (!stuckCheckTicks) {
      pvars->MobVars.MoveVars.StuckCheckTicks = 60;
      pvars->MobVars.MoveVars.IsStuck = pvars->MobVars.MoveVars.HitWall && vector_sqrmag(pvars->MobVars.MoveVars.SumPositionDelta) < (pvars->MobVars.MoveVars.SumSpeedOver * TPS * 0.25);
      if (!pvars->MobVars.MoveVars.IsStuck) {
        pvars->MobVars.MoveVars.StuckJumpCount = 0;
        pvars->MobVars.MoveVars.StuckTicks = 0;
      }

      // reset counters
      vector_write(pvars->MobVars.MoveVars.SumPositionDelta, 0);
      pvars->MobVars.MoveVars.SumSpeedOver = 0;
    }

    if (pvars->MobVars.MoveVars.IsStuck) {
      pvars->MobVars.MoveVars.StuckTicks++;
    }

    // add horizontal delta position to sum
    vector_subtract(temp, pvars->MobVars.MoveVars.NextPosition, pvars->MobVars.MoveVars.LastPosition);
    vector_projectonhorizontal(temp, temp);
    vector_add(pvars->MobVars.MoveVars.SumPositionDelta, pvars->MobVars.MoveVars.SumPositionDelta, temp);
    pvars->MobVars.MoveVars.SumSpeedOver += vector_sqrmag(temp);

    vector_write(pvars->MobVars.MoveVars.AddVelocity, 0);
    pvars->MobVars.MoveVars.MoveSkipTicks = MOB_MOVE_SKIP_TICKS;
  }

  float t = clamp(1 - (pvars->MobVars.MoveVars.MoveSkipTicks / (float)MOB_MOVE_SKIP_TICKS), 0, 1);
  vector_lerp(moby->Position, pvars->MobVars.MoveVars.LastPosition, pvars->MobVars.MoveVars.NextPosition, t);

  mobForceIntoMapBounds(moby);
}

//--------------------------------------------------------------------------
void mobTurnTowards(Moby* moby, VECTOR towards, float turnSpeed)
{
  VECTOR delta;
  MATRIX mRot;
  
  if (!moby || !moby->PVar)
    return;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  //float turnSpeed = pvars->MobVars.MoveVars.Grounded ? ZOMBIE_TURN_RADIANS_PER_SEC : ZOMBIE_TURN_AIR_RADIANS_PER_SEC;
  float radians = turnSpeed * pvars->MobVars.Config.Speed * MATH_DT;

  vector_subtract(delta, towards, moby->Position);
  float targetYaw = atan2f(delta[1], delta[0]);
  float yawDelta = clampAngle(targetYaw - moby->Rotation[2]);

  moby->Rotation[2] = clampAngle(moby->Rotation[2] + clamp(yawDelta, -radians, radians));
}

//--------------------------------------------------------------------------
void mobGetVelocityToTarget(Moby* moby, VECTOR velocity, VECTOR from, VECTOR to, float speed, float acceleration)
{
  VECTOR targetVelocity;
  VECTOR temp;
  float targetSpeed = speed * MATH_DT;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return;

  if (!pvars->MobVars.MoveVars.Grounded && (pvars->MobVars.LastAction == MOB_ACTION_FLINCH || pvars->MobVars.LastAction == MOB_ACTION_BIG_FLINCH))
    return;

  float collRadius = pvars->MobVars.Config.CollRadius + 0.5;
  
  // stop when at target
  if (pvars->MobVars.Target && targetSpeed > 0) {
    vector_subtract(temp, pvars->MobVars.Target->Position, from);
    //temp[2] = 0;
    float sqrDistance = vector_sqrmag(temp);
    if (sqrDistance < ((collRadius+PLAYER_COLL_RADIUS)*(collRadius+PLAYER_COLL_RADIUS))) {
      targetSpeed = 0;
    }
  }

  // target velocity from rotation
  vector_fromyaw(targetVelocity, moby->Rotation[2]);

  // acclerate velocity towards target velocity
  //vector_normalize(targetVelocity, targetVelocity);
  vector_scale(targetVelocity, targetVelocity, targetSpeed);
  vector_subtract(temp, targetVelocity, velocity);
  vector_projectonhorizontal(temp, temp);
  vector_scale(temp, temp, acceleration * MATH_DT);
  vector_add(velocity, velocity, temp);
}

//--------------------------------------------------------------------------
void mobPostDrawQuad(Moby* moby, int texId, u32 color)
{
	struct QuadDef quad;
	float size = moby->Scale * 2;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0,size,size,1};
	VECTOR pTR = {0,-size,size,1};
	VECTOR pBL = {0,size,-size,1};
	VECTOR pBR = {0,-size,-size,1};
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars)
		return;

	//u32 color = MobLODColors[pvars->MobVars.Config.MobType] | (moby->Opacity << 24);

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
	quad.Tex0 = gfxGetFrameTex(texId);
	quad.Tex1 = 0xFF9000000260;
	quad.Alpha = 0x8000000044;

	GameCamera* camera = cameraGetGameCamera(0);
	if (!camera)
		return;
	
	// set world matrix by joint
	mobyGetJointMatrix(moby, 1, m2);

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

//--------------------------------------------------------------------------
void mobOnSpawned(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
  
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  switch (moby->OClass)
  {
#if MOB_ZOMBIE
    case ZOMBIE_MOBY_OCLASS:
    {
      pvars->VTable = &ZombieVTable;
      break;
    }
#endif
#if MOB_EXECUTIONER
    case EXECUTIONER_MOBY_OCLASS:
    {
      pvars->VTable = &ExecutionerVTable;
      break;
    }
#endif
#if MOB_TREMOR
    case TREMOR_MOBY_OCLASS:
    {
      pvars->VTable = &TremorVTable;
      break;
    }
#endif
    default:
    {
      DPRINTF("unhandled mob spawned oclass:%04X\n", moby->OClass);
      break;
    }
  }
}

//--------------------------------------------------------------------------
void mobOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs e)
{
  // update pathfinding state
  pathSetPath(moby, e.PathStartNodeIdx, e.PathEndNodeIdx, e.PathCurrentEdgeIdx, e.PathHasReachedStart, e.PathHasReachedEnd);
}

//--------------------------------------------------------------------------
void mobInit(void)
{
  MapConfig.OnMobSpawnedFunc = &mobOnSpawned;
}
