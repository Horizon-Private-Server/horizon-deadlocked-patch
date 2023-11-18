#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/radar.h>
#include <libdl/color.h>

#include "../../../include/game.h"
#include "../../../include/mob.h"
#include "../include/pathfind.h"
#include "../include/maputils.h"
#include "../include/shared.h"

void reaperPreUpdate(Moby* moby);
void reaperPostUpdate(Moby* moby);
void reaperPostDraw(Moby* moby);
void reaperMove(Moby* moby);
void reaperOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs* e);
void reaperOnDestroy(Moby* moby, int killedByPlayerId, int weaponId);
void reaperOnDamage(Moby* moby, struct MobDamageEventArgs* e);
void reaperOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs* e);
Moby* reaperGetNextTarget(Moby* moby);
int reaperGetPreferredAction(Moby* moby);
void reaperDoAction(Moby* moby);
void reaperDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire);
void reaperForceLocalAction(Moby* moby, int action);
short reaperGetArmor(Moby* moby);

void reaperPlayHitSound(Moby* moby);
void reaperPlayAmbientSound(Moby* moby);
void reaperPlayDeathSound(Moby* moby);
int reaperIsAttacking(struct MobPVar* pvars);
int reaperIsSpawning(struct MobPVar* pvars);
int reaperCanAttack(struct MobPVar* pvars);

struct MobVTable ReaperVTable = {
  .PreUpdate = &reaperPreUpdate,
  .PostUpdate = &reaperPostUpdate,
  .PostDraw = &reaperPostDraw,
  .Move = &reaperMove,
  .OnSpawn = &reaperOnSpawn,
  .OnDestroy = &reaperOnDestroy,
  .OnDamage = &reaperOnDamage,
  .OnStateUpdate = &reaperOnStateUpdate,
  .GetNextTarget = &reaperGetNextTarget,
  .GetPreferredAction = &reaperGetPreferredAction,
  .ForceLocalAction = &reaperForceLocalAction,
  .DoAction = &reaperDoAction,
  .DoDamage = &reaperDoDamage,
  .GetArmor = &reaperGetArmor,
};

SoundDef ReaperSoundDef = {
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
int reaperCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config)
{
	struct MobSpawnEventArgs args;
  
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(REAPER_MOBY_OCLASS, sizeof(struct MobPVar), &guberEvent, NULL);
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
void reaperPreUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (mobIsFrozen(moby))
    return;

  // ambient sounds
	if (!pvars->MobVars.AmbientSoundCooldownTicks) {
		reaperPlayAmbientSound(moby);
		pvars->MobVars.AmbientSoundCooldownTicks = randRangeInt(REAPER_AMBSND_MIN_COOLDOWN_TICKS, REAPER_AMBSND_MAX_COOLDOWN_TICKS);
	}

  // decrement path target pos ticker
  decTimerU8(&pvars->MobVars.MoveVars.PathTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathNewTicks);
}

//--------------------------------------------------------------------------
void reaperPostUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // adjust animSpeed by speed and by animation
	float animSpeed = 0.9 * (pvars->MobVars.Config.Speed / MOB_BASE_SPEED);
  if (moby->AnimSeqId == REAPER_ANIM_JUMP) {
    animSpeed = 0.9 * (1 - powf(moby->AnimSeqT / 35, 2));
    if (pvars->MobVars.MoveVars.Grounded) {
      animSpeed = 0.9;
    }
  }

	if (mobIsFrozen(moby) || (moby->DrawDist == 0 && pvars->MobVars.Action == REAPER_ACTION_WALK)) {
		moby->AnimSpeed = 0;
	} else {
		moby->AnimSpeed = animSpeed;
	}
}

//--------------------------------------------------------------------------
void reaperPostDraw(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  u32 color = MobLODColors[pvars->MobVars.SpawnParamsIdx] | (moby->Opacity << 24);
  mobPostDrawQuad(moby, 127, color, 1);
}

//--------------------------------------------------------------------------
void reaperAlterTarget(VECTOR out, Moby* moby, VECTOR forward, float amount)
{
	VECTOR up = {0,0,1,0};
	
	vector_outerproduct(out, forward, up);
	vector_normalize(out, out);
	vector_scale(out, out, amount);
}

//--------------------------------------------------------------------------
void reaperMove(Moby* moby)
{
  mobMove(moby);
}

//--------------------------------------------------------------------------
void reaperOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs* e)
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
}

//--------------------------------------------------------------------------
void reaperOnDestroy(Moby* moby, int killedByPlayerId, int weaponId)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// set colors before death so that the corn has the correct color
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.SpawnParamsIdx];
  
	// limit corn spawning to prevent freezing/framelag
	if (MapConfig.State && MapConfig.State->RoundMobCount < 30) {
		mobSpawnCorn(moby, REAPER_BANGLE_LARM | REAPER_BANGLE_RARM | REAPER_BANGLE_LLEG | REAPER_BANGLE_RLEG | REAPER_BANGLE_RFOOT | REAPER_BANGLE_HIPS);
	}
}

//--------------------------------------------------------------------------
void reaperOnDamage(Moby* moby, struct MobDamageEventArgs* e)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	float damage = e->DamageQuarters / 4.0;
  float newHp = pvars->MobVars.Health - damage;

	int canFlinch = pvars->MobVars.Action != REAPER_ACTION_FLINCH 
            && pvars->MobVars.Action != REAPER_ACTION_BIG_FLINCH
            && pvars->MobVars.Action != REAPER_ACTION_TIME_BOMB 
            && pvars->MobVars.Action != REAPER_ACTION_TIME_BOMB_EXPLODE
            && pvars->MobVars.FlinchCooldownTicks == 0;

  int isShock = e->DamageFlags & 0x40;

	// destroy
	if (newHp <= 0) {
		if (pvars->MobVars.Action == REAPER_ACTION_TIME_BOMB && moby->AnimSeqId == REAPER_ANIM_CROUCH && moby->AnimSeqT > 3) {
			// explode
			reaperForceLocalAction(moby, REAPER_ACTION_TIME_BOMB_EXPLODE);
		} else {
			reaperForceLocalAction(moby, REAPER_ACTION_DIE);
		}

    pvars->MobVars.LastHitBy = e->SourceUID;
    pvars->MobVars.LastHitByOClass = e->SourceOClass;
	}

	// knockback
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
        mobSetAction(moby, REAPER_ACTION_FLINCH);
      }
      else if (e->Knockback.Force || randRangeInt(0, 10) < e->Knockback.Power) {
        mobSetAction(moby, REAPER_ACTION_BIG_FLINCH);
      }
      else if (randRange(0, 1) < (REAPER_FLINCH_PROBABILITY * damageRatio)) {
        mobSetAction(moby, REAPER_ACTION_FLINCH);
      }
    }
	}
}

//--------------------------------------------------------------------------
void reaperOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs* e)
{
  mobOnStateUpdate(moby, e);
}

//--------------------------------------------------------------------------
Moby* reaperGetNextTarget(Moby* moby)
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
			float distSqr = vector_sqrmag(delta);

			if (distSqr < 300000) {
				// favor existing target
				if (p->SkinMoby == currentTarget)
					distSqr *= (1.0 / REAPER_TARGET_KEEP_CURRENT_FACTOR);
				
				// pick closest target
				if (distSqr < closestPlayerDist) {
					closestPlayer = p;
					closestPlayerDist = distSqr;
				}
			}
		}

		++players;
	}

	if (closestPlayer)
		return closestPlayer->SkinMoby;

	return NULL;
}

//--------------------------------------------------------------------------
int reaperGetPreferredAction(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;

	// no preferred action
	if (reaperIsAttacking(pvars))
		return -1;

	if (reaperIsSpawning(pvars))
		return -1;

	if (pvars->MobVars.Action == REAPER_ACTION_JUMP && !pvars->MobVars.MoveVars.Grounded) {
		return REAPER_ACTION_WALK;
  }

  // wait for grounded to stop flinch
  if ((pvars->MobVars.Action == REAPER_ACTION_FLINCH || pvars->MobVars.Action == REAPER_ACTION_BIG_FLINCH) && !pvars->MobVars.MoveVars.Grounded)
    return -1;

  // jump if we've hit a slope and are grounded
  if (pvars->MobVars.MoveVars.Grounded && pvars->MobVars.MoveVars.HitWall && pvars->MobVars.MoveVars.WallSlope > REAPER_MAX_WALKABLE_SLOPE) {
    return REAPER_ACTION_JUMP;
  }

  // jump if we've hit a jump point on the path
  if (pathShouldJump(moby)) {
    return REAPER_ACTION_JUMP;
  }

	// prevent action changing too quickly
	if (pvars->MobVars.ActionCooldownTicks)
		return -1;

	// get next target
	Moby * target = reaperGetNextTarget(moby);
	if (target) {
		vector_copy(t, target->Position);
		vector_subtract(t, t, moby->Position);
		float distSqr = vector_sqrmag(t);
		float attackRadiusSqr = pvars->MobVars.Config.AttackRadius * pvars->MobVars.Config.AttackRadius;

		if (distSqr <= attackRadiusSqr) {
			if (reaperCanAttack(pvars))
				return pvars->MobVars.Config.MobAttribute != MOB_ATTRIBUTE_EXPLODE ? REAPER_ACTION_ATTACK : REAPER_ACTION_TIME_BOMB;
			return REAPER_ACTION_WALK;
		} else {
			return REAPER_ACTION_WALK;
		}
	}
	
	return REAPER_ACTION_IDLE;
}

//--------------------------------------------------------------------------
#if DEBUGPATH
void reaperRenderPath(Moby* moby)
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
void reaperDoAction(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Moby* target = pvars->MobVars.Target;
	VECTOR t, t2;
  float difficulty = 1;
  float turnSpeed = pvars->MobVars.MoveVars.Grounded ? REAPER_TURN_RADIANS_PER_SEC : REAPER_TURN_AIR_RADIANS_PER_SEC;
  float acceleration = pvars->MobVars.MoveVars.Grounded ? REAPER_MOVE_ACCELERATION : REAPER_MOVE_AIR_ACCELERATION;
  int isInAirFromFlinching = !pvars->MobVars.MoveVars.Grounded 
                      && (pvars->MobVars.LastAction == REAPER_ACTION_FLINCH || pvars->MobVars.LastAction == REAPER_ACTION_BIG_FLINCH);

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

#if DEBUGPATH
  gfxRegisterDrawFunction((void**)0x0022251C, (gfxDrawFuncDef*)&reaperRenderPath, moby);
#endif

	switch (pvars->MobVars.Action)
	{
		case REAPER_ACTION_SPAWN:
		{
      mobTransAnim(moby, REAPER_ANIM_CRAWL_OUT_OF_GROUND, 0);
      mobStand(moby);
			break;
		}
		case REAPER_ACTION_FLINCH:
		case REAPER_ACTION_BIG_FLINCH:
		{
      int animFlinchId = pvars->MobVars.Action == REAPER_ACTION_BIG_FLINCH ? REAPER_ANIM_BIG_FLINCH : REAPER_ANIM_BIG_FLINCH;

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
		case REAPER_ACTION_IDLE:
		{
			mobTransAnim(moby, REAPER_ANIM_IDLE, 0);
      mobStand(moby);
			break;
		}
		case REAPER_ACTION_JUMP:
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
			    mobTransAnim(moby, REAPER_ANIM_JUMP, 5);

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
		case REAPER_ACTION_LOOK_AT_TARGET:
    {
      mobStand(moby);
      if (target)
        mobTurnTowards(moby, target->Position, turnSpeed);
      break;
    }
    case REAPER_ACTION_WALK:
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
            reaperAlterTarget(t2, moby, t, clamp(dist, 0, 10) * 0.3 * dir);
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
      if (moby->AnimSeqId == REAPER_ANIM_JUMP && !pvars->MobVars.MoveVars.Grounded) {
        // wait for jump to land
      }
			else if (mobHasVelocity(pvars))
				mobTransAnim(moby, REAPER_ANIM_RUN, 0);
			else
				mobTransAnim(moby, REAPER_ANIM_IDLE, 0);
			break;
		}
    case REAPER_ACTION_DIE:
    {
      mobStand(moby);
      break;
    }
		case REAPER_ACTION_TIME_BOMB_EXPLODE:
    {
      
      break;
    }
		case REAPER_ACTION_TIME_BOMB:
		{
			mobTransAnim(moby, REAPER_ANIM_CROUCH, 0);

			if (pvars->MobVars.TimeBombTicks == 0) {
				moby->Opacity = 0x80;
				pvars->MobVars.OpacityFlickerDirection = 0;
				mobSetAction(moby, REAPER_ACTION_TIME_BOMB_EXPLODE);
			} else {

				// cycle opacity from 1.0 to 2.0
				int newOpacity = (u8)moby->Opacity + pvars->MobVars.OpacityFlickerDirection;
				if (newOpacity <= 0x80) {
					pvars->MobVars.OpacityFlickerDirection *= -1.25;
					newOpacity = 0x80;
				}
				else if (newOpacity >= 0xFF) {
					pvars->MobVars.OpacityFlickerDirection *= -1.25;
					newOpacity = 0xFF;
				}
			
				// limit cycle rate
				if (pvars->MobVars.OpacityFlickerDirection < -0x40)
					pvars->MobVars.OpacityFlickerDirection = -0x40;
				if (pvars->MobVars.OpacityFlickerDirection > 0x40)
					pvars->MobVars.OpacityFlickerDirection = 0x40;
				
				moby->Opacity = (u8)newOpacity;
        mobStand(moby);
			}
			break;
		}
		case REAPER_ACTION_ATTACK:
		{
      int attack1AnimId = REAPER_ANIM_SLAP;
			mobTransAnim(moby, attack1AnimId, 0);

			float speedMult = (moby->AnimSeqId == attack1AnimId && moby->AnimSeqT < 5) ? (difficulty * 2) : 1;
			int swingAttackReady = moby->AnimSeqId == attack1AnimId && moby->AnimSeqT >= 11 && moby->AnimSeqT < 14;
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
				reaperDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 0);
			}
			break;
		}
	}

  pvars->MobVars.CurrentActionForTicks ++;
}

//--------------------------------------------------------------------------
void reaperDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire)
{
  mobDoDamage(moby, radius, amount, damageFlags, friendlyFire, 2);
}

//--------------------------------------------------------------------------
void reaperForceLocalAction(Moby* moby, int action)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  float difficulty = 1;

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	// from
	switch (pvars->MobVars.Action)
	{
		case REAPER_ACTION_SPAWN:
		{
			// enable collision
			moby->CollActive = 0;
			break;
		}
    case REAPER_ACTION_DIE:
    {
      // can't undie
      return;
    }
	}

	// to
	switch (action)
	{
		case REAPER_ACTION_SPAWN:
		{
			// disable collision
			moby->CollActive = 1;
			break;
		}
		case REAPER_ACTION_WALK:
		{
			
			break;
		}
		case REAPER_ACTION_DIE:
		{
      pvars->MobVars.Destroy = 1;
			break;
		}
		case REAPER_ACTION_ATTACK:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
			break;
		}
    case REAPER_ACTION_TIME_BOMB_EXPLODE:
    {
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;

      u32 damageFlags = 0x00008801;
      u32 color = 0x003064FF;

			// attribute damage
      switch (pvars->MobVars.Config.MobAttribute)
      {
        case MOB_ATTRIBUTE_FREEZE:
        {
          color = 0x00FF6430;
          damageFlags |= 0x00800000;
          break;
        }
        case MOB_ATTRIBUTE_ACID:
        {
          color = 0x0064FF30;
          damageFlags |= 0x00000080;
          break;
        }
      }
  
      spawnExplosion(moby->Position, pvars->MobVars.Config.HitRadius, color);
      reaperDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 1);
      pvars->MobVars.Destroy = 1;
      pvars->MobVars.LastHitBy = -1;
      break;
    }
		case REAPER_ACTION_TIME_BOMB:
		{
			pvars->MobVars.OpacityFlickerDirection = 4;
			pvars->MobVars.TimeBombTicks = REAPER_TIMEBOMB_TICKS / clamp(difficulty, 0.5, 2);
			break;
		}
		case REAPER_ACTION_FLINCH:
		case REAPER_ACTION_BIG_FLINCH:
		{
			reaperPlayHitSound(moby);
			pvars->MobVars.FlinchCooldownTicks = REAPER_FLINCH_COOLDOWN_TICKS;
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
	pvars->MobVars.ActionCooldownTicks = REAPER_ACTION_COOLDOWN_TICKS;
}

//--------------------------------------------------------------------------
short reaperGetArmor(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	float t = pvars->MobVars.Health / pvars->MobVars.Config.MaxHealth;
  int bangles = pvars->MobVars.Config.Bangles;

  if (t < 0.3)
    return 0x0000;
  else if (t < 0.7)
    return bangles & 0x1f; // remove torso bangle

	return bangles;
}

//--------------------------------------------------------------------------
void reaperPlayHitSound(Moby* moby)
{
	ReaperSoundDef.Index = 0x17D;
	soundPlay(&ReaperSoundDef, 0, moby, 0, 0x400);
}	

//--------------------------------------------------------------------------
void reaperPlayAmbientSound(Moby* moby)
{
  const int ambientSoundIds[] = { 0x17A, 0x179 };
	ReaperSoundDef.Index = ambientSoundIds[rand(2)];
	soundPlay(&ReaperSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
void reaperPlayDeathSound(Moby* moby)
{
	ReaperSoundDef.Index = 0x171;
	soundPlay(&ReaperSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
int reaperIsAttacking(struct MobPVar* pvars)
{
	return pvars->MobVars.Action == REAPER_ACTION_TIME_BOMB || pvars->MobVars.Action == REAPER_ACTION_TIME_BOMB_EXPLODE || (pvars->MobVars.Action == REAPER_ACTION_ATTACK && !pvars->MobVars.AnimationLooped);
}

//--------------------------------------------------------------------------
int reaperIsSpawning(struct MobPVar* pvars)
{
	return pvars->MobVars.Action == REAPER_ACTION_SPAWN && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
int reaperCanAttack(struct MobPVar* pvars)
{
	return pvars->MobVars.AttackCooldownTicks == 0;
}
