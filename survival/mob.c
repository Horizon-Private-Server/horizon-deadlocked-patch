#include "include/mob.h"
#include "include/utils.h"
#include "include/game.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>
#include <libdl/random.h>

#define ZOMBIE_ANIM_ATTACK_TICKS						(30)
#define ZOMBIE_TIMEBOMB_TICKS								(60 * 3)
#define ZOMBIE_FLINCH_COOLDOWN_TICKS				(60)
#define ZOMBIE_ACTION_COOLDOWN_TICKS				(30)
#define ZOMBIE_RESPAWN_AFTER_TICKS					(60 * 30)
#define ZOMBIE_AUTO_DIRTY_COOLDOWN_TICKS		(60 * 5)
#define ZOMBIE_AMBSND_MIN_COOLDOWN_TICKS    (60 * 2)
#define ZOMBIE_AMBSND_MAX_COOLDOWN_TICKS    (60 * 3)


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

struct MoveVars_V2 DefaultMoveVars;
struct TargetVars DefaultTargetVars;
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

void mobSendDamageEvent(Moby* moby, Moby* source, float amount, int damageFlags)
{
	struct MobDamageEventArgs args;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_DAMAGE);
	if (guberEvent) {
		args.SourceUID = guberGetUID(source);
		args.Damage = amount;
		args.DamageFlags = damageFlags;
		guberEventWrite(guberEvent, &args, sizeof(struct MobDamageEventArgs));
	}
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

void mobDestroy(Moby* moby, int hitByUID)
{
	char killedByPlayerId = -1;

	// create event
	GuberEvent * guberEvent = mobCreateEvent(moby, MOB_EVENT_DESTROY);
	if (guberEvent) {

		// get damager player id
		Player* damager = (Player*)playerGetFromUID(hitByUID);
		if (damager)
			killedByPlayerId = (char)damager->PlayerId;

		// 
		guberEventWrite(guberEvent, &killedByPlayerId, sizeof(killedByPlayerId));
	}
}

void mobMutate(struct MobSpawnParams* spawnParams, enum MobMutateAttribute attribute)
{
	if (!spawnParams)
		return;
	
	switch (attribute)
	{
		case MOB_MUTATE_DAMAGE: if (spawnParams->Config.Damage < ZOMBIE_MAX_DAMAGE) { spawnParams->Config.Damage += ZOMBIE_DAMAGE_MUTATE * ZOMBIE_BASE_DAMAGE; } break;
		case MOB_MUTATE_SPEED: if (spawnParams->Config.Speed < ZOMBIE_MAX_SPEED) { spawnParams->Config.Speed += ZOMBIE_SPEED_MUTATE * ZOMBIE_BASE_SPEED; } break;
		case MOB_MUTATE_HEALTH: if (spawnParams->Config.MaxHealth < ZOMBIE_MAX_HEALTH) { spawnParams->Config.MaxHealth += ZOMBIE_HEALTH_MUTATE * ZOMBIE_BASE_HEALTH; } break;
		case MOB_MUTATE_COST: spawnParams->Cost += ZOMBIE_COST_MUTATE; break;
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
	vector_fromyaw(p, moby->Rotation[2]);
	vector_scale(p, p, offset);
	p[2] = 1;
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
	pvars->MobVars.ScoutCooldownTicks = 30;
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

	// set animation
	switch (action)
	{
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
					mobDoDamage(moby, pvars->MobVars.Config.AttackRadius, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, 0x00881801, 0);
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
					mobDoDamage(moby, pvars->MobVars.Config.AttackRadius, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, 0x00081881, 0);
					break;
				}
				default:
				{
					mobDoDamage(moby, pvars->MobVars.Config.AttackRadius, pvars->MobVars.Config.HitRadius, pvars->MobVars.Config.Damage, 0x00081801, 0);
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
	return vector_sqrmag(pvars->MoveVars.vel) > 0.001;
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
			float dist = vector_length(delta);

			if (dist < 300000) {
				// favor existing target
				if (p->SkinMoby == currentTarget && dist > 5)
					dist = 5;
				
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
		float dist = vector_length(t);

		if (dist <= pvars->MobVars.Config.AttackRadius) {
			if (mobCanAttack(pvars))
				return pvars->MobVars.Config.MobType != MOB_EXPLODE ? MOB_ACTION_ATTACK : MOB_ACTION_TIME_BOMB;
			
			return MOB_ACTION_LOOK_AT_TARGET;
		} else {
			return MOB_ACTION_WALK;
		}
	}
	
	return MOB_ACTION_IDLE;
}

void mobDoAction(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	float speed = pvars->MobVars.Config.Speed;
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
			mobyStand(moby);
			break;
		}
		case MOB_ACTION_FLINCH:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_BIG_FLINCH);
			speed = 0;
			mobyStand(moby);
			break;
		}
		case MOB_ACTION_IDLE:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_IDLE);
			speed = 0;
			mobyStand(moby);
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
					vector_scale(t, t, 2 / dist);
					t[2] = dY;
					vector_add(t, moby->Position, t);

					// move
					mobyMove(moby, vector_read(t), speed);
					mobTransAnim(moby, ZOMBIE_ANIM_JUMP);
				} else {
					// stand
					speed = 0;
					mobyStand(moby);
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
				vector_scale(t, t, 1 / dist);
				vector_add(t, moby->Position, t);
				if (dist < pvars->MobVars.Config.AttackRadius)
					speed = 0;

				// move
				mobyMove(moby, vector_read(t), speed);
			} else {
				// stand
				speed = 0;
				mobyStand(moby);
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
		pvars->MobVars.MovingTicks++;
		pvars->MobVars.StuckTicks = 0;
	}
	else {
		if (pvars->MobVars.HasSpeed)
			pvars->MobVars.StuckTicks++;
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
			if (pvars->MobVars.MovingTicks > 10) {
				pvars->MoveVars.maxStepUp = 2;
				pvars->MoveVars.maxStepDown = 2;
				pvars->MobVars.IsTraversing = 0;
			} else {
				// stuck so cycle step
				pvars->MoveVars.maxStepUp = pvars->MoveVars.maxStepDown = clamp(pvars->MoveVars.maxStepUp + 0.5, 0, 20);
				mobSetAction(moby, MOB_ACTION_JUMP);
			}
		}
	} else if (pvars->MobVars.Action == MOB_ACTION_JUMP) {
		pvars->MoveVars.maxStepUp = pvars->MoveVars.maxStepDown = clamp(pvars->MoveVars.maxStepUp + 0.5, 0, 20);
	} else {
		pvars->MoveVars.maxStepUp = pvars->MoveVars.maxStepDown = 2;
	}
}

void mobUpdate(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars || pvars->MobVars.Destroyed)
		return;
	int isOwner;

	// dec timers
	u16 nextActionTicks = decTimerU16(&pvars->MobVars.NextActionDelayTicks);
	decTimerU16(&pvars->MobVars.ActionCooldownTicks);
	decTimerU16(&pvars->MobVars.AttackCooldownTicks);
	u16 scoutCooldownTicks = decTimerU16(&pvars->MobVars.ScoutCooldownTicks);
	decTimerU16(&pvars->MobVars.FlinchCooldownTicks);
	decTimerU16(&pvars->MobVars.TimeBombTicks);
	u16 autoDirtyCooldownTicks = decTimerU16(&pvars->MobVars.AutoDirtyCooldownTicks);
	u16 ambientSoundCooldownTicks = decTimerU16(&pvars->MobVars.AmbientSoundCooldownTicks);

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
	mobHandleStuck(moby);

	// 
	mobDoAction(moby);

	// 
	mobyUpdateFlash(moby, 0);

	//
	if (isOwner) {
		int nextAction = mobGetPreferredAction(moby);
		if (nextAction >= 0 && nextAction != pvars->MobVars.NextAction) {
			pvars->MobVars.NextAction = nextAction;
			pvars->MobVars.NextActionDelayTicks = nextAction >= MOB_ACTION_ATTACK ? pvars->MobVars.Config.ReactionTickCount : 0;
		} else if (pvars->MobVars.NextAction >= 0 && nextActionTicks == 0) {
			mobSetAction(moby, pvars->MobVars.NextAction);
			pvars->MobVars.NextAction = -1;
		}

		// get new target
		if (scoutCooldownTicks == 0) {
			mobSetTarget(moby, mobGetNextTarget(moby));
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
		//DPRINTF("damage: %f\n", damage);
		Player * damager = guberMobyGetPlayerDamager(colDamage->Damager);
		if (damager && playerIsLocal(damager)) {
			mobSendDamageEvent(moby, damager->PlayerMoby, damage, colDamage->DamageFlags);
		} else if (isOwner) {
			mobSendDamageEvent(moby, colDamage->Damager, damage, colDamage->DamageFlags);
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
				mobCreate(p, 0, &config);
				mobDestroy(moby, -1);
			}

			pvars->MobVars.Respawn = 0;
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
	struct MobSpawnEventArgs args;

	// read event
	guberEventRead(event, p, 12);
	guberEventRead(event, &yaw, 4);
	guberEventRead(event, &args, sizeof(struct MobSpawnEventArgs));

	// set position and rotation
	vector_copy(moby->Position, p);
	moby->Rotation[2] = yaw;
	moby->Scale *= 0.7;

	// set update
	moby->PUpdate = &mobUpdate;

	// 
	moby->ModeBits |= 0x30;
	moby->GlowRGBA = MobSecondaryColors[(int)args.MobType];
	moby->PrimaryColor = MobPrimaryColors[(int)args.MobType];


	// update pvars
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	memset(pvars, 0, sizeof(struct MobPVar));
	pvars->TargetVarsPtr = &pvars->TargetVars;
	pvars->MoveVarsPtr = &pvars->MoveVars;
	pvars->FlashVarsPtr = &pvars->FlashVars;
	pvars->ReactVarsPtr = &pvars->ReactVars;
	
	// initialize mob vars
	pvars->MobVars.Config.MobType = args.MobType;
	pvars->MobVars.Config.Bolts = args.Bolts;
	pvars->MobVars.Config.MaxHealth = (float)args.MaxHealth;
	pvars->MobVars.Config.Bangles = args.Bangles;
	pvars->MobVars.Config.Damage = (float)args.Damage;
	pvars->MobVars.Config.AttackRadius = (float)args.AttackRadiusEighths / 8.0;
	pvars->MobVars.Config.HitRadius = (float)args.HitRadiusEighths / 8.0;
	pvars->MobVars.Config.Speed = (float)args.SpeedHundredths / 100.0;
	pvars->MobVars.Config.ReactionTickCount = args.ReactionTickCount;
	pvars->MobVars.Config.AttackCooldownTickCount = args.AttackCooldownTickCount;
	pvars->MobVars.Health = pvars->MobVars.Config.MaxHealth;
#if MOB_NO_MOVE
	pvars->MobVars.Config.Speed = 0;
#endif
#if MOB_NO_DAMAGE
	pvars->MobVars.Config.Damage = 0;
#endif

	// initialize target vars
	memcpy(pvars->TargetVarsPtr, &DefaultTargetVars, sizeof(struct TargetVars));
	pvars->TargetVars.hitPoints = pvars->MobVars.Health;
	pvars->TargetVars.team = 10;
	pvars->TargetVars.targetHeight = 1;

	// initialize move vars
	memcpy(pvars->MoveVarsPtr, &DefaultMoveVars, sizeof(struct MoveVars_V2));
	pvars->MoveVars.groupCache = 0;
	pvars->MoveVars.elv_state = 0;
	pvars->MoveVars.slopeLimit = MATH_PI / 2;
	pvars->MoveVars.maxStepUp = 2;
	pvars->MoveVars.maxStepDown = 2;

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

	static int gameTotal = 0;
	++gameTotal;
	++State.RoundMobCount;
	//DPRINTF("mob created event %08X, %08X, %08X mobtype:%d spawnedNum:%d roundTotal:%d gameTotal:%d)\n", (u32)moby, (u32)event, (u32)moby->GuberMoby, args.MobType, State.RoundMobCount, State.RoundMobSpawnedCount, gameTotal);
	return 0;
}

int mobHandleEvent_Destroy(Moby* moby, GuberEvent* event)
{
	char killedByPlayerId;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	if (!pvars || pvars->MobVars.Destroyed)
		return 0;

	// 
	guberEventRead(event, &killedByPlayerId, sizeof(killedByPlayerId));
	if (killedByPlayerId >= 0)
		State.PlayerStates[(int)killedByPlayerId].State.Bolts += pvars->MobVars.Config.Bolts;

	// set colors before death so that the corn has the correct color
	moby->PrimaryColor = MobPrimaryColors[pvars->MobVars.Config.MobType];

	//mobPlayDeathSound(moby);
	mobSpawnCorn(moby, ZOMBIE_BANGLE_LARM | ZOMBIE_BANGLE_RARM | ZOMBIE_BANGLE_LLEG | ZOMBIE_BANGLE_RLEG | ZOMBIE_BANGLE_RFOOT | ZOMBIE_BANGLE_HIPS);
	//((void (*)(Moby*, int, int))0x0051add0)(moby, -1, 1);
	guberMobyDestroy(moby);
	moby->ModeBits &= ~0x30;
	--State.RoundMobCount;
	pvars->MobVars.Destroyed = 1;
	//DPRINTF("mob destroy event %08X, %08X, by client %d, (%d)\n", (u32)moby, (u32)event, killedByPlayerId, State.RoundMobCount);
	return 0;
}

int mobHandleEvent_Damage(Moby* moby, GuberEvent* event)
{
	struct MobDamageEventArgs args;
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	int armorStart = mobGetArmor(pvars);
	if (!pvars)
		return 0;

	// read event
	guberEventRead(event, &args, sizeof(struct MobDamageEventArgs));

	// get damager
	GuberMoby* damager = (GuberMoby*)guberGetObjectByUID(args.SourceUID);

	// flash
	mobyStartFlash(moby, FT_HIT, 0x800000FF, 0);

	// decrement health
	float newHp = pvars->MobVars.Health - args.Damage;
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
		}
	}

	if (mobAmIOwner(moby))
	{
		// flinch
		if (args.Damage >= 10 && pvars->MobVars.Action != MOB_ACTION_TIME_BOMB && pvars->MobVars.FlinchCooldownTicks == 0)
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

GuberMoby * mobCreate(VECTOR position, float yaw, struct MobConfig *config)
{
	struct MobSpawnEventArgs args;

	// create guber object
	GuberEvent * guberEvent = 0;
	int uid = (int)guberMobyCreateSpawned(0x20F6, sizeof(struct MobPVar), &guberEvent, NULL);
	if (guberEvent)
	{
		args.Bolts = config->Bolts + randRangeInt(-50, 50);
		args.MaxHealth = (u16)config->MaxHealth;
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
		guberEventWrite(guberEvent, &args, sizeof(struct MobSpawnEventArgs));
	}
	else
	{
		DPRINTF("failed to guberevent mob\n");
	}
  
  return guberEvent != 0;
}

void mobInitialize(void)
{
	Moby* firstMoby = mobyGetFirst();

	// set 
	*(u32*)0x003A0A84 = (u32)&mobGetGuber;
	*(u32*)0x003A0A94 = (u32)&mobHandleEvent;

	// spawn sheep so we can get the default movevars
	Moby * moby = ((Moby* (*)(Moby*, int ,int))0x003a5cd8)(firstMoby, 0, -1);
	if (moby) {
		struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
		memcpy(&DefaultMoveVars, pvars->MoveVarsPtr, sizeof(struct MoveVars_V2));
		memcpy(&DefaultTargetVars, pvars->TargetVarsPtr, sizeof(struct TargetVars));
		DefaultMoveVars.pGroundMoby = NULL;
		mobyDestroy(moby);
	}
}
