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

void reactorPreUpdate(Moby* moby);
void reactorPostUpdate(Moby* moby);
void reactorPostDraw(Moby* moby);
void reactorMove(Moby* moby);
void reactorOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs e);
void reactorOnDestroy(Moby* moby, int killedByPlayerId, int weaponId);
void reactorOnDamage(Moby* moby, struct MobDamageEventArgs e);
void reactorOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs e);
Moby* reactorGetNextTarget(Moby* moby);
enum ReactorAction reactorGetPreferredAction(Moby* moby);
void reactorDoAction(Moby* moby);
void reactorDoChargeDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire);
void reactorDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire);
void reactorForceLocalAction(Moby* moby, enum ReactorAction action);
short reactorGetArmor(Moby* moby);

void reactorPlayHitSound(Moby* moby);
void reactorPlayAmbientSound(Moby* moby);
void reactorPlayDeathSound(Moby* moby);
int reactorIsAttacking(struct MobPVar* pvars);
int reactorIsSpawning(struct MobPVar* pvars);
int reactorCanAttack(struct MobPVar* pvars, enum ReactorAction action);

struct MobVTable ReactorVTable = {
  .PreUpdate = &reactorPreUpdate,
  .PostUpdate = &reactorPostUpdate,
  .PostDraw = &reactorPostDraw,
  .Move = &reactorMove,
  .OnSpawn = &reactorOnSpawn,
  .OnDestroy = &reactorOnDestroy,
  .OnDamage = &reactorOnDamage,
  .OnStateUpdate = &reactorOnStateUpdate,
  .GetNextTarget = &reactorGetNextTarget,
  .GetPreferredAction = &reactorGetPreferredAction,
  .ForceLocalAction = &reactorForceLocalAction,
  .DoAction = &reactorDoAction,
  .DoDamage = &reactorDoDamage,
  .GetArmor = &reactorGetArmor,
};

SoundDef ReactorSoundDef = {
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
void reactorTransAnim(Moby* moby, int animId, float startOff)
{
  mobTransAnim(moby, animId, startOff);
}

//--------------------------------------------------------------------------
int reactorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config)
{
	struct MobSpawnEventArgs args;
  
	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(REACTOR_MOBY_OCLASS, sizeof(struct MobPVar) + sizeof(ReactorMobVars_t), &guberEvent, NULL);
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
void reactorPreUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (mobIsFrozen(moby))
    return;

	if (!pvars->MobVars.AmbientSoundCooldownTicks) {
		reactorPlayAmbientSound(moby);
		pvars->MobVars.AmbientSoundCooldownTicks = randRangeInt(REACTOR_AMBSND_MIN_COOLDOWN_TICKS, REACTOR_AMBSND_MAX_COOLDOWN_TICKS);
	}

  mobPreUpdate(moby);

  // decrement path target pos ticker
  decTimerU8(&pvars->MobVars.MoveVars.PathTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathCheckNearAndSeeTargetTicks);
  decTimerU8(&pvars->MobVars.MoveVars.PathNewTicks);
}

//--------------------------------------------------------------------------
void reactorPostUpdate(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  ReactorMobVars_t* reactorVars = (ReactorMobVars_t*)pvars->AdditionalMobVarsPtr;

  // adjust animSpeed by speed and by animation
	float animSpeed = reactorVars->AnimSpeedAdditive + 0.6 * (pvars->MobVars.Config.Speed / MOB_BASE_SPEED);
  if (moby->AnimSeqId == REACTOR_ANIM_JUMP_UP) {
    animSpeed = 1 * (1 - powf(moby->AnimSeqT / 21, 1));
    if (pvars->MobVars.MoveVars.Grounded) {
      animSpeed = 0.6;
    }
  }

	if (mobIsFrozen(moby) || (moby->DrawDist == 0 && pvars->MobVars.Action == REACTOR_ACTION_WALK)) {
		moby->AnimSpeed = 0;
	} else {
		moby->AnimSpeed = animSpeed;
	}
}

//--------------------------------------------------------------------------
void reactorPostDraw(Moby* moby)
{
  if (!moby || !moby->PVar)
    return;
    
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  u32 color = MobLODColors[pvars->MobVars.SpawnParamsIdx] | (moby->Opacity << 24);
  mobPostDrawQuad(moby, 127, color, 1);
}

//--------------------------------------------------------------------------
void reactorAlterTarget(VECTOR out, Moby* moby, VECTOR forward, float amount)
{
	VECTOR up = {0,0,1,0};
	
	vector_outerproduct(out, forward, up);
	vector_normalize(out, out);
	vector_scale(out, out, amount);
}

//--------------------------------------------------------------------------
void reactorMove(Moby* moby)
{
  mobMove(moby);
}

//--------------------------------------------------------------------------
void reactorOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs e)
{
  
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // set scale
  moby->Scale = 0.6;

  // colors by mob type
	moby->GlowRGBA = MobSecondaryColors[pvars->MobVars.SpawnParamsIdx];
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.SpawnParamsIdx];

  // targeting
	pvars->TargetVars.targetHeight = 1.5;
  pvars->MobVars.BlipType = 5;
}

//--------------------------------------------------------------------------
void reactorOnDestroy(Moby* moby, int killedByPlayerId, int weaponId)
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
    (expPos, 1, 0, 0, 0, 0, 16, 0, 16, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, expColor, expColor, expColor, expColor, expColor, expColor, expColor, expColor,
    0, 0, 0, 0, 0);
}

//--------------------------------------------------------------------------
void reactorOnDamage(Moby* moby, struct MobDamageEventArgs e)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	float damage = e.DamageQuarters / 4.0;
  float newHp = pvars->MobVars.Health - damage;

	int canFlinch = pvars->MobVars.Action != REACTOR_ACTION_FLINCH 
            && pvars->MobVars.Action != REACTOR_ACTION_BIG_FLINCH
            && pvars->MobVars.FlinchCooldownTicks == 0;

  int isShock = e.DamageFlags & 0x40;

	// destroy
	if (newHp <= 0) {
    reactorForceLocalAction(moby, REACTOR_ACTION_DIE);
    pvars->MobVars.LastHitBy = e.SourceUID;
    pvars->MobVars.LastHitByOClass = e.SourceOClass;
	}

	// knockback
	if (e.Knockback.Power > 0 && (canFlinch || e.Knockback.Force))
	{
		memcpy(&pvars->MobVars.Knockback, &e.Knockback, sizeof(struct Knockback));
	}

  // flinch
	if (mobAmIOwner(moby))
	{
		float damageRatio = damage / pvars->MobVars.Config.Health;
    if (canFlinch) {
      if (isShock) {
        mobSetAction(moby, REACTOR_ACTION_FLINCH);
      }
      else if (e.Knockback.Force || randRangeInt(0, 10) < e.Knockback.Power) {
        mobSetAction(moby, REACTOR_ACTION_BIG_FLINCH);
      }
      else if (randRange(0, 1) < (REACTOR_FLINCH_PROBABILITY * damageRatio)) {
        mobSetAction(moby, REACTOR_ACTION_FLINCH);
      }
    }
	}
}

//--------------------------------------------------------------------------
void reactorOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs e)
{
  mobOnStateUpdate(moby, e);
}

//--------------------------------------------------------------------------
Moby* reactorGetNextTarget(Moby* moby)
{
  asm (".set noreorder;");

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
					distSqr *= (1 / REACTOR_TARGET_KEEP_CURRENT_FACTOR);
				
				// pick closest target
				if (distSqr < closestPlayerDist) {
					closestPlayer = p;
					closestPlayerDist = distSqr;
				}
			}
		}

		++players;
	}

	if (closestPlayer) {
    return closestPlayer->SkinMoby;
  }

	return NULL;
}

//--------------------------------------------------------------------------
enum ReactorAction reactorGetPreferredAction(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;
  VECTOR mobyPosUp, targetPosUp;
  VECTOR up = {0,0,0.5,0};

	// no preferred action
	if (reactorIsAttacking(pvars))
		return -1;

	if (reactorIsSpawning(pvars))
		return -1;

	if (pvars->MobVars.Action == REACTOR_ACTION_JUMP && !pvars->MobVars.MoveVars.Grounded) {
		return REACTOR_ACTION_WALK;
  }

  // wait for grounded to stop flinch
  if ((pvars->MobVars.Action == REACTOR_ACTION_FLINCH || pvars->MobVars.Action == REACTOR_ACTION_BIG_FLINCH) && !pvars->MobVars.MoveVars.Grounded)
    return -1;

  // jump if we've hit a slope and are grounded
  if (pvars->MobVars.MoveVars.Grounded && pvars->MobVars.MoveVars.HitWall && pvars->MobVars.MoveVars.WallSlope > REACTOR_MAX_WALKABLE_SLOPE) {
    return REACTOR_ACTION_JUMP;
  }

  // jump if we've hit a jump point on the path
  if (pathShouldJump(moby)) {
    return REACTOR_ACTION_JUMP;
  }

	// get next target
	Moby * target = reactorGetNextTarget(moby);
	if (target) {
      
    // prevent action changing too quickly
    if (pvars->MobVars.AttackCooldownTicks) {
      return REACTOR_ACTION_WALK;
    }

		vector_subtract(t, target->Position, moby->Position);
		float distSqr = vector_sqrmag(t);
		float attackRadiusSqr = pvars->MobVars.Config.AttackRadius * pvars->MobVars.Config.AttackRadius;

    // near then swing
		if (distSqr <= attackRadiusSqr && reactorCanAttack(pvars, REACTOR_ACTION_ATTACK_SWING)) {
      return REACTOR_ACTION_ATTACK_SWING;
		}

    vector_add(mobyPosUp, moby->Position, up);
    vector_add(targetPosUp, target->Position, up);
    int targetInSight = !CollLine_Fix(mobyPosUp, targetPosUp, 2, 0, 0);
    if (!targetInSight) {
      return REACTOR_ACTION_WALK;
    }

    // near but not for swing then charge
		if (distSqr <= (REACTOR_MAX_DIST_FOR_CHARGE*REACTOR_MAX_DIST_FOR_CHARGE) && reactorCanAttack(pvars, REACTOR_ACTION_ATTACK_CHARGE)) {
      return REACTOR_ACTION_ATTACK_CHARGE;
		}

    // far but in sight, fire laser
    if (reactorCanAttack(pvars, REACTOR_ACTION_ATTACK_LASER)) {
      return REACTOR_ACTION_ATTACK_LASER;
    }

    return REACTOR_ACTION_WALK;
	}
	
	return REACTOR_ACTION_IDLE;
}

//--------------------------------------------------------------------------
void reactorDoAction(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  ReactorMobVars_t* reactorVars = (ReactorMobVars_t*)pvars->AdditionalMobVarsPtr;
	Moby* target = pvars->MobVars.Target;
	VECTOR t, t2;
  int i;
  float difficulty = 1;
  float turnSpeed = pvars->MobVars.MoveVars.Grounded ? REACTOR_TURN_RADIANS_PER_SEC : REACTOR_TURN_AIR_RADIANS_PER_SEC;
  float acceleration = pvars->MobVars.MoveVars.Grounded ? REACTOR_MOVE_ACCELERATION : REACTOR_MOVE_AIR_ACCELERATION;
  int isInAirFromFlinching = !pvars->MobVars.MoveVars.Grounded 
                      && (pvars->MobVars.LastAction == REACTOR_ACTION_FLINCH || pvars->MobVars.LastAction == REACTOR_ACTION_BIG_FLINCH);

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

  MATRIX* joints = (MATRIX*)moby->JointCache;
  static int asd = 3;
  //printf("a:%d id:%d f:%d j:%d: ", pvars->MobVars.Action, moby->AnimSeqId, moby->AnimFlags, asd); vector_print(&joints[asd][12]); printf("\n");

  // reset anim speed add
  reactorVars->AnimSpeedAdditive = 0;

  //
	switch (pvars->MobVars.Action)
	{
		case REACTOR_ACTION_SPAWN:
		{
      reactorTransAnim(moby, REACTOR_ANIM_JUMP_UP, 0);
      mobStand(moby);
			break;
		}
		case REACTOR_ACTION_FLINCH:
		case REACTOR_ACTION_BIG_FLINCH:
		{
      int animFlinchId = pvars->MobVars.Action == REACTOR_ACTION_BIG_FLINCH ? REACTOR_ANIM_FLINCH_FALL_DOWN : REACTOR_ANIM_FLINCH_SMALL;

      reactorTransAnim(moby, animFlinchId, 0);
      
			if (pvars->MobVars.Knockback.Ticks > 0) {
				float power = PLAYER_KNOCKBACK_BASE_POWER * pvars->MobVars.Knockback.Power;
				vector_fromyaw(t, pvars->MobVars.Knockback.Angle / 1000.0);
				t[2] = 1.0;
				vector_scale(t, t, power * 1 * MATH_DT);
				vector_add(pvars->MobVars.MoveVars.AddVelocity, pvars->MobVars.MoveVars.AddVelocity, t);
			} else if (pvars->MobVars.MoveVars.Grounded) {
        mobStand(moby);
      }
			break;
		}
		case REACTOR_ACTION_IDLE:
		{
			reactorTransAnim(moby, REACTOR_ANIM_IDLE, 0);
      mobStand(moby);
			break;
		}
		case REACTOR_ACTION_JUMP:
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
			    reactorTransAnim(moby, REACTOR_ANIM_JUMP_UP, 5);

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
            jumpSpeed = 8; //clamp(2 + (target->Position[2] - moby->Position[2]) * fabsf(pvars->MobVars.MoveVars.WallSlope) * 2, 3, 15);
          }

          pvars->MobVars.MoveVars.Velocity[2] = jumpSpeed * MATH_DT;
          pvars->MobVars.MoveVars.Grounded = 0;
        }
				break;
			}
		case REACTOR_ACTION_LOOK_AT_TARGET:
    {
      mobStand(moby);
      if (target) {
        mobTurnTowards(moby, target->Position, turnSpeed);
      }
      break;
    }
    case REACTOR_ACTION_WALK:
		{
      if (!isInAirFromFlinching) {
        if (target) {

          float dir = ((pvars->MobVars.ActionId + pvars->MobVars.Random) % 3) - 1;

          // determine next position
          vector_copy(t, target->Position);
          vector_subtract(t, t, moby->Position);
          float dist = vector_length(t);

          if (dist > (pvars->MobVars.Config.AttackRadius - pvars->MobVars.Config.HitRadius)) {

            pathGetTargetPos(t, moby);
            vector_subtract(t, t, moby->Position);
            float dist = vector_length(t);
            if (dist < 10.0) {
              reactorAlterTarget(t2, moby, t, clamp(dist, 0, 10) * 0.3 * dir);
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
      if (moby->AnimSeqId == REACTOR_ANIM_JUMP_UP && !pvars->MobVars.MoveVars.Grounded) {
        // wait for jump to land
      }
			else if (mobHasVelocity(pvars))
				reactorTransAnim(moby, REACTOR_ANIM_RUN, 0);
			else
				reactorTransAnim(moby, REACTOR_ANIM_IDLE, 0);
			break;
		}
    case REACTOR_ACTION_DIE:
    {
      mobTransAnimLerp(moby, REACTOR_ANIM_DIE, 5, 0);

      if (moby->AnimSeqId == REACTOR_ANIM_DIE && moby->AnimSeqT > 124) {
        pvars->MobVars.Destroy = 1;
      }

      mobStand(moby);
      break;
    }
		case REACTOR_ACTION_ATTACK_SWING:
		{
      int attack1AnimId = REACTOR_ANIM_SWING;
      if (pvars->MobVars.CurrentActionForTicks > 60) {
        reactorTransAnim(moby, REACTOR_ANIM_IDLE, 0);
        if (target) {
          mobTurnTowards(moby, target->Position, turnSpeed);
        }
        mobStand(moby);
        reactorForceLocalAction(moby, REACTOR_ACTION_IDLE);
        break;
      }

			reactorTransAnim(moby, attack1AnimId, 0);
      moby->AnimFlags = 0x10;

			float speedMult = 1;
			int swingAttackReady = moby->AnimSeqId == attack1AnimId && moby->AnimSeqT >= 14 && moby->AnimSeqT < 18;
			u32 damageFlags = 0x00081801;

      if (!isInAirFromFlinching) {
        if (target) {
          mobTurnTowards(moby, target->Position, turnSpeed);
          mobStand(moby);
          //mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, target->Position, speedMult * pvars->MobVars.Config.Speed, acceleration);
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
				reactorDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 0);
			}
			break;
		}
    case REACTOR_ACTION_ATTACK_CHARGE:
		{
      float speedMult = 0;
      int facePlayer = 1;
      int nextAnimId = moby->AnimSeqId;
			int attackCanDoChargeDamage = 0;
			int attackCanDoSwingDamage = 0;
			u32 damageFlags = 0x00081801;

      switch (moby->AnimSeqId)
      {
        // begins squat and foot rub animation
        // don't loop, continue into foot rub
        case REACTOR_ANIM_SQUAT_PREPARE_FOR_DASH:
        {
          if (pvars->MobVars.AnimationLooped) {
            nextAnimId = REACTOR_ANIM_SQUAT_PREPARE_FOR_DASH_FOOT_RUB_GROUND;
          }
          break;
        }
        // repeat foot rub for the ChargeChargeupTargetCount times
        case REACTOR_ANIM_SQUAT_PREPARE_FOR_DASH_FOOT_RUB_GROUND:
        {
          if (pvars->MobVars.AnimationLooped >= reactorVars->ChargeChargeupTargetCount) {
            nextAnimId = REACTOR_ANIM_SQUAT_PREPARE_FOR_DASH_FOOT_RUB_GROUND2;
          }
          break;
        }
        // enters pre dash animation
        case REACTOR_ANIM_SQUAT_PREPARE_FOR_DASH_FOOT_RUB_GROUND2:
        {
          if (pvars->MobVars.AnimationLooped) {
            nextAnimId = REACTOR_ANIM_SWING;
          }
          
          break;
        }
        // dash
        case REACTOR_ANIM_SWING:
        {
          if (moby->AnimSeqT < 14) {
            speedMult = REACTOR_CHARGE_SPEED;
            acceleration = REACTOR_CHARGE_ACCELERATION;
            reactorVars->AnimSpeedAdditive = 2;
            facePlayer = 0;
          }

          // damage during swing
          attackCanDoChargeDamage = moby->AnimSeqT >= 0 && moby->AnimSeqT < 14;
          attackCanDoSwingDamage = moby->AnimSeqT >= 14 && moby->AnimSeqT < 20;

          // exit
          if (pvars->MobVars.AnimationLooped) {
            reactorForceLocalAction(moby, REACTOR_ACTION_IDLE);
          }
          break;
        }
        default:
        {
          nextAnimId = REACTOR_ANIM_SQUAT_PREPARE_FOR_DASH;
          break;
        }
      }

      // transition
			reactorTransAnim(moby, nextAnimId, 0);

			if (target) {
        if (facePlayer) mobTurnTowards(moby, target->Position, turnSpeed);
        mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, target->Position, speedMult * pvars->MobVars.Config.Speed, acceleration);
			} else {
				// stand
				mobStand(moby);
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
			
			if (attackCanDoChargeDamage && damageFlags) {
				reactorDoChargeDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage * 2, damageFlags, 0);
			} else if (attackCanDoSwingDamage && damageFlags) {
				reactorDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage * 2, damageFlags, 0);
			}
			break;
		}
	}

  pvars->MobVars.CurrentActionForTicks ++;
}

//--------------------------------------------------------------------------
void reactorDoChargeDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire)
{
  mobDoDamage(moby, radius * 2.5, amount, damageFlags, friendlyFire, 2);
}

//--------------------------------------------------------------------------
void reactorDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire)
{
  mobDoDamage(moby, radius, amount, damageFlags, friendlyFire, 4);
}

//--------------------------------------------------------------------------
void reactorForceLocalAction(Moby* moby, enum ReactorAction action)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  ReactorMobVars_t* reactorVars = (ReactorMobVars_t*)pvars->AdditionalMobVarsPtr;
  float difficulty = 1;

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	// from
	switch (pvars->MobVars.Action)
	{
		case REACTOR_ACTION_SPAWN:
		{
			// enable collision
			moby->CollActive = 0;
			break;
		}
		case REACTOR_ACTION_DIE:
		{
      // can't undie
      return;
		}
	}

	// to
	switch (action)
	{
		case REACTOR_ACTION_SPAWN:
		{
			// disable collision
			moby->CollActive = 1;
			break;
		}
		case REACTOR_ACTION_WALK:
		{
			
			break;
		}
		case REACTOR_ACTION_DIE:
		{
			
			break;
		}
		case REACTOR_ACTION_ATTACK_SWING:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
			break;
		}
		case REACTOR_ACTION_ATTACK_CHARGE:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
      pvars->MobVars.Attack2CooldownTicks = REACTOR_CHARGE_ATTACK_COOLDOWN_TICKS;
      reactorVars->ChargeChargeupTargetCount = REACTOR_CHARGE_ATTACK_MIN_STALL_LOOPS + ((u8)pvars->MobVars.DynamicRandom % (REACTOR_CHARGE_ATTACK_MAX_STALL_LOOPS - REACTOR_CHARGE_ATTACK_MIN_STALL_LOOPS));
			break;
		}
		case REACTOR_ACTION_ATTACK_LASER:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
      pvars->MobVars.Attack3CooldownTicks = REACTOR_LASER_ATTACK_COOLDOWN_TICKS;
			break;
		}
		case REACTOR_ACTION_FLINCH:
		case REACTOR_ACTION_BIG_FLINCH:
		{
			reactorPlayHitSound(moby);
			pvars->MobVars.FlinchCooldownTicks = REACTOR_FLINCH_COOLDOWN_TICKS;
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
	pvars->MobVars.ActionCooldownTicks = REACTOR_ACTION_COOLDOWN_TICKS;
}

//--------------------------------------------------------------------------
short reactorGetArmor(Moby* moby)
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
void reactorPlayHitSound(Moby* moby)
{
	ReactorSoundDef.Index = 0x17D;
	soundPlay(&ReactorSoundDef, 0, moby, 0, 0x400);
}	

//--------------------------------------------------------------------------
void reactorPlayAmbientSound(Moby* moby)
{
  const int ambientSoundIds[] = { 0x17A, 0x179 };
	ReactorSoundDef.Index = ambientSoundIds[rand(2)];
	soundPlay(&ReactorSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
void reactorPlayDeathSound(Moby* moby)
{
	ReactorSoundDef.Index = 0x171;
	soundPlay(&ReactorSoundDef, 0, moby, 0, 0x400);
}

//--------------------------------------------------------------------------
int reactorIsAttacking(struct MobPVar* pvars)
{
	return (pvars->MobVars.Action == REACTOR_ACTION_ATTACK_SWING && !pvars->MobVars.AnimationLooped)
        || pvars->MobVars.Action == REACTOR_ACTION_ATTACK_CHARGE
        || pvars->MobVars.Action == REACTOR_ACTION_ATTACK_LASER;
}

//--------------------------------------------------------------------------
int reactorIsSpawning(struct MobPVar* pvars)
{
	return pvars->MobVars.Action == REACTOR_ACTION_SPAWN && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
int reactorCanAttack(struct MobPVar* pvars, enum ReactorAction action)
{
	if (pvars->MobVars.AttackCooldownTicks != 0) return 0;

  switch (action)
  {
    case REACTOR_ACTION_ATTACK_SWING:
    {
      // swing
      return 1;
    }
    case REACTOR_ACTION_ATTACK_CHARGE:
    {
      // charge
      return pvars->MobVars.Attack2CooldownTicks == 0;
    }
    case REACTOR_ACTION_ATTACK_LASER:
    {
      return 0;
      // laser
      return pvars->MobVars.Attack3CooldownTicks == 0;
    }
    default: return 0;
  }
}