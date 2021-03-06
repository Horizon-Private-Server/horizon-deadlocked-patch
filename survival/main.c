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
#include "module.h"
#include "messageid.h"
#include "include/mob.h"
#include "include/game.h"
#include "include/utils.h"

const char * SURVIVAL_ROUND_COMPLETE_MESSAGE = "Round %d Complete!";
const char * SURVIVAL_ROUND_START_MESSAGE = "Round %d";
const char * SURVIVAL_NEXT_ROUND_BEGIN_SKIP_MESSAGE = "\x1d   Start Round";
const char * SURVIVAL_NEXT_ROUND_TIMER_MESSAGE = "Next Round";
const char * SURVIVAL_GAME_OVER = "GAME OVER";
const char * SURVIVAL_REVIVE_MESSAGE = "\x1c Revive [\x0E%d\x08]";
const char * SURVIVAL_UPGRADE_MESSAGE = "\x11 Upgrade [\x0E%d\x08]";
const char * SURVIVAL_OPEN_WEAPONS_MESSAGE = "\x12 Manage Mods";

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

// NOTE
// These must be ordered from least probable to most probable
struct MobSpawnParams defaultSpawnParams[] = {
	// ghost zombie
	{
		.Cost = 10,
		.MinRound = 4,
		.Probability = 0.05,
		.Config = {
			.MobType = MOB_GHOST,
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
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2
		}
	},
	// explode zombie
	{
		.Cost = 10,
		.MinRound = 6,
		.Probability = 0.08,
		.Config = {
			.MobType = MOB_EXPLODE,
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
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2
		}
	},
	// acid zombie
	{
		.Cost = 20,
		.MinRound = 10,
		.Probability = 0.09,
		.Config = {
			.MobType = MOB_ACID,
			.Bangles = ZOMBIE_BANGLE_HEAD_4 | ZOMBIE_BANGLE_TORSO_4,
			.Damage = ZOMBIE_BASE_DAMAGE * 1.0,
			.MaxDamage = ZOMBIE_BASE_DAMAGE * 1.0,
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxSpeed = ZOMBIE_BASE_SPEED * 1.8,
			.Health = ZOMBIE_BASE_HEALTH * 1.0,
			.MaxHealth = 0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2
		}
	},
	// freeze zombie
	{
		.Cost = 20,
		.MinRound = 8,
		.Probability = 0.1,
		.Config = {
			.MobType = MOB_FREEZE,
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
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2
		}
	},
	// runner zombie
	{
		.Cost = 10,
		.MinRound = 0,
		.Probability = 0.2,
		.Config = {
			.MobType = MOB_NORMAL,
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
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2
		}
	},
	// normal zombie
	{
		.Cost = 5,
		.MinRound = 0,
		.Probability = 1.0,
		.Config = {
			.MobType = MOB_NORMAL,
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
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS,
			.MaxCostMutation = 2
		}
	},
};

const int defaultSpawnParamsCount = sizeof(defaultSpawnParams) / sizeof(struct MobSpawnParams);


//--------------------------------------------------------------------------
void drawRoundMessage(const char * message, float scale)
{
	int fw = gfxGetFontWidth(message, -1, scale);
	float x = 0.5;
	float y = 0.16;

	// draw message
	y *= SCREEN_HEIGHT;
	x *= SCREEN_WIDTH;
	gfxScreenSpaceText(x, y + 5, scale, scale * 1.5, 0x80FFFFFF, message, -1, 1);
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
Player * playerGetRandom(void)
{
	int r = rand(GAME_MAX_PLAYERS);
	int i = 0, c = 0;
	Player ** players = playerGetAll();

	do {
		if (players[i] && !playerIsDead(players[i]))
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
int spawnGetRandomPoint(VECTOR out) {
	// larger the map, better chance mob spawns near you
	// harder difficulty, better chance mob spawns near you
	float r = randRange(0, 1) / (BakedConfig.MapSize * State.Difficulty);

#if QUICK_SPAWN
	r = MOB_SPAWN_NEAR_PLAYER_PROBABILITY;
#endif

	// spawn on player
	if (r <= MOB_SPAWN_AT_PLAYER_PROBABILITY) {
		Player * targetPlayer = playerGetRandom();
		if (targetPlayer) {
			vector_write(out, randVectorRange(-1, 1));
			out[2] = 0;
			vector_add(out, out, targetPlayer->PlayerPosition);
			return 1;
		}
	}

	// spawn near player
	if (r <= MOB_SPAWN_NEAR_PLAYER_PROBABILITY) {
		Player * targetPlayer = playerGetRandom();
		if (targetPlayer)
			return spawnPointGetNearestTo(targetPlayer->PlayerPosition, out, 5);
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
struct MobSpawnParams* spawnGetRandomMobParams(void)
{
	int i;
	if (State.RoundBudget < State.MinMobCost)
		return NULL;

	for (i = 0; i < defaultSpawnParamsCount; ++i) {
		struct MobSpawnParams* mob = &defaultSpawnParams[i];
		if (spawnCanSpawnMob(mob) && randRange(0,1) <= mob->Probability)
			return mob;
	}

	for (i = 0; i < defaultSpawnParamsCount; ++i) {
		struct MobSpawnParams* mob = &defaultSpawnParams[i];
		DPRINTF("CANT SPAWN budget:%d round:%d mobIdx:%d mobMinRound:%d mobCost:%d mobProb:%f\n", State.RoundBudget, State.RoundNumber, i, mob->MinRound, mob->Cost, mob->Probability);
	}
	
	
	return NULL;
}

//--------------------------------------------------------------------------
int spawnRandomMob(void) {
	VECTOR sp;
	if (spawnGetRandomPoint(sp)) {
		struct MobSpawnParams* mob = spawnGetRandomMobParams();
		if (mob) {
			if (mobCreate(sp, 0, -1, &mob->Config)) {
				State.RoundBudget -= mob->Cost;
				return 1;
			} else { DPRINTF("failed to create mob\n"); }
		} else { DPRINTF("failed to get random mob params %f\n", randRange(0, 1)); }
	} else { DPRINTF("failed to get random spawn point\n"); }

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
void onPlayerUpgradeWeapon(int playerId, int weaponId, int level, int alphaMod)
{
	int i;
	Player* p = State.PlayerStates[playerId].Player;
	State.PlayerStates[playerId].State.AlphaMods[alphaMod]++;
	if (!p)
		return;

	GadgetBox* gBox = p->GadgetBox;
	playerGiveWeapon(p, weaponId, level);
	gBox->Gadgets[weaponId].Level = level;
	gBox->Gadgets[weaponId].Ammo = ((short (*)(GadgetBox*, int))0x00626FB8)(gBox, weaponId);
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
	Player* player = State.PlayerStates[playerId].Player;
	if (!player)
		return;

	// backup current position/rotation
	VECTOR deadPos, deadRot;
	vector_copy(deadPos, player->PlayerPosition);
	vector_copy(deadRot, player->PlayerRotation);

	// respawn
	PlayerVTable* vtable = playerGetVTable(player);
	if (vtable)
		vtable->UpdateState(player, PLAYER_STATE_IDLE, 1, 1, 1);
	playerSetHealth(player, 50);
	playerSetPosRot(player, deadPos, deadRot);
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
	Player* p = State.PlayerStates[playerId].Player;
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
		
	return UPGRADE_COST[level];
}

//--------------------------------------------------------------------------
void respawnDeadPlayers(void) {
	int i;
	Player** players = playerGetAll();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		if (p && p->IsLocal && playerIsDead(p)) {
			playerRespawn(p);
		}
		
		State.PlayerStates[i].IsDead = 0;
		State.PlayerStates[i].ReviveCooldownTicks = 0;
	}
}

//--------------------------------------------------------------------------
int getPlayerReviveCost(Player * player) {
	if (!player)
		return 0;

	GameData * gameData = gameGetData();
	return gameData->PlayerStats.Deaths[player->PlayerId] * (PLAYER_BASE_REVIVE_COST + (PLAYER_REVIVE_COST_PER_ROUND * State.RoundNumber));
}

//--------------------------------------------------------------------------
void processPlayer(int pIndex) {
	VECTOR t;
	int i = 0, localPlayerIndex, heldWeapon, hasMessage = 0;
	char strBuf[32];

	struct SurvivalPlayer * playerData = &State.PlayerStates[pIndex];
	Player * player = playerData->Player;
	if (!player)
		return;

	int isDeadState = playerIsDead(player);
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
	
	if (player->IsLocal && !player->timers.noInput) {
		
		GadgetBox* gBox = player->GadgetBox;
		localPlayerIndex = player->LocalPlayerIndex;
		heldWeapon = player->WeaponHeldId;

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
		if (actionCooldownTicks > 0)
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

		// handle revive logic
		if (!isDeadState) {
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				if (i != pIndex && !hasMessage) {

					// ensure player exists, is dead, and is on the same team
					struct SurvivalPlayer * otherPlayerData = &State.PlayerStates[i];
					Player * otherPlayer = otherPlayerData->Player;
					if (otherPlayer && otherPlayerData->IsDead && otherPlayerData->ReviveCooldownTicks > 0 && otherPlayer->Team == player->Team) {

						// check distance
						vector_subtract(t, player->PlayerPosition, otherPlayer->PlayerPosition);
						if (vector_sqrmag(t) < (PLAYER_REVIVE_MAX_DIST * PLAYER_REVIVE_MAX_DIST)) {
							
							// get revive cost
							int cost = getPlayerReviveCost(otherPlayer);

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
							}
						}
					}
				}
			}
		}

		// hide message right away
		if (!hasMessage && messageCooldownTicks > 0) {
			((void (*)(int))0x0054e5e8)(localPlayerIndex);
		}
	}
}

//--------------------------------------------------------------------------
int getRoundBonus(int roundNumber, int numPlayers)
{
	int bonus = State.RoundNumber * ROUND_BASE_BOLT_BONUS * numPlayers;
	if (bonus > ROUND_MAX_BOLT_BONUS)
		return ROUND_MAX_BOLT_BONUS;

	return bonus;
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
			if (*(int*)moby->PVar == 5) {
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
	if (gameOptions->WeaponFlags.Chargeboots) { wepEnabled[13] = 1; pickupOptionCount++; DPRINTF("cboots\n"); }

	if (pickupOptionCount > 0) {
		Moby* moby = mobyListGetStart();
		Moby* mEnd = mobyListGetEnd();

		while (moby < mEnd) {
			if (moby->OClass == MOBY_ID_WEAPON_PICKUP && moby->PVar) {
				
				int target = pickupCount / pickupOptionCount;
				do { j = rand(pickupOptionCount); } while (wepCounts[j] != target);

				++wepCounts[j];
				
				i = -1;
				do
				{
					++i;
					if (wepEnabled[i])
						--j;
				} while (j >= 0);

				DPRINTF("setting pickup at %08X to %d\n", (u32)moby, i);
				((void (*)(Moby*, int))0x0043A370)(moby, i);

				++pickupCount;
			}

			++moby;
		}
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
void resetRoundState(void)
{
	GameSettings* gameSettings = gameGetSettings();
	int gameTime = gameGetTime();

	// 
	static int accum = 0;
	accum += State.RoundNumber * BUDGET_START_ACCUM * State.Difficulty;
	State.RoundBudget = BUDGET_START + accum + 
					(State.RoundNumber * gameSettings->PlayerCountAtStart * gameSettings->PlayerCountAtStart * (int)(State.Difficulty * BUDGET_PLAYER_WEIGHT));
	State.RoundMaxMobCount = MAX_MOBS_BASE + (MAX_MOBS_ROUND_WEIGHT * State.RoundNumber * gameSettings->PlayerCountAtStart);
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
	State.RoundInitialized = 1;
}

//--------------------------------------------------------------------------
void initialize(PatchGameConfig_t* gameConfig)
{
	char hasTeam[10] = {0,0,0,0,0,0,0,0,0,0};
	Player** players = playerGetAll();
	int i, j;

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

	// set game over string (replaces "Draw!")
	strncpy((char*)0x015A00AA, SURVIVAL_GAME_OVER, 12);
	strncpy(uiMsgString(0x3152), SURVIVAL_UPGRADE_MESSAGE, strlen(SURVIVAL_UPGRADE_MESSAGE));
	strncpy(uiMsgString(0x3153), SURVIVAL_REVIVE_MESSAGE, strlen(SURVIVAL_REVIVE_MESSAGE));

	// disable v2s and packs
	cheatsApplyNoV2s();
	cheatsApplyNoPacks();

	// change hud
	forcePlayerHUD();

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

	// 
	mobInitialize();

	// initialize player states
	State.LocalPlayerState = NULL;
	State.NumTeams = 0;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].State.Bolts = 0;
		State.PlayerStates[i].State.TotalBolts = 0;
		State.PlayerStates[i].State.Kills = 0;
		State.PlayerStates[i].State.Revives = 0;
		State.PlayerStates[i].State.TimesRevived = 0;
		State.PlayerStates[i].Player = p;
		State.PlayerStates[i].ActionCooldownTicks = 0;
		State.PlayerStates[i].ReviveCooldownTicks = 0;
		State.PlayerStates[i].MessageCooldownTicks = 0;
		State.PlayerStates[i].IsDead = 0;
		State.PlayerStates[i].MinSqrDistFromMob = 0;
		State.PlayerStates[i].MaxSqrDistFromMob = 0;
		State.PlayerStates[i].IsLocal = 0;
		memset(State.PlayerStates[i].State.AlphaMods, 0, sizeof(State.PlayerStates[i].State.AlphaMods));
		memset(State.PlayerStates[i].State.BestWeaponLevel, 0, sizeof(State.PlayerStates[i].State.BestWeaponLevel));

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

	DPRINTF("vendor %08X\n", (u32)State.Vendor);
	DPRINTF("big al %08X\n", (u32)State.BigAl);

	// initialize state
	State.GameOver = 0;
	State.RoundNumber = 0;
	State.MobsDrawnCurrent = 0;
	State.MobsDrawnLast = 0;
	State.MobsDrawGameTime = 0;
	State.InitializedTime = gameGetTime();
	State.Difficulty = DIFFICULTY_MAP[(int)gameConfig->survivalConfig.difficulty];
	resetRoundState();

	// scale default mob params by difficulty
	for (i = 0; i < defaultSpawnParamsCount; ++i)
	{
		struct MobConfig* config = &defaultSpawnParams[i].Config;
		config->Bolts /= State.Difficulty;
		config->MaxHealth *= State.Difficulty;
		config->Health *= State.Difficulty;
		config->Damage *= State.Difficulty;
	}


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
	int i;
	char buffer[32];
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
	Player * localPlayer = (Player*)0x00347AA0;
	if (padGetButtonDown(0, PAD_DOWN) > 0) {
		static int manSpawnMobId = 0;
		manSpawnMobId = defaultSpawnParamsCount - 1;
		VECTOR t;
		vector_copy(t, localPlayer->PlayerPosition);
		t[0] += 1;

		mobCreate(t, 0, -1, &defaultSpawnParams[(manSpawnMobId++ % defaultSpawnParamsCount)].Config);
	}
#endif

	// draw round number
	char* roundStr = uiMsgString(0x25A9);
	gfxScreenSpaceText(481, 281, 0.7, 0.7, 0x40000000, roundStr, -1, 1);
	gfxScreenSpaceText(480, 280, 0.7, 0.7, 0x80E0E0E0, roundStr, -1, 1);
	sprintf(buffer, "%d", State.RoundNumber + 1);
	gfxScreenSpaceText(481, 292, 1, 1, 0x40000000, buffer, -1, 1);
	gfxScreenSpaceText(480, 291, 1, 1, 0x8029E5E6, buffer, -1, 1);

	// draw number of mobs spawned
	sprintf(buffer, "%d", State.RoundMobCount);
	gfxScreenSpaceText(481, 58, 1, 1, 0x40000000, buffer, -1, 1);
	gfxScreenSpaceText(480, 57, 1, 1, 0x80B0B0E0, buffer, -1, 1);

	// mob tick
	mobTick();

	if (!State.GameOver)
	{
		*LocalBoltCount = 0;
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			// update player refs
			State.PlayerStates[i].Player = players[i];

			// update bolt counter
			// since there is only one bolt count just show the sum of each local client's bolts
			if (State.PlayerStates[i].Player && State.PlayerStates[i].Player->IsLocal)
				*LocalBoltCount += State.PlayerStates[i].State.Bolts;
		}

		// 
		for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
			processPlayer(i);
		}

		// replace normal scoreboard with bolt counter
		forcePlayerHUD();

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
				if (players[i] && !playerIsDead(players[i])) {
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
				drawRoundMessage(buffer, 1.5);
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
					if (!State.LocalPlayerState->Player->timers.noInput && padGetButtonDown(0, PAD_UP) > 0) {
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
				drawRoundMessage(buffer, 1.5);
			}

#if !defined(DISABLE_SPAWNING)
			// host specific logic
			if (State.IsHost && (gameTime - State.RoundStartTime) > ROUND_START_DELAY_MS)
			{
				// handle spawning
				if (State.RoundSpawnTicker == 0) {
					if (State.RoundMobCount < MAX_MOBS_SPAWNED) {
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
	// set game options
	GameOptions * gameOptions = gameGetOptions();
	if (!gameOptions)
		return;
	
	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 0;
}

//--------------------------------------------------------------------------
float computePlayerRank(int playerIdx)
{
	// current stats are stored in the calling function's stack
	int * sp;
	int * gameOverData = (int*)0x001E0D78;

	asm __volatile__ (
		"move %0, $sp"
		: : "r" (sp)
	);

	// get index of gamemode rank's stat in widestats
	// and return that as a float
	int currentRank = sp[8 + ((int (*)(int))0x0077a8f8)(gameOverData[2])];

	// uninstall hook
	*(u32*)0x0077b0d4 = 0x0C1DEAE6;

	// return current rank
	// don't compute new rank for base gamemode
	return (float)currentRank;
	//return ((float (*)(int))0x0077AB98)(playerIdx);
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

			// hook compute rank
			*(u32*)0x0077b0d4 = 0x0C000000 | ((u32)&computePlayerRank >> 2);
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
void loadStart(void)
{
	
}
