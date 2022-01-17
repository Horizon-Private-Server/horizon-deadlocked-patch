#include "include/mob.h"
#include "include/utils.h"
#include <string.h>
#include <libdl/stdio.h>
#include <libdl/game.h>
#include <libdl/collision.h>
#include <libdl/moby.h>

#define ZOMBIE_ANIM_ATTACK_TICKS						(30)
#define ZOMBIE_TIMEBOMB_TICKS								(60 * 3)
#define ZOMBIE_FLINCH_COOLDOWN_TICKS				(60)
#define ZOMBIE_ACTION_COOLDOWN_TICKS				(30)
#define ZOMBIE_RESPAWN_AFTER_TICKS					(60 * 30)
#define MAX_MOB_COUNT												(40)

struct MoveVars_V2 DefaultMoveVars;
struct TargetVars DefaultTargetVars;

int mobCount = 0;


u16 mobDecTimer(u16* timeValue)
{
	int value = *timeValue;
	if (value == 0)
		return 0;

	*timeValue = --value;
	return value;
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

void mobSetAction(Moby* moby, int action)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

	// don't set if already action
	if (pvars->MobVars.Action == action)
		return;
		
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

short mobGetArmor(struct MobPVar* pvars) {
	float t = pvars->MobVars.Health / pvars->MobVars.Config.MaxHealth;
	if (t < 0.3)
		return 0x0000;
	else if (t < 0.7)
		return 0x0001;
	
	return 0x0101;
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
		if (p) {
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

	switch (pvars->MobVars.Action)
	{
		case MOB_ACTION_SPAWN:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_CRAWL_OUT_OF_GROUND);
			mobyStand(moby);
			break;
		}
		case MOB_ACTION_FLINCH:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_BIG_FLINCH);
			mobyStand(moby);
			break;
		}
		case MOB_ACTION_IDLE:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_IDLE);
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

				moby->Opacity = (u8)newOpacity;
			}
			break;
		}
		case MOB_ACTION_ATTACK:
		{
			mobTransAnim(moby, ZOMBIE_ANIM_SLAP);
			break;
		}
	}
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
		if (pvars->MobVars.Action == MOB_ACTION_WALK)
			pvars->MobVars.StuckTicks++;
		else
			pvars->MobVars.StuckTicks = 0;
		pvars->MobVars.MovingTicks = 0;
	}

	// 
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
}

void mobUpdate(Moby* moby)
{
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
	
	// dec timers
	u16 nextActionTicks = mobDecTimer(&pvars->MobVars.NextActionDelayTicks);
	mobDecTimer(&pvars->MobVars.ActionCooldownTicks);
	mobDecTimer(&pvars->MobVars.AttackCooldownTicks);
	u16 scoutCooldownTicks = mobDecTimer(&pvars->MobVars.ScoutCooldownTicks);
	mobDecTimer(&pvars->MobVars.FlinchCooldownTicks);
	mobDecTimer(&pvars->MobVars.TimeBombTicks);

	// 
	mobHandleStuck(moby);

	// 
	mobDoAction(moby);

	// 
	mobyUpdateFlash(moby, 0);

	//
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
		pvars->MobVars.Target = mobGetNextTarget(moby);
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

	if (colDamage) {
		if (damage > 0) {
			// decrement health
			float newHp = pvars->MobVars.Health - damage;
			pvars->MobVars.Health = newHp;
			pvars->TargetVars.hitPoints = newHp;

			// flinch
			if (damage > 0 && pvars->MobVars.Action != MOB_ACTION_TIME_BOMB && pvars->MobVars.FlinchCooldownTicks == 0)
				mobSetAction(moby, MOB_ACTION_FLINCH);

			// flash
			mobyStartFlash(moby, FT_HIT, 0x800000FF, 0);

			// set target
			pvars->MobVars.Target = colDamage->Damager;

			// destroy
			if (newHp <= 0)
				pvars->MobVars.Destroy = 1;
		}

		// 
		moby->CollDamage = -1;
	}

	// handle falling under map
	if (moby->Position[2] < gameGetDeathHeight())
		pvars->MobVars.Respawn = 1;

	// respawn
	if (pvars->MobVars.Respawn) {
		VECTOR p;
		struct MobConfig config;
		if (spawnGetRandomPoint(p)) {
			memcpy(&config, &pvars->MobVars.Config, sizeof(struct MobConfig));
			mobyDestroy(moby);
			--mobCount;

			mobCreate(p, 0, &config);
		}

	}

	// destroy
	if (pvars->MobVars.Destroy) {
		mobyDestroy(moby);
		--mobCount;
	}
}

struct GuberMoby* mobGetGuber(Moby* moby)
{
	if (moby->OClass == 0x20F6)
		return moby->GuberMoby;
	
	return 0;
}

int mobCreatedEvent(Moby* moby, GuberEvent* event)
{
	VECTOR p;
	float yaw;
	struct MobConfig config;

	DPRINTF("mob created event %08X, %08X\n", (u32)moby, (u32)event);

	if (moby->OClass == 0x20F6) {
		
		// read event
		guberEventRead(event, p, 12);
		guberEventRead(event, &yaw, 4);
		guberEventRead(event, &config, sizeof(struct MobConfig));

		// set position and rotation
		vector_copy(moby->Position, p);
		moby->Rotation[2] = yaw;
		moby->Scale *= 0.7;

		// set update
		moby->PUpdate = &mobUpdate;

		// set armor
		moby->Bangles = 0x0101;

		// 
		moby->ModeBits |= 0x30;
		moby->GlowRGBA = 0x80202020;

		// disable targeting
		switch (config.MobType)
		{
			case MOB_GHOST:
			{
				// disable targeting
				moby->ModeBits &= ~0x1000;
				break;
			}
			case MOB_ACID:
			{
				moby->GlowRGBA = 0x8000FF00;
				break;
			}
			case MOB_FREEZE:
			{
				moby->PrimaryColor = 0x00804000;
				moby->GlowRGBA = 0x80FF2000;
				break;
			}
			case MOB_EXPLODE:
			{
				moby->PrimaryColor = 0x00202080;
				moby->GlowRGBA = 0x000040C0;
				break;
			}
		}

		// update pvars
		struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
		pvars->TargetVarsPtr = &pvars->TargetVars;
		pvars->MoveVarsPtr = &pvars->MoveVars;
		pvars->FlashVarsPtr = &pvars->FlashVars;
		
		// initialize target vars
		memcpy(pvars->TargetVarsPtr, &DefaultTargetVars, sizeof(struct TargetVars));
		pvars->TargetVars.hitPoints = config.MaxHealth;
		pvars->TargetVars.team = 10;
		pvars->TargetVars.targetHeight = 1.5;

		// initialize move vars
		memcpy(pvars->MoveVarsPtr, &DefaultMoveVars, sizeof(struct MoveVars_V2));
		pvars->MoveVars.groupCache = 0;
		pvars->MoveVars.elv_state = 0;
		pvars->MoveVars.slopeLimit = MATH_PI / 2;
		pvars->MoveVars.maxStepUp = 2;
		pvars->MoveVars.maxStepDown = 2;

		// initialize mob vars
		memcpy(&pvars->MobVars.Config, &config, sizeof(struct MobConfig));
		pvars->MobVars.Health = config.MaxHealth;

		// 
		mobySetState(moby, 0, -1);
	}

	++mobCount;
	return 0;
}

GuberMoby * mobCreate(VECTOR position, float yaw, struct MobConfig *config)
{
	if (mobCount > MAX_MOB_COUNT)
		return NULL;

	// create guber object
	GuberEvent * guberEvent = 0;
	GuberMoby * guberMoby = guberMobyCreateSpawned(0x20F6, sizeof(struct MobPVar), &guberEvent, 0);
	if (guberEvent)
	{
		guberEventWrite(guberEvent, position, 12);
		guberEventWrite(guberEvent, &yaw, 4);
		guberEventWrite(guberEvent, config, sizeof(struct MobConfig));
	}
	else
	{
		DPRINTF("failed to guberevent mob\n");
	}
  
  return guberMoby;
}

void mobInitialize(void)
{
	Moby* firstMoby = mobyGetFirst();

	// set 
	*(u32*)0x003A0A84 = (u32)&mobGetGuber;
	*(u32*)0x003A0A94 = (u32)&mobCreatedEvent;

	// spawn sheep so we can get the default movevars
	Moby * moby = ((Moby* (*)(Moby*, int ,int))0x003a5cd8)(firstMoby, 0, -1);
	if (moby) {
		struct MobPVar* pvars = (struct MobPVar*)moby->PVar;
		memcpy(&DefaultMoveVars, pvars->MoveVarsPtr, sizeof(struct MoveVars_V2));
		memcpy(&DefaultTargetVars, pvars->TargetVarsPtr, sizeof(struct TargetVars));
		mobyDestroy(moby);
	}
}
