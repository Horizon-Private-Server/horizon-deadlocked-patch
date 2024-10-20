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
#include "../include/maputils.h"
#include "../include/shared.h"

void executionerPreUpdate(Moby* moby);
void executionerPostUpdate(Moby* moby);
void executionerPostDraw(Moby* moby);
void executionerMove(Moby* moby);
void executionerOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs* e);
void executionerOnDestroy(Moby* moby, int killedByPlayerId, int weaponId);
void executionerOnDamage(Moby* moby, struct MobDamageEventArgs* e);
int executionerOnLocalDamage(Moby* moby, struct MobLocalDamageEventArgs* e);
void executionerOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs* e);
Moby* executionerGetNextTarget(Moby* moby);
int executionerGetPreferredAction(Moby* moby, int * delayTicks);
void executionerDoAction(Moby* moby);
void executionerDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire);
void executionerForceLocalAction(Moby* moby, int action);
short executionerGetArmor(Moby* moby);
int executionerIsAttacking(Moby* moby);
int executionerCanNonOwnerTransitionToAction(Moby* moby, int action);
int executionerShouldForceStateUpdateOnAction(Moby* moby, int action);

int executionerIsSpawning(struct MobPVar* pvars);
int executionerCanAttack(struct MobPVar* pvars);
int executionerIsFlinching(Moby* moby);

struct MobVTable ExecutionerVTable = {
  .PreUpdate = &executionerPreUpdate,
  .PostUpdate = &executionerPostUpdate,
  .PostDraw = &executionerPostDraw,
  .Move = &executionerMove,
  .OnSpawn = &executionerOnSpawn,
  .OnDestroy = &executionerOnDestroy,
  .OnDamage = &executionerOnDamage,
  .OnLocalDamage = &executionerOnLocalDamage,
  .OnStateUpdate = &executionerOnStateUpdate,
  .GetNextTarget = &executionerGetNextTarget,
  .GetPreferredAction = &executionerGetPreferredAction,
  .ForceLocalAction = &executionerForceLocalAction,
  .DoAction = &executionerDoAction,
  .DoDamage = &executionerDoDamage,
  .GetArmor = &executionerGetArmor,
  .IsAttacking = &executionerIsAttacking,
  .CanNonOwnerTransitionToAction = &executionerCanNonOwnerTransitionToAction,
  .ShouldForceStateUpdateOnAction = &executionerShouldForceStateUpdateOnAction,
};

extern u32 MobPrimaryColors[];
extern u32 MobSecondaryColors[];
extern u32 MobLODColors[];

//--------------------------------------------------------------------------
int executionerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config)
{
	struct MobSpawnEventArgs args;
  
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(EXECUTIONER_MOBY_OCLASS, sizeof(struct MobPVar), &guberEvent, NULL);
	if (guberEvent)
	{
    if (MapConfig.PopulateSpawnArgsFunc) {
      MapConfig.PopulateSpawnArgsFunc(&args, config, spawnParamsIdx, spawnFromUID == -1, spawnFlags);
    }

		u8 random = (u8)rand(100);

    position[2] += 1; // spawn slightly above point
		guberEventWrite(guberEvent, position, 12);
		guberEventWrite(guberEvent, &yaw, 4);
		guberEventWrite(guberEvent, &spawnFromUID, 4);
		guberEventWrite(guberEvent, &spawnFlags, 4);
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
void executionerPreUpdate(Moby* moby)
{
  int i;
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  ExecutionerMobVars_t* executionerVars = (ExecutionerMobVars_t*)pvars->AdditionalMobVarsPtr;
  
  // decrement tickers regardless of frozen state
  for (i = 0; i < GAME_MAX_LOCALS; ++i)
    decTimerU8(&executionerVars->LocalPlayerDamageHitInvTimer[i]);

  if (mobIsFrozen(moby))
    return;

  // decrement path target pos ticker
  decTimerU8(&pvars->MobVars.MoveVars.PathTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathCheckSkipEndTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathNewTicks);

  mobPreUpdate(moby);
}

//--------------------------------------------------------------------------
void executionerPostUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // adjust animSpeed by speed and by animation
	float animSpeed = 0.6 * (pvars->MobVars.Config.Speed / MOB_BASE_SPEED);
  if (moby->AnimSeqId == EXECUTIONER_ANIM_JUMP) {
    animSpeed = 1 * (1 - powf(moby->AnimSeqT / 21, 1));
    if (pvars->MobVars.MoveVars.Grounded) {
      animSpeed = 0.6;
    }
  } else if (executionerIsFlinching(moby) && !pvars->MobVars.MoveVars.Grounded) {
    animSpeed = 0.5 * (1 - powf(moby->AnimSeqT / 20, 2));
  }

	if (mobIsFrozen(moby) || (moby->DrawDist == 0 && pvars->MobVars.Action == EXECUTIONER_ACTION_WALK)) {
		moby->AnimSpeed = 0;
	} else {
		moby->AnimSpeed = animSpeed;
	}
}

//--------------------------------------------------------------------------
void executionerPostDraw(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  u32 color = MobLODColors[pvars->MobVars.SpawnParamsIdx] | (moby->Opacity << 24);
  mobPostDrawQuad(moby, 127, color, 1);
}

//--------------------------------------------------------------------------
void executionerAlterTarget(VECTOR out, Moby* moby, VECTOR forward, float amount)
{
	VECTOR up = {0,0,1,0};
	
	vector_outerproduct(out, forward, up);
	vector_normalize(out, out);
	vector_scale(out, out, amount);
}

//--------------------------------------------------------------------------
void executionerMove(Moby* moby)
{
  mobMove(moby);
}

//--------------------------------------------------------------------------
void executionerOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs* e)
{
  
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // set scale
  moby->Scale = 0.4;

  // colors by mob type
	moby->GlowRGBA = MobSecondaryColors[pvars->MobVars.SpawnParamsIdx];
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.SpawnParamsIdx];

  // targeting
	pvars->TargetVars.targetHeight = 2.5;
  pvars->MobVars.BlipType = 6;

#if MOB_DAMAGETYPES
  pvars->TargetVars.damageTypes = MOB_DAMAGETYPES;
#endif

  // russion doll
  if (pvars->MobVars.SpawnFlags & MOB_SPAWN_FLAG_RUSSIAN_DOLL) {
    mobSetAction(moby, EXECUTIONER_ACTION_BIG_FLINCH);
  }

  // default move step
  pvars->MobVars.MoveVars.MoveStep = MOB_MOVE_SKIP_TICKS;
}

//--------------------------------------------------------------------------
void executionerOnDestroy(Moby* moby, int killedByPlayerId, int weaponId)
{
  VECTOR expOffset;
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// set colors before death so that the corn has the correct color
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.SpawnParamsIdx];

  // spawn explosion
  u32 expColor = 0x801E70D6;
  vector_copy(expOffset, moby->Position);
  expOffset[2] += pvars->TargetVars.targetHeight;
  u128 expPos = vector_read(expOffset);
  mobySpawnExplosion
    (expPos, 0, 0, 0, 0, 16, 0, 16, 0, 1, 0, 0, 0, 0,
    0, 0, expColor, expColor, expColor, expColor, expColor, expColor, expColor, expColor,
    0, 0, 0, 0, 0, 1, 0, 0, 0);
}

//--------------------------------------------------------------------------
void executionerOnDamage(Moby* moby, struct MobDamageEventArgs* e)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	float damage = e->DamageQuarters / 4.0;
  float newHp = pvars->MobVars.Health - damage;

	int canFlinch = pvars->MobVars.Action != EXECUTIONER_ACTION_FLINCH 
            && pvars->MobVars.Action != EXECUTIONER_ACTION_BIG_FLINCH
            && pvars->MobVars.FlinchCooldownTicks == 0;

#if ALWAYS_FLINCH
  canFlinch = 1;
#endif

  int isShock = e->DamageFlags & 0x40;
  int isShortFreeze = e->DamageFlags & 0x40000000;

	// destroy
	if (newHp <= 0) {
    executionerForceLocalAction(moby, EXECUTIONER_ACTION_DIE);
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
    float powerFactor = EXECUTIONER_FLINCH_PROBABILITY_PWR_FACTOR * e->Knockback.Power;
    float probability = clamp((damageRatio * EXECUTIONER_FLINCH_PROBABILITY) + powerFactor, 0, MOB_MAX_FLINCH_PROBABILITY);

#if ALWAYS_FLINCH
    probability = 2;
    powerFactor = 2;
#endif

    if (canFlinch) {
      if (e->Knockback.Force) {
        mobSetAction(moby, EXECUTIONER_ACTION_BIG_FLINCH);
      } else if (isShock) {
        mobSetAction(moby, EXECUTIONER_ACTION_FLINCH);
      } else if (randRange(0, 1) < probability) {
        if (randRange(0, 1) < powerFactor) {
          mobSetAction(moby, EXECUTIONER_ACTION_BIG_FLINCH);
        } else {
          mobSetAction(moby, EXECUTIONER_ACTION_FLINCH);
        }
      }
    }
	}

  // short freeze
  if (isShortFreeze && pvars->MobVars.SlowTicks < MOB_SHORT_FREEZE_DURATION_TICKS) {
    pvars->MobVars.SlowTicks = MOB_SHORT_FREEZE_DURATION_TICKS;
    mobResetMoveStep(moby);
  }
}

//--------------------------------------------------------------------------
int executionerOnLocalDamage(Moby* moby, struct MobLocalDamageEventArgs* e)
{
  // we want to give each local player a cooldown on damage they can apply to reactor
  if (!e->PlayerDamager) return 1;
  if (!e->PlayerDamager->IsLocal) return 1;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  ExecutionerMobVars_t* executionerVars = (ExecutionerMobVars_t*)pvars->AdditionalMobVarsPtr;

  // only accept local damage when timer is 0
  int timer = executionerVars->LocalPlayerDamageHitInvTimer[e->PlayerDamager->LocalPlayerIndex];
  if (timer == 0) {
    executionerVars->LocalPlayerDamageHitInvTimer[e->PlayerDamager->LocalPlayerIndex] = EXECUTIONER_HIT_INV_TICKS;
    return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
void executionerOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs* e)
{
  mobOnStateUpdate(moby, e);
}

//--------------------------------------------------------------------------
Moby* executionerGetNextTarget(Moby* moby)
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
					dist *= (1.0 / EXECUTIONER_TARGET_KEEP_CURRENT_FACTOR);
				
				// pick closest target
				if (dist < closestPlayerDist) {
					closestPlayer = p;
					closestPlayerDist = dist;
				}
			}
		}

		++players;
	}

	if (closestPlayer) {
    return playerGetTargetMoby(closestPlayer);
  }

	return NULL;
}

//--------------------------------------------------------------------------
int executionerGetPreferredAction(Moby* moby, int * delayTicks)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;

	// no preferred action
	if (executionerIsAttacking(moby))
		return -1;

	if (executionerIsSpawning(pvars))
		return -1;

  if (executionerIsFlinching(moby))
    return -1;

	if (pvars->MobVars.Action == EXECUTIONER_ACTION_JUMP && !pvars->MobVars.MoveVars.Grounded) {
		return EXECUTIONER_ACTION_WALK;
  }

  // jump if we've hit a slope and are grounded
  if (pvars->MobVars.MoveVars.Grounded && pvars->MobVars.MoveVars.HitWall && pvars->MobVars.MoveVars.WallSlope > EXECUTIONER_MAX_WALKABLE_SLOPE) {
    return EXECUTIONER_ACTION_JUMP;
  }

  // jump if we've hit a jump point on the path
  if (pvars->MobVars.MoveVars.QueueJumpSpeed) {
    return EXECUTIONER_ACTION_JUMP;
  }

	// prevent action changing too quickly
	if (pvars->MobVars.ActionCooldownTicks)
		return -1;

	// get next target
	Moby * target = executionerGetNextTarget(moby);
	if (target) {
		vector_copy(t, target->Position);
		vector_subtract(t, t, moby->Position);
		float distSqr = vector_sqrmag(t);
		float attackRadiusSqr = pvars->MobVars.Config.AttackRadius * pvars->MobVars.Config.AttackRadius;

		if (distSqr <= attackRadiusSqr) {
			if (executionerCanAttack(pvars) && distSqr > (EXECUTIONER_TOO_CLOSE_TO_TARGET_RADIUS*EXECUTIONER_TOO_CLOSE_TO_TARGET_RADIUS)) {
        if (delayTicks) *delayTicks = pvars->MobVars.Config.ReactionTickCount;
				return EXECUTIONER_ACTION_ATTACK;
      }
			return EXECUTIONER_ACTION_WALK;
		} else {
			return EXECUTIONER_ACTION_WALK;
		}
	}
	
	return EXECUTIONER_ACTION_IDLE;
}

//--------------------------------------------------------------------------
void executionerDoAction(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Moby* target = pvars->MobVars.Target;
	VECTOR t, t2;
  float difficulty = 1;
  float turnSpeed = pvars->MobVars.MoveVars.Grounded ? EXECUTIONER_TURN_RADIANS_PER_SEC : EXECUTIONER_TURN_AIR_RADIANS_PER_SEC;
  float acceleration = pvars->MobVars.MoveVars.Grounded ? EXECUTIONER_MOVE_ACCELERATION : EXECUTIONER_MOVE_AIR_ACCELERATION;
  int isInAirFromFlinching = !pvars->MobVars.MoveVars.Grounded 
                      && (pvars->MobVars.LastAction == EXECUTIONER_ACTION_FLINCH || pvars->MobVars.LastAction == EXECUTIONER_ACTION_BIG_FLINCH);

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	switch (pvars->MobVars.Action)
	{
		case EXECUTIONER_ACTION_SPAWN:
		{
      mobTransAnim(moby, EXECUTIONER_ANIM_SPAWN, 0);
      mobStand(moby);
			break;
		}
		case EXECUTIONER_ACTION_FLINCH:
		case EXECUTIONER_ACTION_BIG_FLINCH:
		{
      int animFlinchId = pvars->MobVars.Action == EXECUTIONER_ACTION_BIG_FLINCH ? EXECUTIONER_ANIM_BIG_FLINCH : EXECUTIONER_ANIM_FLINCH;

      mobTransAnim(moby, animFlinchId, 0);
      
			if (pvars->MobVars.Knockback.Ticks > 0) {
        mobGetKnockbackVelocity(moby, t);
				vector_scale(t, t, EXECUTIONER_KNOCKBACK_MULTIPLIER);
				vector_add(pvars->MobVars.MoveVars.AddVelocity, pvars->MobVars.MoveVars.AddVelocity, t);
			} else if (pvars->MobVars.MoveVars.Grounded) {
        mobStand(moby);
      } else if (pvars->MobVars.CurrentActionForTicks > (1*TPS) && pvars->MobVars.MoveVars.HitWall && pvars->MobVars.MoveVars.StuckCounter) {
        mobStand(moby);
      }
			break;
		}
		case EXECUTIONER_ACTION_IDLE:
		{
			mobTransAnim(moby, EXECUTIONER_ANIM_IDLE, 0);
      mobStand(moby);
			break;
		}
		case EXECUTIONER_ACTION_JUMP:
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
			    mobTransAnim(moby, EXECUTIONER_ANIM_JUMP, 5);

          // check if we're near last jump pos
          // if so increment StuckJumpCount
          if (pvars->MobVars.MoveVars.IsStuck) {
            if (pvars->MobVars.MoveVars.StuckJumpCount < 255)
              pvars->MobVars.MoveVars.StuckJumpCount++;
          }

          // use delta height between target as base of jump speed
          // with min speed
          float jumpSpeed = pvars->MobVars.MoveVars.QueueJumpSpeed;
          if (jumpSpeed <= 0 && target) {
            jumpSpeed = 8; //clamp(2 + (target->Position[2] - moby->Position[2]) * fabsf(pvars->MobVars.MoveVars.WallSlope) * 2, 3, 15);
          }

          pvars->MobVars.MoveVars.Velocity[2] = jumpSpeed * MATH_DT;
          pvars->MobVars.MoveVars.Grounded = 0;
          pvars->MobVars.MoveVars.QueueJumpSpeed = 0;
        }
				break;
			}
		case EXECUTIONER_ACTION_LOOK_AT_TARGET:
    {
      mobStand(moby);
      if (target) {
        mobTurnTowards(moby, target->Position, turnSpeed);
      }
      break;
    }
    case EXECUTIONER_ACTION_WALK:
		{
      int walkBackwards = 0;

      if (!isInAirFromFlinching) {
        if (target) {

          float dir = ((pvars->MobVars.ActionId + pvars->MobVars.Random) % 3) - 1;

          // determine next position
          vector_copy(t, target->Position);
          vector_subtract(t, t, moby->Position);
          float dist = vector_length(t);

          // walk backwards if too close
          if (dist < EXECUTIONER_TOO_CLOSE_TO_TARGET_RADIUS) {
            walkBackwards = 1;

            vector_normalize(t, t);
            vector_scale(t, t, 5);
            vector_subtract(t, target->Position, t);
            mobTurnTowards(moby, target->Position, turnSpeed);
            mobGetVelocityToTargetSimple(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, t, pvars->MobVars.Config.Speed, acceleration);
            DPRINTF("%f\n", vector_length(pvars->MobVars.MoveVars.Velocity));
          }
          else if (dist > (pvars->MobVars.Config.AttackRadius - pvars->MobVars.Config.HitRadius)) {

            pathGetTargetPos(t, moby);
            vector_subtract(t, t, moby->Position);
            float dist = vector_length(t);
            if (dist < 10.0) {
              executionerAlterTarget(t2, moby, t, clamp(dist, 0, 10) * 0.3 * dir);
              vector_add(t, t, t2);
            }
            vector_scale(t, t, 1 / dist);
            vector_add(t, moby->Position, t);

            mobTurnTowards(moby, t, turnSpeed);
            mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, t, pvars->MobVars.Config.Speed, acceleration);
          } else {
            mobStand(moby);
          }
        } else {
          // stand
          mobStand(moby);
        }
      }

			// 
      if (moby->AnimSeqId == EXECUTIONER_ANIM_JUMP && !pvars->MobVars.MoveVars.Grounded) {
        // wait for jump to land
      } else if (pvars->MobVars.MoveVars.QueueJumpSpeed) {
        executionerForceLocalAction(moby, EXECUTIONER_ACTION_JUMP);
			} else if (mobHasVelocity(pvars)) {
				mobTransAnim(moby, walkBackwards ? EXECUTIONER_ANIM_WALK_BACKWARD : EXECUTIONER_ANIM_RUN, 0);
			} else if (moby->AnimSeqId != EXECUTIONER_ANIM_WALK_BACKWARD || moby->AnimSeqId != EXECUTIONER_ANIM_RUN || pvars->MobVars.AnimationLooped) {
				mobTransAnim(moby, EXECUTIONER_ANIM_IDLE, 0);
      }
			break;
		}
    case EXECUTIONER_ACTION_DIE:
    {
      mobTransAnimLerp(moby, EXECUTIONER_ANIM_BIG_FLINCH, 5, 0);

      if (moby->AnimSeqId == EXECUTIONER_ANIM_BIG_FLINCH && moby->AnimSeqT > 3) {
        pvars->MobVars.Destroy = 1;
      }

      mobStand(moby);
      break;
    }
		case EXECUTIONER_ACTION_ATTACK:
		{
      int attack1AnimId = EXECUTIONER_ANIM_SWING;
			mobTransAnim(moby, attack1AnimId, 0);

			float speedMult = 0; // (moby->AnimSeqId == attack1AnimId && moby->AnimSeqT < 5) ? (difficulty * 2) : 1;
			int swingAttackReady = moby->AnimSeqId == attack1AnimId && moby->AnimSeqT >= 4 && moby->AnimSeqT < 10;
			u32 damageFlags = 0x00081801;

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
				executionerDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 0);
			}
			break;
		}
	}

  pvars->MobVars.CurrentActionForTicks ++;
}

//--------------------------------------------------------------------------
void executionerDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire)
{
  mobDoDamage(moby, radius, amount, damageFlags, friendlyFire, 6, 1, 0);
}

//--------------------------------------------------------------------------
void executionerForceLocalAction(Moby* moby, int action)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  float difficulty = 1;

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	// from
	switch (pvars->MobVars.Action)
	{
		case EXECUTIONER_ACTION_SPAWN:
		{
			// enable collision
			moby->CollActive = 0;
			break;
		}
		case EXECUTIONER_ACTION_DIE:
		{
      // can't undie
      return;
		}
	}

	// to
	switch (action)
	{
		case EXECUTIONER_ACTION_SPAWN:
		{
			// disable collision
			moby->CollActive = 1;
			break;
		}
		case EXECUTIONER_ACTION_WALK:
		{
			
			break;
		}
		case EXECUTIONER_ACTION_DIE:
		{
			
			break;
		}
		case EXECUTIONER_ACTION_ATTACK:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
			break;
		}
		case EXECUTIONER_ACTION_FLINCH:
		case EXECUTIONER_ACTION_BIG_FLINCH:
		{
			pvars->MobVars.FlinchCooldownTicks = EXECUTIONER_FLINCH_COOLDOWN_TICKS;
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
	pvars->MobVars.ActionCooldownTicks = EXECUTIONER_ACTION_COOLDOWN_TICKS;
}

//--------------------------------------------------------------------------
short executionerGetArmor(Moby* moby)
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
int executionerIsAttacking(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return pvars->MobVars.Action == EXECUTIONER_ACTION_ATTACK && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
int executionerCanNonOwnerTransitionToAction(Moby* moby, int action)
{
  // always let non-owners simulate an action unless its the death action
  if (action == EXECUTIONER_ACTION_DIE) return 0;

  return 1;
}

//--------------------------------------------------------------------------
int executionerShouldForceStateUpdateOnAction(Moby* moby, int action)
{
  // only send state updates at regular intervals, unless dying
  if (action == EXECUTIONER_ACTION_DIE) return 1;

  return 0;
}

//--------------------------------------------------------------------------
int executionerIsSpawning(struct MobPVar* pvars)
{
	return pvars->MobVars.Action == EXECUTIONER_ACTION_SPAWN && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
int executionerCanAttack(struct MobPVar* pvars)
{
	return pvars->MobVars.AttackCooldownTicks == 0;
}

//--------------------------------------------------------------------------
int executionerIsFlinching(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return (moby->AnimSeqId == EXECUTIONER_ANIM_FLINCH || moby->AnimSeqId == EXECUTIONER_ANIM_BIG_FLINCH) && !pvars->MobVars.AnimationLooped;
}
