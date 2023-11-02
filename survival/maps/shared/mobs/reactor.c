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

int mobAmIOwner(Moby* moby);
int mobIsFrozen(Moby* moby);
void mobDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire, int jointId);
void mobSetAction(Moby* moby, int action);
void mobTransAnimLerp(Moby* moby, int animId, int lerpFrames, float startOff);
void mobTransAnim(Moby* moby, int animId, float startOff);
int mobHasVelocity(struct MobPVar* pvars);
void mobStand(Moby* moby);
int mobMoveCheck(Moby* moby, VECTOR outputPos, VECTOR from, VECTOR to);
void mobMove(Moby* moby);
void mobTurnTowards(Moby* moby, VECTOR towards, float turnSpeed);
void mobGetVelocityToTarget(Moby* moby, VECTOR velocity, VECTOR from, VECTOR to, float speed, float acceleration);
void mobPostDrawQuad(Moby* moby, int texId, u32 color);
void mobOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs e);

void reactorPreUpdate(Moby* moby);
void reactorPostUpdate(Moby* moby);
void reactorPostDraw(Moby* moby);
void reactorMove(Moby* moby);
void reactorOnSpawn(Moby* moby, VECTOR position, float yaw, u32 spawnFromUID, char random, struct MobSpawnEventArgs e);
void reactorOnDestroy(Moby* moby, int killedByPlayerId, int weaponId);
void reactorOnDamage(Moby* moby, struct MobDamageEventArgs e);
void reactorOnStateUpdate(Moby* moby, struct MobStateUpdateEventArgs e);
Moby* reactorGetNextTarget(Moby* moby);
enum MobAction reactorGetPreferredAction(Moby* moby);
void reactorDoAction(Moby* moby);
void reactorDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire);
void reactorForceLocalAction(Moby* moby, enum MobAction action);
short reactorGetArmor(Moby* moby);

void reactorPlayHitSound(Moby* moby);
void reactorPlayAmbientSound(Moby* moby);
void reactorPlayDeathSound(Moby* moby);
int reactorIsAttacking(struct MobPVar* pvars);
int reactorIsSpawning(struct MobPVar* pvars);
int reactorCanAttack(struct MobPVar* pvars, enum MobAction action);

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
	guberMobyCreateSpawned(REACTOR_MOBY_OCLASS, sizeof(struct MobPVar), &guberEvent, NULL);
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

  // adjust animSpeed by speed and by animation
	float animSpeed = 0.6 * (pvars->MobVars.Config.Speed / MOB_BASE_SPEED);
  if (moby->AnimSeqId == REACTOR_ANIM_JUMP_UP) {
    animSpeed = 1 * (1 - powf(moby->AnimSeqT / 21, 1));
    if (pvars->MobVars.MoveVars.Grounded) {
      animSpeed = 0.6;
    }
  }

	if (mobIsFrozen(moby) || (moby->DrawDist == 0 && pvars->MobVars.Action == MOB_ACTION_WALK)) {
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
  mobPostDrawQuad(moby, 127, color);
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

	int canFlinch = pvars->MobVars.Action != MOB_ACTION_FLINCH 
            && pvars->MobVars.Action != MOB_ACTION_BIG_FLINCH
            && pvars->MobVars.Action != MOB_ACTION_TIME_BOMB 
            && pvars->MobVars.FlinchCooldownTicks == 0;

  int isShock = e.DamageFlags & 0x40;

	// destroy
	if (newHp <= 0) {
    reactorForceLocalAction(moby, MOB_ACTION_DIE);
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
        mobSetAction(moby, MOB_ACTION_FLINCH);
      }
      else if (e.Knockback.Force || randRangeInt(0, 10) < e.Knockback.Power) {
        mobSetAction(moby, MOB_ACTION_BIG_FLINCH);
      }
      else if (randRange(0, 1) < (REACTOR_FLINCH_PROBABILITY * damageRatio)) {
        mobSetAction(moby, MOB_ACTION_FLINCH);
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
enum MobAction reactorGetPreferredAction(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;

	// no preferred action
	if (reactorIsAttacking(pvars))
		return -1;

	if (reactorIsSpawning(pvars))
		return -1;

	if (pvars->MobVars.Action == MOB_ACTION_JUMP && !pvars->MobVars.MoveVars.Grounded) {
		return MOB_ACTION_WALK;
  }

  // wait for grounded to stop flinch
  if ((pvars->MobVars.Action == MOB_ACTION_FLINCH || pvars->MobVars.Action == MOB_ACTION_BIG_FLINCH) && !pvars->MobVars.MoveVars.Grounded)
    return -1;

  // jump if we've hit a slope and are grounded
  if (pvars->MobVars.MoveVars.Grounded && pvars->MobVars.MoveVars.WallSlope > REACTOR_MAX_WALKABLE_SLOPE) {
    return MOB_ACTION_JUMP;
  }

  // jump if we've hit a jump point on the path
  if (pathShouldJump(moby)) {
    return MOB_ACTION_JUMP;
  }

	// prevent action changing too quickly
	if (pvars->MobVars.AttackCooldownTicks)
		return -1;

	// get next target
	Moby * target = reactorGetNextTarget(moby);
	if (target) {
		vector_copy(t, target->Position);
		vector_subtract(t, t, moby->Position);
		float distSqr = vector_sqrmag(t);
		float attackRadiusSqr = pvars->MobVars.Config.AttackRadius * pvars->MobVars.Config.AttackRadius;

    // near then swing
		if (distSqr <= attackRadiusSqr && reactorCanAttack(pvars, MOB_ACTION_ATTACK)) {
      return MOB_ACTION_ATTACK;
		}

    int targetInSight = !CollLine_Fix(moby->Position, target->Position, 2, 0, 0);
    if (!targetInSight) {
      return MOB_ACTION_WALK;
    }

    // near but not for swing then charge
		if (distSqr <= (REACTOR_MAX_DIST_FOR_CHARGE*REACTOR_MAX_DIST_FOR_CHARGE) && reactorCanAttack(pvars, MOB_ACTION_ATTACK_2)) {
      return MOB_ACTION_ATTACK_2;
		}

    // far but in sight, fire laser
    if (reactorCanAttack(pvars, MOB_ACTION_ATTACK_3)) {
      return MOB_ACTION_ATTACK_3;
    }

    return MOB_ACTION_WALK;
	}
	
	return MOB_ACTION_IDLE;
}

//--------------------------------------------------------------------------
void reactorDoAction(Moby* moby)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Moby* target = pvars->MobVars.Target;
	VECTOR t, t2;
  int i;
  float difficulty = 1;
  float turnSpeed = pvars->MobVars.MoveVars.Grounded ? REACTOR_TURN_RADIANS_PER_SEC : REACTOR_TURN_AIR_RADIANS_PER_SEC;
  float acceleration = pvars->MobVars.MoveVars.Grounded ? REACTOR_MOVE_ACCELERATION : REACTOR_MOVE_AIR_ACCELERATION;

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	switch (pvars->MobVars.Action)
	{
		case MOB_ACTION_SPAWN:
		{
      reactorTransAnim(moby, REACTOR_ANIM_JUMP_UP, 0);
      mobStand(moby);
			break;
		}
		case MOB_ACTION_FLINCH:
		case MOB_ACTION_BIG_FLINCH:
		{
      int animFlinchId = pvars->MobVars.Action == MOB_ACTION_BIG_FLINCH ? REACTOR_ANIM_FLINCH_FALL_DOWN : REACTOR_ANIM_FLINCH_SMALL;

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
		case MOB_ACTION_IDLE:
		{
			reactorTransAnim(moby, REACTOR_ANIM_IDLE, 0);
      mobStand(moby);
			break;
		}
		case MOB_ACTION_JUMP:
			{
        // move
        if (target) {
          pathGetTargetPos(t, moby);
          mobTurnTowards(moby, t, turnSpeed);
          mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, t, pvars->MobVars.Config.Speed, acceleration);
        } else {
          mobStand(moby);
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
		case MOB_ACTION_LOOK_AT_TARGET:
    {
      mobStand(moby);
      if (target) {
        mobTurnTowards(moby, target->Position, turnSpeed);
      }
      break;
    }
    case MOB_ACTION_WALK:
		{
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
    case MOB_ACTION_DIE:
    {
      mobTransAnimLerp(moby, REACTOR_ANIM_DIE, 5, 0);

      if (moby->AnimSeqId == REACTOR_ANIM_DIE && moby->AnimSeqT > 124) {
        pvars->MobVars.Destroy = 1;
      }

      mobStand(moby);
      break;
    }
		case MOB_ACTION_ATTACK:
		{
      int attack1AnimId = REACTOR_ANIM_SWING;
			reactorTransAnim(moby, attack1AnimId, 0);

			float speedMult = 1;
			int swingAttackReady = moby->AnimSeqId == attack1AnimId && moby->AnimSeqT >= 14 && moby->AnimSeqT < 18;
			u32 damageFlags = 0x00081801;

			if (target) {
        mobTurnTowards(moby, target->Position, turnSpeed);
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
			
			if (swingAttackReady && damageFlags) {
				reactorDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 0);
			}
			break;
		}
    case MOB_ACTION_ATTACK_2:
		{
      int attack1AnimId = REACTOR_ANIM_SQUAT_PREPARE_FOR_DASH;
			reactorTransAnim(moby, attack1AnimId, 0);

			float speedMult = 1;
			int swingAttackReady = moby->AnimSeqId == attack1AnimId && moby->AnimSeqT >= 14 && moby->AnimSeqT < 18;
			u32 damageFlags = 0x00081801;

			if (target) {
        mobTurnTowards(moby, target->Position, turnSpeed);
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
			
			if (swingAttackReady && damageFlags) {
				reactorDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 0);
			}
			break;
		}
	}

  pvars->MobVars.CurrentActionForTicks ++;
}

//--------------------------------------------------------------------------
void reactorDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire)
{
  mobDoDamage(moby, radius, amount, damageFlags, friendlyFire, 4);
}

//--------------------------------------------------------------------------
void reactorForceLocalAction(Moby* moby, enum MobAction action)
{
  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  float difficulty = 1;

  if (MapConfig.State)
    difficulty = MapConfig.State->Difficulty;

	// from
	switch (pvars->MobVars.Action)
	{
		case MOB_ACTION_SPAWN:
		{
			// enable collision
			moby->CollActive = 0;
			break;
		}
		case MOB_ACTION_DIE:
		{
      // can't undie
      return;
		}
	}

	// to
	switch (action)
	{
		case MOB_ACTION_SPAWN:
		{
			// disable collision
			moby->CollActive = 1;
			break;
		}
		case MOB_ACTION_WALK:
		{
			
			break;
		}
		case MOB_ACTION_DIE:
		{
			
			break;
		}
		case MOB_ACTION_ATTACK:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
			break;
		}
		case MOB_ACTION_ATTACK_2:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
      pvars->MobVars.Attack2CooldownTicks = REACTOR_CHARGE_ATTACK_COOLDOWN_TICKS;
			break;
		}
		case MOB_ACTION_ATTACK_3:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;
      pvars->MobVars.Attack3CooldownTicks = REACTOR_LASER_ATTACK_COOLDOWN_TICKS;
			break;
		}
		case MOB_ACTION_TIME_BOMB:
		{
			pvars->MobVars.OpacityFlickerDirection = 4;
			pvars->MobVars.TimeBombTicks = REACTOR_TIMEBOMB_TICKS / clamp(difficulty, 0.5, 2);
			break;
		}
		case MOB_ACTION_FLINCH:
		case MOB_ACTION_BIG_FLINCH:
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
	return pvars->MobVars.Action == MOB_ACTION_TIME_BOMB || (pvars->MobVars.Action == MOB_ACTION_ATTACK && !pvars->MobVars.AnimationLooped);
}

//--------------------------------------------------------------------------
int reactorIsSpawning(struct MobPVar* pvars)
{
	return pvars->MobVars.Action == MOB_ACTION_SPAWN && !pvars->MobVars.AnimationLooped;
}

//--------------------------------------------------------------------------
int reactorCanAttack(struct MobPVar* pvars, enum MobAction action)
{
	if (pvars->MobVars.AttackCooldownTicks != 0) return 0;

  switch (action)
  {
    case MOB_ACTION_ATTACK:
    {
      // swing
      return 1;
    }
    case MOB_ACTION_ATTACK_2:
    {
      // charge
      return pvars->MobVars.Attack2CooldownTicks == 0;
    }
    case MOB_ACTION_ATTACK_3:
    {
      // laser
      return pvars->MobVars.Attack3CooldownTicks == 0;
    }
    default: return 0;
  }
}
