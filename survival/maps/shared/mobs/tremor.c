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

void tremorPreUpdate(Moby* moby);
void tremorPostUpdate(Moby* moby);
void tremorPostDraw(Moby* moby);
void tremorMove(Moby* moby);
void tremorOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs* e);
void tremorOnDestroy(Moby* moby, int killedByPlayerId, int weaponId);
void tremorOnDamage(Moby* moby, struct MobDamageEventArgs* e);
int tremorOnLocalDamage(Moby* moby, struct MobLocalDamageEventArgs* e);
void tremorOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs* e);
Moby* tremorGetNextTarget(Moby* moby);
int tremorGetPreferredAction(Moby* moby, int * delayTicks);
void tremorDoAction(Moby* moby);
void tremorDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire);
void tremorForceLocalAction(Moby* moby, int action);
short tremorGetArmor(Moby* moby);
int tremorIsAttacking(Moby* moby);

void tremorPlayHitSound(Moby* moby);
void tremorPlayAmbientSound(Moby* moby);
void tremorPlayDeathSound(Moby* moby);
int tremorIsSpawning(struct MobPVar* pvars);
int tremorCanAttack(struct MobPVar* pvars);
int tremorIsFlinching(Moby* moby);

struct MobVTable TremorVTable = {
  .PreUpdate = &tremorPreUpdate,
  .PostUpdate = &tremorPostUpdate,
  .PostDraw = &tremorPostDraw,
  .Move = &tremorMove,
  .OnSpawn = &tremorOnSpawn,
  .OnDestroy = &tremorOnDestroy,
  .OnDamage = &tremorOnDamage,
  .OnLocalDamage = &tremorOnLocalDamage,
  .OnStateUpdate = &tremorOnStateUpdate,
  .GetNextTarget = &tremorGetNextTarget,
  .GetPreferredAction = &tremorGetPreferredAction,
  .ForceLocalAction = &tremorForceLocalAction,
  .DoAction = &tremorDoAction,
  .DoDamage = &tremorDoDamage,
  .GetArmor = &tremorGetArmor,
  .IsAttacking = &tremorIsAttacking,
};

SoundDef TremorSoundDef = {
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
int tremorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config)
{
	struct MobSpawnEventArgs args;
  
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(TREMOR_MOBY_OCLASS, sizeof(struct MobPVar), &guberEvent, NULL);
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
void tremorPreUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (mobIsFrozen(moby))
    return;

	if (!pvars->MobVars.AmbientSoundCooldownTicks && rand(50) == 0) {
		tremorPlayAmbientSound(moby);
		pvars->MobVars.AmbientSoundCooldownTicks = randRangeInt(TREMOR_AMBSND_MIN_COOLDOWN_TICKS, TREMOR_AMBSND_MAX_COOLDOWN_TICKS);
	}

  // decrement path target pos ticker
  decTimerU8(&pvars->MobVars.MoveVars.PathTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathCheckSkipEndTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathNewTicks);

  mobPreUpdate(moby);
}

//--------------------------------------------------------------------------
void tremorPostUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // adjust animSpeed by speed and by animation
	float animSpeed = 0.5 * (pvars->MobVars.Config.Speed / MOB_BASE_SPEED);
  if (moby->AnimSeqId == TREMOR_ANIM_JUMP) {
    animSpeed = 0.5 * (1 - powf(moby->AnimSeqT / 21, 2));
    if (pvars->MobVars.MoveVars.Grounded) {
      animSpeed = 0.5;
    }
  } else if (tremorIsFlinching(moby) && !pvars->MobVars.MoveVars.Grounded) {
    animSpeed = 0.5 * (1 - powf(moby->AnimSeqT / 20, 2));
  }

	if (mobIsFrozen(moby) || (moby->DrawDist == 0 && pvars->MobVars.Action == TREMOR_ACTION_WALK)) {
		moby->AnimSpeed = 0;
	} else {
		moby->AnimSpeed = animSpeed;
	}
}

//--------------------------------------------------------------------------
void tremorPostDraw(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  u32 color = MobLODColors[pvars->MobVars.SpawnParamsIdx] | (moby->Opacity << 24);
  mobPostDrawQuad(moby, 127, color, 1);
}

//--------------------------------------------------------------------------
void tremorAlterTarget(VECTOR out, Moby* moby, VECTOR forward, float amount)
{
	VECTOR up = {0,0,1,0};
	
	vector_outerproduct(out, forward, up);
	vector_normalize(out, out);
	vector_scale(out, out, amount);
}

//--------------------------------------------------------------------------
void tremorMove(Moby* moby)
{
  mobMove(moby);
}

//--------------------------------------------------------------------------
void tremorOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs* e)
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

#if MOB_DAMAGETYPES
  pvars->TargetVars.damageTypes = MOB_DAMAGETYPES;
#endif

  // default move step
  pvars->MobVars.MoveVars.MoveStep = MOB_MOVE_SKIP_TICKS;
}

//--------------------------------------------------------------------------
void tremorOnDestroy(Moby* moby, int killedByPlayerId, int weaponId)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// set colors before death so that the corn has the correct color
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.SpawnParamsIdx];
}

//--------------------------------------------------------------------------
void tremorOnDamage(Moby* moby, struct MobDamageEventArgs* e)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	float damage = e->DamageQuarters / 4.0;
  float newHp = pvars->MobVars.Health - damage;

	int canFlinch = pvars->MobVars.Action != TREMOR_ACTION_FLINCH 
            && pvars->MobVars.Action != TREMOR_ACTION_BIG_FLINCH
            && pvars->MobVars.FlinchCooldownTicks == 0;

  int isShock = e->DamageFlags & 0x40;
  int isShortFreeze = e->DamageFlags & 0x40000000;

	// destroy
	if (newHp <= 0) {
    tremorForceLocalAction(moby, TREMOR_ACTION_DIE);
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
    float powerFactor = TREMOR_FLINCH_PROBABILITY_PWR_FACTOR * e->Knockback.Power;
    float probability = (damageRatio * TREMOR_FLINCH_PROBABILITY) + powerFactor;
    if (canFlinch) {
      if (e->Knockback.Force) {
        mobSetAction(moby, TREMOR_ACTION_BIG_FLINCH);
      } else if (isShock) {
        mobSetAction(moby, TREMOR_ACTION_FLINCH);
      } else if (randRange(0, 1) < probability) {
        if (randRange(0, 1) < powerFactor) {
          mobSetAction(moby, TREMOR_ACTION_BIG_FLINCH);
        } else {
          mobSetAction(moby, TREMOR_ACTION_FLINCH);
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
int tremorOnLocalDamage(Moby* moby, struct MobLocalDamageEventArgs* e)
{
  // don't filter local damage
  return 1;
}

//--------------------------------------------------------------------------
void tremorOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs* e)
{
  mobOnStateUpdate(moby, e);
}

//--------------------------------------------------------------------------
Moby* tremorGetNextTarget(Moby* moby)
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
					dist *= (1.0 / TREMOR_TARGET_KEEP_CURRENT_FACTOR);
				
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
int tremorGetPreferredAction(Moby* moby, int * delayTicks)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;

	// no preferred action
	if (tremorIsAttacking(moby))
		return -1;

	if (tremorIsSpawning(pvars))
		return -1;

  if (tremorIsFlinching(moby))
    return -1;

	if (pvars->MobVars.Action == TREMOR_ACTION_JUMP && !pvars->MobVars.MoveVars.Grounded) {
		return TREMOR_ACTION_WALK;
  }

  // jump if we've hit a slope and are grounded
  if (pvars->MobVars.MoveVars.Grounded && pvars->MobVars.MoveVars.HitWall && pvars->MobVars.MoveVars.WallSlope > TREMOR_MAX_WALKABLE_SLOPE) {
    return TREMOR_ACTION_JUMP;
  }

  // jump if we've hit a jump point on the path
  if (pvars->MobVars.MoveVars.QueueJumpSpeed) {
    return TREMOR_ACTION_JUMP;
  }

	// prevent action changing too quickly
	if (pvars->MobVars.ActionCooldownTicks)
		return -1;

	// get next target
	Moby * target = tremorGetNextTarget(moby);
	if (target) {
		vector_copy(t, target->Position);
		vector_subtract(t, t, moby->Position);
		float distSqr = vector_sqrmag(t);
		float attackRadiusSqr = pvars->MobVars.Config.AttackRadius * pvars->MobVars.Config.AttackRadius;

		if (distSqr <= attackRadiusSqr) {
			if (tremorCanAttack(pvars)) {
        if (delayTicks) *delayTicks = pvars->MobVars.Config.ReactionTickCount;
				return TREMOR_ACTION_ATTACK;
      }
			return TREMOR_ACTION_WALK;
		} else {
			return TREMOR_ACTION_WALK;
		}
	}
	
	return TREMOR_ACTION_IDLE;
}

//--------------------------------------------------------------------------
void tremorDoAction(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Moby* target = pvars->MobVars.Target;
	VECTOR t, t2;
  float difficulty = 1;
  float turnSpeed = pvars->MobVars.MoveVars.Grounded ? TREMOR_TURN_RADIANS_PER_SEC : TREMOR_TURN_AIR_RADIANS_PER_SEC;
  float acceleration = pvars->MobVars.MoveVars.Grounded ? TREMOR_MOVE_ACCELERATION : TREMOR_MOVE_AIR_ACCELERATION;
  int isInAirFromFlinching = !pvars->MobVars.MoveVars.Grounded 
                      && (pvars->MobVars.LastAction == TREMOR_ACTION_FLINCH || pvars->MobVars.LastAction == TREMOR_ACTION_BIG_FLINCH);

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	switch (pvars->MobVars.Action)
	{
		case TREMOR_ACTION_SPAWN:
		{
      mobTransAnim(moby, TREMOR_ANIM_SPAWN, 0);
      mobStand(moby);
			break;
		}
		case TREMOR_ACTION_FLINCH:
		case TREMOR_ACTION_BIG_FLINCH:
		{
      int animFlinchId = pvars->MobVars.Action == TREMOR_ACTION_BIG_FLINCH ? TREMOR_ANIM_FLINCH_FALL_GET_UP : TREMOR_ANIM_FLINCH;

      mobTransAnim(moby, animFlinchId, 0);
      
			if (pvars->MobVars.Knockback.Ticks > 0) {
				float power = PLAYER_KNOCKBACK_BASE_POWER;
				vector_fromyaw(t, pvars->MobVars.Knockback.Angle / 1000.0);
				t[2] = 1.0;
				vector_scale(t, t, power * 2 * MATH_DT);
				vector_add(pvars->MobVars.MoveVars.AddVelocity, pvars->MobVars.MoveVars.AddVelocity, t);
			} else if (pvars->MobVars.MoveVars.Grounded) {
        mobStand(moby);
      } else if (pvars->MobVars.CurrentActionForTicks > (1*TPS) && pvars->MobVars.MoveVars.HitWall && pvars->MobVars.MoveVars.StuckCounter) {
        mobStand(moby);
      }
			break;
		}
		case TREMOR_ACTION_IDLE:
		{
			mobTransAnim(moby, TREMOR_ANIM_IDLE, 0);
      mobStand(moby);
			break;
		}
		case TREMOR_ACTION_JUMP:
			{
        // move
        if (!isInAirFromFlinching) {
          if (target) {
            pathGetTargetPos(t, moby);
            mobTurnTowards(moby, t, turnSpeed);
            mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, t, 0, acceleration);
          } else {
            mobStand(moby);
          }
        }

        // handle jumping
        if (pvars->MobVars.MoveVars.Grounded) {
			    mobTransAnim(moby, TREMOR_ANIM_JUMP, 10);

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
            jumpSpeed = 5; //clamp(2 + (target->Position[2] - moby->Position[2]) * fabsf(pvars->MobVars.MoveVars.WallSlope) * 2, 3, 15);
          }

          vector_write(pvars->MobVars.MoveVars.Velocity, 0);
          pvars->MobVars.MoveVars.Velocity[2] = jumpSpeed * MATH_DT;
          pvars->MobVars.MoveVars.Grounded = 0;
          pvars->MobVars.MoveVars.QueueJumpSpeed = 0;
        }
				break;
			}
		case TREMOR_ACTION_LOOK_AT_TARGET:
    {
      mobStand(moby);
      if (target)
        mobTurnTowards(moby, target->Position, turnSpeed);
      break;
    }
    case TREMOR_ACTION_WALK:
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
            tremorAlterTarget(t2, moby, t, clamp(dist, 0, 10) * 0.3 * dir);
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
      if (moby->AnimSeqId == TREMOR_ANIM_JUMP && !pvars->MobVars.MoveVars.Grounded) {
        // wait for jump to land
      } else if (pvars->MobVars.MoveVars.QueueJumpSpeed) {
        tremorForceLocalAction(moby, TREMOR_ACTION_JUMP);
      } else if (mobHasVelocity(pvars)) {
				mobTransAnim(moby, TREMOR_ANIM_RUN, 0);
      } else if (moby->AnimSeqId != TREMOR_ANIM_RUN || pvars->MobVars.AnimationLooped) {
				mobTransAnim(moby, TREMOR_ANIM_IDLE, 0);
      }
			break;
		}
    case TREMOR_ACTION_DIE:
    {
      mobTransAnimLerp(moby, TREMOR_ANIM_FLINCH_BACK_FLIP_FALL, 5, 0);

      if (moby->AnimSeqId == TREMOR_ANIM_FLINCH_BACK_FLIP_FALL && moby->AnimSeqT > 15) {
        pvars->MobVars.Destroy = 1;
      }

      mobStand(moby);
      break;
    }
    case TREMOR_ACTION_ATTACK:
		{
      int attack1AnimId = TREMOR_ANIM_SWING;
			mobTransAnim(moby, attack1AnimId, 0);

			float speedMult = 0; // (moby->AnimSeqId == attack1AnimId && moby->AnimSeqT < 4) ? (difficulty * 2) : 1;
			int swingAttackReady = moby->AnimSeqId == attack1AnimId && moby->AnimSeqT >= 4 && moby->AnimSeqT < 8;
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
				tremorDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 0);
			}
			break;
		}
	}

  pvars->MobVars.CurrentActionForTicks ++;
}

//--------------------------------------------------------------------------
void tremorDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire)
{
  mobDoDamage(moby, radius, amount, damageFlags, friendlyFire, TREMOR_SUBSKELETON_JOINT_RIGHT_HAND_CLAW, 1);
}

//--------------------------------------------------------------------------
void tremorForceLocalAction(Moby* moby, int action)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  float difficulty = 1;

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	// from
	switch (pvars->MobVars.Action)
	{
		case TREMOR_ACTION_SPAWN:
		{
			// enable collision
			moby->CollActive = 0;
			break;
		}
		case TREMOR_ACTION_DIE:
		{
      // can't undie
      return;
		}
	}

	// to
	switch (action)
	{
		case TREMOR_ACTION_SPAWN:
		{
			// disable collision
			moby->CollActive = 1;
			break;
		}
		case TREMOR_ACTION_WALK:
		{
			
			break;
		}
		case TREMOR_ACTION_DIE:
		{
			// disable collision
			moby->CollActive = 1;
			break;
		}
		case TREMOR_ACTION_ATTACK:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
			break;
		}
		case TREMOR_ACTION_FLINCH:
		case TREMOR_ACTION_BIG_FLINCH:
		{
			tremorPlayHitSound(moby);
			pvars->MobVars.FlinchCooldownTicks = TREMOR_FLINCH_COOLDOWN_TICKS;
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
	pvars->MobVars.ActionCooldownTicks = TREMOR_ACTION_COOLDOWN_TICKS;
}

//--------------------------------------------------------------------------
short tremorGetArmor(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  return pvars->MobVars.Config.Bangles;
}

//--------------------------------------------------------------------------
void tremorPlayHitSound(Moby* moby)
{
  if (tremorHitSoundId < 0) return;

	TremorSoundDef.Index = tremorHitSoundId;
	soundPlay(&TremorSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
void tremorPlayAmbientSound(Moby* moby)
{
  if (!tremorAmbientSoundIdsCount) return;

	TremorSoundDef.Index = tremorAmbientSoundIds[rand(tremorAmbientSoundIdsCount)];
	soundPlay(&TremorSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
void tremorPlayDeathSound(Moby* moby)
{
  if (tremorDeathSoundId < 0) return;

	TremorSoundDef.Index = tremorDeathSoundId;
	soundPlay(&TremorSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
int tremorIsAttacking(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return pvars->MobVars.Action == TREMOR_ACTION_ATTACK && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
int tremorIsSpawning(struct MobPVar* pvars)
{
	return pvars->MobVars.Action == TREMOR_ACTION_SPAWN && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
int tremorCanAttack(struct MobPVar* pvars)
{
	return pvars->MobVars.AttackCooldownTicks == 0;
}

//--------------------------------------------------------------------------
int tremorIsFlinching(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return (moby->AnimSeqId == TREMOR_ANIM_FLINCH || moby->AnimSeqId == TREMOR_ANIM_FLINCH_FALL_GET_UP) && !pvars->MobVars.AnimationLooped;
}
