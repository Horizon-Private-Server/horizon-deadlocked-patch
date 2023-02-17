#include "include/mob.h"
#include "include/drop.h"
#include "include/utils.h"
#include "include/game.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/moby.h>
#include <libdl/random.h>
#include <libdl/radar.h>
#include <libdl/color.h>

int mobInMobMove = 0;
Moby* mobFirstInList = 0;
Moby* mobLastInList = 0;

#if FIXEDTARGET
extern Moby* FIXEDTARGETMOBY;
#endif

/* 
 * 
 */
SoundDef BaseSoundDef =
{
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


short AmbientSoundIds[] = {
	//0x181,
	//0x180,
	0x17A,
	0x179
};
const int AmbientSoundIdsCount = sizeof(AmbientSoundIds) / sizeof(short);

u32 MobPrimaryColors[] = {
	[MOB_NORMAL] 	0x00464443,
	[MOB_FREEZE] 	0x00804000,
	[MOB_ACID] 		0x00464443,
	[MOB_EXPLODE] 0x00202080,
	[MOB_GHOST] 	0x00464443,
	[MOB_TANK]		0x00464443,
};

u32 MobSecondaryColors[] = {
	[MOB_NORMAL] 	0x80202020,
	[MOB_FREEZE] 	0x80FF2000,
	[MOB_ACID] 		0x8000FF00,
	[MOB_EXPLODE] 0x000040C0,
	[MOB_GHOST] 	0x80202020,
	[MOB_TANK]		0x80202020,
};

u32 MobSpecialMutationColors[] = {
	[MOB_SPECIAL_MUTATION_FREEZE] 0xFFFF2000,
	[MOB_SPECIAL_MUTATION_ACID] 	0xFF00FF00,
};

u32 MobLODColors[] = {
	[MOB_NORMAL] 	0x00808080,
	[MOB_FREEZE] 	0x00F08000,
	[MOB_ACID] 		0x0000F000,
	[MOB_EXPLODE] 0x004040F0,
	[MOB_GHOST] 	0x00464443,
	[MOB_TANK]		0x00808080,
};

short MoveStepMaxByClientCount[GAME_MAX_PLAYERS] = {
	12,
	24,
	32,
	48,
	64,
	72,
	96,
	128,
	128,
	128
};

Moby* AllMobsSorted[MAX_MOBS_SPAWNED];
int AllMobsSortedFreeSpots = MAX_MOBS_SPAWNED;

extern struct SurvivalState State;

GuberEvent* mobCreateEvent(Moby* moby, u32 eventType);



int mobAmIOwner(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	return gameGetMyClientId() == pvars->MobVars.Owner;
}

void mobTransAnimLerp(Moby* moby, int animId, int lerpFrames)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (moby->AnimSeqId != animId) {
		mobyAnimTransition(moby, animId, lerpFrames, 0);

		pvars->MobVars.AnimationReset = 1;
		pvars->MobVars.AnimationLooped = 0;
	} else {
		
		// get current t
		// if our stored start is uninitialized, then set current t as start
		float t = moby->AnimSeqT;
		float end = (float)(*(u8*)((u32)moby->AnimSeq + 0x10)) - (float)lerpFrames;
		if (t >= end && pvars->MobVars.AnimationReset) {
			pvars->MobVars.AnimationLooped++;
			pvars->MobVars.AnimationReset = 0;
		} else if (t < end) {
			pvars->MobVars.AnimationReset = 1;
		}
	}
}

void mobTransAnim(Moby* moby, int animId)
{
	mobTransAnimLerp(moby, animId, 10);
}

void mobPlayHitSound(Moby* moby)
{
	BaseSoundDef.Index = 0x17D;
	soundPlay(&BaseSoundDef, 0, moby, 0, 0x400);
}	

void mobPlayAmbientSound(Moby* moby)
{
	BaseSoundDef.Index = AmbientSoundIds[rand(AmbientSoundIdsCount)];
	soundPlay(&BaseSoundDef, 0, moby, 0, 0x400);
}

void mobPlayDeathSound(Moby* moby)
{
	BaseSoundDef.Index = 0x171;
	soundPlay(&BaseSoundDef, 0, moby, 0, 0x400);
}

void mobSpawnCorn(Moby* moby, int bangle)
{
#if MOB_CORN
	mobyBlowCorn(
			moby
		, bangle
		, 0
		, 3.0
		, 6.0
		, 3.0
		, 6.0
		, -1
		, -1.0
		, -1.0
		, 255
		, 1
		, 0
		, 1
		, 1.0
		, 0x23
		, 3
		, 1.0
		, NULL
		, 0
		);
#endif
}

void mobSendStateUpdate(Moby* moby)
{
	struct MobStateUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_TARGET_UPDATE);
	if (guberEvent) {
		vector_copy(args.Position, moby->Position);
		args.TargetUID = guberGetUID(pvars->MobVars.Target);
		guberEventWrite(guberEvent, &args, sizeof(struct MobStateUpdateEventArgs));
	}
}

void mobSendOwnerUpdate(Moby* moby, char owner)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_OWNER_UPDATE);
	if (guberEvent) {
		guberEventWrite(guberEvent, &owner, sizeof(pvars->MobVars.Owner));
	}
}

void mobSendDamageEvent(Moby* moby, Moby* sourcePlayer, Moby* source, float amount, int damageFlags)
{
	VECTOR delta;
	struct MobDamageEventArgs args;

	// determine knockback
	Player * pDamager = playerGetFromUID(guberGetUID(sourcePlayer));
	if (pDamager)
	{
		int weaponId = getWeaponIdFromOClass(source->OClass);
		if (weaponId >= 0) {
			args.Knockback.Power = (u8)playerGetWeaponAlphaModCount(pDamager, weaponId, ALPHA_MOD_IMPACT);
		}
	}

	// determine angle
	vector_subtract(delta, moby->Position, sourcePlayer->Position);
	float len = vector_length(delta);
	float angle = atan2f(delta[1] / len, delta[0] / len);
	args.Knockback.Angle = (short)(angle * 1000);
	args.Knockback.Ticks = PLAYER_KNOCKBACK_BASE_TICKS;
	
	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_DAMAGE);
	if (guberEvent) {
		args.SourceUID = guberGetUID(sourcePlayer);
		args.SourceOClass = source->OClass;
		args.DamageQuarters = amount*4;
		guberEventWrite(guberEvent, &args, sizeof(struct MobDamageEventArgs));
	}
}

void mobDestroy(Moby* moby, int hitByUID)
{
	char killedByPlayerId = -1;
	char weaponId = -1;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_DESTROY);
	if (guberEvent) {

		// get weapon id from source oclass
		weaponId = getWeaponIdFromOClass(pvars->MobVars.LastHitByOClass);

		// get damager player id
		Player* damager = (Player*)playerGetFromUID(hitByUID);
		if (damager)
			killedByPlayerId = (char)damager->PlayerId;

		// 
		guberEventWrite(guberEvent, &killedByPlayerId, sizeof(killedByPlayerId));
		guberEventWrite(guberEvent, &weaponId, sizeof(weaponId));
	}
}

void mobMutate(struct MobSpawnParams* spawnParams, enum MobMutateAttribute attribute)
{
	if (!spawnParams)
		return;
	
	switch (attribute)
	{
		case MOB_MUTATE_DAMAGE: if (spawnParams->Config.Damage < spawnParams->Config.MaxDamage) { spawnParams->Config.Damage += ZOMBIE_DAMAGE_MUTATE * ZOMBIE_BASE_DAMAGE * State.Difficulty; break; } 
		case MOB_MUTATE_SPEED: if (spawnParams->Config.Speed < spawnParams->Config.MaxSpeed) { spawnParams->Config.Speed += ZOMBIE_SPEED_MUTATE * ZOMBIE_BASE_SPEED * State.Difficulty; break; } 
		case MOB_MUTATE_HEALTH: if (spawnParams->Config.MaxHealth <= 0 || spawnParams->Config.Health < spawnParams->Config.MaxHealth) { spawnParams->Config.Health += ZOMBIE_HEALTH_MUTATE * ZOMBIE_BASE_HEALTH * maxf(0.5, State.Difficulty); } break;
		case MOB_MUTATE_COST: spawnParams->Cost += rand(spawnParams->Config.MaxCostMutation); break;
		default: break;
	}
}

void mobDamage(Moby* source, Moby* target, float amount, int damageFlags)
{
	MobyColDamageIn in;

	vector_write(in.Momentum, 0);
	in.Damager = source;
	in.DamageFlags = damageFlags;
	in.DamageClass = 0;
	in.DamageStrength = 1;
	in.DamageIndex = source->OClass;
	in.Flags = 1;
	in.DamageHp = amount;

	//DPRINTF("%08X do %f damage to %08X\n", (u32)source, amount, (u32)target);

	mobyCollDamageDirect(target, &in);
}

void mobDoDamage(Moby* moby, float radius, float amount, int damageFlags, int friendlyFire)
{
	VECTOR p;
	MATRIX jointMtx;
	
	// get position of right hand joint
	mobyGetJointMatrix(moby, 2, jointMtx);
	vector_copy(p, &jointMtx[12]);

	// 
	if (CollMobysSphere_Fix(p, 2, moby, 0, radius) > 0) {
		Moby** hitMobies = CollMobysSphere_Fix_GetHitMobies();
		Moby* hitMoby;
		while ((hitMoby = *hitMobies++)) {
			if (friendlyFire || mobyIsHero(hitMoby))
				mobDamage(moby, hitMoby, amount, damageFlags);
		}
	}
}

void mobSetTarget(Moby* moby, Moby* target)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	if (pvars->MobVars.Target == target)
		return;

	// set target and dirty
	pvars->MobVars.Target = target;
	pvars->MobVars.ScoutCooldownTicks = 60;
	pvars->MobVars.Dirty = 1;
}

void mobSetAction(Moby* moby, int action)
{
	struct MobActionUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// don't set if already action
	if (pvars->MobVars.Action == action)
		return;

	GuberEvent* event = mobCreateEvent(moby, MOB_EVENT_STATE_UPDATE);
	if (event) {
		args.Action = action;
		args.ActionId = ++pvars->MobVars.ActionId;
		guberEventWrite(event, &args, sizeof(struct MobActionUpdateEventArgs));
	}
}

void mobForceLocalAction(Moby* moby, int action)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// from
	switch (pvars->MobVars.Action)
	{
		case MOB_EVENT_SPAWN:
		{
			// enable collision
			moby->CollActive = 0;
			break;
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
		case MOB_ACTION_ATTACK:
		{
			pvars->MobVars.AttackCooldownTicks = pvars->MobVars.Config.AttackCooldownTickCount;

			switch (pvars->MobVars.Config.MobType)
			{
				case MOB_EXPLODE:
				{
					u32 damageFlags = 0x00008801;
					u32 color = 0x003064FF;

					// special mutation settings
					switch (pvars->MobVars.Config.MobSpecialMutation)
					{
						case MOB_SPECIAL_MUTATION_FREEZE:
						{
							color = 0x00FF6430;
							damageFlags |= 0x00800000;
							break;
						}
						case MOB_SPECIAL_MUTATION_ACID:
						{
							color = 0x0064FF30;
							damageFlags |= 0x00000080;
							break;
						}
					}
			
					spawnExplosion(moby->Position, pvars->MobVars.Config.HitRadius, color);
					mobDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 1);
					pvars->MobVars.Destroy = 1;
					pvars->MobVars.LastHitBy = -1;
					break;
				}
			}
			break;
		}
		case MOB_ACTION_TIME_BOMB:
		{
			pvars->MobVars.OpacityFlickerDirection = 4;
			pvars->MobVars.TimeBombTicks = ZOMBIE_TIMEBOMB_TICKS / clamp(State.Difficulty, 0.5, 2);
			break;
		}
		case MOB_ACTION_FLINCH:
		{
			mobPlayHitSound(moby);
			pvars->MobVars.FlinchCooldownTicks = ZOMBIE_FLINCH_COOLDOWN_TICKS;
			break;
		}
		default:
		{
			break;
		}
	}

	// 
	pvars->MobVars.Action = action;
	pvars->MobVars.ActionCooldownTicks = ZOMBIE_ACTION_COOLDOWN_TICKS;
	pvars->MobVars.MoveStepCooldownTicks = 0;
}

int mobIsAttacking(struct MobPVar* pvars) {
	return pvars->MobVars.Action == MOB_ACTION_TIME_BOMB || (pvars->MobVars.Action == MOB_ACTION_ATTACK && !pvars->MobVars.AnimationLooped);
}

int mobIsSpawning(struct MobPVar* pvars) {
	return pvars->MobVars.Action == MOB_ACTION_SPAWN && !pvars->MobVars.AnimationLooped;
}

int mobCanAttack(struct MobPVar* pvars) {
	return pvars->MobVars.AttackCooldownTicks == 0;
}

int mobHasVelocity(struct MobPVar* pvars) {
	VECTOR t;
	vectorProjectOnHorizontal(t, pvars->MobVars.MoveVars.Velocity);
	//float minSqrSpeed = pvars->MobVars.LastSpeed * pvars->MobVars.LastSpeed * 0.25;
	return vector_sqrmag(t) >= 0.0001;
				//&& vector_sqrmag(pvars->MoveVars.vel) >= minSqrSpeed;
}

int mobGetLostArmorBangle(short armorStart, short armorEnd)
{
	return (armorStart - armorEnd) & armorStart;
}

short mobGetArmor(struct MobPVar* pvars) {
	float t = pvars->MobVars.Health / pvars->MobVars.Config.MaxHealth;
	if (t < 0.3)
		return 0x0000;
	else if (t < 0.7)
		return pvars->MobVars.Config.Bangles & 0x1f; // remove torso bangle
	
	return pvars->MobVars.Config.Bangles;
}

Moby* mobGetNextTarget(Moby* moby)
{
#if FIXEDTARGET
  return FIXEDTARGETMOBY;
#endif
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Player ** players = playerGetAll();
	int i;
	VECTOR delta;
	Moby * currentTarget = pvars->MobVars.Target;
	Player * closestPlayer = NULL;
	float closestPlayerDist = 100000;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = *players;
		if (p && !playerIsDead(p) && p->Health > 0) {
			vector_subtract(delta, p->PlayerPosition, moby->Position);
			float distSqr = vector_sqrmag(delta);

			if (distSqr < 300000) {
				// favor existing target
				if (p->SkinMoby == currentTarget && distSqr > (5*5))
					distSqr = (5*5);
				
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

int mobGetPreferredAction(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;

	// no preferred action
	if (mobIsAttacking(pvars))
		return -1;

	if (mobIsSpawning(pvars))
		return -1;

	if (pvars->MobVars.Action == MOB_ACTION_JUMP && !pvars->MobVars.MoveVars.Grounded)
		return -1;

	// prevent action changing too quickly
	if (pvars->MobVars.ActionCooldownTicks)
		return -1;

  // jump if we've hit a slope and are grounded
  if (pvars->MobVars.MoveVars.Grounded && pvars->MobVars.MoveVars.WallSlope > ZOMBIE_MAX_WALKABLE_SLOPE) {
    return MOB_ACTION_JUMP;
  }

	// get next target
	Moby * target = mobGetNextTarget(moby);
	if (target) {
		vector_copy(t, target->Position);
		vector_subtract(t, t, moby->Position);
		float distSqr = vector_sqrmag(t);
		float attackRadiusSqr = pvars->MobVars.Config.AttackRadius * pvars->MobVars.Config.AttackRadius;

		if (distSqr <= attackRadiusSqr) {
			if (mobCanAttack(pvars))
				return pvars->MobVars.Config.MobType != MOB_EXPLODE ? MOB_ACTION_ATTACK : MOB_ACTION_TIME_BOMB;
			
			return MOB_ACTION_WALK;
		} else {
			return MOB_ACTION_WALK;
		}
	}
	
	return MOB_ACTION_IDLE;
}

int colHotspot(int colId)
{
	if (mobInMobMove && (colId == 0xf || colId == 0x8))
		return -1;
	else if (colId == 0xf)
		return -1;

	return colId;
}

void mobAlterTarget(VECTOR out, Moby* moby, VECTOR forward, float amount)
{
	VECTOR up = {0,0,1,0};
	
	vector_outerproduct(out, forward, up);
	vector_normalize(out, out);
	vector_scale(out, out, amount);
}

void vector_rotateTowardsHorizontal(VECTOR output, VECTOR input0, VECTOR input1, float radians)
{
  VECTOR nInput0, nInput1;
  MATRIX mRot;

  vectorProjectOnHorizontal(nInput0, input0);
  vectorProjectOnHorizontal(nInput1, input1);
  vector_normalize(nInput0, nInput0);
  vector_normalize(nInput1, nInput1);

  float angleBetween = atan2f(0, vector_innerproduct(nInput0, nInput1));
  float rotateAmount = clamp(angleBetween, -radians, radians);
  
  if (rotateAmount < 0.01) {
    vector_copy(output, nInput1);
  } else {
    matrix_unit(mRot);
    matrix_rotate_z(mRot, mRot, rotateAmount);
    vector_apply(output, nInput0, mRot);
  }
}

void mobMoveSimple2(Moby* moby, struct MobPVar* pvars)
{
	// apply velocity
	VECTOR oldPos, newPos;
	VECTOR preOff = {0,0,0.5,0};
	VECTOR hitOff = {0,0,0.01,0};
	VECTOR gravity = {0,0,-GRAVITY_MAGNITUDE * MATH_DT,0};
  VECTOR delta, reflectedDelta;
  VECTOR horizontalDelta;
  VECTOR hitNormal, hitDelta;
  VECTOR newPosEx;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return;

	vector_add(oldPos, moby->Position, preOff);
	vector_add(newPos, moby->Position, pvars->MobVars.MoveVars.Velocity);
  vector_subtract(delta, newPos, oldPos);
  vectorProjectOnHorizontal(newPosEx, delta);
  vector_scale(newPosEx, newPosEx, pvars->MobVars.Config.CollRadius);
  newPosEx[2] = delta[2];
  vector_add(newPosEx, newPos, newPosEx);
  vectorProjectOnHorizontal(horizontalDelta, delta);

  pvars->MobVars.MoveVars.Grounded = 0;
	if (CollLine_Fix(oldPos, newPosEx, 2, moby, 0)) {
    
    vector_normalize(hitNormal, CollLine_Fix_GetHitNormal());
    vector_outerproduct(hitDelta, horizontalDelta, hitNormal);
    pvars->MobVars.MoveVars.WallSlope = atan2f(hitDelta[2], vector_innerproduct(horizontalDelta, hitNormal)) - MATH_PI/2;
    pvars->MobVars.MoveVars.HitWall = 1;
    DPRINTF("slope:%f\n", pvars->MobVars.MoveVars.WallSlope * MATH_RAD2DEG);

    if (pvars->MobVars.MoveVars.WallSlope > ZOMBIE_MAX_WALKABLE_SLOPE) {
      
      vector_subtract(hitDelta, CollLine_Fix_GetHitPosition(), moby->Position);
      float dist = vector_length(hitDelta);

      vector_normalize(hitNormal, CollLine_Fix_GetHitNormal());
      vector_reflect(reflectedDelta, delta, hitNormal);

      vector_normalize(reflectedDelta, reflectedDelta);
      vector_scale(reflectedDelta, reflectedDelta, dist);

      //vector_add(moby->Position, newPos, reflectedDelta);
      //moby->Position[2] = CollLine_Fix_GetHitPosition()[2] + 0.01;

      if (pvars->MobVars.MoveVars.CorrectionTicks == 0) {
        //vector_normalize(reflectedDelta, reflectedDelta);
        //vector_scale(pvars->MobVars.MoveVars.Velocity, reflectedDelta, vector_length(pvars->MobVars.MoveVars.Velocity));
        vectorProjectOnHorizontal(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity);
        pvars->MobVars.MoveVars.CorrectionTicks = 0;
      }
    } else {
      
		  vector_copy(moby->Position, CollLine_Fix_GetHitPosition());
      moby->Position[2] += 0.01;
      vectorProjectOnHorizontal(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity);
      pvars->MobVars.MoveVars.CorrectionTicks = 0;
    }

    pvars->MobVars.MoveVars.Grounded = 1;
	} else {
		vector_copy(moby->Position, newPos);
    pvars->MobVars.MoveVars.CorrectionTicks = 0;
		//vector_add(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity, gravity);
	}

	//vector_scale(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity, 0.95);
}

void mobMoveSimple(Moby* moby, struct MobPVar* pvars)
{
	// apply velocity
	VECTOR oldPos, newPos;
	VECTOR preOff = {0,0,0.5,0};
	VECTOR hitOff = {0,0,0.01,0};
	VECTOR gravity = {0,0,-GRAVITY_MAGNITUDE * MATH_DT,0};
  VECTOR delta, reflectedDelta;
  VECTOR horizontalDelta;
  VECTOR hitNormal, hitDelta;
  VECTOR newPosEx;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return;

	vector_add(oldPos, moby->Position, preOff);
	vector_add(newPos, moby->Position, pvars->MobVars.MoveVars.Velocity);
  vector_subtract(delta, newPos, oldPos);
  vectorProjectOnHorizontal(horizontalDelta, delta);
  vector_scale(newPosEx, horizontalDelta, 1 + pvars->MobVars.Config.CollRadius);
  newPosEx[2] = delta[2];
  vector_add(newPosEx, newPos, newPosEx);

	if (CollLine_Fix(oldPos, newPosEx, 2, moby, 0)) {
    
    vector_normalize(hitNormal, CollLine_Fix_GetHitNormal());
    pvars->MobVars.MoveVars.WallSlope = getSignedSlope(horizontalDelta, hitNormal);
    pvars->MobVars.MoveVars.HitWall = 1;


    // 
    vector_copy(newPos, CollLine_Fix_GetHitPosition());
    newPos[2] += 0.01;
    if (CollLine_Fix(oldPos, newPos, 2, moby, 0)) {
      vector_copy(newPos, CollLine_Fix_GetHitPosition());
      newPos[2] += 0.01;
    }
    
		vector_copy(moby->Position, newPos);
    vectorProjectOnHorizontal(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity);
    pvars->MobVars.MoveVars.CorrectionTicks = 0;

    pvars->MobVars.MoveVars.Grounded = 1;
	} else {
		vector_copy(moby->Position, newPos);
    pvars->MobVars.MoveVars.CorrectionTicks = 0;
	}
}

int mobMoveCheck(Moby* moby, VECTOR outputPos, VECTOR from, VECTOR to)
{
  VECTOR delta, horizontalDelta, reflectedDelta;
  VECTOR hitTo, hitFrom;
  VECTOR hitToEx, hitNormal;
  VECTOR up = {0,0,0,0};
  float angle;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return 0;

  up[2] = pvars->MobVars.Config.CollRadius;

  // offset hit scan to center of mob
  //vector_add(hitFrom, from, up);
  //vector_add(hitTo, to, up);
  
  // get horizontal delta between to and from
  vector_subtract(delta, to, from);
  vectorProjectOnHorizontal(horizontalDelta, delta);

  // offset hit scan to center of mob
  vector_add(hitFrom, from, up);
  vector_add(hitTo, hitFrom, horizontalDelta);
  
  vector_normalize(horizontalDelta, horizontalDelta);

  // move to further out to factor in the radius of the mob
  vector_normalize(hitToEx, delta);
  vector_scale(hitToEx, hitToEx, pvars->MobVars.Config.CollRadius);
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

	u16 moveStepCooldownTicks = decTimerU8(&pvars->MobVars.MoveStepCooldownTicks);
  u8 stuckCheckTicks = decTimerU8(&pvars->MobVars.MoveVars.StuckCheckTicks);
  u8 ungroundedTicks = decTimerU8(&pvars->MobVars.MoveVars.UngroundedTicks);
  u8 correctionTicks = decTimerU8(&pvars->MobVars.MoveVars.CorrectionTicks);

  // indicate we're in the process of a mob move
  mobInMobMove = 1;

  // save last position
  vector_copy(pvars->MobVars.MoveVars.LastPosition, moby->Position);

  // force correct
  if (correctionTicks)
    vector_copy(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.CorrectionVelocity);

  // set rotation to planar velocity
  if (mobHasVelocity(pvars))
    moby->Rotation[2] = atan2f(pvars->MobVars.MoveVars.Velocity[1], pvars->MobVars.MoveVars.Velocity[0]);

  // reset state
  pvars->MobVars.MoveVars.Grounded = 0;
  pvars->MobVars.MoveVars.WallSlope = 0;
  pvars->MobVars.MoveVars.HitWall = 0;

  if (mobFirstInList == moby) {

    // turn on holos so we can collide with them
    // optimization to only turn on for first mob in moby list (this is expensive)
    weaponTurnOnHoloshields(-1);

    // disable colliding with gates
    for (i = 0; i < GATE_MAX_COUNT; ++i) {
      if (State.GateMobies[i]) {
        State.GateMobies[i]->CollActive = -1;
      }
    }
  }

  if (0) {
    pvars->MobVars.MoveVars.Grounded = 1;
  }
  else if (moveStepCooldownTicks) {
    mobMoveSimple(moby, pvars);
  } else {

    // compute next position
    vector_add(nextPos, moby->Position, pvars->MobVars.MoveVars.Velocity);

    // move physics check twice to prevent clipping walls
    if (mobMoveCheck(moby, nextPos, moby->Position, nextPos))
      mobMoveCheck(moby, nextPos, moby->Position, nextPos);

    // check ground or ceiling
    isMovingDown = vector_innerproduct(pvars->MobVars.MoveVars.Velocity, up) < 0;
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
        vectorProjectOnHorizontal(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity);
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
    vector_copy(moby->Position, nextPos);
    pvars->MobVars.MoveStepCooldownTicks = pvars->MobVars.MoveStep;
  }

  // add gravity to velocity with clamp on downwards speed
  pvars->MobVars.MoveVars.Velocity[2] -= GRAVITY_MAGNITUDE * MATH_DT;
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

  if (correctionTicks)
    vector_copy(pvars->MobVars.MoveVars.CorrectionVelocity, pvars->MobVars.MoveVars.Velocity);
  
  // add horizontal delta position to sum
  vector_subtract(temp, moby->Position, pvars->MobVars.MoveVars.LastPosition);
  vectorProjectOnHorizontal(temp, temp);
  vector_add(pvars->MobVars.MoveVars.SumPositionDelta, pvars->MobVars.MoveVars.SumPositionDelta, temp);
  pvars->MobVars.MoveVars.SumSpeedOver += vector_sqrmag(temp);

  // done moving
  mobInMobMove = 0;

  // re-enable colliding with gates
  if (mobLastInList == moby) {

    // turn off holos for everyone else
    // optimization to only turn off for last mob in moby list (this is expensive)
    weaponTurnOffHoloshields();

    for (i = 0; i < GATE_MAX_COUNT; ++i) {
      if (State.GateMobies[i] && State.GateMobies[i]->State == 1) {
        State.GateMobies[i]->CollActive = 0;
      }
    }
  }
}

void mobGetVelocityToTarget(Moby* moby, VECTOR velocity, VECTOR from, VECTOR to, float speed)
{
  VECTOR normalizedTargetDirection, planarTargetDirection;
  VECTOR targetVelocity;
  VECTOR temp;
  float targetSpeed = speed * MATH_DT;

	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return;

  float collRadius = pvars->MobVars.Config.CollRadius;
  
  // stop when at target
  if (pvars->MobVars.Target) {
    vector_subtract(temp, pvars->MobVars.Target->Position, from);
    float sqrDistance = vector_sqrmag(temp);
    if (sqrDistance < ((collRadius+PLAYER_COLL_RADIUS)*(collRadius+PLAYER_COLL_RADIUS))) {
      targetSpeed = 0;
    }
  }

  // give starting velocity
  if (targetSpeed > 0 && !mobHasVelocity(pvars)) {
    vector_fromyaw(velocity, moby->Rotation[2]);
    vector_scale(velocity, velocity, targetSpeed * 0.01);
  }

  // get velocity
  vector_subtract(normalizedTargetDirection, to, from);
  vector_normalize(normalizedTargetDirection, normalizedTargetDirection);

  // rotate current velocity towards target
  vector_rotateTowardsHorizontal(targetVelocity, velocity, normalizedTargetDirection, ZOMBIE_TURN_RADIANS_PER_SEC * pvars->MobVars.Config.Speed * MATH_DT);
  //vectorProjectOnHorizontal(targetVelocity, normalizedTargetDirection);

  // acclerate velocity towards target velocity
  //vector_normalize(targetVelocity, targetVelocity);
  vector_scale(targetVelocity, targetVelocity, targetSpeed);

  vector_subtract(temp, targetVelocity, velocity);
  vectorProjectOnHorizontal(temp, temp);
  vector_scale(temp, temp, ZOMBIE_MOVE_ACCELERATION * MATH_DT);
  vector_add(velocity, velocity, temp);
}

void mobStand(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
  if (!pvars)
    return;

  // remove horizontal velocity
  vectorProjectOnVertical(pvars->MobVars.MoveVars.Velocity, pvars->MobVars.MoveVars.Velocity);
}

void mobDoAction(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Moby* target = pvars->MobVars.Target;
	VECTOR t, t2;
  int i;

	switch (pvars->MobVars.Action)
	{
		case MOB_ACTION_SPAWN:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_CRAWL_OUT_OF_GROUND);
      mobStand(moby);
			break;
		}
		case MOB_ACTION_FLINCH:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_BIG_FLINCH);
      mobStand(moby);

			if (pvars->MobVars.Knockback.Ticks > 0) {
				float power = PLAYER_KNOCKBACK_BASE_POWER * pvars->MobVars.Knockback.Power * pvars->MobVars.MoveStep;
				vector_fromyaw(t, pvars->MobVars.Knockback.Angle / 1000.0);
				t[2] = 1.0;
				vector_scale(t, t, power);
				vector_copy(pvars->MobVars.MoveVars.Velocity, t);
			}
			break;
		}
		case MOB_ACTION_IDLE:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_IDLE);
      mobStand(moby);
			break;
		}
		case MOB_ACTION_JUMP:
			{
        // move
        if (target) {
          mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, target->Position, pvars->MobVars.Config.Speed);
        } else {
          mobStand(moby);
        }

        // handle jumping
        if (pvars->MobVars.MoveVars.WallSlope > ZOMBIE_MAX_WALKABLE_SLOPE && pvars->MobVars.MoveVars.Grounded) {
			    mobTransAnim(moby, ZOMBIE_ANIM_JUMP);

          // check if we're near last jump pos
          // if so increment StuckJumpCount
          if (pvars->MobVars.MoveVars.IsStuck) {
            if (pvars->MobVars.MoveVars.StuckJumpCount < 255)
              pvars->MobVars.MoveVars.StuckJumpCount++;
          }

          // use delta height between target as base of jump speed
          // with min speed
          float jumpSpeed = 4;
          if (target) {
            jumpSpeed = clamp((target->Position[2] - moby->Position[2]) * fabsf(pvars->MobVars.MoveVars.WallSlope) * 2, 3, 15);
          }

          pvars->MobVars.MoveVars.Velocity[2] = jumpSpeed * MATH_DT;
          pvars->MobVars.MoveVars.Grounded = 0;
        }
				break;
			}
		case MOB_ACTION_LOOK_AT_TARGET:
    {
      mobStand(moby);
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
				mobAlterTarget(t2, moby, t, clamp(dist, 0, 10) * 0.3 * dir);
				vector_add(t, t, t2);
				vector_scale(t, t, 1 / dist);
				vector_add(t, moby->Position, t);

        mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, t, pvars->MobVars.Config.Speed);
			} else {
        // stand
        mobStand(moby);
			}

			// 
			if (mobHasVelocity(pvars))
				mobTransAnim(moby, ZOMBIE_ANIM_RUN);
			else
				mobTransAnim(moby, ZOMBIE_ANIM_IDLE);
			break;
		}
		case MOB_ACTION_TIME_BOMB:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_CROUCH);

			if (pvars->MobVars.TimeBombTicks == 0) {
				moby->Opacity = 0x80;
				pvars->MobVars.OpacityFlickerDirection = 0;
				mobSetAction(moby, MOB_ACTION_ATTACK);
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
		case MOB_ACTION_ATTACK:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_SLAP);

			int swingAttackReady = moby->AnimSeqId == ZOMBIE_ANIM_SLAP && moby->AnimSeqT >= 11 && moby->AnimSeqT < 14;
			u32 damageFlags = 0x00081801;
			float speedMult = (moby->AnimSeqId == ZOMBIE_ANIM_SLAP && moby->AnimSeqT < 5) ? (State.Difficulty * 2) : 1; //powf(clamp(10 - moby->AnimSeqT, 1, 10), 0.5);
			if (speedMult < 1)
				speedMult = 1;

			if (target) {
        mobGetVelocityToTarget(moby, pvars->MobVars.MoveVars.Velocity, moby->Position, target->Position, speedMult * pvars->MobVars.Config.Speed);
			} else {
				// stand
				mobStand(moby);
			}

			switch (pvars->MobVars.Config.MobType)
			{
				case MOB_FREEZE:
				{
					damageFlags = 0x00881801;
					break;
				}
				case MOB_ACID:
				{
					damageFlags = 0x00081881;
					break;
				}
				case MOB_EXPLODE:
				{
					// explode is handled on action (once)
					damageFlags = 0;
					break;
				}
			}

			// special mutation settings
			switch (pvars->MobVars.Config.MobSpecialMutation)
			{
				case MOB_SPECIAL_MUTATION_FREEZE:
				{
					damageFlags |= 0x00800000;
					break;
				}
				case MOB_SPECIAL_MUTATION_ACID:
				{
					damageFlags |= 0x00000080;
					break;
				}
			}
			
			if (swingAttackReady && damageFlags) {
				mobDoDamage(moby, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, damageFlags, 0);
			}
			break;
		}
	}
}

//--------------------------------------------------------------------------
void mobPostDrawQuad(Moby* moby)
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

	u32 color = MobLODColors[pvars->MobVars.Config.MobType] | (moby->Opacity << 24);

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
	quad.Tex0 = gfxGetFrameTex(127);
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

void mobHandleDraw(Moby* moby)
{
	int i;
	VECTOR t;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// if we aren't in the sorted list, try and find an empty spot
	if (pvars->MobVars.Order < 0 && AllMobsSortedFreeSpots > 0) {
		for (i = 0; i < MAX_MOBS_SPAWNED; ++i) {
			Moby* m = AllMobsSorted[i];
			if (m == NULL) {
				AllMobsSorted[i] = moby;
				pvars->MobVars.Order = i;
				--AllMobsSortedFreeSpots;
				//DPRINTF("set %08X to order %d (free %d)\n", (u32)moby, i, AllMobsSortedFreeSpots);
				break;
			}
		}
	}

	int order = pvars->MobVars.Order;
	moby->DrawDist = 128;
	if (order >= 0) {
		float rank = 1 - clamp(order / (float)State.RoundMaxSpawnedAtOnce, 0, 1);
		float rankCurve = powf(rank, 6);
		
		pvars->MobVars.MoveStep = 1 + (u8)((MoveStepMaxByClientCount[State.ActivePlayerCount-1]-1)*(1-rankCurve));
		if (State.RoundMobCount > 20) {
			if (rank < 0.75) {
				gfxRegisterDrawFunction((void**)0x0022251C, &mobPostDrawQuad, moby);
				moby->DrawDist = 0;
			}
		}
	}
	else {
		pvars->MobVars.MoveStep = 1;
	}

	/*
	if (pvars->MobVars.Order > 10) {
		if (State.RoundMobCount >= 70)
			pvars->MobVars.MoveStep = 8;
		else if (State.RoundMobCount >= 60)
			pvars->MobVars.MoveStep = 6;
		else if (State.RoundMobCount >= 45)
			pvars->MobVars.MoveStep = 4;
		else if (State.RoundMobCount >= 30)
			pvars->MobVars.MoveStep = 2;
		else
			pvars->MobVars.MoveStep = 1;
	} else {
		pvars->MobVars.MoveStep = 1;
	}
	*/
	

	//pvars->MoveVars.runSpeed = 0.20 * pvars->MobVars.MoveStep;
	//pvars->MoveVars.flySpeed = 0.20 * pvars->MobVars.MoveStep;
	//pvars->MoveVars.angularAccel = 0.005 * pvars->MobVars.MoveStep;
	//pvars->MoveVars.angularDecel = 0.005 * pvars->MobVars.MoveStep;
	//pvars->MoveVars.angularLimit = 0.20 * pvars->MobVars.MoveStep;
	//pvars->MoveVars.gravity = 20 * pvars->MobVars.MoveStep;
}

void mobUpdate(Moby* moby)
{
	int i;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	GameOptions* gameOptions = gameGetOptions();
	if (!pvars || pvars->MobVars.Destroyed)
		return;
	int isOwner;

	// handle radar
	if (pvars->MobVars.Config.MobType != MOB_GHOST && pvars->MobVars.Config.MobSpecialMutation != MOB_SPECIAL_MUTATION_GHOST && gameOptions->GameFlags.MultiplayerGameFlags.RadarBlips > 0)
	{
		if (gameOptions->GameFlags.MultiplayerGameFlags.RadarBlips == 1 || pvars->MobVars.ClosestDist < (20 * 20))
		{
			int blipId = radarGetBlipIndex(moby);
			if (blipId >= 0)
			{
				RadarBlip * blip = radarGetBlips() + blipId;
				blip->X = moby->Position[0];
				blip->Y = moby->Position[1];
				blip->Life = 0x1F;
				blip->Type = 4;
				blip->Team = 1;
			}
		}
	}

	// keep track of number of mobs drawn on screen to try and reduce the lag
	int gameTime = gameGetTime();
	if (gameTime != State.MobsDrawGameTime) {
		State.MobsDrawGameTime = gameTime;
		State.MobsDrawnLast = State.MobsDrawnCurrent;
		State.MobsDrawnCurrent = 0;
	}
	if (moby->Drawn)
		State.MobsDrawnCurrent++;

	// dec timers
	u16 nextCheckActionDelayTicks = decTimerU16(&pvars->MobVars.NextCheckActionDelayTicks);
	u16 nextActionTicks = decTimerU16(&pvars->MobVars.NextActionDelayTicks);
	decTimerU16(&pvars->MobVars.ActionCooldownTicks);
	decTimerU16(&pvars->MobVars.AttackCooldownTicks);
	u16 scoutCooldownTicks = decTimerU16(&pvars->MobVars.ScoutCooldownTicks);
	decTimerU16(&pvars->MobVars.FlinchCooldownTicks);
	decTimerU16(&pvars->MobVars.TimeBombTicks);
	u16 autoDirtyCooldownTicks = decTimerU16(&pvars->MobVars.AutoDirtyCooldownTicks);
	u16 ambientSoundCooldownTicks = decTimerU16(&pvars->MobVars.AmbientSoundCooldownTicks);
	decTimerU8(&pvars->MobVars.Knockback.Ticks);
  
	// validate owner
	Player * ownerPlayer = playerGetAll()[(int)pvars->MobVars.Owner];
	if (!ownerPlayer || !ownerPlayer->PlayerMoby) {
		pvars->MobVars.Owner = gameGetHostId(); // default as host
		//mobSendOwnerUpdate(moby, pvars->MobVars.Owner);
	}
	
	// determine if I'm the owner
	isOwner = mobAmIOwner(moby);

	// change owner to target
	if (isOwner && pvars->MobVars.Target) {
		Player * targetPlayer = (Player*)guberGetObjectByMoby(pvars->MobVars.Target);
		if (targetPlayer && targetPlayer->Guber.Id.GID.HostId != pvars->MobVars.Owner) {
			mobSendOwnerUpdate(moby, targetPlayer->Guber.Id.GID.HostId);
		}
	}

	// 
	if (!State.Freeze && ambientSoundCooldownTicks == 0) {
		mobPlayAmbientSound(moby);
		pvars->MobVars.AmbientSoundCooldownTicks = randRangeInt(ZOMBIE_AMBSND_MIN_COOLDOWN_TICKS, ZOMBIE_AMBSND_MAX_COOLDOWN_TICKS);
	}

	// 
	mobHandleDraw(moby);

	// 
	if (!State.Freeze)
		mobDoAction(moby);

	// 
	if (pvars->FlashVars.type)
		mobyUpdateFlash(moby, 0);

	// count ticks since last grounded
	if (pvars->MobVars.MoveVars.Grounded)
		pvars->MobVars.TimeLastGroundedTicks = 0;
	else
		pvars->MobVars.TimeLastGroundedTicks++;

	// auto destruct after 15 seconds of falling
	if (pvars->MobVars.TimeLastGroundedTicks > (TPS * 15)) {
		pvars->MobVars.Respawn = 1;
	}

  // move
  if (!State.Freeze)
    mobMove(moby);

	//
	if (isOwner && !State.Freeze) {
		// set next state
		if (pvars->MobVars.NextAction >= 0 && pvars->MobVars.ActionCooldownTicks == 0) {
			if (nextActionTicks == 0) {
				mobSetAction(moby, pvars->MobVars.NextAction);
				pvars->MobVars.NextAction = -1;
			} else {
				//mobSetAction(moby, MOB_ACTION_LOOK_AT_TARGET);
			}
		}
		
		// 
		if (nextCheckActionDelayTicks == 0) {
			int nextAction = mobGetPreferredAction(moby);
			if (nextAction >= 0 && nextAction != pvars->MobVars.NextAction && nextAction != pvars->MobVars.Action && nextActionTicks == 0) {
				pvars->MobVars.NextAction = nextAction;
				pvars->MobVars.NextActionDelayTicks = nextAction >= MOB_ACTION_ATTACK ? pvars->MobVars.Config.ReactionTickCount : 0;
			} 

			// get new target
			if (scoutCooldownTicks == 0) {
				mobSetTarget(moby, mobGetNextTarget(moby));
			}

			pvars->MobVars.NextCheckActionDelayTicks = 2;
		}
	}

	// 
	if (pvars->MobVars.Config.MobType == MOB_GHOST || pvars->MobVars.Config.MobSpecialMutation == MOB_SPECIAL_MUTATION_GHOST) {
		u8 defaultOpacity = pvars->MobVars.Config.MobSpecialMutation ? 0xFF : 0x80;
		u8 targetOpacity = pvars->MobVars.Action > MOB_ACTION_WALK ? defaultOpacity : 0x10;
		u8 opacity = moby->Opacity;
		if (opacity < targetOpacity)
			opacity = targetOpacity;
		else if (opacity > targetOpacity)
			opacity--;
		moby->Opacity = opacity;
	}

	float animSpeed = 0.9 * (pvars->MobVars.Config.Speed / ZOMBIE_BASE_SPEED);
	if (State.Freeze || (moby->DrawDist == 0 && pvars->MobVars.Action == MOB_ACTION_WALK)) {
		moby->AnimSpeed = 0;
	} else {
		moby->AnimSpeed = animSpeed;
	}

	// update armor
	moby->Bangles = mobGetArmor(pvars);

	// process damage
  int damageIndex = moby->CollDamage;
  MobyColDamage* colDamage = NULL;
  float damage = 0.0;

  if (damageIndex >= 0) {
    colDamage = mobyGetDamage(moby, 0x400C00, 0);
    if (colDamage)
      damage = colDamage->DamageHp;
  }
  ((void (*)(Moby*, float*, MobyColDamage*))0x005184d0)(moby, &damage, colDamage);

  if (colDamage && damage > 0) {
    Player * damager = guberMobyGetPlayerDamager(colDamage->Damager);
    if (damager) {
      damage *= 1 + (0.05 * State.PlayerStates[damager->PlayerId].State.Upgrades[UPGRADE_DAMAGE]);
    }

    if (damager && (isOwner || playerIsLocal(damager))) {
      mobSendDamageEvent(moby, damager->PlayerMoby, colDamage->Damager, damage, colDamage->DamageFlags);	
    } else if (isOwner) {
      mobSendDamageEvent(moby, colDamage->Damager, colDamage->Damager, damage, colDamage->DamageFlags);	
    }

    // 
    moby->CollDamage = -1;
  }

	// handle death stuff
	if (isOwner) {

		// handle falling under map
		if (moby->Position[2] < gameGetDeathHeight())
			pvars->MobVars.Respawn = 1;

		// respawn
		if (pvars->MobVars.Respawn) {
			VECTOR p;
			struct MobConfig config;
			if (spawnGetRandomPoint(p)) {
				memcpy(&config, &pvars->MobVars.Config, sizeof(struct MobConfig));
				mobCreate(p, 0, guberGetUID(moby), &config);
			}	

			pvars->MobVars.Respawn = 0;
			pvars->MobVars.Destroyed = 2;
		}

		// destroy
		else if (pvars->MobVars.Destroy) {
			mobDestroy(moby, pvars->MobVars.LastHitBy);
		}

		// send changes
		else if (pvars->MobVars.Dirty || autoDirtyCooldownTicks == 0) {
			mobSendStateUpdate(moby);
			pvars->MobVars.Dirty = 0;
			pvars->MobVars.AutoDirtyCooldownTicks = ZOMBIE_AUTO_DIRTY_COOLDOWN_TICKS;
		}
	}
}

GuberEvent* mobCreateEvent(Moby* moby, u32 eventType)
{
	GuberEvent * event = NULL;

	// create guber object
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
		event = guberEventCreateEvent(guber, eventType, 0, 0);

	return event;
}

int mobHandleEvent_Spawn(Moby* moby, GuberEvent* event)
{
	
	VECTOR p;
	float yaw;
	int i;
	int spawnFromUID;
	char random;
	struct MobSpawnEventArgs args;

	// read event
	guberEventRead(event, p, 12);
	guberEventRead(event, &yaw, 4);
	guberEventRead(event, &spawnFromUID, 4);
	guberEventRead(event, &random, 1);
	guberEventRead(event, &args, sizeof(struct MobSpawnEventArgs));

	// set position and rotation
	vector_copy(moby->Position, p);
	moby->Rotation[2] = yaw;
	moby->Scale *= 0.7;
	//moby->AnimFlags

	// set update
	moby->PUpdate = &mobUpdate;

	// 
	moby->ModeBits |= 0x30;
	moby->GlowRGBA = MobSecondaryColors[(int)args.MobType];
	moby->PrimaryColor = MobPrimaryColors[(int)args.MobType];
	moby->Opacity = args.MobSpecialMutation ? 0xFF : 0x80;
	moby->CollActive = 1;


	// update pvars
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	memset(pvars, 0, sizeof(struct MobPVar));
	pvars->TargetVarsPtr = &pvars->TargetVars;
	pvars->MoveVarsPtr = NULL; // &pvars->MoveVars;
	pvars->FlashVarsPtr = &pvars->FlashVars;
	pvars->ReactVarsPtr = &pvars->ReactVars;
	
	// initialize mob vars
	pvars->MobVars.Config.MobType = args.MobType;
	pvars->MobVars.Config.MobSpecialMutation = args.MobSpecialMutation;
	pvars->MobVars.Config.Bolts = args.Bolts;
	pvars->MobVars.Config.Xp = args.Xp;
	pvars->MobVars.Config.MaxHealth = (float)args.StartHealth;
	pvars->MobVars.Config.Health = (float)args.StartHealth;
	pvars->MobVars.Config.Bangles = args.Bangles;
	pvars->MobVars.Config.Damage = (float)args.Damage;
	pvars->MobVars.Config.AttackRadius = (float)args.AttackRadiusEighths / 8.0;
	pvars->MobVars.Config.HitRadius = (float)args.HitRadiusEighths / 8.0;
	pvars->MobVars.Config.CollRadius = (float)args.CollRadiusEighths / 8.0;
	pvars->MobVars.Config.Speed = (float)args.SpeedEighths / 8.0;
	pvars->MobVars.Config.ReactionTickCount = args.ReactionTickCount;
	pvars->MobVars.Config.AttackCooldownTickCount = args.AttackCooldownTickCount;
	pvars->MobVars.Health = pvars->MobVars.Config.MaxHealth;
	pvars->MobVars.Order = -1;
	pvars->MobVars.TimeLastGroundedTicks = 0;
	pvars->MobVars.Random = random;
#if MOB_NO_MOVE
	pvars->MobVars.Config.Speed = 0;
#endif
#if MOB_NO_DAMAGE
	pvars->MobVars.Config.Damage = 0;
#endif
	//pvars->MobVars.Config.Health = pvars->MobVars.Health = 1;

	// initialize target vars
	pvars->TargetVars.hitPoints = pvars->MobVars.Health;
	pvars->TargetVars.team = 10;
	pvars->TargetVars.targetHeight = 1;

	// 
	Guber* guber = guberGetObjectByMoby(moby);
	if (guber)
	{
		((GuberMoby*)guber)->TeamNum = 10;
	}

	// initialize move vars
	//mobyMoveSystemInit(moby);
	//mobySetMoveDistanceThresholds(moby, 0.5, 0.5, 1.0, 5.0);
	//mobySetMoveSpeedLimits(moby, 1, 1, 1/6.0, 1/6.0, 1/60.0, 1/60.0, 1/60.0);
	//mobySetMoveAngularSpeeds(moby, 0.00349066, 0.00349066, 0.20944);
	//pvars->MoveVars.groupCache = 0;
	//pvars->MoveVars.elv_state = 0;
	//pvars->MoveVars.slopeLimit = MATH_PI / 2;
	//pvars->MoveVars.maxStepUp = ZOMBIE_BASE_STEP_HEIGHT;
	//pvars->MoveVars.maxStepDown = ZOMBIE_BASE_STEP_HEIGHT;
	//pvars->MoveVars.flags |= 2;
	//pvars->MoveVars.boundArea = -1;
	//pvars->MoveVars.groundHotspot = -1;
	//vector_copy(pvars->MoveVars.passThruPoint, p);
	mobySetAnimCache(moby, (void*)0x36f980, 0);
	moby->ModeBits &= ~0x100;

	// initialize react vars
	pvars->ReactVars.acidDamage = 1.0;
	pvars->ReactVars.shieldDamageReduction = 1.0;

	// mob specifc settings
	switch (args.MobType)
	{
		case MOB_GHOST:
		{
			// disable targeting
			pvars->ReactVars.deathType = 0; // normal
			moby->ModeBits &= ~0x1000;
			break;
		}
		case MOB_ACID:
		{
			pvars->ReactVars.deathType = 4; // acid
			break;
		}
		case MOB_FREEZE:
		{
			pvars->ReactVars.deathType = 3; // blue explosion
			break;
		}
		case MOB_EXPLODE:
		{
			pvars->ReactVars.deathType = 2; // explosion
			break;
		}
		case MOB_TANK:
		{
			pvars->ReactVars.deathType = 0; // normal
			moby->Scale = 0.6;
			pvars->TargetVars.targetHeight = 2.5;
			break;
		}
	}

	// special mutation settings
	switch (pvars->MobVars.Config.MobSpecialMutation)
	{
		case MOB_SPECIAL_MUTATION_FREEZE:
		{
			moby->GlowRGBA = MobSpecialMutationColors[(int)pvars->MobVars.Config.MobSpecialMutation];
			break;
		}
		case MOB_SPECIAL_MUTATION_ACID:
		{
			moby->GlowRGBA = MobSpecialMutationColors[(int)pvars->MobVars.Config.MobSpecialMutation];
			break;
		}
		case MOB_SPECIAL_MUTATION_GHOST:
		{
			// disable targeting
			pvars->ReactVars.deathType = 0; // normal
			moby->ModeBits &= ~0x1000;
			break;
		}
	}

	// 
	mobySetState(moby, 0, -1);

	++State.RoundMobCount;

	// destroy spawn from
	if (spawnFromUID != -1) {
		GuberMoby* gm = (GuberMoby*)guberGetObjectByUID(spawnFromUID);
		if (gm && gm->Moby && gm->Moby->PVar && !mobyIsDestroyed(gm->Moby) && gm->Moby->OClass == ZOMBIE_MOBY_OCLASS) {
			struct MobPVar* spawnFromPVars = (struct MobPVar*)gm->Moby->PVar;
			if (spawnFromPVars->MobVars.Destroyed != 1) {
				guberMobyDestroy(gm->Moby);
				State.RoundMobCount--;
			}
		}
	}
	
#if LOG_STATS2
	DPRINTF("mob created event %08X, %08X, %08X mobtype:%d spawnedNum:%d roundTotal:%d)\n", (u32)moby, (u32)event, (u32)moby->GuberMoby, args.MobType, State.RoundMobCount, State.RoundMobSpawnedCount);
#endif
	return 0;
}

int mobHandleEvent_Destroy(Moby* moby, GuberEvent* event)
{
	char killedByPlayerId, weaponId;
	int i;
	Player** players = playerGetAll();
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars || pvars->MobVars.Destroyed)
		return 0;

	// 
	guberEventRead(event, &killedByPlayerId, sizeof(killedByPlayerId));
	guberEventRead(event, &weaponId, sizeof(weaponId));

	int bolts = pvars->MobVars.Config.Bolts;
	int xp = pvars->MobVars.Config.Xp;

#if DROPS
	if (killedByPlayerId >= 0 && gameAmIHost()) {
		Player * killedByPlayer = players[killedByPlayerId];
		if (killedByPlayer) {
			float randomValue = randRange(0.0, 1.0);
			if (randomValue < ZOMBIE_HAS_DROP_PROBABILITY) {
				dropCreate(moby->Position, randRangeInt(0, DROP_COUNT-1), gameGetTime() + DROP_DURATION, killedByPlayer->Team);
			}
		}
	}
#endif

#if SHARED_BOLTS
	if (killedByPlayerId >= 0) {
		Player * killedByPlayer = players[killedByPlayerId];
		if (killedByPlayer) {
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				Player* p = players[i];
				if (p && !playerIsDead(p) && p->Team == killedByPlayer->Team) {
					int multiplier = State.PlayerStates[i].IsDoublePoints ? 2 : 1;
					State.PlayerStates[i].State.Bolts += bolts * multiplier;
					State.PlayerStates[i].State.TotalBolts += bolts * multiplier;
				}
			}
		}
	}
#else
	if (killedByPlayerId >= 0) {
		int multiplier = State.PlayerStates[killedByPlayerId].IsDoublePoints ? 2 : 1;
		State.PlayerStates[(int)killedByPlayerId].State.Bolts += bolts * multiplier;
		State.PlayerStates[(int)killedByPlayerId].State.TotalBolts += bolts * multiplier;
	}
#endif

	if (killedByPlayerId >= 0) {
		Player * killedByPlayer = players[killedByPlayerId];
		struct SurvivalPlayer* pState = &State.PlayerStates[(int)killedByPlayerId];
		GameData * gameData = gameGetData();

		// give xp
		pState->State.XP += xp;
		u64 targetForNextToken = getXpForNextToken(pState->State.TotalTokens);

		// give tokens
		while (pState->State.XP >= targetForNextToken)
		{
			pState->State.XP -= targetForNextToken;
			pState->State.TotalTokens += 1;
			pState->State.CurrentTokens += 1;
			targetForNextToken = getXpForNextToken(pState->State.TotalTokens);

			if (killedByPlayer->IsLocal) {
				sendPlayerStats(killedByPlayerId);
			}
		} 

		// handle weapon jackpot
		if (weaponId > 1 && killedByPlayer) {
			int jackpotCount = playerGetWeaponAlphaModCount(killedByPlayer, weaponId, ALPHA_MOD_JACKPOT);

			pState->State.Bolts += jackpotCount * JACKPOT_BOLTS;
			pState->State.TotalBolts += jackpotCount * JACKPOT_BOLTS;
		}

		// handle stats
		pState->State.Kills++;
		gameData->PlayerStats.Kills[killedByPlayerId]++;
		int weaponSlotId = weaponIdToSlot(weaponId);
		if (weaponId > 0 && (weaponId != WEAPON_ID_WRENCH || weaponSlotId == WEAPON_SLOT_WRENCH))
			gameData->PlayerStats.WeaponKills[killedByPlayerId][weaponSlotId]++;
	}

	// set colors before death so that the corn has the correct color
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.Config.MobType];

	// limit corn spawning to prevent freezing/framelag
	if (State.RoundMobCount < 30) {
		mobSpawnCorn(moby, ZOMBIE_BANGLE_LARM | ZOMBIE_BANGLE_RARM | ZOMBIE_BANGLE_LLEG | ZOMBIE_BANGLE_RLEG | ZOMBIE_BANGLE_RFOOT | ZOMBIE_BANGLE_HIPS);
	}

	if (pvars->MobVars.Order >= 0) {
		AllMobsSorted[(int)pvars->MobVars.Order] = NULL;
		++AllMobsSortedFreeSpots;
	}
	guberMobyDestroy(moby);
	moby->ModeBits &= ~0x30;
	--State.RoundMobCount;
	pvars->MobVars.Destroyed = 1;

#if LOG_STATS2
	DPRINTF("mob destroy event %08X, %08X, by client %d, (%d)\n", (u32)moby, (u32)event, killedByPlayerId, State.RoundMobCount);
#endif
	return 0;
}

int mobHandleEvent_Damage(Moby* moby, GuberEvent* event)
{
	VECTOR t;
	struct MobDamageEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	int armorStart = mobGetArmor(pvars);
	if (!pvars)
		return 0;

	int canFlinch = pvars->MobVars.Action != MOB_ACTION_FLINCH && pvars->MobVars.Action != MOB_ACTION_TIME_BOMB && pvars->MobVars.FlinchCooldownTicks == 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobDamageEventArgs));

	// get damager
	GuberMoby* damager = (GuberMoby*)guberGetObjectByUID(args.SourceUID);

	// flash
	mobyStartFlash(moby, FT_HIT, 0x800000FF, 0);

	// decrement health
	float damage = args.DamageQuarters / 4.0;
	float newHp = pvars->MobVars.Health - damage;
	pvars->MobVars.Health = newHp;
	pvars->TargetVars.hitPoints = newHp;

	// drop armor bangle
	/*
	int armorNew = mobGetArmor(pvars);
	if (armorNew != armorStart) {
		int b = mobGetLostArmorBangle(armorStart, armorNew);
		mobSpawnCorn(moby, b);
	}
	*/

	// destroy
	if (newHp <= 0) {
		if (pvars->MobVars.Action == MOB_ACTION_TIME_BOMB && moby->AnimSeqId == ZOMBIE_ANIM_CROUCH && moby->AnimSeqT > 3) {
			// explode
			mobForceLocalAction(moby, MOB_ACTION_ATTACK);
		} else {
			//pvars->MoveVars.reaction_state = 5;
			pvars->MobVars.Destroy = 1;
			pvars->MobVars.LastHitBy = args.SourceUID;
			pvars->MobVars.LastHitByOClass = args.SourceOClass;
		}
	}

	// knockback
	if (args.Knockback.Power > 0 && canFlinch)
	{
		memcpy(&pvars->MobVars.Knockback, &args.Knockback, sizeof(struct Knockback));
	}

	if (mobAmIOwner(moby))
	{
		// flinch
		float damageRatio = damage / pvars->MobVars.Config.Health;
		if ((((args.Knockback.Power + 1) * damageRatio) > 0.25) && canFlinch)
			mobSetAction(moby, MOB_ACTION_FLINCH);

		// set target
		//if (damager)
		//	mobSetTarget(moby, damager->Moby);
	}

	return 0;
}

int mobHandleEvent_ActionUpdate(Moby* moby, GuberEvent* event)
{
	struct MobActionUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobActionUpdateEventArgs));

	// 
	if (pvars->MobVars.LastActionId != args.ActionId && pvars->MobVars.Action != args.Action) {
		pvars->MobVars.LastActionId = args.ActionId;
		mobForceLocalAction(moby, args.Action);
	}
	
	//DPRINTF("mob state update event %08X, %08X, %d\n", (u32)moby, (u32)event, args.Action);
	return 0;
}

int mobHandleEvent_StateUpdate(Moby* moby, GuberEvent* event)
{
	struct MobStateUpdateEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	VECTOR t;
	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobStateUpdateEventArgs));

	// teleport position if far away
	vector_subtract(t, moby->Position, args.Position);
	if (vector_sqrmag(t) > 25)
		vector_copy(moby->Position, args.Position);

	// 
	Player* target = (Player*)guberGetObjectByUID(args.TargetUID);
	if (target)
		pvars->MobVars.Target = target->SkinMoby;


	//DPRINTF("mob target update event %08X, %08X, %d:%08X, %08X\n", (u32)moby, (u32)event, args.TargetUID, (u32)target, (u32)pvars->MobVars.Target);
	return 0;
}

int mobHandleEvent_OwnerUpdate(Moby* moby, GuberEvent* event)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	char newOwner;
	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &newOwner, 1);

	// 
	pvars->MobVars.Owner = newOwner;

	if (gameGetMyClientId() == newOwner) {
		pvars->MobVars.NextAction = -1; // indicate we have no new action since we just became owner
	}
	
	//DPRINTF("mob owner update event %08X, %08X, %d\n", (u32)moby, (u32)event, newOwner);
	return 0;
}

int mobHandleEvent(Moby* moby, GuberEvent* event)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	if (isInGame() && moby->OClass == ZOMBIE_MOBY_OCLASS && pvars) {
		u32 mobEvent = event->NetEvent.EventID;
		int isFromHost = gameIsHost(event->NetEvent.OriginClientIdx);
		if (!isFromHost && mobEvent != MOB_EVENT_SPAWN && mobEvent != MOB_EVENT_DAMAGE && mobEvent != MOB_EVENT_DESTROY && pvars->MobVars.Owner != event->NetEvent.OriginClientIdx)
		{
			DPRINTF("ignoring mob event %d from %d (not owner, %d)\n", mobEvent, event->NetEvent.OriginClientIdx, pvars->MobVars.Owner);
			return 0;
		}

		switch (mobEvent)
		{
			case MOB_EVENT_SPAWN: return mobHandleEvent_Spawn(moby, event);
			case MOB_EVENT_DESTROY: return mobHandleEvent_Destroy(moby, event);
			case MOB_EVENT_DAMAGE: return mobHandleEvent_Damage(moby, event);
			case MOB_EVENT_STATE_UPDATE: return mobHandleEvent_ActionUpdate(moby, event);
			case MOB_EVENT_TARGET_UPDATE: return mobHandleEvent_StateUpdate(moby, event);
			case MOB_EVENT_OWNER_UPDATE: return mobHandleEvent_OwnerUpdate(moby, event);
			default:
			{
				DPRINTF("unhandle mob event %d\n", mobEvent);
				break;
			}
		}
	}

	return 0;
}

int mobCreate(VECTOR position, float yaw, int spawnFromUID, struct MobConfig *config)
{
	GameSettings* gs = gameGetSettings();
	struct MobSpawnEventArgs args;

	// create guber object
	GuberEvent * guberEvent = 0;
	guberMobyCreateSpawned(ZOMBIE_MOBY_OCLASS, sizeof(struct MobPVar), &guberEvent, NULL);
	if (guberEvent)
	{
		args.Bolts = (config->Bolts + randRangeInt(-50, 50)) * BOLT_TAX[(int)gs->PlayerCount];
		args.Xp = config->Xp;
		args.StartHealth = (u16)config->Health;
		args.Bangles = (u16)config->Bangles;
		args.MobType = (char)config->MobType;
		args.Damage = (u8)config->Damage;
		args.AttackRadiusEighths = (u8)(config->AttackRadius * 8);
		args.HitRadiusEighths = (u8)(config->HitRadius * 8);
		args.CollRadiusEighths = (u8)(config->CollRadius * 8);
		args.SpeedEighths = (u8)(config->Speed * 8);
		args.ReactionTickCount = (u8)config->ReactionTickCount;
		args.AttackCooldownTickCount = (u8)config->AttackCooldownTickCount;
		args.MobSpecialMutation = config->MobSpecialMutation;

		u8 random = (u8)rand(100);

    position[2] += 1; // spawn slightly above point
		guberEventWrite(guberEvent, position, 12);
		guberEventWrite(guberEvent, &yaw, 4);
		guberEventWrite(guberEvent, &spawnFromUID, 4);
		guberEventWrite(guberEvent, &random, 1);
		guberEventWrite(guberEvent, &args, sizeof(struct MobSpawnEventArgs));
	}
	else
	{
		DPRINTF("failed to guberevent mob\n");
	}
  
  return guberEvent != NULL;
}

void mobInitialize(void)
{
	// set vtable callbacks
	*(u32*)0x003A0A84 = (u32)&getGuber;
	*(u32*)0x003A0A94 = (u32)&handleEvent;

	// collision hit type
	*(u32*)0x004bd1f0 = 0x08000000 | ((u32)&colHotspot / 4);
	*(u32*)0x004bd1f4 = 0x00402021;

	// 
	memset(AllMobsSorted, 0, sizeof(AllMobsSorted));
}

void mobNuke(int killedByPlayerId)
{
	int i;
	Player** players = playerGetAll();
	u32 playerUid = 0;
	if (killedByPlayerId >= 0 && killedByPlayerId < GAME_MAX_PLAYERS) {
		Player* p = players[killedByPlayerId];
		if (p) {
			playerUid = p->Guber.Id.UID;
		}
	}

	for (i = 0; i < MAX_MOBS_SPAWNED; ++i) {
		Moby* m = AllMobsSorted[i];
		if (m) {
			mobDestroy(m, playerUid);
		}
	}
}

void mobTick(void)
{
	int i, j;
	VECTOR t;
	Player* locals[] = { playerGetFromSlot(0), playerGetFromSlot(1) };
	int localsCount = playerGetNumLocals();

	if (mobFirstInList && (mobyIsDestroyed(mobFirstInList) || mobFirstInList->OClass != ZOMBIE_MOBY_OCLASS))
		mobFirstInList = NULL;
	if (mobLastInList && (mobyIsDestroyed(mobLastInList) || mobLastInList->OClass != ZOMBIE_MOBY_OCLASS))
		mobLastInList = NULL;
	
	// run single pass on sort
	for (i = 0; i < MAX_MOBS_SPAWNED; ++i)
	{
		Moby* m = AllMobsSorted[i];

		if (m && (!mobFirstInList || m < mobFirstInList))
			mobFirstInList = m;
		if (m && (!mobLastInList || m > mobLastInList))
			mobLastInList = m;

		// remove invalid moby ref
		if (m && (mobyIsDestroyed(m) || !m->PVar || m->OClass != ZOMBIE_MOBY_OCLASS)) {
			AllMobsSorted[i] = NULL;
			m = NULL;
			++AllMobsSortedFreeSpots;
		}

		if (m) {
			struct MobPVar* pvars = (struct MobPVar*)m->PVar;

			// find closest dist to local
			pvars->MobVars.ClosestDist = 10000000;
			for (j = 0; j < localsCount; ++j) {
				Player * p = locals[j];
				vector_subtract(t, m->Position, p->CameraPos);
				float dist = vector_sqrmag(t);
				if (dist < pvars->MobVars.ClosestDist)
					pvars->MobVars.ClosestDist = dist;
			}

			// if closer than last, swap
			if (i > 0) {
				Moby* last = AllMobsSorted[i-1];
				int swap = 0;

				// swap if previous is empty
				// or if this is closer than previous
				if (!last) {
					swap = 1;
				} else {
					struct MobPVar* lPvars = (struct MobPVar*)last->PVar;
					if (lPvars && pvars->MobVars.ClosestDist < lPvars->MobVars.ClosestDist) {
						swap = 1;
						lPvars->MobVars.Order = i;
					}
				}
				
				// move current to previous
				if (swap) {
					AllMobsSorted[i] = AllMobsSorted[i-1];
					AllMobsSorted[i-1] = m;
					pvars->MobVars.Order = i-1;
				}
			}
		}
	}
}
