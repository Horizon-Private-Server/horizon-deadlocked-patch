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
const char * SURVIVAL_NEXT_ROUND_BEGIN_MESSAGE = "\x1c   start round";
const char * SURVIVAL_GAME_OVER = "GAME OVER";
const char * SURVIVAL_REVIVE_MESSAGE = "\x11   revive player         [\x0E%d\x08]";
const char * SURVIVAL_UPGRADE_MESSAGE = "\x11   upgrade to v%d       [\x0E%d\x08]";

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
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxHealth = ZOMBIE_BASE_HEALTH * 1.0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.5,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS
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
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxHealth = ZOMBIE_BASE_HEALTH * 2.0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_EXPLODE_HIT_RADIUS,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS
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
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxHealth = ZOMBIE_BASE_HEALTH * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS
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
			.Speed = ZOMBIE_BASE_SPEED * 0.8,
			.MaxHealth = ZOMBIE_BASE_HEALTH * 1.2,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.2,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS
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
			.Damage = ZOMBIE_BASE_DAMAGE * 0.8,
			.Speed = ZOMBIE_BASE_SPEED * 2.0,
			.MaxHealth = ZOMBIE_BASE_HEALTH * 0.6,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS
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
			.Speed = ZOMBIE_BASE_SPEED * 1.0,
			.MaxHealth = ZOMBIE_BASE_HEALTH * 1.0,
			.Bolts = ZOMBIE_BASE_BOLTS * 1.0,
			.AttackRadius = ZOMBIE_MELEE_ATTACK_RADIUS,
			.HitRadius = ZOMBIE_MELEE_HIT_RADIUS,
			.ReactionTickCount = ZOMBIE_BASE_REACTION_TICKS,
			.AttackCooldownTickCount = ZOMBIE_BASE_ATTACK_COOLDOWN_TICKS
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
void disableSetWeaponsOnRespawn(void) {
	*(u32*)0x005e2b2c = 0;
	*(u32*)0x005e2b40 = 0;
	*(u32*)0x005e2b48 = 0;
}

//--------------------------------------------------------------------------
void enableSetWeaponsOnRespawn(void) {
	*(u32*)0x005e2b2c = 0x0C178B9A;
	*(u32*)0x005e2b40 = 0x10600005;
	*(u32*)0x005e2b48 = 0x0C17DD44;
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
			d += randRange(0, 20);
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
	float r = randRange(0, 1);

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
	SpawnPoint* sp = spawnPointGet(rand(spawnPointGetCount()));
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
	
	return NULL;
}

//--------------------------------------------------------------------------
int spawnRandomMob(void) {
	VECTOR sp;
	if (spawnGetRandomPoint(sp)) {
		struct MobSpawnParams* mob = spawnGetRandomMobParams();
		if (mob) {
			if (mobCreate(sp, 0, &mob->Config)) {
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
void onPlayerUpgradeWeapon(int playerId, int weaponId, int level)
{
	Player* p = State.PlayerStates[playerId].Player;
	if (!p)
		return;

	PlayerWeaponData* wepData = playerGetWeaponData(playerId);
	playerGiveWeapon(p, weaponId, level);
	wepData[weaponId].Level = level;
	wepData[weaponId].AlphaMods[level] = rand(7);
	wepData[weaponId].Ammo = ((short (*)(PlayerWeaponData*, int))0x00626fb8)(wepData, weaponId);

	// play upgrade sound
	playUpgradeSound(p);

	// set exp bar to max
	if (level == VENDOR_MAX_WEAPON_LEVEL)
		wepData[weaponId].Experience = level;

#if LOG_STATS
	DPRINTF("%d (%08X) weapon %d upgraded to %d\n", playerId, (u32)p, weaponId, level);
#endif
}

//--------------------------------------------------------------------------
int onPlayerUpgradeWeaponRemote(void * connection, void * data)
{
	SurvivalWeaponUpgradeMessage_t * message = (SurvivalWeaponUpgradeMessage_t*)data;
	onPlayerUpgradeWeapon(message->PlayerId, message->WeaponId, message->Level);

	return sizeof(SurvivalWeaponUpgradeMessage_t);
}

//--------------------------------------------------------------------------
void playerUpgradeWeapon(Player* player, int weaponId)
{
	SurvivalWeaponUpgradeMessage_t message;
	PlayerWeaponData* wepData = playerGetWeaponData(player->PlayerId);

	// don't allow overwriting existing outcome
	if (State.RoundEndTime)
		return;

	// send out
	message.PlayerId = player->PlayerId;
	message.WeaponId = weaponId;
	message.Level = wepData[weaponId].Level + 1;
	netBroadcastCustomAppMessage(netGetDmeServerConnection(), CUSTOM_MSG_WEAPON_UPGRADE, sizeof(SurvivalWeaponUpgradeMessage_t), &message);

	// set locally
	onPlayerUpgradeWeapon(message.PlayerId, message.WeaponId, message.Level);
}

//--------------------------------------------------------------------------
void onPlayerRevive(int playerId)
{
	Player* player = State.PlayerStates[playerId].Player;
	if (!player)
		return;

	// backup current position/rotation
	VECTOR deadPos, deadRot;
	vector_copy(deadPos, player->PlayerPosition);
	vector_copy(deadRot, player->PlayerRotation);

	// respawn and warp
	disableSetWeaponsOnRespawn();
	playerRespawn(player);
	enableSetWeaponsOnRespawn();
	playerSetPosRot(player, deadPos, deadRot);
}

//--------------------------------------------------------------------------
int onPlayerReviveRemote(void * connection, void * data)
{
	SurvivalReviveMessage_t * message = (SurvivalReviveMessage_t*)data;
	onPlayerRevive(message->PlayerId);

	return sizeof(SurvivalReviveMessage_t);
}

//--------------------------------------------------------------------------
void playerRevive(Player* player)
{
	SurvivalReviveMessage_t message;

	// don't allow overwriting existing outcome
	if (State.RoundEndTime)
		return;

	// send out
	message.PlayerId = player->PlayerId;
	netSendCustomAppMessage(netGetDmeServerConnection(), player->Guber.Id.GID.HostId, CUSTOM_MSG_REVIVE_PLAYER, sizeof(SurvivalReviveMessage_t), &message);

	// set locally
	//onPlayerRevive(message.PlayerId);
}

//--------------------------------------------------------------------------
int canUpgradeWeapon(Player * player, enum WEAPON_IDS weaponId) {
	if (!player)
		return 0;

	PlayerWeaponData* wepData = playerGetWeaponData(player->PlayerId);

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
			return wepData[(int)weaponId].Level < VENDOR_MAX_WEAPON_LEVEL;
		default: return 0;
	}
}

//--------------------------------------------------------------------------
int getUpgradeCost(Player * player, enum WEAPON_IDS weaponId) {
	if (!player)
		return 0;

	PlayerWeaponData* wepData = playerGetWeaponData(player->PlayerId);
	return WEAPON_UPGRADE_BASE_COST + (WEAPON_UPGRADE_COST_PER_UPGRADE * wepData[(int)weaponId].Level);
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
	}
}

//--------------------------------------------------------------------------
int getPlayerReviveCost(Player * player) {
	if (!player)
		return 0;

	PlayerGameStats* stats = gameGetPlayerStats();
	return stats->Deaths[player->PlayerId] * (PLAYER_BASE_REVIVE_COST + (PLAYER_REVIVE_COST_PER_ROUND * State.RoundNumber));
}

//--------------------------------------------------------------------------
void processPlayer(int pIndex) {
	VECTOR t;
	int i = 0, localPlayerIndex, heldWeapon, hasMessage = 0;

	struct SurvivalPlayer * playerData = &State.PlayerStates[pIndex];
	Player * player = playerData->Player;
	if (!player || !player->IsLocal || playerIsDead(player))
		return;
	
	PlayerWeaponData* pWep = playerGetWeaponData(player->PlayerId);
	localPlayerIndex = player->LocalPlayerIndex;
	heldWeapon = player->WeaponHeldId;
	int upgradeCooldownTicks = decTimerU8(&playerData->UpgradeCooldownTicks);


	// set experience to min of level and max level 
	for (i = WEAPON_ID_VIPERS; i <= WEAPON_ID_FLAIL; ++i) {
		if (pWep[i].Level >= 0) {
			pWep[i].Experience = pWep[i].Level < VENDOR_MAX_WEAPON_LEVEL ? pWep[i].Level : VENDOR_MAX_WEAPON_LEVEL;
		}
	}

	// decrement flail ammo while spinning flail
	GameOptions* gameOptions = gameGetOptions();
	if (!gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo
		&& player->PlayerState == PLAYER_STATE_FLAIL_ATTACK
		&& player->timers.state >= 60) {
			PlayerWeaponData* pWep = playerGetWeaponData(pIndex);
			if (pWep[WEAPON_ID_FLAIL].Ammo > 0) {
				if ((player->timers.state % 60) == 0) {
					pWep[WEAPON_ID_FLAIL].Ammo -= 1;
				}
			} else {
				PlayerVTable* vtable = playerGetVTable(player);
				if (vtable && vtable->UpdateState)
					vtable->UpdateState(player, PLAYER_STATE_IDLE, 1, 1, 1);
			}
	}

	// handle vendor logic
	if (State.Vendor && upgradeCooldownTicks == 0) {

		// check player is holding upgradable weapon
		if (canUpgradeWeapon(player, heldWeapon)) {

			// check distance
			vector_subtract(t, player->PlayerPosition, State.Vendor->Position);
			if (vector_sqrmag(t) < (WEAPON_VENDOR_MAX_DIST * WEAPON_VENDOR_MAX_DIST)) {
				
				// get upgrade cost
				int cost = getUpgradeCost(player, heldWeapon);

				// draw help popup
				sprintf(LocalPlayerStrBuffer[localPlayerIndex], SURVIVAL_UPGRADE_MESSAGE, pWep[heldWeapon].Level+2, cost);
				uiShowPopup(localPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
				hasMessage = 1;

				// handle purchase
				if (playerData->State.Bolts >= cost) {

					// handle pad input
					if (padGetButtonDown(localPlayerIndex, PAD_CIRCLE) > 0) {
						playerData->State.Bolts -= cost;
						playerUpgradeWeapon(player, heldWeapon);
						uiShowPopup(localPlayerIndex, uiMsgString(0x2330));
						playerData->UpgradeCooldownTicks = WEAPON_UPGRADE_COOLDOWN_TICKS;
					}
				}
			}
		}
	}

	// handle revive logic
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		if (i != pIndex && !hasMessage) {

			// ensure player exists, is dead, and is on the same team
			struct SurvivalPlayer * otherPlayerData = &State.PlayerStates[i];
			Player * otherPlayer = otherPlayerData->Player;
			if (otherPlayer && playerIsDead(otherPlayer) && otherPlayer->Team == player->Team) {

				// check distance
				vector_subtract(t, player->PlayerPosition, otherPlayer->PlayerPosition);
				if (vector_sqrmag(t) < (PLAYER_REVIVE_MAX_DIST * PLAYER_REVIVE_MAX_DIST)) {
					
					// get revive cost
					int cost = getPlayerReviveCost(otherPlayer);

					// draw help popup
					sprintf(LocalPlayerStrBuffer[localPlayerIndex], SURVIVAL_REVIVE_MESSAGE, cost);
					uiShowPopup(localPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
					hasMessage = 1;

					// handle pad input
					if (padGetButtonDown(localPlayerIndex, PAD_CIRCLE) > 0 && playerData->State.Bolts >= cost) {
						playerData->State.Bolts -= cost;
						playerRevive(otherPlayer);
					}
				}
			}
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
		State.PlayerStates[i].State.Bolts += boltBonus;
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
	netBroadcastCustomAppMessage(netGetDmeServerConnection(), CUSTOM_MSG_ROUND_COMPLETE, sizeof(SurvivalRoundCompleteMessage_t), &message);

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
void setRoundStart(void)
{
	SurvivalRoundStartMessage_t message;

	// don't allow overwriting existing outcome
	if (State.RoundEndTime)
		return;

	// don't allow changing outcome when not host
	if (!State.IsHost)
		return;

	// send out
	message.RoundNumber = State.RoundNumber + 1;
	message.GameTime = gameGetTime();
	netBroadcastCustomAppMessage(netGetDmeServerConnection(), CUSTOM_MSG_ROUND_START, sizeof(SurvivalRoundStartMessage_t), &message);

	// set locally
	onSetRoundStart(message.RoundNumber, message.GameTime);
}

//--------------------------------------------------------------------------
void resetRoundState(void)
{
	GameSettings* gameSettings = gameGetSettings();
	int gameTime = gameGetTime();
	int i;
	Player ** players = playerGetAll();

	// 
	static int accum = 0;
	accum += State.RoundNumber * BUDGET_START_ACCUM;
	State.RoundBudget = BUDGET_START + accum + (State.RoundNumber * gameSettings->PlayerCountAtStart * BUDGET_PLAYER_WEIGHT);
	State.RoundInitialized = 0;
	State.RoundStartTime = gameTime;
	State.RoundCompleteTime = 0;
	State.RoundEndTime = 0;
	State.RoundMobCount = 0;
	State.RoundMobSpawnedCount = 0;
	State.RoundSpawnTicker = 0;
	State.MinMobCost = getMinMobCost();
	State.RoundMaxMobCount = MAX_MOBS_BASE + (MAX_MOBS_ROUND_WEIGHT * State.RoundNumber * gameSettings->PlayerCountAtStart);

	// 
	State.RoundInitialized = 1;
}

//--------------------------------------------------------------------------
void initialize(void)
{
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

	// Hook custom net events
	netInstallCustomMsgHandler(CUSTOM_MSG_ROUND_COMPLETE, &onSetRoundCompleteRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_ROUND_START, &onSetRoundStartRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_WEAPON_UPGRADE, &onPlayerUpgradeWeaponRemote);
	netInstallCustomMsgHandler(CUSTOM_MSG_REVIVE_PLAYER, &onPlayerReviveRemote);

	// set game over string (replaces "Draw!")
	strncpy((char*)0x015A00AA, SURVIVAL_GAME_OVER, 12);

	// disable v2s and packs
	cheatsApplyNoV2s();
	cheatsApplyNoPacks();

	// 
	mobInitialize();

	// initialize player states
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		State.PlayerStates[i].State.Bolts = 0;
		State.PlayerStates[i].Player = players[i];
		State.PlayerStates[i].UpgradeCooldownTicks = 0;
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

	// find vendor
#if defined(VENDOR_OCLASS)
	int vendorOClass = VENDOR_OCLASS;
	State.Vendor = mobyGetFirst();
	while (State.Vendor && State.Vendor->OClass != vendorOClass)
		State.Vendor = State.Vendor->PChain;
#endif

	// if can't find vendor then just spawn a beta box at a spawn point
	if (!State.Vendor) {
		SpawnPoint* vendorSp = spawnPointGet(1);

		//
		State.Vendor = mobySpawn(MOBY_ID_BETA_BOX, 0);
		vector_copy(State.Vendor->Position, &vendorSp->M0[12]);

#if DEBUG
		printf("no vendor oclass specified... spawned vendor box (%08X) at ", (u32)State.Vendor);
		vector_print(&vendorSp->M0[12]);
		printf("\n");
#endif

	}

	DPRINTF("vendor %08X\n", (u32)State.Vendor);

	// initialize state
	State.GameOver = 0;
	State.RoundNumber = 0;
	resetRoundState();

	Initialized = 1;
}

//--------------------------------------------------------------------------
void gameStart(void)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player ** players = playerGetAll();
	int i;
	char buffer[32];
	int gameTime = gameGetTime();
	static int startDelay = 60 * 5;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	// first
	dlPreUpdate();

	// Ensure in game
	if (!gameSettings || !gameIsIn())
		return;

	// Determine if host
	State.IsHost = gameAmIHost();

	if (!Initialized)
		initialize();

#if LOG_STATS
	static int statsTicker = 0;
	if (statsTicker <= 0) {
		DPRINTF("budget:%d liveMobCount:%d totalSpawnedThisRound:%d roundNumber:%d roundSpawnTicker:%d\n", State.RoundBudget, State.RoundMobCount, State.RoundMobSpawnedCount, State.RoundNumber, State.RoundSpawnTicker);
		statsTicker = 60 * 15;
	} else {
		--statsTicker;
	}
#endif

	if (padGetButton(0, PAD_L3 | PAD_R3) > 0)
		State.GameOver = 1;
	
#if MANUAL_SPAWN
	Player * localPlayer = (Player*)0x00347AA0;
	if (padGetButtonDown(0, PAD_DOWN) > 0) {
		static int manSpawnMobId = 0;
		VECTOR t;
		vector_copy(t, localPlayer->PlayerPosition);
		t[0] += 1;

		mobCreate(t, 0, &defaultSpawnParams[(manSpawnMobId++ % defaultSpawnParamsCount)].Config);
	}
#endif

	// draw round number
	sprintf(buffer, "%d", State.RoundNumber + 1);
	gfxScreenSpaceText(5, SCREEN_HEIGHT - 5, 1, 1, 0x80FFFFFF, buffer, -1, 6);

	if (!gameHasEnded() && !State.GameOver)
	{
		// decrement corn cob ticker
		decTimerU32(&State.CornTicker);

		// update local bolt count
		*LocalBoltCount = 0;
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			// since there is only one bolt count just show the sum of each local client's bolts
			if (State.PlayerStates[i].Player && State.PlayerStates[i].Player->IsLocal)
				*LocalBoltCount += State.PlayerStates[i].State.Bolts;
		}

		// replace normal scoreboard with bolt counter
		for (i = 0; i < 2; ++i)
		{
			PlayerHUDFlags* hudFlags = hudGetPlayerFlags(i);
			if (hudFlags) {
				hudFlags->Flags.BoltCounter = 1;
				hudFlags->Flags.NormalScoreboard = 0;
			}
		}

		// handle game over
		if (State.IsHost && gameOptions->GameFlags.MultiplayerGameFlags.Survivor)
		{
			int isAnyPlayerAlive = 0;
			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				if (players[i] && !playerIsDead(players[i])) {
					isAnyPlayerAlive = 1;
					break;
				}
			}

			// everyone died, end game
			if (!isAnyPlayerAlive) {
				State.GameOver = 1;
				gameSetWinner(10, 1);
			}
		}

		if (State.RoundCompleteTime)
		{
#if !AUTOSTART
			// draw round complete message
			if (gameTime < (State.RoundCompleteTime + ROUND_MESSAGE_DURATION_MS)) {
				sprintf(buffer, SURVIVAL_ROUND_COMPLETE_MESSAGE, State.RoundNumber+1);
				drawRoundMessage(buffer, 1.5);
			}
#endif

			if (State.RoundEndTime)
			{
				// handle when round properly ends
				if (gameTime > State.RoundEndTime)
				{
					// reset round state
					resetRoundState();
				}
			}
			else if (State.IsHost)
			{
#if AUTOSTART
				setRoundStart();
#else 
				gfxScreenSpaceText(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, 1, 1, 0x80FFFFFF, SURVIVAL_NEXT_ROUND_BEGIN_MESSAGE, -1, 4);

				if (padGetButtonDown(0, PAD_UP) > 0) {
					setRoundStart();
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

			for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
				processPlayer(i);
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
								State.RoundSpawnTicker = 20;
#else
								State.RoundSpawnTicker = 60;
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
								DPRINTF("\t\thealth: %f\n", defaultSpawnParams[i].Config.MaxHealth);
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
	return;
	int i;

	// deathmatch options
	static char options[] = { 
		0, 0, 			// 0x06 - 0x08
		0, 0, 0, 0, 	// 0x08 - 0x0C
		1, 1, 1, 0,  	// 0x0C - 0x10
		0, 1, 0, 0,		// 0x10 - 0x14
		-1, -1, 0, 1,	// 0x14 - 0x18
	};

	//
	GameSettings * gameSettings = gameGetSettings();
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		if (gameSettings->PlayerTeams[i] > 0)
			gameSettings->PlayerTeams[i] = 0;

	// set game options
	GameOptions * gameOptions = gameGetOptions();
	if (!gameOptions)
		return;
	
	// apply options
	memcpy((void*)&gameOptions->GameFlags.Raw[6], (void*)options, sizeof(options)/sizeof(char));
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
}

//--------------------------------------------------------------------------
void setEndGameScoreboard(void)
{
	
}

//--------------------------------------------------------------------------
void lobbyStart(void)
{
	int activeId = uiGetActive();
	static int initializedScoreboard = 0;

	// scoreboard
	switch (activeId)
	{
		case 0x15C:
		{
			if (initializedScoreboard)
				break;

			setEndGameScoreboard();
			initializedScoreboard = 1;
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
