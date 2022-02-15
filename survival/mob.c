#include "include/mob.h"
#include "include/utils.h"
#include "include/game.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/random.h>

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
};

u32 MobSecondaryColors[] = {
	[MOB_NORMAL] 	0x80202020,
	[MOB_FREEZE] 	0x80FF2000,
	[MOB_ACID] 		0x8000FF00,
	[MOB_EXPLODE] 0x000040C0,
	[MOB_GHOST] 	0x80202020,
};

Moby* AllMobsSorted[MAX_MOBS_SPAWNED];
int AllMobsSortedFreeSpots = MAX_MOBS_SPAWNED;
void * mobCollData = NULL;

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
		mobyAnimTransition(moby, animId, lerpFrames, 32);

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
		case MOB_MUTATE_DAMAGE: if (spawnParams->Config.Damage < spawnParams->Config.MaxDamage) { spawnParams->Config.Damage += ZOMBIE_DAMAGE_MUTATE * ZOMBIE_BASE_DAMAGE; break; } 
		case MOB_MUTATE_SPEED: if (spawnParams->Config.Speed < spawnParams->Config.MaxSpeed) { spawnParams->Config.Speed += ZOMBIE_SPEED_MUTATE * ZOMBIE_BASE_SPEED; break; } 
		case MOB_MUTATE_HEALTH: if (spawnParams->Config.MaxHealth <= 0 || spawnParams->Config.Health < spawnParams->Config.MaxHealth) { spawnParams->Config.Health += ZOMBIE_HEALTH_MUTATE * ZOMBIE_BASE_HEALTH; } break;
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

	mobyCollDamageDirect(target, &in);
}

void mobDoDamage(Moby* moby, float offset, float radius, float amount, int damageFlags, int friendlyFire)
{
	VECTOR p;

	// determine hit center
	if (offset < 0)
		offset = 0;
	vector_fromyaw(p, moby->Rotation[2]);
	vector_scale(p, p, offset);
	p[2] = 0.75;
	vector_add(p, p, moby->Position);

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
			moby->CollData = mobCollData;
			break;
		}
	}

	// to
	switch (action)
	{
		case MOB_ACTION_SPAWN:
		{
			moby->CollData = NULL;
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
				case MOB_FREEZE:
				{
					mobDoDamage(moby, pvars->MobVars.Config.AttackRadius - pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, 0x00881801, 0);
					break;
				}
				case MOB_EXPLODE:
				{
					spawnExplosion(moby->Position, pvars->MobVars.Config.HitRadius);
					mobDoDamage(moby, 0, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, 0x00008801, 1);
					pvars->MobVars.Destroy = 1;
					pvars->MobVars.LastHitBy = -1;
					break;
				}
				case MOB_ACID:
				{
					mobDoDamage(moby, pvars->MobVars.Config.AttackRadius - pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, 0x00081881, 0);
					break;
				}
				default:
				{
					mobDoDamage(moby, pvars->MobVars.Config.AttackRadius - pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, 0x00081801, 0);
					break;
				}
			}
			break;
		}
		case MOB_ACTION_TIME_BOMB:
		{
			pvars->MobVars.OpacityFlickerDirection = 4;
			pvars->MobVars.TimeBombTicks = ZOMBIE_TIMEBOMB_TICKS;
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
	return vector_sqrmag(pvars->MoveVars.passThruNormal) > (pvars->MobVars.Config.Speed * pvars->MobVars.Config.Speed * 0.5)
				&& vector_sqrmag(pvars->MoveVars.vel) > 0.001;
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
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Player ** players = playerGetAll();
	int i;
	VECTOR delta;
	Moby * currentTarget = pvars->MobVars.Target;
	Player * closestPlayer = NULL;
	float closestPlayerDist = 100000;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = *players;
		if (p && !playerIsDead(p)) {
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

	if (pvars->MobVars.Action == MOB_ACTION_JUMP && pvars->MobVars.IsTraversing)
		return -1;

	// prevent action changing too quickly
	if (pvars->MobVars.ActionCooldownTicks)
		return -1;

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

void mobMove(Moby* moby, u128 to, float speed)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	if (pvars->MobVars.MoveStepCooldownTicks == 0)
	{
		// move
		mobyMove(moby, to, speed);

		// calculate delta position
		vector_subtract(pvars->MoveVars.passThruNormal, moby->Position, pvars->MoveVars.passThruPoint);
		vector_copy(pvars->MoveVars.passThruPoint, moby->Position);

		//
		pvars->MobVars.MoveStepCooldownTicks = pvars->MobVars.MoveStep;
	}
	else if (speed > 0)
	{
		// apply velocity
		vector_add(moby->Position, moby->Position, pvars->MoveVars.vel);
	}
}

void mobStand(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	vector_write(pvars->MoveVars.passThruNormal, 0);
	vector_copy(pvars->MoveVars.passThruPoint, moby->Position);
	mobyStand(moby);
}

void mobDoAction(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	float speed = pvars->MobVars.Config.Speed;
	float speedMult = 1; //(float)pvars->MobVars.MoveStep;
	Moby* target = pvars->MobVars.Target;
	VECTOR t;

	// turn on holos so we can collide with them
	weaponTurnOnHoloshields(-1);

	switch (pvars->MobVars.Action)
	{
		case MOB_ACTION_SPAWN:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_CRAWL_OUT_OF_GROUND);
			speed = 0;
			mobStand(moby);
			break;
		}
		case MOB_ACTION_FLINCH:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_BIG_FLINCH);
			speed = 0;
			mobStand(moby);

			if (pvars->MobVars.Knockback.Ticks > 0) {
				pvars->MoveVars.onGround = 0;
				pvars->MoveVars.offGround = 1;
				pvars->MoveVars.gravityVel = -0.1;
				VECTOR t;
				float power = PLAYER_KNOCKBACK_BASE_POWER * pvars->MobVars.Knockback.Power * pvars->MobVars.MoveStep;
				vector_fromyaw(t, pvars->MobVars.Knockback.Angle / 1000.0);
				t[2] = 1.0;
				vector_scale(t, t, power);
				vector_copy(pvars->MoveVars.vel, t);
			}
			break;
		}
		case MOB_ACTION_IDLE:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_IDLE);
			speed = 0;
			mobStand(moby);
			break;
		}
		case MOB_ACTION_JUMP:
			{
				if (target) {
					// determine next position
					vector_copy(t, target->Position);
					vector_subtract(t, t, moby->Position);
					float dY = t[2];
					t[2] = 0;
					float dist = vector_length(t);
					vector_scale(t, t, (2 * speedMult) / dist);
					t[2] = dY;
					vector_add(t, moby->Position, t);

					// move
					mobMove(moby, vector_read(t), speed * speedMult);
					mobTransAnim(moby, ZOMBIE_ANIM_JUMP);
				} else {
					// stand
					speed = 0;
					mobStand(moby);
					mobTransAnim(moby, ZOMBIE_ANIM_IDLE);
				}
				break;
			}
		case MOB_ACTION_LOOK_AT_TARGET:
			speed = 0;
		case MOB_ACTION_WALK:
		{
			
			if (target) {
				// determine next position
				vector_copy(t, target->Position);
				vector_subtract(t, t, moby->Position);
				float dist = vector_length(t);
				vector_scale(t, t, (1 * speedMult) / dist);
				vector_add(t, moby->Position, t);

				// slow down as they near their target
				float slowDownFar = pvars->MobVars.Config.AttackRadius * 1.0;
				float slowDownNear = pvars->MobVars.Config.AttackRadius * 0.5;
				if (dist < slowDownFar) {
					float slowDown = ((dist - slowDownNear) / (slowDownFar - slowDownNear)) - 0.0;
					if (slowDown < 0.1)
						slowDown = 0;
					speed *= slowDown;
				}

				// move
				mobMove(moby, vector_read(t), speed * speedMult);
			} else {
				// stand
				speed = 0;
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
				
				speed = 0;
				moby->Opacity = (u8)newOpacity;
			}
			break;
		}
		case MOB_ACTION_ATTACK:
		{
			speed = 0;
			mobTransAnim(moby, ZOMBIE_ANIM_SLAP);
			break;
		}
	}

	// turn off holos for everyone else
	weaponTurnOffHoloshields();

	// update if we've attempted to move
	pvars->MobVars.HasSpeed = speed > 0.001;
}

void mobHandleStuck(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	Moby* target = pvars->MobVars.Target;
	int hasVelocity = mobHasVelocity(pvars);

	// increment ticks stuck
	if (hasVelocity) {
		pvars->MobVars.MovingTicks += 1;
		if (pvars->MobVars.MovingTicks > 5)
			pvars->MobVars.StuckTicks = 0;
	}
	else {
		if (pvars->MobVars.HasSpeed)
			pvars->MobVars.StuckTicks += 1;
		else
			pvars->MobVars.StuckTicks = 0;
		pvars->MobVars.MovingTicks = 0;
	}

	// 
	if (mobAmIOwner(moby)) {
		if (pvars->MobVars.StuckTicks > ZOMBIE_RESPAWN_AFTER_TICKS)
			pvars->MobVars.Respawn = 1;
		else if (pvars->MobVars.StuckTicks > 60)
			pvars->MobVars.IsTraversing = 1;

		// 
		if (pvars->MobVars.IsTraversing) {
			if (pvars->MobVars.MovingTicks > 20) {
				pvars->MoveVars.maxStepUp = ZOMBIE_BASE_STEP_HEIGHT;
				pvars->MoveVars.maxStepDown = ZOMBIE_BASE_STEP_HEIGHT;
				pvars->MoveVars.collRadius = ZOMBIE_BASE_COLL_RADIUS;
				pvars->MobVars.IsTraversing = 0;
			} else {
				// stuck so cycle step
				pvars->MoveVars.maxStepUp = clamp(pvars->MoveVars.maxStepUp + 0.5, 0, ZOMBIE_MAX_STEP_UP);
				pvars->MoveVars.maxStepDown = clamp(pvars->MoveVars.maxStepDown + 0.5, 0, ZOMBIE_MAX_STEP_DOWN);
				pvars->MoveVars.collRadius = clamp(pvars->MoveVars.collRadius + 0.25, 0, ZOMBIE_MAX_COLL_RADIUS);
				
				mobSetAction(moby, MOB_ACTION_JUMP);
			}
		}
	} else if (pvars->MobVars.Action == MOB_ACTION_JUMP) {
		pvars->MoveVars.maxStepUp = clamp(pvars->MoveVars.maxStepUp + 0.5, 0, ZOMBIE_MAX_STEP_UP);
		pvars->MoveVars.maxStepDown = clamp(pvars->MoveVars.maxStepDown + 0.5, 0, ZOMBIE_MAX_STEP_DOWN);
		pvars->MoveVars.collRadius = clamp(pvars->MoveVars.collRadius + 0.25, 0, ZOMBIE_MAX_COLL_RADIUS);
	} else {
		pvars->MoveVars.maxStepUp = pvars->MoveVars.maxStepDown = ZOMBIE_BASE_STEP_HEIGHT;
		pvars->MoveVars.collRadius = ZOMBIE_BASE_COLL_RADIUS;
	}
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
				DPRINTF("set %08X to order %d (free %d)\n", (u32)moby, i, AllMobsSortedFreeSpots);
				break;
			}
		}
	}

	int order = pvars->MobVars.Order;
	if (order >= 0) {
		float rank = 1 - clamp(order / (float)State.RoundMobCount, 0, 1);
		float rankCubed = rank*rank*rank;
		if (State.RoundMobCount > 30) {
			moby->DrawDist = 4 + (128 - 4)*rankCubed;
			pvars->MobVars.MoveStep = 1 + (12-1)*(1-rank);
		}
		else {
			moby->DrawDist = 128;
			pvars->MobVars.MoveStep = 1;
		}
	} else {
		moby->DrawDist = 128;
		pvars->MobVars.MoveStep = 1;
	}

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
	

	//pvars->MoveVars.runSpeed = 0.20 * pvars->MobVars.MoveStep;
	//pvars->MoveVars.flySpeed = 0.20 * pvars->MobVars.MoveStep;
	pvars->MoveVars.angularAccel = 0.005 * pvars->MobVars.MoveStep;
	pvars->MoveVars.angularDecel = 0.005 * pvars->MobVars.MoveStep;
	pvars->MoveVars.angularLimit = 0.20 * pvars->MobVars.MoveStep;
	//pvars->MoveVars.gravity = 20 * pvars->MobVars.MoveStep;
}

void mobUpdate(Moby* moby)
{
	int i;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars || pvars->MobVars.Destroyed)
		return;
	int isOwner;

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
	u16 moveStepCooldownTicks = decTimerU8(&pvars->MobVars.MoveStepCooldownTicks);
	decTimerU8(&pvars->MobVars.Knockback.Ticks);

	// validate owner
	Player * ownerPlayer = playerGetAll()[(int)pvars->MobVars.Owner];
	if (!ownerPlayer) {
		pvars->MobVars.Owner = gameGetHostId(); // default as host
		//mobSendOwnerUpdate(moby, pvars->MobVars.Owner);
	}
	
	// determine if I'm the owner
	isOwner = mobAmIOwner(moby);

	// change owner to target
	if (isOwner && pvars->MobVars.Target) {
		Player * targetPlayer = (Player*)guberGetObjectByMoby(pvars->MobVars.Target);
		if (targetPlayer && targetPlayer->Guber.Id.GID.HostId != pvars->MobVars.Owner)
			mobSendOwnerUpdate(moby, targetPlayer->Guber.Id.GID.HostId);
	}

	// 
	if (ambientSoundCooldownTicks == 0) {
		mobPlayAmbientSound(moby);
		pvars->MobVars.AmbientSoundCooldownTicks = randRangeInt(ZOMBIE_AMBSND_MIN_COOLDOWN_TICKS, ZOMBIE_AMBSND_MAX_COOLDOWN_TICKS);
	}

	// 
	mobHandleDraw(moby);

	// 
	mobDoAction(moby);

	// 
	mobyUpdateFlash(moby, 0);

	// 
	mobHandleStuck(moby);

	//
	if (isOwner) {
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
	if (pvars->MobVars.Config.MobType == MOB_GHOST) {
		u8 targetOpacity = pvars->MobVars.Action > MOB_ACTION_WALK ? 0x80 : 0x10;
		u8 opacity = moby->Opacity;
		if (opacity < targetOpacity)
			opacity = targetOpacity;
		else if (opacity > targetOpacity)
			opacity--;
		moby->Opacity = opacity;
	}

	// update armor
	moby->Bangles = mobGetArmor(pvars);

	// move system update
	mobyMoveSystemUpdate(moby);

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
			pvars->MobVars.Destroyed = 1;
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

struct GuberMoby* mobGetGuber(Moby* moby)
{
	if (moby->OClass == 0x20F6 && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
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
	struct MobSpawnEventArgs args;

	// read event
	guberEventRead(event, p, 12);
	guberEventRead(event, &yaw, 4);
	guberEventRead(event, &spawnFromUID, 4);
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

	// update reference to moby collision
	if (!mobCollData)
		mobCollData = moby->CollData;
	moby->CollData = NULL;


	// update pvars
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	memset(pvars, 0, sizeof(struct MobPVar));
	pvars->TargetVarsPtr = &pvars->TargetVars;
	pvars->MoveVarsPtr = &pvars->MoveVars;
	pvars->FlashVarsPtr = &pvars->FlashVars;
	pvars->ReactVarsPtr = &pvars->ReactVars;
	
	// initialize mob vars
	pvars->MobVars.Config.MobType = args.MobType;
	pvars->MobVars.Config.Bolts = args.Bolts / State.Difficulty;
	pvars->MobVars.Config.MaxHealth = (float)args.StartHealth * State.Difficulty;
	pvars->MobVars.Config.Health = (float)args.StartHealth * State.Difficulty;
	pvars->MobVars.Config.Bangles = args.Bangles;
	pvars->MobVars.Config.Damage = (float)args.Damage * State.Difficulty;
	pvars->MobVars.Config.AttackRadius = (float)args.AttackRadiusEighths / 8.0;
	pvars->MobVars.Config.HitRadius = (float)args.HitRadiusEighths / 8.0;
	pvars->MobVars.Config.Speed = (float)args.SpeedHundredths / 100.0;
	pvars->MobVars.Config.ReactionTickCount = args.ReactionTickCount; // (u8)(args.ReactionTickCount / State.Difficulty);
	pvars->MobVars.Config.AttackCooldownTickCount = args.AttackCooldownTickCount;
	pvars->MobVars.Health = pvars->MobVars.Config.MaxHealth;
	pvars->MobVars.Order = -1;
#if MOB_NO_MOVE
	pvars->MobVars.Config.Speed = 0;
#endif
#if MOB_NO_DAMAGE
	pvars->MobVars.Config.Damage = 0;
#endif

	// initialize target vars
	pvars->TargetVars.hitPoints = pvars->MobVars.Health;
	pvars->TargetVars.team = 10;
	pvars->TargetVars.targetHeight = 1;

	// initialize move vars
	mobyMoveSystemInit(moby);
	mobySetMoveDistanceThresholds(moby, 0.5, 0.5, 1.0, 5.0);
	mobySetMoveSpeedLimits(moby, 1, 1, 1/6.0, 1/6.0, 1/60.0, 1/60.0, 1/60.0);
	mobySetMoveAngularSpeeds(moby, 0.00349066, 0.00349066, 0.20944);
	pvars->MoveVars.groupCache = 0;
	pvars->MoveVars.elv_state = 0;
	pvars->MoveVars.slopeLimit = MATH_PI / 2;
	pvars->MoveVars.maxStepUp = ZOMBIE_BASE_STEP_HEIGHT;
	pvars->MoveVars.maxStepDown = ZOMBIE_BASE_STEP_HEIGHT;
	pvars->MoveVars.flags |= 2;
	pvars->MoveVars.boundArea = -1;
	pvars->MoveVars.groundHotspot = -1;
	vector_copy(pvars->MoveVars.passThruPoint, p);
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
	}

	// 
	mobySetState(moby, 0, -1);

	++State.RoundMobCount;

	// destroy spawn from
	if (spawnFromUID != -1) {
		GuberMoby* gm = (GuberMoby*)guberGetObjectByUID(spawnFromUID);
		if (gm && gm->Moby && gm->Moby->PVar) {
			guberMobyDestroy(gm->Moby);
			State.RoundMobCount--;
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
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars || pvars->MobVars.Destroyed)
		return 0;

	// 
	guberEventRead(event, &killedByPlayerId, sizeof(killedByPlayerId));
	guberEventRead(event, &weaponId, sizeof(weaponId));

#if SHARED_BOLTS
	if (killedByPlayerId >= 0) {
		Player * killedByPlayer = State.PlayerStates[killedByPlayerId].Player;
		if (killedByPlayer) {
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				if (State.PlayerStates[i].Player && !playerIsDead(State.PlayerStates[i].Player) && State.PlayerStates[i].Player->Team == killedByPlayer->Team) {
					State.PlayerStates[i].State.Bolts += pvars->MobVars.Config.Bolts;
					State.PlayerStates[i].State.TotalBolts += pvars->MobVars.Config.Bolts;
				}
			}
		}
	}
#else
	if (killedByPlayerId >= 0) {
		State.PlayerStates[(int)killedByPlayerId].State.Bolts += pvars->MobVars.Config.Bolts;
		State.PlayerStates[(int)killedByPlayerId].State.TotalBolts += pvars->MobVars.Config.Bolts;
	}
#endif

	if (killedByPlayerId >= 0) {
		struct SurvivalPlayer* pState = &State.PlayerStates[(int)killedByPlayerId];
	 	PlayerGameStats* stats = gameGetPlayerStats();
		PlayerWeaponStats* pWStats = gameGetPlayerWeaponStats();

		// handle weapon jackpot
		if (weaponId > 1 && pState->Player) {
			int jackpotCount = playerGetWeaponAlphaModCount(pState->Player, weaponId, ALPHA_MOD_JACKPOT);

			pState->State.Bolts += jackpotCount * JACKPOT_BOLTS;
			pState->State.TotalBolts += jackpotCount * JACKPOT_BOLTS;
		}

		// handle stats
		pState->State.Kills++;
		stats->Kills[killedByPlayerId]++;
		if (weaponId > 0)
			pWStats->WeaponKills[killedByPlayerId][weaponId]++;
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
		if (pvars->MobVars.Action == MOB_ACTION_TIME_BOMB) {
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
		if ((args.Knockback.Power > 0 || damage >= 10) && canFlinch)
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
	
	//DPRINTF("mob owner update event %08X, %08X, %d\n", (u32)moby, (u32)event, newOwner);
	return 0;
}

int mobHandleEvent(Moby* moby, GuberEvent* event)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	if (moby->OClass == 0x20F6 && pvars) {
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
		args.StartHealth = (u16)config->Health;
		args.Bangles = (u16)config->Bangles;
		args.MobType = (char)config->MobType;
		args.Damage = (u8)config->Damage;
		args.AttackRadiusEighths = (u8)(config->AttackRadius * 8);
		args.HitRadiusEighths = (u8)(config->HitRadius * 8);
		args.SpeedHundredths = (u8)(config->Speed * 100);
		args.ReactionTickCount = (u8)config->ReactionTickCount;
		args.AttackCooldownTickCount = (u8)config->AttackCooldownTickCount;

		guberEventWrite(guberEvent, position, 12);
		guberEventWrite(guberEvent, &yaw, 4);
		guberEventWrite(guberEvent, &spawnFromUID, 4);
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
	*(u32*)0x003A0A84 = (u32)&mobGetGuber;
	*(u32*)0x003A0A94 = (u32)&mobHandleEvent;

	// 
	memset(AllMobsSorted, 0, sizeof(AllMobsSorted));
}

void mobTick(void)
{
	int i, j;
	VECTOR t;
	Player* locals[] = { playerGetFromSlot(0), playerGetFromSlot(1) };
	int localsCount = playerGetNumLocals();

	// run single pass on sort
	for (i = 0; i < MAX_MOBS_SPAWNED; ++i)
	{
		Moby* m = AllMobsSorted[i];

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
				vector_subtract(t, m->Position, p->PlayerPosition);
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
