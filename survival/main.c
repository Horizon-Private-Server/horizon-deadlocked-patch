/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		SURVIVAL.
 * 
 * NOTES :
 * 		Each offset is determined per app id.
 * 		This is to ensure compatibility between versions of Deadlocked/Gladiator.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/weapon.h>
#include <libdl/hud.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/dialog.h>
#include <libdl/ui.h>
#include <libdl/stdio.h>
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/random.h>
#include <libdl/net.h>
#include <libdl/sound.h>
#include <libdl/dl.h>
#include <libdl/utils.h>
#include <libdl/collision.h>
#include "module.h"
#include "messageid.h"
#include "include/mob.h"
#include "include/drop.h"
#include "include/game.h"
#include "include/utils.h"
#include "include/gate.h"

const char * SURVIVAL_ROUND_COMPLETE_MESSAGE = "Round %d Complete!";
const char * SURVIVAL_ROUND_START_MESSAGE = "Round %d";
const char * SURVIVAL_NEXT_ROUND_BEGIN_SKIP_MESSAGE = "\x1d   Start Round";
const char * SURVIVAL_NEXT_ROUND_TIMER_MESSAGE = "Next Round";
const char * SURVIVAL_GAME_OVER = "GAME OVER";
const char * SURVIVAL_REVIVE_MESSAGE = "\x1c Revive [\x0E%d\x08]";
const char * SURVIVAL_UPGRADE_MESSAGE = "\x11 Upgrade [\x0E%d\x08]";
const char * SURVIVAL_OPEN_WEAPONS_MESSAGE = "\x12 Manage Mods";
const char * SURVIVAL_BUY_GATE_MESSAGE = "\x11 %d Tokens to Open";
const char * SURVIVAL_BUY_UPGRADE_MESSAGES[] = {
	[UPGRADE_HEALTH] "\x11 Health Upgrade",
	[UPGRADE_SPEED] "\x11 Speed Upgrade",
	[UPGRADE_DAMAGE] "\x11 Damage Upgrade",
	[UPGRADE_MEDIC] "\x11 Medic Upgrade",
	[UPGRADE_VENDOR] "\x11 Vendor Discount",
};

const char * ALPHA_MODS[] = {
	"",
	"Speed Mod",
	"Ammo Mod",
	"Aiming Mod",
	"Impact Mod",
	"Area Mod",
	"Xp Mod",
	"Jackpot Mod",
	"Nanoleech Mod"
};

const char * DIFFICULTY_NAMES[] = {
	"Couch Potato",
	"Contestant",
	"Gladiator",
	"Hero",
	"Exterminator"
};

const u8 DIFFICULTY_HITINVTIMERS[] = {
	[0] 180,
	[1] 120,
	[2] 80,
	[3] 47,
	[4] 40,
};

const u8 UPGRADEABLE_WEAPONS[] = {
	WEAPON_ID_VIPERS,
	WEAPON_ID_MAGMA_CANNON,
	WEAPON_ID_ARBITER,
	WEAPON_ID_FUSION_RIFLE,
	WEAPON_ID_MINE_LAUNCHER,
	WEAPON_ID_B6,
	WEAPON_ID_OMNI_SHIELD,
	WEAPON_ID_FLAIL
};

#if FIXEDTARGET
Moby* FIXEDTARGETMOBY = NULL;
#endif

char LocalPlayerStrBuffer[2][48];

int * LocalBoltCount = (int*)0x00171B40;

int Initialized = 0;

struct SurvivalState State;

SoundDef TestSoundDef =
{
	0.0,	  // MinRange
	50.0,	  // MaxRange
	10,		  // MinVolume
	1000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	0xF5,		// Index
	3			  // Bank
};

struct SurvivalSpecialRoundParam specialRoundParams[] = {
	// ROUND 5
	{
		.MaxSpawnedAtOnce = MAX_MOBS_SPAWNED,
		.SpawnParamCount = 2,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_RUNNER,
			MOB_SPAWN_PARAM_GHOST,
			-1, -1
		},
		.Name = "Ghost Runner Round"
	},
	// ROUND 10
	{
		.MaxSpawnedAtOnce = MAX_MOBS_SPAWNED,
		.SpawnParamCount = 3,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_EXPLOSION,
			MOB_SPAWN_PARAM_ACID,
			MOB_SPAWN_PARAM_FREEZE,
			-1
		},
		.Name = "Elemental Round"
	},
	// ROUND 15
	{
		.MaxSpawnedAtOnce = MAX_MOBS_SPAWNED,
		.SpawnParamCount = 2,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_RUNNER,
			MOB_SPAWN_PARAM_FREEZE,
			-1
		},
		.Name = "Freeze Runner Round"
	},
	// ROUND 20
	{
		.MaxSpawnedAtOnce = MAX_MOBS_SPAWNED,
		.SpawnParamCount = 3,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_GHOST,
			MOB_SPAWN_PARAM_EXPLOSION,
			MOB_SPAWN_PARAM_RUNNER,
			-1
		},
		.Name = "Evade Round"
	},
	// ROUND 25
	{
		.MaxSpawnedAtOnce = 10,
		.SpawnParamCount = 1,
		.SpawnParamIds = {
			MOB_SPAWN_PARAM_TITAN,
			-1
		},
		.Name = "Titan Round"
	},
};

const int specialRoundParamsCount = sizeof(specialRoundParams) / sizeof(struct SurvivalSpecialRoundParam);

// NOTE
// These must be ordered from least probable to most probable
struct MobSpawnParams defaultSpawnParams[] = {
	// titan zombie
	[MOB_SPAWN_PARAM_TITAN]
	{
		.Cost = 1000,
		.MinRound = 10,
		.CooldownTicks = TPS * 10,
		.Probability = 0.01,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM,
		.Name = "Titan",
		.Config = {
			.MobType = MOB_TANK,
			.Xp = 250,
			.Bangles = ZOMBIE_BANGLE_HEAD_3 | ZOMBIE_BANGLE_TORSO_3,
			.Damage = ZOMBIE_BASE_DAMAGE * 2.5,
			.MaxDamage = ZOMBIE_BASE_DAMAGE * 5,
			.Speed = ZOMBIE_BASE_SPEED * 0.85,
			.MaxSpeed = ZOMBIE_BASE_SPEED * 1.0,
			.Health = ZOMBIE_BASE_HEALTH * 50.0,
			.MaxHealth = 0,
			.Bolts = ZOMBIE_BASE_BOLTS * 10,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS * 1.5,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS * 1.5,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 4,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS * 0.35,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS * 1.5,
			.MaxCostMutation = 10,
			.MobSpecialMutation = 0
		}
	},
	// ghost zombie
	[MOB_SPAWN_PARAM_GHOST]
	{
		.Cost = 10,
		.MinRound = 4,
		.CooldownTicks = TPS * 1,
		.Probability = 0.05,
		.SpawnType = SPAWN_TYPE_DEFAULT_RANDOM | SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_HEALTHBOX,
		.Name = "Ghost",
		.Config = {
			.MobType = MOB_GHOST,
			.Xp = 25,
			.Bangles = ZOMBIE_BANGLE_HEAD_2 | ZOMBIE_BANGLE_TORSO_2,
			.Damage = ZOMBIE_BASE_DAMAGE * 1.0,
			.MaxDamage = ZOMBIE_BASE_DAMAGE * 1.3,
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxSpeed = ZOMBIE_BASE_SPEED * 2.0,
			.Health = ZOMBIE_BASE_HEALTH * 1.0,
			.MaxHealth = 0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.5,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobSpecialMutation = 0
		}
	},
	// explode zombie
	[MOB_SPAWN_PARAM_EXPLOSION]
	{
		.Cost = 10,
		.MinRound = 6,
		.CooldownTicks = TPS * 2,
		.Probability = 0.08,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER | SPAWN_TYPE_ON_PLAYER,
		.Name = "Explosion",
		.Config = {
			.MobType = MOB_EXPLODE,
			.Xp = 40,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = ZOMBIE_BASE_DAMAGE * 2.0,
			.MaxDamage = ZOMBIE_BASE_DAMAGE * 10.0,
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxSpeed = ZOMBIE_BASE_SPEED * 1.5,
			.Health = ZOMBIE_BASE_HEALTH * 2.0,
			.MaxHealth = 0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_EXPLODE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobSpecialMutation = 0
		}
	},
	// acid zombie
	[MOB_SPAWN_PARAM_ACID]
	{
		.Cost = 20,
		.MinRound = 10,
		.Probability = 0.09,
		.CooldownTicks = TPS * 1,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER | SPAWN_TYPE_NEAR_HEALTHBOX,
		.Name = "Acid",
		.Config = {
			.MobType = MOB_ACID,
			.Xp = 50,
			.Bangles = ZOMBIE_BANGLE_HEAD_4 | ZOMBIE_BANGLE_TORSO_4,
			.Damage = ZOMBIE_BASE_DAMAGE * 1.0,
			.MaxDamage = ZOMBIE_BASE_DAMAGE * 1.0,
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxSpeed = ZOMBIE_BASE_SPEED * 1.8,
			.Health = ZOMBIE_BASE_HEALTH * 1.0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.2,
			.MaxHealth = 0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobSpecialMutation = 0
		}
	},
	// freeze zombie
	[MOB_SPAWN_PARAM_FREEZE]
	{
		.Cost = 20,
		.MinRound = 8,
		.CooldownTicks = TPS * 1,
		.Probability = 0.1,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Freeze",
		.Config = {
			.MobType = MOB_FREEZE,
			.Xp = 50,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = ZOMBIE_BASE_DAMAGE * 1.0,
			.MaxDamage = ZOMBIE_BASE_DAMAGE * 1.6,
			.Speed = ZOMBIE_BASE_SPEED * 0.8,
			.MaxSpeed = ZOMBIE_BASE_SPEED * 1.5,
			.Health = ZOMBIE_BASE_HEALTH * 1.2,
			.MaxHealth = 0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.2,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobSpecialMutation = 0
		}
	},
	// runner zombie
	[MOB_SPAWN_PARAM_RUNNER]
	{
		.Cost = 10,
		.MinRound = 0,
		.CooldownTicks = 0,
		.Probability = 0.2,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Runner",
		.Config = {
			.MobType = MOB_NORMAL,
			.Xp = 15,
			.Bangles = ZOMBIE_BANGLE_HEAD_5,
			.Damage = ZOMBIE_BASE_DAMAGE * 0.6,
			.MaxDamage = ZOMBIE_BASE_DAMAGE * 0.9,
			.Speed = ZOMBIE_BASE_SPEED * 2.0,
			.MaxSpeed = ZOMBIE_BASE_SPEED * 3.0,
			.Health = ZOMBIE_BASE_HEALTH * 0.6,
			.MaxHealth = 0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobSpecialMutation = 0
		}
	},
	// normal zombie
	[MOB_SPAWN_PARAM_NORMAL]
	{
		.Cost = 5,
		.MinRound = 0,
		.CooldownTicks = 0,
		.Probability = 1.0,
		.SpawnType = SPAWN_TYPE_SEMI_NEAR_PLAYER | SPAWN_TYPE_NEAR_PLAYER,
		.Name = "Zombie",
		.Config = {
			.MobType = MOB_NORMAL,
			.Xp = 10,
			.Bangles = ZOMBIE_BANGLE_HEAD_1 | ZOMBIE_BANGLE_TORSO_1,
			.Damage = ZOMBIE_BASE_DAMAGE * 1.0,
			.MaxDamage = ZOMBIE_BASE_DAMAGE * 1.3,
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxSpeed = ZOMBIE_BASE_SPEED * 2.0,
			.Health = ZOMBIE_BASE_HEALTH * 1.0,
			.MaxHealth = 0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
      .CollRadius = ZOMBIE_BASE_COLL_RADIUS * 1.0,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2,
			.MobSpecialMutation = 0
		}
	},
};

const int defaultSpawnParamsCount = sizeof(defaultSpawnParams) / sizeof(struct MobSpawnParams);
int defaultSpawnParamsCooldowns[MOB_SPAWN_PARAM_COUNT];

//--------------------------------------------------------------------------
struct GuberMoby* getGuber(Moby* moby)
{
	if (moby->OClass == UPGRADE_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	if (moby->OClass == DROP_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	if (moby->OClass == ZOMBIE_MOBY_OCLASS && moby->PVar)
		return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int handleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event || !isInGame())
		return 0;

	switch (moby->OClass)
	{
		case ZOMBIE_MOBY_OCLASS: return mobHandleEvent(moby, event);
		case DROP_MOBY_OCLASS: return dropHandleEvent(moby, event);
		case UPGRADE_MOBY_OCLASS: return upgradeHandleEvent(moby, event);
	}

	return 0;
}

//--------------------------------------------------------------------------
void drawRoundMessage(const char * message, float scale, int yPixelsOffset)
{
	int fw = gfxGetFontWidth(message, -1, scale);
	float x = 0.5;
	float y = 0.16;

	// draw message
	y *= SCREEN_HEIGHT;
	x *= SCREEN_WIDTH;
	gfxScreenSpaceText(x, y + 5 + yPixelsOffset, scale, scale * 1.5, 0x80FFFFFF, message, -1, 1);
}

//--------------------------------------------------------------------------
void openWeaponsMenu(int localPlayerIndex)
{
	((void (*)(int, int))0x00544748)(localPlayerIndex, 4);
	((void (*)(int, int))0x005415c8)(localPlayerIndex, 2);

	((void (*)(int))0x00543e10)(localPlayerIndex); // hide player
	int s = ((int (*)(int))0x00543648)(localPlayerIndex); // get hud enter state
	((void (*)(int))0x005c2370)(s); // swapto
}

//--------------------------------------------------------------------------
Moby * mobyGetRandomHealthbox(void)
{
	Moby* results[10];
	int count = 0;
	Moby* start = mobyListGetStart();
	Moby* end = mobyListGetEnd();

	while (start < end && count < 10) {
		if (start->OClass == MOBY_ID_HEALTH_BOX_MULT && !mobyIsDestroyed(start))
			results[count++] = start;

		++start;
	}

	if (count > 0)
		return results[rand(count)];

	return NULL;
}

//--------------------------------------------------------------------------
void gatePayToken(Moby* moby)
{
	// create event
	GuberEvent * guberEvent = upgradeCreateEvent(moby, GATE_EVENT_PAY_TOKEN);
}

//--------------------------------------------------------------------------
int gateCanInteract(Moby* gate, VECTOR point)
{
  VECTOR delta, gateTangent, gateClosestToPoint;
  if (!gate || !gate->PVar)
    return 0;

  struct GatePVar* pvars = (struct GatePVar*)gate->PVar;

  // get delta from center of gate to point
  vector_subtract(delta, point, gate->Position);

  // outside vertical range
  if (fabsf(delta[2]) > (pvars->Height/2))
    return 0;

  // get closest point on gate to point
  vector_subtract(gateTangent, pvars->To, pvars->From);
  float gateLength = vector_length(gateTangent);
  vector_scale(gateTangent, gateTangent, 1 / gateLength);

  vector_subtract(delta, point, pvars->From);
  float dot = vector_innerproduct_unscaled(delta, gateTangent);
  if (dot < -GATE_INTERACT_CAP_RADIUS)
    return 0;

  if (dot > (gateLength + GATE_INTERACT_CAP_RADIUS))
    return 0;

  vector_scale(gateClosestToPoint, gateTangent, dot);
  vector_add(gateClosestToPoint, gateClosestToPoint, pvars->From);
  vector_subtract(delta, point, gateClosestToPoint);
  vectorProjectOnHorizontal(delta, delta);
  return vector_sqrmag(delta) < (GATE_INTERACT_RADIUS*GATE_INTERACT_RADIUS);
}

//--------------------------------------------------------------------------
Player * playerGetRandom(void)
{
	int r = rand(GAME_MAX_PLAYERS);
	int i = 0, c = 0;
	Player ** players = playerGetAll();

	do {
		if (players[i] && !playerIsDead(players[i]) && players[i]->Health > 0)
			++c;
		
		++i;
		if (i == GAME_MAX_PLAYERS) {
			if (c == 0)
				return NULL;
			i = 0;
		}
	} while (c < r);

	return players[i-1];
}

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
	int i;
  VECTOR t;

  // spawn at player start
  for (i = 0; i < BAKED_SPAWNPOINT_COUNT; ++i) {
    if (BakedConfig.BakedSpawnPoints[i].Type == BAKED_SPAWNPOINT_PLAYER_START) {

      memcpy(outPos, BakedConfig.BakedSpawnPoints[i].Position, 12);
      memcpy(outRot, BakedConfig.BakedSpawnPoints[i].Rotation, 12);

      float theta = player->PlayerId / (float)GAME_MAX_PLAYERS;
      vector_fromyaw(t, theta * MATH_PI * 2);
      vector_scale(t, t, 2.5);
      vector_add(outPos, outPos, t);
      return;
    }
  }

  // pass to base if we don't have a player start
  playerGetSpawnpoint(player, outPos, outRot, firstRes);
}

//--------------------------------------------------------------------------
int spawnPointGetNearestTo(VECTOR point, VECTOR out, float minDist)
{
	VECTOR t;
	int spCount = spawnPointGetCount();
	int i;
	float bestPointDist = 100000;
	float minDistSqr = minDist * minDist;

	for (i = 0; i < spCount; ++i) {
		SpawnPoint* sp = spawnPointGet(i);
		vector_subtract(t, (float*)&sp->M0[12], point);
		float d = vector_sqrmag(t);
		if (d >= minDistSqr) {
			// randomize order a little
			d += randRange(0, 15 * 15);
			if (d < bestPointDist) {
				vector_copy(out, (float*)&sp->M0[12]);
				vector_fromyaw(t, randRadian());
				vector_scale(t, t, 3);
				vector_add(out, out, t);
				bestPointDist = d;
			}
		}
	}

	return spCount > 0;
}

//--------------------------------------------------------------------------
int spawnGetRandomPoint(VECTOR out, struct MobSpawnParams* mob) {
	// larger the map, better chance mob spawns near you
	// harder difficulty, better chance mob spawns near you
	float r = randRange(0, 1) / (BakedConfig.MapSize * State.Difficulty);

#if QUICK_SPAWN
	r = MOB_SPAWN_NEAR_PLAYER_PROBABILITY;
#endif

	// spawn on player
	if (r <= MOB_SPAWN_AT_PLAYER_PROBABILITY && (mob->SpawnType & SPAWN_TYPE_ON_PLAYER)) {
		Player * targetPlayer = playerGetRandom();
		if (targetPlayer) {
			vector_write(out, randVectorRange(-1, 1));
			out[2] = 0;
			vector_add(out, out, targetPlayer->PlayerPosition);
			return 1;
		}
	}

	// spawn near healthbox
	if (r <= MOB_SPAWN_NEAR_HEALTHBOX_PROBABILITY && (mob->SpawnType & SPAWN_TYPE_NEAR_HEALTHBOX)) {
		Moby* hb = mobyGetRandomHealthbox();
		if (hb)
			return spawnPointGetNearestTo(hb->Position, out, 10);
	}

	// spawn near player
	if (r <= MOB_SPAWN_NEAR_PLAYER_PROBABILITY && (mob->SpawnType & SPAWN_TYPE_NEAR_PLAYER)) {
		Player * targetPlayer = playerGetRandom();
		if (targetPlayer)
			return spawnPointGetNearestTo(targetPlayer->PlayerPosition, out, 5);
	}

	// spawn semi near player
	if (r <= MOB_SPAWN_SEMI_NEAR_PLAYER_PROBABILITY && (mob->SpawnType & SPAWN_TYPE_SEMI_NEAR_PLAYER)) {
		Player * targetPlayer = playerGetRandom();
		if (targetPlayer)
			return spawnPointGetNearestTo(targetPlayer->PlayerPosition, out, 20);
	}

	// spawn randomly
	int spIdx = rand(spawnPointGetCount());
	while (!spawnPointIsPlayer(spIdx))
		spIdx = rand(spawnPointGetCount());
	SpawnPoint* sp = spawnPointGet(spIdx);
	if (sp) {
		vector_copy(out, &sp->M0[12]);
		return 1;
	}

	return 0;
}

//--------------------------------------------------------------------------
int spawnCanSpawnMob(struct MobSpawnParams* mob)
{
	return State.RoundNumber >= mob->MinRound && mob->Cost <= State.RoundBudget && mob->Probability > 0;
}

//--------------------------------------------------------------------------
struct MobSpawnParams* spawnGetRandomMobParams(int * mobIdx)
{
	int i,j;

	if (State.RoundIsSpecial) {
		struct SurvivalSpecialRoundParam* params = &specialRoundParams[State.RoundSpecialIdx];
		i = rand(params->SpawnParamCount);
		if (mobIdx)
			*mobIdx = (int)params->SpawnParamIds[i];
		return &defaultSpawnParams[(int)params->SpawnParamIds[i]];
	}

	if (State.RoundBudget < State.MinMobCost)
		return NULL;

	for (i = 0; i < defaultSpawnParamsCount; ++i) {
		struct MobSpawnParams* mob = &defaultSpawnParams[i];
		if (spawnCanSpawnMob(mob) && randRange(0,1) <= mob->Probability) {
			if (mobIdx)
				*mobIdx = i;
			return mob;
		}
	}

	for (i = 0; i < defaultSpawnParamsCount; ++i) {
		struct MobSpawnParams* mob = &defaultSpawnParams[i];
		DPRINTF("CANT SPAWN budget:%ld round:%d mobIdx:%d mobMinRound:%d mobCost:%d mobProb:%f\n", State.RoundBudget, State.RoundNumber, i, mob->MinRound, mob->Cost, mob->Probability);
	}
	
	return NULL;
}

//--------------------------------------------------------------------------
int spawnRandomMob(void) {
	VECTOR sp;
	int mobIdx = 0;
	struct MobSpawnParams* mob = spawnGetRandomMobParams(&mobIdx);
	if (mob) {
		int cost = mob->Cost;
		char bakedSpecialMutation = mob->Config.MobSpecialMutation;

		// skip if cooldown not 0
		int cooldown = defaultSpawnParamsCooldowns[mobIdx];
		if (cooldown > 0) {
			return 1;
		}
		else {
			defaultSpawnParamsCooldowns[mobIdx] = mob->CooldownTicks;
		}

		// try and get special mutation
		float r = randRange(0, 1);
		if (!bakedSpecialMutation && r < ZOMBIE_SPECIAL_MUTATION_PROBABILITY) {
			int newCost = cost + ZOMBIE_SPECIAL_MUTATION_BASE_COST + (ZOMBIE_SPECIAL_MUTATION_REL_COST * mob->Cost);
			if (newCost < State.RoundBudget) {
				mob->Config.MobSpecialMutation = rand(MOB_SPECIAL_MUTATION_COUNT - 1) + 1;
				cost = newCost;
				DPRINTF("spawning %s with special mutation %d\n", mob->Name, mob->Config.MobSpecialMutation);
			}
		}

		// try and spawn
		if (spawnGetRandomPoint(sp, mob)) {
			if (mobCreate(sp, 0, -1, &mob->Config)) {
				mob->Config.MobSpecialMutation = bakedSpecialMutation;
				State.RoundBudget -= cost;
				return 1;
			} else { DPRINTF("failed to create mob\n"); }
		} else { DPRINTF("failed to get random spawn point\n"); }

		// reset special mutation back to original if failed to spawn
		mob->Config.MobSpecialMutation = bakedSpecialMutation;
	} else { DPRINTF("failed to get random mob params\n"); }

	// no more budget left
	return 0;
}

//--------------------------------------------------------------------------
int getMinMobCost(void) {
	int i;
	int cost = defaultSpawnParams[0].Cost;

	for (i = 1; i < defaultSpawnParamsCount; ++i) {
		if (defaultSpawnParams[i].Cost < cost)
			cost = defaultSpawnParams[i].Cost;
	}

	return cost;
}

//--------------------------------------------------------------------------
void customBangelizeWeapons(Moby* weaponMoby, int weaponId, int weaponLevel)
{
	switch (weaponId)
	{
		case WEAPON_ID_VIPERS:
		{
			weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? 0 : 1;
			break;
		}
		case WEAPON_ID_MAGMA_CANNON:
		{
			weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 4 : 3) : 0x31;
			break;
		}
		case WEAPON_ID_ARBITER:
		{
			weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 3 : 1) : 0xC;
			break;
		}
		case WEAPON_ID_FUSION_RIFLE:
		{
			weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 2 : 1) : 6;
			break;
		}
		case WEAPON_ID_MINE_LAUNCHER:
		{
			weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 0xC : 0) : 0xF;
			break;
		}
		case WEAPON_ID_B6:
		{
			weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 6 : 4) : 7;
			break;
		}
		case WEAPON_ID_OMNI_SHIELD:
		{
			weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 0xC : 1) : 0xF;
			break;
		}
		case WEAPON_ID_FLAIL:
		{
			if (weaponMoby->PVar) {
				weaponMoby = *(Moby**)((u32)weaponMoby->PVar + 0x33C);
				if (weaponMoby)
					weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 3 : 1) : 0x1F;
			}
			break;
		}
	}
}

//--------------------------------------------------------------------------
// TODO: Move this into the code segment, overwriting GuiMain_GetGadgetVersionName at (0x00541850)
char * customGetGadgetVersionName(int localPlayerIndex, int weaponId, int showWeaponLevel, int capitalize, int minLevel)
{
	Player* p = playerGetFromSlot(localPlayerIndex);
	char* buf = (char*)(0x2F9D78 + localPlayerIndex*0x40);
	short* gadgetDef = ((short* (*)(int, int))0x00627f48)(weaponId, 0);
	int level = 0;
	int strIdOffset = capitalize ? 3 : 4;
	if (p && p->GadgetBox) {
		level = p->GadgetBox->Gadgets[weaponId].Level;
	}

	if (level >= 9)
		strIdOffset = capitalize ? 12 : 10;

	char* str = uiMsgString(gadgetDef[strIdOffset]);

	if (level < 9 && level >= minLevel) {
		sprintf(buf, "%s V%d", str, level+1);
		return buf;
	} else {
		return str;
	}
}

//--------------------------------------------------------------------------
void customMineMobyUpdate(Moby* moby)
{
	// handle auto destructing v10 child mines after N seconds
	// and auto destroy v10 child mines if they came from a remote client
	u32 pvar = (u32)moby->PVar;
	if (pvar) {
		int mLayer = *(int*)(pvar + 0x200);
		Player * p = playerGetAll()[*(short*)(pvar + 0xC0)];
		int createdLocally = p && p->IsLocal;
		int gameTime = gameGetTime();
		int timeCreated = *(int*)(&moby->Rotation[3]);

		// set initial time created
		if (timeCreated == 0) {
			timeCreated = gameTime;
			*(int*)(&moby->Rotation[3]) = timeCreated;
		}

		// don't spawn child mines if mine was created by remote client
		if (!createdLocally) {
			*(int*)(pvar + 0x200) = 2;
		} else if (p && mLayer > 0 && (gameTime - timeCreated) > (5 * TIME_SECOND)) {
			*(int*)(&moby->Rotation[3]) = gameTime;
			((void (*)(Moby*, Player*, int))0x003C90C0)(moby, p, 0);
			return;
		}
	}
	
	// call base
	((void (*)(Moby*))0x003C6C28)(moby);
}

//--------------------------------------------------------------------------
short playerGetWeaponAmmo(Player* player, int weaponId)
{
	return ((short (*)(GadgetBox*, int))0x00626FB8)(player->GadgetBox, weaponId);
}

//--------------------------------------------------------------------------
void onPlayerUpgradeWeapon(int playerId, int weaponId, int level, int alphaMod)
{
	int i;
	Player* p = playerGetAll()[playerId];
	State.PlayerStates[playerId].State.AlphaMods[alphaMod]++;
	if (!p)
		return;

	GadgetBox* gBox = p->GadgetBox;
	playerGiveWeapon(p, weaponId, level);
	gBox->Gadgets[weaponId].Level = level;
	gBox->Gadgets[weaponId].Ammo = playerGetWeaponAmmo(p, weaponId);
	if (p->Gadgets[0].pMoby && p->Gadgets[0].id == weaponId)
		customBangelizeWeapons(p->Gadgets[0].pMoby, weaponId, level);

	if (p->IsLocal) {
		gBox->ModBasic[alphaMod-1]++;
		char* a = uiMsgString(0x2400);
		sprintf(a, "Got %s", ALPHA_MODS[alphaMod]);
		((void (*)(int, int, int))0x0054ea30)(p->LocalPlayerIndex, 0x2400, 0);
	}
	
	// stats
	int wepSlotId = weaponIdToSlot(weaponId);
	if (level > State.PlayerStates[playerId].State.BestWeaponLevel[wepSlotId])
		State.PlayerStates[playerId].State.BestWeaponLevel[wepSlotId] = level;

	// play upgrade sound
	playUpgradeSound(p);

	// set exp bar to max
	if (level == VENDOR_MAX_WEAPON_LEVEL)
		gBox->Gadgets[weaponId].Experience = level;

#if LOG_STATS2
	DPRINTF("%d (%08X) weapon %d upgraded to %d\n", playerId, (u32)p, weaponId, level);
#endif
}

//--------------------------------------------------------------------------
int onPlayerUpgradeWeaponRemote(void * connection, void * data)
{
	SurvivalWeaponUpgradeMessage_t * message = (SurvivalWeaponUpgradeMessage_t*)data;
	onPlayerUpgradeWeapon(message->PlayerId, message->WeaponId, message->Level, message->Alphamod);

	return sizeof(SurvivalWeaponUpgradeMessage_t);
}

//--------------------------------------------------------------------------
void playerUpgradeWeapon(Player* player, int weaponId)
{
	SurvivalWeaponUpgradeMessage_t message;
	GadgetBox* gBox = player->GadgetBox;

	// send out
	message.PlayerId = player->PlayerId;
	message.WeaponId = weaponId;
	message.Level = gBox->Gadgets[weaponId].Level + 1;
	message.Alphamod = rand(6) + 1;
	if (message.Alphamod == ALPHA_MOD_XP)
		message.Alphamod++;
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_WEAPON_UPGRADE, sizeof(SurvivalWeaponUpgradeMessage_t), &message);

	// set locally
	onPlayerUpgradeWeapon(message.PlayerId, message.WeaponId, message.Level, message.Alphamod);
}

//--------------------------------------------------------------------------
void onPlayerRevive(int playerId, int fromPlayerId)
{
	if (fromPlayerId >= 0)
		State.PlayerStates[fromPlayerId].State.Revives++;
	
	State.PlayerStates[playerId].IsDead = 0;
	State.PlayerStates[playerId].State.TimesRevived++;
	State.PlayerStates[playerId].State.TimesRevivedSinceLastFullDeath++;
	Player* player = playerGetAll()[playerId];
	if (!player)
		return;

	// backup current position/rotation
	VECTOR deadPos, deadPosDown, deadRot;
	vector_copy(deadPos, player->PlayerPosition);
	vector_copy(deadPosDown, player->PlayerPosition);
	vector_copy(deadRot, player->PlayerRotation);
  deadPos[2] += 0.5;
  deadPosDown[2] -= 1;

	// respawn
	PlayerVTable* vtable = playerGetVTable(player);
	if (vtable)
		vtable->UpdateState(player, PLAYER_STATE_IDLE, 1, 1, 1);
  getResurrectPoint(player, player->PlayerPosition, player->PlayerRotation, 0);
  playerSetPosRot(player, player->PlayerPosition, player->PlayerRotation);
	playerSetHealth(player, player->MaxHealth);

  // only reset back to dead pos if player died on ground
  if (CollLine_Fix(deadPos, deadPosDown, 2, player->PlayerMoby, 0)) {
    int colId = CollLine_Fix_GetHitCollisionId() & 0xF;
    if (colId == 0xF || colId == 0x7 || colId == 0x9 || colId == 0xA)
	    playerSetPosRot(player, deadPos, deadRot);
  }

	player->timers.acidTimer = 0;
	player->timers.freezeTimer = 1; // triggers the game to handle resetting movement speed on 0
	player->timers.postHitInvinc = TPS * 3;
}

//--------------------------------------------------------------------------
int onPlayerReviveRemote(void * connection, void * data)
{
	SurvivalReviveMessage_t * message = (SurvivalReviveMessage_t*)data;
	onPlayerRevive(message->PlayerId, message->FromPlayerId);

	return sizeof(SurvivalReviveMessage_t);
}

//--------------------------------------------------------------------------
void playerRevive(Player* player, int fromPlayerId)
{
	SurvivalReviveMessage_t message;

	// send out
	message.PlayerId = player->PlayerId;
	message.FromPlayerId = fromPlayerId;
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_REVIVE_PLAYER, sizeof(SurvivalReviveMessage_t), &message);

	// set locally
	onPlayerRevive(message.PlayerId, message.FromPlayerId);
}

//--------------------------------------------------------------------------
void onSetPlayerDead(int playerId, char isDead)
{
	State.PlayerStates[playerId].IsDead = isDead;
	State.PlayerStates[playerId].ReviveCooldownTicks = isDead ? 60 * 60 : 0;
	DPRINTF("%d died (%d)\n", playerId, isDead);
}

//--------------------------------------------------------------------------
int onSetPlayerDeadRemote(void * connection, void * data)
{
	SurvivalSetPlayerDeadMessage_t * message = (SurvivalSetPlayerDeadMessage_t*)data;
	onSetPlayerDead(message->PlayerId, message->IsDead);

	return sizeof(SurvivalSetPlayerDeadMessage_t);
}

//--------------------------------------------------------------------------
void setPlayerDead(Player* player, char isDead)
{
	SurvivalSetPlayerDeadMessage_t message;

	// send out
	message.PlayerId = player->PlayerId;
	message.IsDead = isDead;
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_DIED, sizeof(SurvivalSetPlayerDeadMessage_t), &message);

	// set locally
	onSetPlayerDead(message.PlayerId, message.IsDead);
}

//--------------------------------------------------------------------------
void onSetPlayerWeaponMods(int playerId, int weaponId, u8* mods)
{
	int i;
	Player* p = playerGetAll()[playerId];
	if (!p)
		return;

	GadgetBox* gBox = p->GadgetBox;
	for (i = 0; i < 10; ++i)
		gBox->Gadgets[weaponId].AlphaMods[i] = mods[i];
		
	DPRINTF("%d set %d mods\n", playerId, weaponId);
}

//--------------------------------------------------------------------------
int onSetPlayerWeaponModsRemote(void * connection, void * data)
{
	SurvivalSetWeaponModsMessage_t * message = (SurvivalSetWeaponModsMessage_t*)data;
	onSetPlayerWeaponMods(message->PlayerId, message->WeaponId, message->Mods);

	return sizeof(SurvivalSetPlayerDeadMessage_t);
}

//--------------------------------------------------------------------------
void setPlayerWeaponMods(Player* player, int weaponId, int* mods)
{
	int i;
	SurvivalSetWeaponModsMessage_t message;

	// send out
	message.PlayerId = player->PlayerId;
	message.WeaponId = (u8)weaponId;
	for (i = 0; i < 10; ++i)
		message.Mods[i] = (u8)mods[i];
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_WEAPON_MODS, sizeof(SurvivalSetWeaponModsMessage_t), &message);

	// set locally
	onSetPlayerWeaponMods(message.PlayerId, message.WeaponId, message.Mods);
}

//--------------------------------------------------------------------------
void onSetPlayerStats(int playerId, struct SurvivalPlayerState* stats)
{
	memcpy(&State.PlayerStates[playerId].State, stats, sizeof(struct SurvivalPlayerState));
}

//--------------------------------------------------------------------------
int onSetPlayerStatsRemote(void * connection, void * data)
{
	SurvivalSetPlayerStatsMessage_t * message = (SurvivalSetPlayerStatsMessage_t*)data;
	onSetPlayerStats(message->PlayerId, &message->Stats);

	return sizeof(SurvivalSetPlayerStatsMessage_t);
}

//--------------------------------------------------------------------------
void sendPlayerStats(int playerId)
{
	int i;
	SurvivalSetPlayerStatsMessage_t message;

	// send out
	message.PlayerId = playerId;
	memcpy(&message.Stats, &State.PlayerStates[playerId].State, sizeof(struct SurvivalPlayerState));
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_STATS, sizeof(SurvivalSetPlayerStatsMessage_t), &message);
}

//--------------------------------------------------------------------------
void onSetPlayerDoublePoints(char isActive[GAME_MAX_PLAYERS], int timeOfDoublePoints[GAME_MAX_PLAYERS])
{
	int i;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		State.PlayerStates[i].IsDoublePoints = isActive[i];
		State.PlayerStates[i].TimeOfDoublePoints = timeOfDoublePoints[i];
	}
}

//--------------------------------------------------------------------------
int onSetPlayerDoublePointsRemote(void * connection, void * data)
{
	SurvivalSetPlayerDoublePointsMessage_t * message = (SurvivalSetPlayerDoublePointsMessage_t*)data;
	onSetPlayerDoublePoints(message->IsActive, message->TimeOfDoublePoints);

	return sizeof(SurvivalSetPlayerDoublePointsMessage_t);
}

//--------------------------------------------------------------------------
void sendDoublePoints(void)
{
	int i;
	SurvivalSetPlayerDoublePointsMessage_t message;

	// build active list
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		message.IsActive[i] = State.PlayerStates[i].IsDoublePoints;
		message.TimeOfDoublePoints[i] = State.PlayerStates[i].TimeOfDoublePoints;
	}

	// send out
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_DOUBLE_POINTS, sizeof(SurvivalSetPlayerDoublePointsMessage_t), &message);
}

//--------------------------------------------------------------------------
void setDoublePointsForTeam(int team, int isActive)
{
	int i;
	SurvivalSetPlayerDoublePointsMessage_t message;
	Player** players = playerGetAll();

	// build active list
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player* p = players[i];
		if (p && p->Team == team) {
			State.PlayerStates[i].TimeOfDoublePoints = gameGetTime();
			State.PlayerStates[i].IsDoublePoints = isActive;
		}

		message.IsActive[i] = State.PlayerStates[i].IsDoublePoints;
		message.TimeOfDoublePoints[i] = State.PlayerStates[i].TimeOfDoublePoints;
	}

	// send out
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_DOUBLE_POINTS, sizeof(SurvivalSetPlayerDoublePointsMessage_t), &message);

	// locally
	onSetPlayerDoublePoints(message.IsActive, message.TimeOfDoublePoints);
}

//--------------------------------------------------------------------------
void onSetFreeze(char isActive)
{
	State.Freeze = isActive;
	if (isActive)
		State.TimeOfFreeze = gameGetTime();
}

//--------------------------------------------------------------------------
int onSetFreezeRemote(void * connection, void * data)
{
	SurvivalSetFreezeMessage_t * message = (SurvivalSetFreezeMessage_t*)data;
	onSetFreeze(message->IsActive);

	return sizeof(SurvivalSetFreezeMessage_t);
}

//--------------------------------------------------------------------------
void setFreeze(int isActive)
{
	int i;
	SurvivalSetFreezeMessage_t message;

	// send out
	message.IsActive = isActive;
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_FREEZE, sizeof(SurvivalSetFreezeMessage_t), &message);

	// locally
	onSetFreeze(isActive);
}

//--------------------------------------------------------------------------
int canUpgradeWeapon(Player * player, enum WEAPON_IDS weaponId) {
	if (!player)
		return 0;

	GadgetBox* gBox = player->GadgetBox;

	switch (weaponId)
	{
		case WEAPON_ID_VIPERS:
		case WEAPON_ID_MAGMA_CANNON:
		case WEAPON_ID_ARBITER:
		case WEAPON_ID_FUSION_RIFLE:
		case WEAPON_ID_MINE_LAUNCHER:
		case WEAPON_ID_B6:
		case WEAPON_ID_OMNI_SHIELD:
		case WEAPON_ID_FLAIL:
			return gBox->Gadgets[(int)weaponId].Level >= 0 && gBox->Gadgets[(int)weaponId].Level < VENDOR_MAX_WEAPON_LEVEL;
		default: return 0;
	}
}

//--------------------------------------------------------------------------
int getUpgradeCost(Player * player, enum WEAPON_IDS weaponId) {
	if (!player)
		return 0;

	GadgetBox* gBox = player->GadgetBox;
	int level = gBox->Gadgets[(int)weaponId].Level;
	if (level < 0 || level >= VENDOR_MAX_WEAPON_LEVEL)
		return 0;
		
	// determine discount rate
	float rate = clamp(1 - (0.025 * State.PlayerStates[player->PlayerId].State.Upgrades[UPGRADE_VENDOR]), 0, 1);

	return ceilf(UPGRADE_COST[level] * rate);
}

//--------------------------------------------------------------------------
void respawnDeadPlayers(void) {
	int i;
	Player** players = playerGetAll();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		if (p && p->IsLocal && playerIsDead(p)) {
			State.PlayerStates[i].State.TimesRevivedSinceLastFullDeath = 0;
			playerRespawn(p);
		}
		
		State.PlayerStates[i].IsDead = 0;
		State.PlayerStates[i].ReviveCooldownTicks = 0;
	}
}

//--------------------------------------------------------------------------
int getPlayerReviveCost(Player * fromPlayer, Player * player) {
	if (!player || !fromPlayer)
		return 0;

	GameData * gameData = gameGetData();

	// determine discount rate
	float rate = clamp(1 - (0.05 * State.PlayerStates[fromPlayer->PlayerId].State.Upgrades[UPGRADE_MEDIC]), 0, 1);

	return ceilf(rate * ((State.PlayerStates[player->PlayerId].State.TimesRevivedSinceLastFullDeath + 1) * (PLAYER_BASE_REVIVE_COST + (PLAYER_REVIVE_COST_PER_PLAYER * State.ActivePlayerCount) + (PLAYER_REVIVE_COST_PER_ROUND * State.RoundNumber))));
}

//--------------------------------------------------------------------------
void processPlayer(int pIndex) {
	VECTOR t;
	int i = 0, localPlayerIndex, heldWeapon, hasMessage = 0;
	char strBuf[32];
	Player** players = playerGetAll();
	Player* player = players[pIndex];
	struct SurvivalPlayer * playerData = &State.PlayerStates[pIndex];

	if (!player || !player->PlayerMoby)
		return;

	int isDeadState = playerIsDead(player) || player->Health == 0;
	int actionCooldownTicks = decTimerU8(&playerData->ActionCooldownTicks);
	int messageCooldownTicks = decTimerU8(&playerData->MessageCooldownTicks);
	int reviveCooldownTicks = decTimerU16(&playerData->ReviveCooldownTicks);
	if (playerData->IsDead && reviveCooldownTicks > 0 && playerGetNumLocals() == 1) {
		Player* localPlayer = playerGetFromSlot(0);
		if (localPlayer->Team == player->Team) {
			int x,y;
			VECTOR pos = {0,0,1,0};
			vector_add(pos, player->PlayerPosition, pos);
			if (gfxWorldSpaceToScreenSpace(pos, &x, &y)) {
				sprintf(strBuf, "\x1c  %02d", reviveCooldownTicks/60);
				gfxScreenSpaceText(x, y, 0.75, 0.75, 0x80FFFFFF, strBuf, -1, 4);
			}
		}
	}

	// set max health
	player->MaxHealth = 50 + (5 * State.PlayerStates[pIndex].State.Upgrades[UPGRADE_HEALTH]);

	// set speed
	player->Speed = 1 + (0.05 * State.PlayerStates[pIndex].State.Upgrades[UPGRADE_SPEED]);
	//DPRINTF("speed:%f\n", player->Speed);

	// adjust speed of chargeboot stun
	if (player->PlayerState == 121) {
		*(float*)((u32)player + 0x25C4) = 1.0 / State.Difficulty;
	} else {
		*(float*)((u32)player + 0x25C4) = 1.0;
	}
	
	if (player->IsLocal) {
		
		GadgetBox* gBox = player->GadgetBox;
		localPlayerIndex = player->LocalPlayerIndex;
		heldWeapon = player->WeaponHeldId;

	  // set max xp
		u32 xp = playerData->State.XP;
		u32 nextXp = getXpForNextToken(playerData->State.TotalTokens);
		setPlayerEXP(localPlayerIndex, xp / (float)nextXp);

		// find closest mob
		playerData->MinSqrDistFromMob = 1000000;
		playerData->MaxSqrDistFromMob = 0;
		GuberMoby* gm = guberMobyGetFirst();
		
		while (gm)
		{
			if (gm->Moby && gm->Moby->OClass == ZOMBIE_MOBY_OCLASS)
			{
				vector_subtract(t, player->PlayerPosition, gm->Moby->Position);
				float sqrDist = vector_sqrmag(t);
				if (sqrDist < playerData->MinSqrDistFromMob)
					playerData->MinSqrDistFromMob = sqrDist;
			  if (sqrDist > playerData->MaxSqrDistFromMob)
					playerData->MaxSqrDistFromMob = sqrDist;
			}
			
			gm = (GuberMoby*)gm->Guber.Prev;
		}

		// handle death
		if (isDeadState && !playerData->IsDead) {
			setPlayerDead(player, 1);
		} else if (playerData->IsDead && !isDeadState) {
			setPlayerDead(player, 0);
		}

		// force to last good position
		// if (isDeadState && player->timers.state > TPS) {
		// 	vector_subtract(t, player->PlayerPosition, player->Ground.lastGoodPos);
		// 	if (vector_sqrmag(t) > 1) {
		// 		playerSetPosRot(player, player->Ground.lastGoodPos, player->PlayerRotation);
		// 		player->Health = 0;
		// 		player->PlayerState = PLAYER_STATE_DEATH;
		// 	}
		// }

		// set experience to min of level and max level 
		for (i = WEAPON_ID_VIPERS; i <= WEAPON_ID_FLAIL; ++i) {
			if (gBox->Gadgets[i].Level >= 0) {
				gBox->Gadgets[i].Experience = gBox->Gadgets[i].Level < VENDOR_MAX_WEAPON_LEVEL ? gBox->Gadgets[i].Level : VENDOR_MAX_WEAPON_LEVEL;
			}
		}

		// decrement flail ammo while spinning flail
		GameOptions* gameOptions = gameGetOptions();
		if (!gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo
			&& player->PlayerState == PLAYER_STATE_FLAIL_ATTACK
			&& player->timers.state >= 60) {
				if (gBox->Gadgets[WEAPON_ID_FLAIL].Ammo > 0) {
					if ((player->timers.state % 60) == 0) {
						gBox->Gadgets[WEAPON_ID_FLAIL].Ammo -= 1;
					}
				} else {
					PlayerVTable* vtable = playerGetVTable(player);
					if (vtable && vtable->UpdateState)
						vtable->UpdateState(player, PLAYER_STATE_IDLE, 1, 1, 1);
				}
		}

		//
		if (actionCooldownTicks > 0 || player->timers.noInput)
			return;

		// handle closing weapons menu
		if (playerData->IsInWeaponsMenu) {
			for (i = 0; i < sizeof(UPGRADEABLE_WEAPONS)/sizeof(u8); ++i)
				setPlayerWeaponMods(player, UPGRADEABLE_WEAPONS[i], gBox->Gadgets[UPGRADEABLE_WEAPONS[i]].AlphaMods);
			playerData->IsInWeaponsMenu = 0;
		}

		// handle vendor logic
		if (State.Vendor) {

			// check player is holding upgradable weapon
			if (canUpgradeWeapon(player, heldWeapon)) {

				// check distance
				vector_subtract(t, player->PlayerPosition, State.Vendor->Position);
				if (vector_sqrmag(t) < (WEAPON_VENDOR_MAX_DIST * WEAPON_VENDOR_MAX_DIST)) {
					
					// get upgrade cost
					int cost = getUpgradeCost(player, heldWeapon);

					// draw help popup
					sprintf(LocalPlayerStrBuffer[localPlayerIndex], SURVIVAL_UPGRADE_MESSAGE, cost);
					uiShowPopup(localPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
					hasMessage = 1;
					playerData->MessageCooldownTicks = 2;

					// handle purchase
					if (playerData->State.Bolts >= cost) {

						// handle pad input
						if (padGetButtonDown(localPlayerIndex, PAD_CIRCLE) > 0) {
							playerData->State.Bolts -= cost;
							playerUpgradeWeapon(player, heldWeapon);
							uiShowPopup(localPlayerIndex, uiMsgString(0x2330));
              playPaidSound(player);
							playerData->ActionCooldownTicks = WEAPON_UPGRADE_COOLDOWN_TICKS;
						}
					}
				}
			}
		}

		// handle big al logic
		if (State.BigAl) {

			// check distance
			vector_subtract(t, player->PlayerPosition, State.BigAl->Position);
			if (vector_sqrmag(t) < (BIG_AL_MAX_DIST * BIG_AL_MAX_DIST)) {
				
				// draw help popup
				uiShowPopup(localPlayerIndex, SURVIVAL_OPEN_WEAPONS_MESSAGE);
				hasMessage = 1;
				playerData->MessageCooldownTicks = 2;

				if (padGetButtonDown(localPlayerIndex, PAD_TRIANGLE) > 0) {
					openWeaponsMenu(localPlayerIndex);
					playerData->ActionCooldownTicks = WEAPON_MENU_COOLDOWN_TICKS;
					playerData->IsInWeaponsMenu = 1;
				}
			}
		}

		// handle upgrade logic
		for (i = 0; i < UPGRADE_COUNT; ++i) {
			Moby* upgradeMoby = State.UpgradeMobies[i];
      if (upgradeMoby) {
        if (!playerData->ActionCooldownTicks && playerData->State.Upgrades[i] < UpgradeMax[i]) {
          vector_subtract(t, player->PlayerPosition, upgradeMoby->Position);
          if (vector_sqrmag(t) < (UPGRADE_PICKUP_RADIUS * UPGRADE_PICKUP_RADIUS)) {
            
            // draw help popup
            sprintf(LocalPlayerStrBuffer[localPlayerIndex], SURVIVAL_BUY_UPGRADE_MESSAGES[i]);
            uiShowPopup(player->LocalPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
            hasMessage = 1;
            playerData->MessageCooldownTicks = 2;

            // handle pad input
            if (padGetButtonDown(localPlayerIndex, PAD_CIRCLE) > 0 && playerData->State.CurrentTokens >= UPGRADE_TOKEN_COST) {
              playerData->State.CurrentTokens -= UPGRADE_TOKEN_COST;
              playerData->ActionCooldownTicks = PLAYER_UPGRADE_COOLDOWN_TICKS;
              upgradePickup(upgradeMoby, pIndex);
              playPaidSound(player);
            }
            break;
          }
        }
      }
		}

		// handle gate logic
		for (i = 0; i < GATE_MAX_COUNT; ++i) {
			Moby* gateMoby = State.GateMobies[i];
      if (gateMoby) {
        struct GatePVar* gatePVars = (struct GatePVar*)gateMoby->PVar;
        if (!playerData->ActionCooldownTicks && gateMoby->State == GATE_STATE_ACTIVATED && gatePVars && gatePVars->Cost > 0) {
          if (gateCanInteract(gateMoby, player->PlayerPosition)) {
            
            // draw help popup
            sprintf(LocalPlayerStrBuffer[localPlayerIndex], SURVIVAL_BUY_GATE_MESSAGE, gatePVars->Cost);
            uiShowPopup(player->LocalPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
            hasMessage = 1;
            playerData->MessageCooldownTicks = 2;

            // handle pad input
            if (padGetButtonDown(localPlayerIndex, PAD_CIRCLE) > 0 && playerData->State.CurrentTokens > 0) {
              playerData->State.CurrentTokens -= 1;
              playerData->ActionCooldownTicks = PLAYER_GATE_COOLDOWN_TICKS;
              gatePayToken(gateMoby);
              playPaidSound(player);
            }
            break;
          }
        }
      }
		}

		// handle revive logic
		if (!isDeadState) {
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				if (i != pIndex && !hasMessage) {

					// ensure player exists, is dead, and is on the same team
					struct SurvivalPlayer * otherPlayerData = &State.PlayerStates[i];
					Player * otherPlayer = players[i];
					if (otherPlayer && otherPlayerData->IsDead && otherPlayerData->ReviveCooldownTicks > 0 && otherPlayer->Team == player->Team) {

						// check distance
						vector_subtract(t, player->PlayerPosition, otherPlayer->PlayerPosition);
						if (vector_sqrmag(t) < (PLAYER_REVIVE_MAX_DIST * PLAYER_REVIVE_MAX_DIST)) {
							
							// get revive cost
							int cost = getPlayerReviveCost(player, otherPlayer);

							// draw help popup
							sprintf(LocalPlayerStrBuffer[localPlayerIndex], SURVIVAL_REVIVE_MESSAGE, cost);
							uiShowPopup(localPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
							hasMessage = 1;
							playerData->MessageCooldownTicks = 2;

							// handle pad input
							if (padGetButtonDown(localPlayerIndex, PAD_DOWN) > 0 && playerData->State.Bolts >= cost) {
								playerData->State.Bolts -= cost;
								playerData->ActionCooldownTicks = PLAYER_REVIVE_COOLDOWN_TICKS;
								playerRevive(otherPlayer, player->PlayerId);
                playPaidSound(player);
							}
						}
					}
				}
			}
		}

		/*
		static int aaa = 0;
		if (padGetButtonDown(0, PAD_L1 | PAD_L3) > 0) {
			aaa += 1;
			DPRINTF("%d\n", aaa);
		}
		else if (padGetButtonDown(0, PAD_L1 | PAD_R3) > 0) {
			aaa -= 1;
			DPRINTF("%d\n", aaa);
		}
		*/

		// hide message right away
		if (!hasMessage && messageCooldownTicks > 0) {
			((void (*)(int))0x0054e5e8)(localPlayerIndex);
		}
	}
}

//--------------------------------------------------------------------------
int getRoundBonus(int roundNumber, int numPlayers)
{
	int multiplier = State.RoundIsSpecial ? ROUND_SPECIAL_BONUS_MULTIPLIER : 1;
	int bonus = State.RoundNumber * ROUND_BASE_BOLT_BONUS * numPlayers;
	if (bonus > ROUND_MAX_BOLT_BONUS)
		return ROUND_MAX_BOLT_BONUS * multiplier;

	return bonus * multiplier;
}

//--------------------------------------------------------------------------
void onSetRoundComplete(int gameTime, int boltBonus)
{
	int i;
	DPRINTF("round complete. zombies spawned %d/%d\n", State.RoundMobSpawnedCount, State.RoundMaxMobCount);

	// 
	State.RoundEndTime = 0;
	State.RoundCompleteTime = gameTime;

	// add bonus
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		if (!State.PlayerStates[i].IsDead) {
			State.PlayerStates[i].State.Bolts += boltBonus;
			State.PlayerStates[i].State.TotalBolts += boltBonus;
		}
	}

	respawnDeadPlayers();
}

//--------------------------------------------------------------------------
int onSetRoundCompleteRemote(void * connection, void * data)
{
	SurvivalRoundCompleteMessage_t * message = (SurvivalRoundCompleteMessage_t*)data;
	onSetRoundComplete(message->GameTime, message->BoltBonus);

	return sizeof(SurvivalRoundCompleteMessage_t);
}

//--------------------------------------------------------------------------
void setRoundComplete(void)
{
	SurvivalRoundCompleteMessage_t message;

	// don't allow overwriting existing outcome
	if (State.RoundCompleteTime)
		return;

	// don't allow changing outcome when not host
	if (!State.IsHost)
		return;

	// send out
	GameSettings* gameSettings = gameGetSettings();
	message.GameTime = gameGetTime();
	message.BoltBonus = getRoundBonus(State.RoundNumber, gameSettings->PlayerCount);
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_ROUND_COMPLETE, sizeof(SurvivalRoundCompleteMessage_t), &message);

	// set locally
	onSetRoundComplete(message.GameTime, message.BoltBonus);
}

//--------------------------------------------------------------------------
void onSetRoundStart(int roundNumber, int gameTime)
{
	// 
	State.RoundNumber = roundNumber;
	State.RoundEndTime = gameTime;
	State.RoundIsSpecial = ((roundNumber + 1) % ROUND_SPECIAL_EVERY) == 0;
	State.RoundSpecialIdx = (roundNumber / ROUND_SPECIAL_EVERY) % specialRoundParamsCount;

	DPRINTF("round start %d\n", roundNumber);
}

//--------------------------------------------------------------------------
int onSetRoundStartRemote(void * connection, void * data)
{
	SurvivalRoundStartMessage_t * message = (SurvivalRoundStartMessage_t*)data;
	onSetRoundStart(message->RoundNumber, message->GameTime);

	return sizeof(SurvivalRoundStartMessage_t);
}

//--------------------------------------------------------------------------
void setRoundStart(int skip)
{
	SurvivalRoundStartMessage_t message;

	// don't allow overwriting existing outcome unless skip
	if (State.RoundEndTime && !skip)
		return;

	// don't allow changing outcome when not host
	if (!State.IsHost)
		return;

	// increment round if end
	int targetRound = State.RoundNumber;
	if (!State.RoundEndTime)
		targetRound += 1;
		
	// send out
	message.RoundNumber = targetRound;
	message.GameTime = gameGetTime() + (skip ? 0 : ROUND_TRANSITION_DELAY_MS);
	netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_ROUND_START, sizeof(SurvivalRoundStartMessage_t), &message);

	// set locally
	onSetRoundStart(message.RoundNumber, message.GameTime);
}

//--------------------------------------------------------------------------
void setPlayerEXP(int localPlayerIndex, float expPercent)
{
	Player* player = playerGetFromSlot(localPlayerIndex);
	if (!player || !player->PlayerMoby)
		return;

	u32 canvasId = localPlayerIndex; //hudGetCurrentCanvas() + localPlayerIndex;
	void* canvas = hudGetCanvas(canvasId);
	if (!canvas)
		return;

	struct HUDWidgetRectangleObject* expBar = hudCanvasGetObject(canvas, 0xF000010A);
	if (!expBar) {
		DPRINTF("no exp bar %d canvas:%d\n", localPlayerIndex, canvasId);
    return;
  }
	
	// set exp bar
	expBar->iFrame.ScaleX = 0.2275 * expPercent;
	expBar->iFrame.PositionX = 0.406 + (expBar->iFrame.ScaleX / 2);
	expBar->iFrame.PositionY = 0.0546875;
	expBar->iFrame.Color = hudGetTeamColor(player->Team, 2);
	expBar->Color1 = hudGetTeamColor(player->Team, 0);
	expBar->Color2 = hudGetTeamColor(player->Team, 2);
	expBar->Color3 = hudGetTeamColor(player->Team, 0);

	struct HUDWidgetTextObject* healthText = hudCanvasGetObject(canvas, 0xF000010E);
	if (!healthText) {
		DPRINTF("no health text %d\n", localPlayerIndex);
    return;
  }
	
	// set health string
	snprintf(State.PlayerStates[player->PlayerId].HealthBarStrBuf, sizeof(State.PlayerStates[player->PlayerId].HealthBarStrBuf), "%d/%d", (int)player->Health, (int)player->MaxHealth);
	healthText->ExternalStringMemory = State.PlayerStates[player->PlayerId].HealthBarStrBuf;
}

//--------------------------------------------------------------------------
void forcePlayerHUD(void)
{
	int i;

	// replace normal scoreboard with bolt counter
	for (i = 0; i < 2; ++i)
	{
		PlayerHUDFlags* hudFlags = hudGetPlayerFlags(i);
		if (hudFlags) {
			hudFlags->Flags.BoltCounter = 1;
			hudFlags->Flags.NormalScoreboard = 0;
		}
	}

	// show exp
	POKE_U32(0x0054ffc0, 0x0000102D);
  POKE_U16(0x00550054, 0);
}

//--------------------------------------------------------------------------
void destroyOmegaPads(void)
{
	int c = 0;
	GuberMoby* gm = guberMobyGetFirst();
	while (gm)
	{
		Moby* moby = gm->Moby;
		if (moby && moby->OClass == MOBY_ID_PICKUP_PAD && moby->PVar) {
			if (*(u8*)moby->PVar == 5) {
				guberMobyDestroy(moby);
				++c;
			}
		}

		gm = (GuberMoby*)gm->Guber.Prev;
	}
}

//--------------------------------------------------------------------------
void randomizeWeaponPickups(void)
{
	int i,j;
	GameOptions* gameOptions = gameGetOptions();
	GuberMoby* gm = guberMobyGetFirst();
	char wepCounts[9];
	char wepEnabled[17];
	int pickupCount = 0;
	int pickupOptionCount = 0;
	memset(wepEnabled, 0, sizeof(wepEnabled));
	memset(wepCounts, 0, sizeof(wepCounts));

	if (gameOptions->WeaponFlags.DualVipers) { wepEnabled[2] = 1; pickupOptionCount++; DPRINTF("dv\n"); }
	if (gameOptions->WeaponFlags.MagmaCannon) { wepEnabled[3] = 1; pickupOptionCount++; DPRINTF("mag\n"); }
	if (gameOptions->WeaponFlags.Arbiter) { wepEnabled[4] = 1; pickupOptionCount++; DPRINTF("arb\n"); }
	if (gameOptions->WeaponFlags.FusionRifle) { wepEnabled[5] = 1; pickupOptionCount++; DPRINTF("fusion\n"); }
	if (gameOptions->WeaponFlags.MineLauncher) { wepEnabled[6] = 1; pickupOptionCount++; DPRINTF("mines\n"); }
	if (gameOptions->WeaponFlags.B6) { wepEnabled[7] = 1; pickupOptionCount++; DPRINTF("b6\n"); }
	if (gameOptions->WeaponFlags.Holoshield) { wepEnabled[16] = 1; pickupOptionCount++; DPRINTF("shields\n"); }
	if (gameOptions->WeaponFlags.Flail) { wepEnabled[12] = 1; pickupOptionCount++; DPRINTF("flail\n"); }
	if (gameOptions->WeaponFlags.Chargeboots && gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots == 0) { wepEnabled[13] = 1; pickupOptionCount++; DPRINTF("cboots\n"); }

	if (pickupOptionCount > 0) {
		Moby* moby = mobyListGetStart();
		Moby* mEnd = mobyListGetEnd();

		while (moby < mEnd) {
			if (moby->OClass == MOBY_ID_WEAPON_PICKUP && moby->PVar) {
				
				int target = pickupCount / pickupOptionCount;
				int gadgetId = 1;
				if (target < 2) {
					do { j = rand(pickupOptionCount); } while (wepCounts[j] != target);

					++wepCounts[j];

					i = -1;
					do
					{
						++i;
						if (wepEnabled[i])
							--j;
					} while (j >= 0);

					gadgetId = i;
				}

				// set pickup
				DPRINTF("setting pickup at %08X to %d\n", (u32)moby, gadgetId);
				((void (*)(Moby*, int))0x0043A370)(moby, gadgetId);

				++pickupCount;
			}

			++moby;
		}
	}
}

//--------------------------------------------------------------------------
void setWeaponPickupRespawnTime(void)
{
	GameSettings* gameSettings = gameGetSettings();

	// compute pickup respawn time in ms
	int respawnTimeOffset = WEAPON_PICKUP_PLAYER_RESPAWN_TIME_OFFSETS[(gameSettings->PlayerCountAtStart <= 0 ? 1 : gameSettings->PlayerCountAtStart) - 1];

	Moby* moby = mobyListGetStart();
	Moby* mEnd = mobyListGetEnd();

	while (moby < mEnd) {
		if (moby->OClass == MOBY_ID_WEAPON_PICKUP && moby->PVar) {

			// hide if wrench
			// wrenches are set as pickup id when we've reached the max number of
			// pickups per gadget
			int gadgetId = *(int*)moby->PVar;
			int slotId = weaponIdToSlot(gadgetId) - 1;
			if (gadgetId == 1) {
				moby->Position[2] = 0;
			}
			else if (slotId >= 0 && slotId < 8) {
				int weaponBaseRespawnTime = WEAPON_PICKUP_BASE_RESPAWN_TIMES[slotId];

				// otherwise set cooldown by configuration
				POKE_U32((u32)moby->PVar + 0x0C, (weaponBaseRespawnTime - respawnTimeOffset) * TIME_SECOND);
			}
		}
		++moby;
	}
}

//--------------------------------------------------------------------------
int whoKilledMeHook(Player* player, Moby* moby, int b)
{
	// allow damage from mobs
	if (moby && moby->OClass == ZOMBIE_MOBY_OCLASS)
		return ((int (*)(Player*, Moby*, int))0x005dff08)(player, moby, b);
		
	return 0;
}

//--------------------------------------------------------------------------
Moby* FindMobyOrSpawnBox(int oclass, int defaultToSpawnpointId)
{
	// find
	Moby* m = mobyFindNextByOClass(mobyListGetStart(), oclass);
	
	// if can't find moby then just spawn a beta box at a spawn point
	if (!m) {
		SpawnPoint* sp = spawnPointGet(defaultToSpawnpointId);

		//
		m = mobySpawn(MOBY_ID_BETA_BOX, 0);
		vector_copy(m->Position, &sp->M0[12]);

#if DEBUG
		printf("could not find oclass %04X... spawned box (%08X) at ", oclass, (u32)m);
		vector_print(&sp->M0[12]);
		printf("\n");
#endif
	}

	return m;
}

//--------------------------------------------------------------------------
void spawnUpgrades(void)
{
	VECTOR pos,rot;
	int i,j;
	int r;
	int bakedUpgradeSpawnpointCount = 0;
	char upgradeBakedSpawnpointIdx[UPGRADE_COUNT];

  // only spawn if host
  if (!gameAmIHost())
    return;

	// initialize spawnpoint idx to -1
	// and set moby ref to NULL
	for (i = 0; i < UPGRADE_COUNT; ++i) {
		upgradeBakedSpawnpointIdx[i] = -1;
		State.UpgradeMobies[i] = NULL;
	}

	// count number of baked spawnpoints used for upgrade
	for (i = 0; i < BAKED_SPAWNPOINT_COUNT; ++i) {
		if (BakedConfig.BakedSpawnPoints[i].Type == BAKED_SPAWNPOINT_UPGRADE)
			++bakedUpgradeSpawnpointCount;
	}

	// find free baked spawn idx
	for (i = 0; i < UPGRADE_COUNT; ++i) {

		// if no more free spawnpoints then exit
		if (!bakedUpgradeSpawnpointCount)
			break;

		// randomly select point
		r = 1 + rand(BAKED_SPAWNPOINT_COUNT);
		j = 0;
		while (r) {
			j = (j + 1) % BAKED_SPAWNPOINT_COUNT;
			if (BakedConfig.BakedSpawnPoints[j].Type == BAKED_SPAWNPOINT_UPGRADE && !charArrayContains(upgradeBakedSpawnpointIdx, UPGRADE_COUNT, j))
				--r;
		}

		// assign point to this upgrade
		// decrement list
		upgradeBakedSpawnpointIdx[i] = j;
		--bakedUpgradeSpawnpointCount;
	}

	// spawn
	for (i = 0; i < UPGRADE_COUNT; ++i) {
		int bakedSpIdx = upgradeBakedSpawnpointIdx[i];
		if (bakedSpIdx < 0)
			continue;

		SurvivalBakedSpawnpoint_t* sp = &BakedConfig.BakedSpawnPoints[bakedSpIdx];
		memcpy(pos, sp->Position, sizeof(float)*3);
		memcpy(rot, sp->Rotation, sizeof(float)*3);

		upgradeCreate(pos, rot, i);
	}
}

//--------------------------------------------------------------------------
void resetRoundState(void)
{
	GameSettings* gameSettings = gameGetSettings();
	int gameTime = gameGetTime();

	// 
	static int accum = 0;
	float playerCountMultiplier = powf((float)State.ActivePlayerCount, 0.6);
	accum += State.RoundNumber * BUDGET_START_ACCUM * State.Difficulty;
	State.RoundBudget = BUDGET_START + accum + 
					(State.RoundNumber * (int)(State.Difficulty * BUDGET_PLAYER_WEIGHT * playerCountMultiplier));
	State.RoundMaxMobCount = MAX_MOBS_BASE + (int)(MAX_MOBS_ROUND_WEIGHT * (1 + powf(State.RoundNumber * playerCountMultiplier, 0.75)));
	State.RoundMaxSpawnedAtOnce = MAX_MOBS_SPAWNED;
	State.RoundInitialized = 0;
	State.RoundStartTime = gameTime;
	State.RoundCompleteTime = 0;
	State.RoundEndTime = 0;
	State.RoundMobCount = 0;
	State.RoundMobSpawnedCount = 0;
	State.RoundSpawnTicker = 0;
	State.RoundSpawnTickerCounter = 0;
	State.RoundNextSpawnTickerCounter = 0;
	State.MinMobCost = getMinMobCost();

	// 
	if (State.RoundIsSpecial) {
		State.RoundMaxSpawnedAtOnce = specialRoundParams[State.RoundSpecialIdx].MaxSpawnedAtOnce;
	}

	// 
	State.RoundInitialized = 1;
}

//--------------------------------------------------------------------------
void initialize(PatchGameConfig_t* gameConfig)
{
	char hasTeam[10] = {0,0,0,0,0,0,0,0,0,0};
	Player** players = playerGetAll();
	int i, j;
	VECTOR t;

	// Disable normal game ending
	*(u32*)0x006219B8 = 0;	// survivor (8)
	*(u32*)0x00620F54 = 0;	// time end (1)
	*(u32*)0x00621568 = 0;	// kills reached (2)
	*(u32*)0x006211A0 = 0;	// all enemies leave (9)

	// Disables end game draw dialog
	*(u32*)0x0061fe84 = 0;

	// sets start of colored weapon icons to v10
	*(u32*)0x005420E0 = 0x2A020009;
	*(u32*)0x005420E4 = 0x10400005;

	// Removes MP check on HudAmmo weapon icon color (so v99 is pinkish)
	*(u32*)0x00542114 = 0;

	// Enable sniper to shoot through multiple enemies
	*(u32*)0x003FC2A8 = 0;

	// Disable sniper shot corn
	//*(u32*)0x003FC410 = 0;
	*(u32*)0x003FC5A8 = 0;

	// Fix v10 arb overlapping shots
	*(u32*)0x003F2E70 = 0x24020000;

	// Change mine update function to ours
	*(u32*)0x002499D4 = (u32)&customMineMobyUpdate;

	// Change bangelize weapons call to ours
	*(u32*)0x005DD890 = 0x0C000000 | ((u32)&customBangelizeWeapons >> 2);

	// Enable weapon version and v10 name variant in places that display weapon name
	*(u32*)0x00541850 = 0x08000000 | ((u32)&customGetGadgetVersionName >> 2);
	*(u32*)0x00541854 = 0;

	// patch who killed me to prevent damaging others
	*(u32*)0x005E07C8 = 0x0C000000 | ((u32)&whoKilledMeHook >> 2);
	*(u32*)0x005E11B0 = *(u32*)0x005E07C8;
	
	// set default ammo for flail to 8
	*(u8*)0x0039A3B4 = 8;

	// disable targeting players
	*(u32*)0x005F8A80 = 0x10A20002;
	*(u32*)0x005F8A84 = 0x0000102D;
	*(u32*)0x005F8A88 = 0x24440001;

	// custom damage cooldown time
	POKE_U16(0x0060583C, DIFFICULTY_HITINVTIMERS[gameConfig->survivalConfig.difficulty]);
	POKE_U16(0x0060582C, DIFFICULTY_HITINVTIMERS[gameConfig->survivalConfig.difficulty]);

	// disable vampire
	gameConfig->grVampire = 0;

	//WeaponDefsData* wepDefs = weaponGetGunLevelDefs();
	//wepDefs[WEAPON_ID_MAGMA_CANNON].Entries[9].Damage[2] = 80.0;

	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_ROUND_COMPLETE, &onSetRoundCompleteRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_ROUND_START, &onSetRoundStartRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_WEAPON_UPGRADE, &onPlayerUpgradeWeaponRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_REVIVE_PLAYER, &onPlayerReviveRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_DIED, &onSetPlayerDeadRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_WEAPON_MODS, &onSetPlayerWeaponModsRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_STATS, &onSetPlayerStatsRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_DOUBLE_POINTS, &onSetPlayerDoublePointsRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_FREEZE, &onSetFreezeRemote);

	// set game over string (replaces "Draw!")
	strncpy((char*)0x0197B3AA, SURVIVAL_GAME_OVER, 12);
	strncpy(uiMsgString(0x3152), SURVIVAL_UPGRADE_MESSAGE, strlen(SURVIVAL_UPGRADE_MESSAGE));
	strncpy(uiMsgString(0x3153), SURVIVAL_REVIVE_MESSAGE, strlen(SURVIVAL_REVIVE_MESSAGE));

	// disable v2s and packs
	cheatsApplyNoV2s();
	cheatsApplyNoPacks();

	// change hud
	forcePlayerHUD();
  setPlayerEXP(0, 0);
  setPlayerEXP(1, 0);

	// set bolts to 0
	*LocalBoltCount = 0;

	// destroy omega mod pads
	destroyOmegaPads();

	// give a 3 second delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 60 * 1;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	memset(defaultSpawnParamsCooldowns, 0, sizeof(defaultSpawnParamsCooldowns));

	// 
	mobInitialize();
	dropInitialize();
	upgradeInitialize();

	// initialize player states
	State.LocalPlayerState = NULL;
	State.NumTeams = 0;
	State.ActivePlayerCount = 0;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		memset(&State.PlayerStates[i], 0, sizeof(struct SurvivalPlayer));

#if DEBUG
		State.PlayerStates[i].State.CurrentTokens = 100;
#endif

		if (p) {

			// is local
			State.PlayerStates[i].IsLocal = p->IsLocal;
			if (p->IsLocal && !State.LocalPlayerState)
				State.LocalPlayerState = &State.PlayerStates[i];
				
			// clear alpha mods
			if (p->GadgetBox)
				memset(p->GadgetBox->ModBasic, 0, sizeof(p->GadgetBox->ModBasic));

			if (!hasTeam[p->Team]) {
				State.NumTeams++;
				hasTeam[p->Team] = 1;
			}

#if PAYDAY
      p->GadgetBox->ModBasic[0] = 64;
      p->GadgetBox->ModBasic[1] = 64;
      p->GadgetBox->ModBasic[2] = 64;
      p->GadgetBox->ModBasic[3] = 64;
      p->GadgetBox->ModBasic[4] = 64;
      p->GadgetBox->ModBasic[5] = 64;
      p->GadgetBox->ModBasic[6] = 64;
      p->GadgetBox->ModBasic[7] = 64;
#endif

			++State.ActivePlayerCount;
		}
	}

	// initialize weapon data
	WeaponDefsData* gunDefs = weaponGetGunLevelDefs();
	for (i = 0; i < 7; ++i) {
		for (j = 0; j < 10; ++j) {
			gunDefs[i].Entries[j].MpLevelUpExperience = VENDOR_MAX_WEAPON_LEVEL;
		}
	}
	WeaponDefsData* flailDefs = weaponGetFlailLevelDefs();
	for (j = 0; j < 10; ++j) {
		flailDefs->Entries[j].MpLevelUpExperience = VENDOR_MAX_WEAPON_LEVEL;
	}

	// randomize weapon picks
	if (State.IsHost) {
		randomizeWeaponPickups();
	}

	// find vendor
#if defined(VENDOR_OCLASS)
	State.Vendor = FindMobyOrSpawnBox(VENDOR_OCLASS, 1);
#else
	State.Vendor = FindMobyOrSpawnBox(0, 1);
#endif

	// find al
#if defined(BIGAL_OCLASS)
	State.BigAl = FindMobyOrSpawnBox(BIGAL_OCLASS, 2);
#else
	State.BigAl = FindMobyOrSpawnBox(0, 2);
#endif

  // find gates
  Moby* mStart = mobyListGetStart();
  for (i = 0; i < GATE_MAX_COUNT && mStart; ++i) {
    State.GateMobies[i] = mStart = mobyFindNextByOClass(mStart + 1, GATE_OCLASS);
    DPRINTF("gate found %08X\n", (u32)mStart);
  }

#if UPGRADES
	// spawn upgrades
	spawnUpgrades();
#endif

	DPRINTF("vendor %08X\n", (u32)State.Vendor);
	DPRINTF("big al %08X\n", (u32)State.BigAl);

	// special
	memset(State.RoundSpecialSpawnableZombies, -1, sizeof(State.RoundSpecialSpawnableZombies));

	// initialize state
	State.GameOver = 0;
	State.RoundNumber = 0;
	State.MobsDrawnCurrent = 0;
	State.MobsDrawnLast = 0;
	State.MobsDrawGameTime = 0;
	State.Freeze = 0;
	State.TimeOfFreeze = 0;
	State.RoundIsSpecial = 0;
	State.RoundSpecialIdx = 0;
	State.DropCooldownTicks = DROP_COOLDOWN_TICKS_MAX; // try and stop drops from spawning immediately
	State.InitializedTime = gameGetTime();
	State.Difficulty = DIFFICULTY_MAP[(int)gameConfig->survivalConfig.difficulty];
	resetRoundState();

	// scale default mob params by difficulty
	for (i = 0; i < defaultSpawnParamsCount; ++i)
	{
		struct MobConfig* config = &defaultSpawnParams[i].Config;
		//config->Bolts /= State.Difficulty;
		config->MaxHealth *= State.Difficulty;
		config->Health *= State.Difficulty;
		config->Damage *= State.Difficulty;
	}

#if FIXEDTARGET
  FIXEDTARGETMOBY = mobySpawn(0xE7D, 0);
  FIXEDTARGETMOBY->Position[0] = 329.03;
  FIXEDTARGETMOBY->Position[1] = 564.35;
  FIXEDTARGETMOBY->Position[2] = 436.14;
#endif

	Initialized = 1;
}

void updateGameState(PatchStateContainer_t * gameState)
{
	int i,j;

	// game state update
	if (gameState->UpdateGameState)
	{
		gameState->GameStateUpdate.RoundNumber = State.RoundNumber + 1;
	}

	// stats
	if (gameState->UpdateCustomGameStats)
	{
		struct SurvivalGameData* sGameData = (struct SurvivalGameData*)gameState->CustomGameStats.Payload;
		sGameData->RoundNumber = State.RoundNumber;
		sGameData->Version = 0x00000002;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			sGameData->Kills[i] = State.PlayerStates[i].State.Kills;
			sGameData->Revives[i] = State.PlayerStates[i].State.Revives;
			sGameData->TimesRevived[i] = State.PlayerStates[i].State.TimesRevived;
			sGameData->Points[i] = State.PlayerStates[i].State.TotalBolts;
			for (j = 0; j < 8; ++j)
				sGameData->AlphaMods[i][j] = (u8)State.PlayerStates[i].State.AlphaMods[j];
			memcpy(sGameData->BestWeaponLevel[i], State.PlayerStates[i].State.BestWeaponLevel, sizeof(State.PlayerStates[i].State.BestWeaponLevel));
		}
	}
}

//--------------------------------------------------------------------------
void gameStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player ** players = playerGetAll();
	Player* localPlayer = playerGetFromSlot(0);
	int i;
	char buffer[64];
	int gameTime = gameGetTime();

	// first
	dlPreUpdate();

	// 
	updateGameState(gameState);

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

	// Determine if host
	State.IsHost = gameAmIHost();

	if (!Initialized) {
		initialize(gameConfig);
		return;
	}

	// force weapon pickup respawn times
	setWeaponPickupRespawnTime();

#if LOG_STATS
	static int statsTicker = 0;
	if (statsTicker <= 0) {
		DPRINTF("budget:%d liveMobCount:%d totalSpawnedThisRound:%d roundNumber:%d roundSpawnTicker:%d minCost:%d\n", State.RoundBudget, State.RoundMobCount, State.RoundMobSpawnedCount, State.RoundNumber, State.RoundSpawnTicker, State.MinMobCost);
		statsTicker = 60 * 15;
	} else {
		--statsTicker;
	}
#endif

#if DEBUG
	if (padGetButtonDown(0, PAD_L3 | PAD_R3) > 0)
		State.GameOver = 1;
#endif

#if MANUAL_SPAWN
	{
		if (padGetButtonDown(0, PAD_DOWN) > 0) {
			static int manSpawnMobId = 0;
			manSpawnMobId = defaultSpawnParamsCount - 1;
			VECTOR t;
			vector_copy(t, localPlayer->PlayerPosition);
			t[0] += 1;

			mobCreate(t, 0, -1, &defaultSpawnParams[(manSpawnMobId++ % defaultSpawnParamsCount)].Config);
		}
    else if (padGetButtonDown(0, PAD_L1 | PAD_RIGHT) > 0) {
      State.Freeze = 1;
      State.TimeOfFreeze = 0x6FFFFFFF;
      DPRINTF("freeze\n");
    }
    else if (padGetButtonDown(0, PAD_L1 | PAD_LEFT) > 0) {
      State.Freeze = 0;
      State.TimeOfFreeze = 0;
      DPRINTF("unfreeze\n");
    }
	}
#endif

#if BENCHMARK
  {
    static int manSpawnMobId = 0;
    if (manSpawnMobId < MAX_MOBS_SPAWNED)
    {
      VECTOR t = {396,606,434,0};
      
      t[0] += (manSpawnMobId % 8) * 2;
      t[1] += (manSpawnMobId / 8) * 2;

      mobCreate(t, 0, -1, &defaultSpawnParams[(manSpawnMobId++ % defaultSpawnParamsCount)].Config);
    }
  }
#endif

#if MANUAL_DROP_SPAWN
	{
		if (padGetButtonDown(0, PAD_UP) > 0) {
			static int manSpawnDropId = 0;
			static int manSpawnDropIdx = 0;
			manSpawnDropId = (manSpawnDropId + 1) % 5;
			VECTOR t;
			vector_copy(t, localPlayer->PlayerPosition);
			t[0] += 5;

			State.DropCooldownTicks = 0;
			dropCreate(t, (enum DropType)manSpawnDropId, gameGetTime() + (30 * TIME_SECOND), localPlayer->Team);
			++manSpawnDropIdx;
		}
	}
#endif

	// draw round number
	char* roundStr = uiMsgString(0x25A9);
	gfxScreenSpaceText(481, 281, 0.7, 0.7, 0x40000000, roundStr, -1, 1);
	gfxScreenSpaceText(480, 280, 0.7, 0.7, 0x80E0E0E0, roundStr, -1, 1);
	sprintf(buffer, "%d", State.RoundNumber + 1);
	gfxScreenSpaceText(481, 292, 1, 1, 0x40000000, buffer, -1, 1);
	gfxScreenSpaceText(480, 291, 1, 1, 0x8029E5E6, buffer, -1, 1);

	// draw double points
	if (localPlayer && State.PlayerStates[localPlayer->PlayerId].IsDoublePoints) {
		gfxScreenSpaceText(381, 48, 1, 1, 0x40000000, "x2", -1, 1);
		gfxScreenSpaceText(380, 47, 1, 1, 0x8029E5E6, "x2", -1, 1);
	}

	// draw number of mobs spawned
	char* enemiesStr = "ENEMIES";
	gfxScreenSpaceText(481, 241, 0.7, 0.7, 0x40000000, enemiesStr, -1, 1);
	gfxScreenSpaceText(480, 240, 0.7, 0.7, 0x80E0E0E0, enemiesStr, -1, 1);
	sprintf(buffer, "%d", State.RoundMobCount);
	gfxScreenSpaceText(481, 252, 1, 1, 0x40000000, buffer, -1, 1);
	gfxScreenSpaceText(480, 251, 1, 1, 0x8029E5E6, buffer, -1, 1);

	// draw dread tokens
	for (i = 0; i < 2; ++i) {
		Player* p = playerGetFromSlot(i);
		if (p && p->PlayerMoby) {
			float y = 57 + (i * 0.5 * SCREEN_HEIGHT);
			snprintf(buffer, 32, "%d", State.PlayerStates[p->PlayerId].State.CurrentTokens);
			gfxScreenSpaceText(457+2, y+8+2, 1, 1, 0x40000000, buffer, -1, 2);
			gfxScreenSpaceText(457,   y+8,   1, 1, 0x80C0C0C0, buffer, -1, 2);
			drawDreadTokenIcon(462, y, 32);
		}
	}

	// ticks
	mobTick();
	dropTick();
	upgradeTick();

	if (!State.GameOver)
	{
		*LocalBoltCount = 0;
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			// update bolt counter
			// since there is only one bolt count just show the sum of each local client's bolts
			if (players[i] && players[i]->IsLocal)
				*LocalBoltCount += State.PlayerStates[i].State.Bolts;
		}

		// 
		State.ActivePlayerCount = 0;
		for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
			if (players[i])
				State.ActivePlayerCount++;

			processPlayer(i);
		}

		// replace normal scoreboard with bolt counter
		forcePlayerHUD();

		// handle freeze and double points
		if (State.IsHost) {
			int dblPointsChanged = 0;

			// disable double points for players
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				if (State.PlayerStates[i].IsDoublePoints && gameTime >= (State.PlayerStates[i].TimeOfDoublePoints + DOUBLE_POINTS_DURATION)) {
					State.PlayerStates[i].IsDoublePoints = 0;
					dblPointsChanged = 1;
					DPRINTF("setting player %d dbl points to 0\n", i);
				}
			}

			// disable freeze
			if (State.Freeze && gameTime >= (State.TimeOfFreeze + FREEZE_DROP_DURATION)) {
				DPRINTF("disabling freeze\n");
				setFreeze(0);
			}

			if (dblPointsChanged)
				sendDoublePoints();
		}

		// handle game over
		if (State.IsHost && gameOptions->GameFlags.MultiplayerGameFlags.Survivor && gameTime > (State.InitializedTime + 5*TIME_SECOND))
		{
			int isAnyPlayerAlive = 0;
			int oneTeamLeft = 1;
			int lastTeam = -1;
			int teams = gameOptions->GameFlags.MultiplayerGameFlags.Teamplay;

			// determine number of players alive
			// and if one or more teams are left alive
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				if (players[i] && !playerIsDead(players[i]) && players[i]->Health > 0) {
					isAnyPlayerAlive = 1;
					if (lastTeam != players[i]->Team) {
						if (lastTeam >= 0)
							oneTeamLeft = 0;
						
						lastTeam = players[i]->Team;
					}
				}
			}

			// if everyone has died, end game
			if (!isAnyPlayerAlive)
			{
				State.GameOver = 1;
				gameSetWinner(10, 1);
			}
			// if have teams, and only one team left alive, end game
			else if (State.NumTeams > 1 && oneTeamLeft && lastTeam >= 0)
			{
				State.GameOver = 1;
				gameSetWinner(lastTeam, teams);
			}
		}

		if (State.RoundCompleteTime)
		{
			// draw round complete message
			if (gameTime < (State.RoundCompleteTime + ROUND_MESSAGE_DURATION_MS)) {
				sprintf(buffer, SURVIVAL_ROUND_COMPLETE_MESSAGE, State.RoundNumber);
				drawRoundMessage(buffer, 1.5, 0);
			}

			if (State.RoundEndTime)
			{
				// handle when round properly ends
				if (gameTime > State.RoundEndTime)
				{
					// reset round state
					resetRoundState();
				}
				else if (State.IsHost && State.NumTeams <= 1)
				{
					// draw round countdown
					uiShowTimer(0, SURVIVAL_NEXT_ROUND_BEGIN_SKIP_MESSAGE, (int)((State.RoundEndTime - gameTime) * (60.0 / TIME_SECOND)));

					// handle skip
					if (!localPlayer->timers.noInput && padGetButtonDown(0, PAD_UP) > 0) {
						setRoundStart(1);
					}
				}
				else
				{
					// draw round countdown
					uiShowTimer(0, SURVIVAL_NEXT_ROUND_TIMER_MESSAGE, (int)((State.RoundEndTime - gameTime) * (60.0 / TIME_SECOND)));
				}
			}
			else if (State.IsHost)
			{
#if AUTOSTART
				setRoundStart(0);
#else
				if (State.NumTeams == 1) {
					gfxScreenSpaceText(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, 1, 1, 0x80FFFFFF, SURVIVAL_NEXT_ROUND_BEGIN_SKIP_MESSAGE, -1, 4);
					if (padGetButtonDown(0, PAD_UP) > 0) {
						setRoundStart(1);
					}
				}
#endif
			}
		}
		else
		{
			// draw round start message
			if (gameTime < (State.RoundStartTime + ROUND_MESSAGE_DURATION_MS)) {
				sprintf(buffer, SURVIVAL_ROUND_START_MESSAGE, State.RoundNumber+1);
				drawRoundMessage(buffer, 1.5, 0);
				if (State.RoundIsSpecial) {
					drawRoundMessage(specialRoundParams[State.RoundSpecialIdx].Name, 1, 35);
				}
			}

#if !defined(DISABLE_SPAWNING)
			// host specific logic
			if (State.IsHost && (gameTime - State.RoundStartTime) > ROUND_START_DELAY_MS)
			{
				// decrement mob cooldowns
				for (i = 0; i < defaultSpawnParamsCount; ++i) {
					if (defaultSpawnParamsCooldowns[i]) {
						defaultSpawnParamsCooldowns[i] -= 1;
					}
				}

				// reduce count per player to reduce lag
				int maxSpawn = State.RoundMaxSpawnedAtOnce - State.ActivePlayerCount;

				// handle spawning
				if (State.RoundSpawnTicker == 0) {
					if (State.RoundMobCount < maxSpawn && !State.Freeze) {
						if (State.RoundBudget >= State.MinMobCost && State.RoundMobSpawnedCount < State.RoundMaxMobCount) {
							if (spawnRandomMob()) {
								++State.RoundMobSpawnedCount;
#if QUICK_SPAWN
								State.RoundSpawnTicker = 10;
#else
								++State.RoundSpawnTickerCounter;
								if (State.RoundSpawnTickerCounter > State.RoundNextSpawnTickerCounter)
								{
									State.RoundSpawnTickerCounter = 0;
									State.RoundNextSpawnTickerCounter = randRangeInt(MOB_SPAWN_BURST_MIN + (State.RoundNumber * MOB_SPAWN_BURST_MIN_INC_PER_ROUND), MOB_SPAWN_BURST_MAX + (State.RoundNumber * MOB_SPAWN_BURST_MAX_INC_PER_ROUND)) * State.Difficulty;
									State.RoundSpawnTicker = randRangeInt(MOB_SPAWN_BURST_MIN_DELAY, MOB_SPAWN_BURST_MAX_DELAY) / State.Difficulty;
								}
								else
								{
									State.RoundSpawnTicker = 10 / State.Difficulty;
								}
#endif
							}
						} else {

							DPRINTF("finished spawning zombies... with %d budget leftover..\n", State.RoundBudget);

							// mutate mobs
							while (State.RoundBudget >= 5) {
								mobMutate(&defaultSpawnParams[rand(defaultSpawnParamsCount)], rand(MOB_MUTATE_COUNT));
								State.RoundBudget -= 5;
							}

							// 
							DPRINTF("mutators for next round:\n");
							for (i = 0; i < defaultSpawnParamsCount; ++i) {
								DPRINTF("\tmob %d of type %d:\n", i, defaultSpawnParams[i].Config.MobType);
								DPRINTF("\t\tdamage: %f\n", defaultSpawnParams[i].Config.Damage);
								DPRINTF("\t\tspeed: %f\n", defaultSpawnParams[i].Config.Speed);
								DPRINTF("\t\thealth: %f\n", defaultSpawnParams[i].Config.Health);
								DPRINTF("\t\tcost: %d\n", defaultSpawnParams[i].Cost);
							}

							State.RoundSpawnTicker = -1;
						}
					}
				} else if (State.RoundSpawnTicker < 0) {
					if (State.RoundMobCount < 0) {
						DPRINTF("%d\n", State.RoundMobCount);
					}
					// wait for all zombies to die
					if (State.RoundMobCount == 0) {
						setRoundComplete();
					}
				} else {
					--State.RoundSpawnTicker;
				}
			}
#endif
		}
	}
	else
	{
		// end game
		if (State.GameOver == 1)
		{
			gameEnd(4);
			State.GameOver = 2;
		}
	}

	// last
	dlPostUpdate();
	return;
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(void)
{
	int i;

	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions || gameSettings->GameLoadStartTime <= 0)
		return;

	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickups = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickupsRandom = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;

#if !DEBUG
	gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 1;
#endif

	// no vehicles
	gameOptions->GameFlags.MultiplayerGameFlags.UNK_0D = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Puma = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hovership = 0;

	// enable all weapons
	gameOptions->WeaponFlags.Chargeboots = 1;
	gameOptions->WeaponFlags.DualVipers = 1;
	gameOptions->WeaponFlags.MagmaCannon = 1;
	gameOptions->WeaponFlags.Arbiter = 1;
	gameOptions->WeaponFlags.FusionRifle = 1;
	gameOptions->WeaponFlags.MineLauncher = 1;
	gameOptions->WeaponFlags.B6 = 1;
	gameOptions->WeaponFlags.Holoshield = 1;
	gameOptions->WeaponFlags.Flail = 1;

	// force everyone to same team as host
	for (i = 1; i < GAME_MAX_PLAYERS; ++i) {
		if (gameSettings->PlayerTeams[i] >= 0) {
			gameSettings->PlayerTeams[i] = gameSettings->PlayerTeams[0];
		}
	}
}

//--------------------------------------------------------------------------
void setEndGameScoreboard(PatchGameConfig_t * gameConfig)
{
	u32 * uiElements = (u32*)(*(u32*)(0x011C7064 + 4*18) + 0xB0);
	int i;

	// column headers start at 17
	strncpy((char*)(uiElements[19] + 0x60), "BOLTS", 6);
	strncpy((char*)(uiElements[20] + 0x60), "DEATHS", 7);
	strncpy((char*)(uiElements[21] + 0x60), "REVIVES", 8);

	// rows
	int* pids = (int*)(uiElements[0] - 0x9C);
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		// match scoreboard player row to their respective dme id
		int pid = pids[i];
		char* name = (char*)(uiElements[7 + i] + 0x18);
		if (pid < 0 || name[0] == 0)
			continue;

		struct SurvivalPlayerState* pState = &State.PlayerStates[pid].State;

		// set round number
		sprintf((char*)0x003D3AE0, "Survived %d Rounds on %s!", State.RoundNumber, DIFFICULTY_NAMES[(int)gameConfig->survivalConfig.difficulty]);

		// set kills
		sprintf((char*)(uiElements[22 + (i*4) + 0] + 0x60), "%d", pState->Kills);

		// copy over deaths
		strncpy((char*)(uiElements[22 + (i*4) + 2] + 0x60), (char*)(uiElements[22 + (i*4) + 1] + 0x60), 10);

		// set bolts
		sprintf((char*)(uiElements[22 + (i*4) + 1] + 0x60), "%d", pState->TotalBolts);

		// set revives
		sprintf((char*)(uiElements[22 + (i*4) + 3] + 0x60), "%d", pState->Revives);
	}
}

//--------------------------------------------------------------------------
void lobbyStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	int i;
	int activeId = uiGetActive();
	static int initializedScoreboard = 0;

	// send final local player stats to others
	if (Initialized == 1) {
		for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
			if (State.PlayerStates[i].IsLocal) {
				sendPlayerStats(i);
			}
		}
		Initialized = 2;
	}

	// 
	updateGameState(gameState);

	// scoreboard
	switch (activeId)
	{
		case 0x15C:
		{
			if (initializedScoreboard)
				break;

			setEndGameScoreboard(gameConfig);
			initializedScoreboard = 1;

			// patch rank computation to keep rank unchanged for base mode
			POKE_U32(0x0077ACE4, 0x4600BB06);
			break;
		}
		case UI_ID_GAME_LOBBY:
		{
			setLobbyGameOptions();
			break;
		}
	}
}

//--------------------------------------------------------------------------
void loadStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	setLobbyGameOptions();
  
	// point get resurrect point to ours
	*(u32*)0x00610724 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
	*(u32*)0x005e2d44 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
}
