#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/sha1.h>
#include <libdl/radar.h>
#include <libdl/color.h>

#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../include/pathfind.h"
#include "../include/maputils.h"
#include "../include/shared.h"

void swarmerPreUpdate(Moby* moby);
void swarmerPostUpdate(Moby* moby);
void swarmerPostDraw(Moby* moby);
void swarmerMove(Moby* moby);
void swarmerOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs* e);
void swarmerOnDestroy(Moby* moby, int killedByPlayerId, int weaponId);
void swarmerOnDamage(Moby* moby, struct MobDamageEventArgs* e);
int swarmerOnLocalDamage(Moby* moby, struct MobLocalDamageEventArgs* e);
void swarmerOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs* e);
Moby* swarmerGetNextTarget(Moby* moby);
int swarmerGetPreferredAction(Moby* moby);
void swarmerDoAction(Moby* moby);
void swarmerDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire);
void swarmerForceLocalAction(Moby* moby, int action);
short swarmerGetArmor(Moby* moby);
int swarmerIsAttacking(Moby* moby);

void swarmerPlayHitSound(Moby* moby);
void swarmerPlayAmbientSound(Moby* moby);
void swarmerPlayDeathSound(Moby* moby);
int swarmerIsSpawning(struct MobPVar* pvars);
int swarmerCanAttack(struct MobPVar* pvars);
int swarmerGetSideFlipLeftOrRight(struct MobPVar* pvars);
int swarmerIsFlinching(Moby* moby);
float swarmerGetDodgeProbability(Moby* moby);

struct MobVTable SwarmerVTable = {
  .PreUpdate = &swarmerPreUpdate,
  .PostUpdate = &swarmerPostUpdate,
  .PostDraw = &swarmerPostDraw,
  .Move = &swarmerMove,
  .OnSpawn = &swarmerOnSpawn,
  .OnDestroy = &swarmerOnDestroy,
  .OnDamage = &swarmerOnDamage,
  .OnLocalDamage = &swarmerOnLocalDamage,
  .OnStateUpdate = &swarmerOnStateUpdate,
  .GetNextTarget = &swarmerGetNextTarget,
  .GetPreferredAction = &swarmerGetPreferredAction,
  .ForceLocalAction = &swarmerForceLocalAction,
  .DoAction = &swarmerDoAction,
  .DoDamage = &swarmerDoDamage,
  .GetArmor = &swarmerGetArmor,
  .IsAttacking = &swarmerIsAttacking,
};

SoundDef SwarmerSoundDef = {
	0.0,	  // MinRange
	45.0,	  // MaxRange
	0,		  // MinVolume
	1228,		// MaxVolume
	-635,			// MinPitch
	635,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	0x17D,		// Index
	3			  // Bank
};

extern u32 MobPrimaryColors[];
extern u32 MobSecondaryColors[];
extern u32 MobLODColors[];

//--------------------------------------------------------------------------
int swarmerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config)
{
	struct MobSpawnEventArgs args;
  
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(SWARMER_MOBY_OCLASS, sizeof(struct MobPVar), &guberEvent, NULL);
	if (guberEvent)
	{
    if (MapConfig.PopulateSpawnArgsFunc) {
      MapConfig.PopulateSpawnArgsFunc(&args, config, spawnParamsIdx, spawnFromUID == -1, freeAgent);
    }

		u8 random = (u8)rand(100);

    position[2] += 1; // spawn slightly above point
		guberEventWrite(guberEvent, position, 12);
		guberEventWrite(guberEvent, &yaw, 4);
		guberEventWrite(guberEvent, &spawnFromUID, 4);
		guberEventWrite(guberEvent, &freeAgent, 4);
		guberEventWrite(guberEvent, &random, 1);
		guberEventWrite(guberEvent, &args, sizeof(struct MobSpawnEventArgs));
	}
	else
	{
		DPRINTF("failed to guberevent mob\n");
	}
  
  return guberEvent != NULL;
}

//--------------------------------------------------------------------------
void swarmerPreUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (mobIsFrozen(moby))
    return;

  // ambient sounds
	if (!pvars->MobVars.AmbientSoundCooldownTicks && rand(50) == 0) {
		swarmerPlayAmbientSound(moby);
		pvars->MobVars.AmbientSoundCooldownTicks = randRangeInt(SWARMER_AMBSND_MIN_COOLDOWN_TICKS, SWARMER_AMBSND_MAX_COOLDOWN_TICKS);
	}

  // decrement path target pos ticker
  decTimerU8(&pvars->MobVars.MoveVars.PathTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathNewTicks);

  mobPreUpdate(moby);
}

//--------------------------------------------------------------------------
void swarmerPostUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // adjust animSpeed by speed and by animation
	float animSpeed = 0.9 * (pvars->MobVars.Config.Speed / MOB_BASE_SPEED);
  if (moby->AnimSeqId == SWARMER_ANIM_JUMP) {
    animSpeed = 0.9 * (1 - powf(moby->AnimSeqT / 35, 2));
    if (pvars->MobVars.MoveVars.Grounded) {
      animSpeed = 0.9;
    }
  } else if (moby->AnimSeqId == SWARMER_ANIM_JUMP_AND_FALL) {
    //animSpeed = 0.9 * (1 - powf(moby->AnimSeqT / 15, 2));
    if (pvars->MobVars.MoveVars.Grounded) {
      animSpeed = 0.9;
    }
  } else if (swarmerIsFlinching(moby) && !pvars->MobVars.MoveVars.Grounded) {
    animSpeed = 0.5 * (1 - powf(moby->AnimSeqT / 20, 2));
  }

  if (moby->AnimSeqId == SWARMER_ANIM_FLINCH_BACKFLIP_AND_STAND) {
    animSpeed = 1;
  }

	if (mobIsFrozen(moby) || (moby->DrawDist == 0 && pvars->MobVars.Action == SWARMER_ACTION_WALK)) {
		moby->AnimSpeed = 0;
	} else {
		moby->AnimSpeed = animSpeed;
	}
}

//--------------------------------------------------------------------------
void swarmerPostDraw(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  u32 color = MobLODColors[pvars->MobVars.SpawnParamsIdx] | (moby->Opacity << 24);
  mobPostDrawQuad(moby, 127, color, 0);
}

//--------------------------------------------------------------------------
void swarmerAlterTarget(VECTOR out, Moby* moby, VECTOR forward, float amount)
{
	VECTOR up = {0,0,1,0};
	
	vector_outerproduct(out, forward, up);
	vector_normalize(out, out);
	vector_scale(out, out, amount);
}

//--------------------------------------------------------------------------
void swarmerMove(Moby* moby)
{
  mobMove(moby);
}

//--------------------------------------------------------------------------
void swarmerOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs* e)
{
  
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // set scale
  moby->Scale = 0.256339;

  // colors by mob type
	moby->GlowRGBA = MobSecondaryColors[pvars->MobVars.SpawnParamsIdx];
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.SpawnParamsIdx];

  // targeting
	pvars->TargetVars.targetHeight = 1;
  pvars->MobVars.BlipType = 4;

  // default move step
  pvars->MobVars.MoveVars.MoveStep = MOB_MOVE_SKIP_TICKS;
}

//--------------------------------------------------------------------------
void swarmerOnDestroy(Moby* moby, int killedByPlayerId, int weaponId)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// set colors before death so that the corn has the correct color
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.SpawnParamsIdx];
}

//--------------------------------------------------------------------------
void swarmerOnDamage(Moby* moby, struct MobDamageEventArgs* e)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	float damage = e->DamageQuarters / 4.0;
  float newHp = pvars->MobVars.Health - damage;

	int canFlinch = pvars->MobVars.Action != SWARMER_ACTION_FLINCH 
            && pvars->MobVars.Action != SWARMER_ACTION_BIG_FLINCH
            && pvars->MobVars.Action != SWARMER_ACTION_TIME_BOMB 
            && pvars->MobVars.Action != SWARMER_ACTION_TIME_BOMB_EXPLODE
            && pvars->MobVars.FlinchCooldownTicks == 0;

  int isShock = e->DamageFlags & 0x40;

	// destroy
	if (newHp <= 0) {
    tremorForceLocalAction(moby, SWARMER_ACTION_DIE);
    pvars->MobVars.LastHitBy = e->SourceUID;
    pvars->MobVars.LastHitByOClass = e->SourceOClass;
	}

	// knockback
  // swarmers always have knockback
  e->Knockback.Power += 3;
	if (e->Knockback.Power > 0 && (canFlinch || e->Knockback.Force))
	{
		memcpy(&pvars->MobVars.Knockback, &e->Knockback, sizeof(struct Knockback));
	}

  // flinch
	if (mobAmIOwner(moby))
	{
		float damageRatio = damage / pvars->MobVars.Config.Health;
    if (canFlinch) {
      if (isShock) {
        mobSetAction(moby, SWARMER_ACTION_FLINCH);
      }
      else if (e->Knockback.Force || randRangeInt(0, 10) < e->Knockback.Power) {
        mobSetAction(moby, SWARMER_ACTION_BIG_FLINCH);
      }
      else if (randRange(0, 1) < (SWARMER_FLINCH_PROBABILITY * damageRatio)) {
        mobSetAction(moby, SWARMER_ACTION_FLINCH);
      }
    }
	}
}

//--------------------------------------------------------------------------
int swarmerOnLocalDamage(Moby* moby, struct MobLocalDamageEventArgs* e)
{
  // don't filter local damage
  return 1;
}

//--------------------------------------------------------------------------
void swarmerOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs* e)
{
  mobOnStateUpdate(moby, e);
}

//--------------------------------------------------------------------------
Moby* swarmerGetNextTarget(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Player ** players = playerGetAll();
	int i;
	VECTOR delta;
	Moby * currentTarget = pvars->MobVars.Target;
	Player * closestPlayer = NULL;
	float closestPlayerDist = 100000;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = *players;
		if (p && p->SkinMoby && !playerIsDead(p) && p->Health > 0 && p->SkinMoby->Opacity >= 0x80) {
			vector_subtract(delta, p->PlayerPosition, moby->Position);
      float dist = vector_length(delta);

			if (dist < 300) {
				// favor existing target
				if (playerGetTargetMoby(p) == currentTarget)
					dist *= (1.0 / SWARMER_TARGET_KEEP_CURRENT_FACTOR);
				
				// pick closest target
				if (dist < closestPlayerDist) {
					closestPlayer = p;
					closestPlayerDist = dist;
				}
			}
		}

		++players;
	}

	if (closestPlayer)
		return playerGetTargetMoby(closestPlayer);

	return NULL;
}

//--------------------------------------------------------------------------
int swarmerGetPreferredAction(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;

	// no preferred action
	if (swarmerIsAttacking(moby))
		return -1;

	if (swarmerIsSpawning(pvars))
		return -1;

  if (swarmerIsFlinching(moby))
    return -1;

	if (pvars->MobVars.Action == SWARMER_ACTION_DODGE) {
		return -1;
  }

	if (pvars->MobVars.Action == SWARMER_ACTION_JUMP && !pvars->MobVars.MoveVars.Grounded) {
		return SWARMER_ACTION_WALK;
  }

  // jump if we've hit a slope and are grounded
  if (pvars->MobVars.MoveVars.Grounded && pvars->MobVars.MoveVars.HitWall && pvars->MobVars.MoveVars.WallSlope > SWARMER_MAX_WALKABLE_SLOPE) {
    return SWARMER_ACTION_JUMP;
  }

  // jump if we've hit a jump point on the path
  if (pathShouldJump(moby)) {
    return SWARMER_ACTION_JUMP;
  }

	// prevent action changing too quickly
	if (pvars->MobVars.ActionCooldownTicks)
		return -1;

  // check if a projectile is coming towards us
  if (pvars->MobVars.MoveVars.Grounded && mobIsProjectileComing(moby) && randRange(0, 1) <= swarmerGetDodgeProbability(moby)) {
    return SWARMER_ACTION_DODGE;
  }

	// get next target
	Moby * target = swarmerGetNextTarget(moby);
	if (target) {
		vector_copy(t, target->Position);
		vector_subtract(t, t, moby->Position);
		float distSqr = vector_sqrmag(t);
		float attackRadiusSqr = pvars->MobVars.Config.AttackRadius * pvars->MobVars.Config.AttackRadius;

		if (distSqr <= attackRadiusSqr) {
			if (swarmerCanAttack(pvars))
				return pvars->MobVars.Config.MobAttribute != MOB_ATTRIBUTE_EXPLODE ? SWARMER_ACTION_ATTACK : SWARMER_ACTION_TIME_BOMB;
			return SWARMER_ACTION_WALK;
		} else {
			return SWARMER_ACTION_WALK;
		}
	}
	
	return SWARMER_ACTION_IDLE;
}

//--------------------------------------------------------------------------
#if DEBUGPATH
void swarmerRenderPath(Moby* moby)
{
  int x,y;
  int i;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  u8* path = (u8*)pvars->MobVars.MoveVars.CurrentPath;
  int pathLen = pvars->MobVars.MoveVars.PathEdgeCount;
  int pathIdx = pvars->MobVars.MoveVars.PathEdgeCurrent;


  for (i = 0; i < pathLen; ++i) {
    char* edge = MOB_PATHFINDING_EDGES[path[i]];
    if (gfxWorldSpaceToScreenSpace(MOB_PATHFINDING_NODES[edge[1]], &x, &y)) {
      gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, i == pathIdx ? "o" : "-", -1, 4);
    }
  }

  VECTOR t;
  pathGetTargetPos(t, moby);
  if (gfxWorldSpaceToScreenSpace(t, &x, &y)) {
    gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "+", -1, 4);
  }
}
#endif

//--------------------------------------------------------------------------
void swarmerDoAction(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Moby* target = pvars->MobVars.Target;
	VECTOR t, t2;
  float difficulty = 1;
  float turnSpeed = pvars->MobVars.MoveVars.Grounded ? SWARMER_TURN_RADIANS_PER_SEC : SWARMER_TURN_AIR_RADIANS_PER_SEC;
  float acceleration = pvars->MobVars.MoveVars.Grounded ? SWARMER_MOVE_ACCELERATION : SWARMER_MOVE_AIR_ACCELERATION;
  int isInAirFromFlinching = !pvars->MobVars.MoveVars.Grounded 
                      && (pvars->MobVars.LastAction == SWARMER_ACTION_FLINCH || pvars->MobVars.LastAction == SWARMER_ACTION_BIG_FLINCH);

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

#if DEBUGPATH
  gfxRegisterDrawFunction((void**)0x0022251C, (gfxDrawFuncDef*)&swarmerRenderPath, moby);
#endif

	switch (pvars->MobVars.Action)
	{
		case SWARMER_ACTION_SPAWN:
		{
      mobTransAnim(moby, SWARMER_ANIM_ROAR, 0);
      mobStand(moby);
			break;
		}
		case SWARMER_ACTION_FLINCH:
		case SWARMER_ACTION_BIG_FLINCH:
		{
      int animFlinchId = pvars->MobVars.Action == SWARMER_ACTION_BIG_FLINCH ? SWARMER_ANIM_FLINCH_SPIN_AND_STAND2 : SWARMER_ANIM_FLINCH_SPIN_AND_STAND;

      mobTransAnim(moby, animFlinchId, 0);

      if (pvars->MobVars.Knockback.Ticks > 0) {
        float power = PLAYER_KNOCKBACK_BASE_POWER * pvars->MobVars.Knockback.Power;
        vector_fromyaw(t, pvars->MobVars.Knockback.Angle / 1000.0);
        t[2] = 1.0;
        vector_scale(t, t, power * 2 * MATH_DT);
        vector_add(pvars->MobVars.MoveVars.AddVelocity, pvars->MobVars.MoveVars.AddVelocity, t);
      } else if (pvars->MobVars.MoveVars.Grounded) {
        mobStand(moby);
      }
			break;
		}
		case SWARMER_ACTION_IDLE:
		{
			mobTransAnim(moby, SWARMER_ANIM_IDLE, 0);
      mobStand(moby);
			break;
		}
		case SWARMER_ACTION_JUMP:
			{
        // move
        if (!isInAirFromFlinching) {
          if (target) {
            pathGetTargetPos(t, moby);
            mobTurnTowards(moby, t, turnSpeed);
            mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, t, pvars->MobVars.Config.Speed, acceleration);
          } else {
            mobStand(moby);
          }
        }

        // handle jumping
        if (pvars->MobVars.MoveVars.Grounded) {
			    mobTransAnim(moby, SWARMER_ANIM_JUMP_AND_FALL, 5);

          // check if we're near last jump pos
          // if so increment StuckJumpCount
          if (pvars->MobVars.MoveVars.IsStuck) {
            if (pvars->MobVars.MoveVars.StuckJumpCount < 255)
              pvars->MobVars.MoveVars.StuckJumpCount++;
          }

          // use delta height between target as base of jump speed
          // with min speed
          float jumpSpeed = pathGetJumpSpeed(moby);
          if (jumpSpeed <= 0 && target) {
            jumpSpeed = 8; //clamp(0 + (target->Position[2] - moby->Position[2]) * fabsf(pvars->MobVars.MoveVars.WallSlope) * 1, 3, 15);
          }

          //DPRINTF("jump %f\n", jumpSpeed);
          pvars->MobVars.MoveVars.Velocity[2] = jumpSpeed * MATH_DT;
          pvars->MobVars.MoveVars.Grounded = 0;
        }
				break;
			}
		case SWARMER_ACTION_LOOK_AT_TARGET:
    {
      mobStand(moby);
      if (target)
        mobTurnTowards(moby, target->Position, turnSpeed);
      break;
    }
    case SWARMER_ACTION_WALK:
		{
      if (!isInAirFromFlinching) {
        if (target) {

          float dir = ((pvars->MobVars.ActionId + pvars->MobVars.Random) % 3) - 1;

          // determine next position
          pathGetTargetPos(t, moby);
          //vector_copy(t, target->Position);
          vector_subtract(t, t, moby->Position);
          float dist = vector_length(t);
          if (dist < 10.0) {
            swarmerAlterTarget(t2, moby, t, clamp(dist, 0, 10) * 0.3 * dir);
            vector_add(t, t, t2);
          }
          vector_scale(t, t, 1 / dist);
          vector_add(t, moby->Position, t);


          mobTurnTowards(moby, t, turnSpeed);
          mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, t, pvars->MobVars.Config.Speed, acceleration);
        } else {
          // stand
          mobStand(moby);
        }
      }
      
			// 
      if (moby->AnimSeqId == SWARMER_ANIM_JUMP_AND_FALL && !pvars->MobVars.MoveVars.Grounded) {
        // wait for jump to land
      }
			else if (mobHasVelocity(pvars))
				mobTransAnim(moby, SWARMER_ANIM_WALK, 0);
			else
				mobTransAnim(moby, SWARMER_ANIM_IDLE, 0);
			break;
		}
    case SWARMER_ACTION_DODGE:
    {
      if (!pvars->MobVars.CurrentActionForTicks) {
        // determine left or right flip
        int leftOrRight = swarmerGetSideFlipLeftOrRight(pvars);
        float dir = leftOrRight*2 - 1;

        // compute velocity from forward and direction (strafe)
        vector_fromyaw(t, moby->Rotation[2] + (MATH_PI/2)*dir);
        vector_scale(t, t, pvars->MobVars.Config.Speed * 4 * MATH_DT);
        t[2] = 4 * MATH_DT;
        vector_copy(pvars->MobVars.MoveVars.Velocity, t);
        pvars->MobVars.MoveVars.Grounded = 0;
        
        mobTransAnim(moby, SWARMER_ANIM_SIDE_FLIP, 0);
      }
      
      // face target
      if (target)
        mobTurnTowards(moby, target->Position, SWARMER_TURN_RADIANS_PER_SEC);

      // when we land, transition to idle
      //if (pvars->MobVars.AnimationLooped && (moby->AnimSeqId == SWARMER_ANIM_FLINCH_SPIN_AND_STAND || moby->AnimSeqId == SWARMER_ANIM_FLINCH_SPIN_AND_STAND2)) {
      if (pvars->MobVars.MoveVars.Grounded && moby->AnimSeqId == SWARMER_ANIM_SIDE_FLIP) {
        mobTransAnim(moby, SWARMER_ANIM_IDLE, 0);
        swarmerForceLocalAction(moby, SWARMER_ACTION_WALK);
        break;
      }

      if (moby->AnimSeqId == SWARMER_ANIM_SIDE_FLIP && pvars->MobVars.AnimationLooped) {
        mobTransAnim(moby, SWARMER_ANIM_JUMP_AND_FALL, 5);
        break;
      }

      if (moby->AnimSeqId == SWARMER_ANIM_SIDE_FLIP) {
        mobTransAnim(moby, SWARMER_ANIM_SIDE_FLIP, 0);
      }
      break;
    }
    case SWARMER_ACTION_DIE:
    {
      // on destroy, give some upwards velocity for the backflip
      if (moby->AnimSeqId != SWARMER_ANIM_FLINCH_BACKFLIP_AND_STAND) {
        vector_fromyaw(t, pvars->MobVars.Knockback.Angle / 1000.0);
        t[2] = 1.0;
        vector_scale(t, t, 4 * 2 * MATH_DT);
        vector_add(pvars->MobVars.MoveVars.AddVelocity, pvars->MobVars.MoveVars.AddVelocity, t);
      }

      mobTransAnimLerp(moby, SWARMER_ANIM_FLINCH_BACKFLIP_AND_STAND, 5, 0);
      if (moby->AnimSeqId == SWARMER_ANIM_FLINCH_BACKFLIP_AND_STAND && moby->AnimSeqT > 25) {
        pvars->MobVars.Destroy = 1;
      }

      //mobStand(moby);
      break;
    }
		case SWARMER_ACTION_ATTACK:
		{
      int attack1AnimId = SWARMER_ANIM_JUMP_FORWARD_BITE;
			mobTransAnim(moby, attack1AnimId, 0);

      float speedCurve = powf(clamp(5 - moby->AnimSeqT, 1, 2.25), 2);
			float speedMult = (moby->AnimSeqId == attack1AnimId && moby->AnimSeqT < 5) ? speedCurve : 1;
			int swingAttackReady = moby->AnimSeqId == attack1AnimId && moby->AnimSeqT >= 5 && moby->AnimSeqT < 8;
			u32 damageFlags = 0x00081801;

      if (speedMult < 1)
				speedMult = 1;

      if (!isInAirFromFlinching) {
        if (target) {
          mobTurnTowards(moby, target->Position, turnSpeed);
          mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, target->Position, speedMult * pvars->MobVars.Config.Speed, acceleration);
        } else {
          // stand
          mobStand(moby);
        }
      }

			// attribute damage
			switch (pvars->MobVars.Config.MobAttribute)
			{
				case MOB_ATTRIBUTE_FREEZE:
				{
					damageFlags |= 0x00800000;
					break;
				}
				case MOB_ATTRIBUTE_ACID:
				{
					damageFlags |= 0x00000080;
					break;
				}
			}

			if (swingAttackReady && damageFlags) {
				swarmerDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 0);
			}
			break;
		}
	}

  pvars->MobVars.CurrentActionForTicks ++;
}

//--------------------------------------------------------------------------
void swarmerDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire)
{
  mobDoDamage(moby, radius, amount, damageFlags, friendlyFire, SWARMER_SUBSKELETON_JOINT_JAW);
}

//--------------------------------------------------------------------------
void swarmerForceLocalAction(Moby* moby, int action)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  float difficulty = 1;

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	// from
	switch (pvars->MobVars.Action)
	{
		case SWARMER_ACTION_SPAWN:
		{
			// enable collision
			moby->CollActive = 0;
			break;
		}
    case SWARMER_ACTION_DIE:
    {
      // can't undie
      return;
    }
	}

	// to
	switch (action)
	{
		case SWARMER_ACTION_SPAWN:
		{
			// disable collision
			moby->CollActive = 1;
			break;
		}
		case SWARMER_ACTION_WALK:
		{
			
			break;
		}
		case SWARMER_ACTION_DIE:
		{
			// disable collision
			moby->CollActive = 1;
			break;
		}
		case SWARMER_ACTION_ATTACK:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
			break;
		}
		case SWARMER_ACTION_FLINCH:
		case SWARMER_ACTION_BIG_FLINCH:
		{
			swarmerPlayHitSound(moby);
			pvars->MobVars.FlinchCooldownTicks = SWARMER_FLINCH_COOLDOWN_TICKS;
			break;
		}
		default:
		{
			break;
		}
	}

	// 
  if (action != pvars->MobVars.Action)
    pvars->MobVars.CurrentActionForTicks = 0;

	pvars->MobVars.Action = action;
	pvars->MobVars.NextAction = -1;
	pvars->MobVars.ActionCooldownTicks = SWARMER_ACTION_COOLDOWN_TICKS;
}

//--------------------------------------------------------------------------
short swarmerGetArmor(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  return pvars->MobVars.Config.Bangles;
}

//--------------------------------------------------------------------------
void swarmerPlayHitSound(Moby* moby)
{
  if (swarmerHitSoundId < 0) return;

	SwarmerSoundDef.Index = swarmerHitSoundId;
	soundPlay(&SwarmerSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
void swarmerPlayAmbientSound(Moby* moby)
{
  if (!swarmerAmbientSoundIdsCount) return;

	SwarmerSoundDef.Index = swarmerAmbientSoundIds[rand(swarmerAmbientSoundIdsCount)];
	soundPlay(&SwarmerSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
void swarmerPlayDeathSound(Moby* moby)
{
  if (swarmerDeathSoundId < 0) return;

	SwarmerSoundDef.Index = swarmerDeathSoundId;
	soundPlay(&SwarmerSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
int swarmerIsAttacking(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return pvars->MobVars.Action == SWARMER_ACTION_TIME_BOMB || pvars->MobVars.Action == SWARMER_ACTION_TIME_BOMB_EXPLODE || (pvars->MobVars.Action == SWARMER_ACTION_ATTACK && !pvars->MobVars.AnimationLooped);
}

//--------------------------------------------------------------------------
int swarmerIsSpawning(struct MobPVar* pvars)
{
	return pvars->MobVars.Action == SWARMER_ACTION_SPAWN && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
int swarmerCanAttack(struct MobPVar* pvars)
{
	return pvars->MobVars.AttackCooldownTicks == 0;
}

//--------------------------------------------------------------------------
int swarmerGetSideFlipLeftOrRight(struct MobPVar* pvars)
{
  int seed = pvars->MobVars.Random + pvars->MobVars.ActionId;
  sha1(&seed, 4, &seed, 4);
  return seed % 2;
}

//--------------------------------------------------------------------------
int swarmerIsFlinching(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return (moby->AnimSeqId == SWARMER_ANIM_FLINCH_SPIN_AND_STAND || moby->AnimSeqId == SWARMER_ANIM_FLINCH_SPIN_AND_STAND2) && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
float swarmerGetDodgeProbability(Moby* moby)
{
  int roundNo = 0;
  if (MapConfig.State) roundNo = MapConfig.State->RoundNumber;

  float factor = clamp(powf(roundNo / 100.0, 3), 0, 1);
  return lerpf(0.01, 0.25, factor);
}
