/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Manages and applies all Deadlocked patches.
 * 
 * NOTES :
 * 		Each offset is determined per app id.
 * 		This is to ensure compatibility between versions of Deadlocked/Gladiator.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include "module.h"
#include "messageid.h"
#include "config.h"
#include "include/config.h"
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/sha1.h>
#include <libdl/utils.h>
#include <libdl/music.h>
#include <libdl/color.h>

/*
 * Array of game modules.
 */
#define GLOBAL_GAME_MODULES_START			((GameModule*)0x000CF000)

/*
 * Camera speed patch offsets.
 * Each offset is used by PatchCameraSpeed() to change the max
 * camera speed setting in game.
 */
#define CAMERA_SPEED_PATCH_OFF1			(*(u16*)0x00561BB8)
#define CAMERA_SPEED_PATCH_OFF2			(*(u16*)0x00561BDC)

#define UI_POINTERS						((u32*)0x011C7064)

#define ANNOUNCEMENTS_CHECK_PATCH		(*(u32*)0x00621D58)

#define GAMESETTINGS_LOAD_PATCH			(*(u32*)0x0072C3FC)
#define GAMESETTINGS_LOAD_FUNC			(0x0072EF78)
#define GAMESETTINGS_GET_INDEX_FUNC		(0x0070C410)
#define GAMESETTINGS_GET_VALUE_FUNC		(0x0070C538)

#define GAMESETTINGS_BUILD_PTR			(*(u32*)0x004B882C)
#define GAMESETTINGS_BUILD_FUNC			(0x00712BF0)

#define GAMESETTINGS_CREATE_PATCH		(*(u32*)0x0072E5B4)
#define GAMESETTINGS_CREATE_FUNC		(0x0070B540)

#define GAMESETTINGS_RESPAWN_TIME      	(*(char*)0x0017380C)
#define GAMESETTINGS_RESPAWN_TIME2      (*(char*)0x012B3638)

#define GAMESETTINGS_SURVIVOR						(*(u8*)0x00173806)
#define GAMESETTINGS_CRAZYMODE					(*(u8*)0x0017381F)

#define KILL_STEAL_WHO_HIT_ME_PATCH				(*(u32*)0x005E07C8)
#define KILL_STEAL_WHO_HIT_ME_PATCH2			(*(u32*)0x005E11B0)

#define FRAME_SKIP_WRITE0				(*(u32*)0x004A9400)
#define FRAME_SKIP						(*(u32*)0x0021E1D8)

#define VOICE_UPDATE_FUNC				((u32*)0x00161e60)

#define IS_PROGRESSIVE_SCAN					(*(int*)0x0021DE6C)

#define EXCEPTION_DISPLAY_ADDR			(0x000C8000)

#define SHRUB_RENDER_DISTANCE				(*(float*)0x0022308C)

#define DRAW_SHADOW_FUNC						((u32*)0x00587b30)

#define GADGET_EVENT_MAX_TLL				(*(short*)0x005DF5C8)
#define FUSION_SHOT_BACKWARDS_BRANCH 		(*(u32*)0x003FA614)

#define COLOR_CODE_EX1							(0x802020E0)
#define COLOR_CODE_EX2							(0x80E0E040)

#define GAME_UPDATE_SENDRATE				(5 * TIME_SECOND)


#define GAME_SCOREBOARD_ARRAY               ((ScoreboardItem**)0x002FA04C)
#define GAME_SCOREBOARD_ITEM_COUNT          (*(u32*)0x002F9FCC)

// 
void processSpectate(void);
void runMapLoader(void);
void onMapLoaderOnlineMenu(void);
void onConfigOnlineMenu(void);
void onConfigGameMenu(void);
void onConfigInitialize(void);
void onConfigUpdate(void);
void configMenuEnable(void);
void configMenuDisable(void);
void configTrySendGameConfig(void);

#if FREECAM
void processFreecam(void);
#endif

#if COMP
void runCompMenuLogic(void);
void runCompLogic(void);
#endif

#if MAPEDITOR
void onMapEditorGameUpdate(void);
#endif

// gamerules
void grGameStart(void);
void grLobbyStart(void);
void grLoadStart(void);

// 
int hasInitialized = 0;
int sentGameStart = 0;
int lastMenuInvokedTime = 0;
int lastGameState = 0;
int isInStaging = 0;
int hasSendReachedEndScoreboard = 0;
int hasInstalledExceptionHandler = 0;
int lastSurvivor = 0;
int lastRespawnTime = 5;
int lastCrazyMode = 0;
char mapOverrideResponse = 1;
char showNoMapPopup = 0;
const char * patchConfigStr = "PATCH CONFIG";
char weaponOrderBackup[2][3] = { {0,0,0}, {0,0,0} };
//char hasSetRanks = 0;
//ServerSetRanksRequest_t lastSetRanksRequest;

// 
struct GameDataBlock
{
  short Offset;
  short Length;
  char EndOfList;
  u8 Payload[484];
};

// 
PatchStateContainer_t patchStateContainer;

extern float _lodScale;
extern void* _correctTieLod;
extern MenuElem_ListData_t dataCustomMaps;

typedef struct ChangeTeamRequest {
	u32 Seed;
	int PoolSize;
	char Pool[GAME_MAX_PLAYERS];
} ChangeTeamRequest_t;

typedef struct SetLobbyClientPatchConfigRequest {
	int PlayerId;
	PatchConfig_t Config;
} SetLobbyClientPatchConfigRequest_t;

//
enum PlayerStateConditionType
{
	PLAYERSTATECONDITION_REMOTE_EQUALS,
	PLAYERSTATECONDITION_LOCAL_EQUALS,
	PLAYERSTATECONDITION_LOCAL_OR_REMOTE_EQUALS
};

typedef struct PlayerStateRemoteHistory
{
	int CurrentRemoteState;
	int TimeRemoteStateLastChanged;
	int TimeLastRemoteStateForced;
} PlayerStateRemoteHistory_t;

PlayerStateRemoteHistory_t RemoteStateTimeStart[GAME_MAX_PLAYERS];

typedef struct PlayerStateCondition
{
	enum PlayerStateConditionType Type;
	int TicksSince;
	int StateId;
	int MaxTicks; // number of ticks since start of the remote state before state is ignored
} PlayerStateCondition_t;

//
const PlayerStateCondition_t stateSkipRemoteConditions[] = {
	{	// skip when player is swinging
		.Type = PLAYERSTATECONDITION_LOCAL_OR_REMOTE_EQUALS,
		.TicksSince = 0,
		.StateId = PLAYER_STATE_SWING,
		.MaxTicks = 0
	},
	{	// skip when player is drowning
		.Type = PLAYERSTATECONDITION_LOCAL_OR_REMOTE_EQUALS,
		.TicksSince = 0,
		.StateId = PLAYER_STATE_DROWN,
		.MaxTicks = 0
	},
	{	// skip when player is falling into death void
		.Type = PLAYERSTATECONDITION_LOCAL_OR_REMOTE_EQUALS,
		.TicksSince = 0,
		.StateId = PLAYER_STATE_DEATH_FALL,
		.MaxTicks = 0
	},
};

const PlayerStateCondition_t stateForceRemoteConditions[] = {
	{ // force chargebooting
		.Type = PLAYERSTATECONDITION_LOCAL_OR_REMOTE_EQUALS,
		.TicksSince = 15,
		.StateId = PLAYER_STATE_CHARGE,
		.MaxTicks = 60
	},
	// { // force remote if local is still wrenching
	// 	PLAYERSTATECONDITION_LOCAL_EQUALS,
	// 	15,
	// 	19
	// },
	// { // force remote if local is still hyper striking
	// 	PLAYERSTATECONDITION_LOCAL_EQUALS,
	// 	15,
	// 	20
	// }
};

// 
PatchConfig_t config __attribute__((section(".config"))) = {
	.disableFramelimiter = 0,
	.enableGamemodeAnnouncements = 0,
	.enableSpectate = 0,
	.enableSingleplayerMusic = 0,
	.levelOfDetail = 1,
	.enablePlayerStateSync = 0,
	.enableAutoMaps = 0,
	.enableFpsCounter = 0,
	.disableCircleToHackerRay = 0,
	.playerAggTime = 0,
};

// 
PatchConfig_t lobbyPlayerConfigs[GAME_MAX_PLAYERS];
PatchGameConfig_t gameConfig;
PatchGameConfig_t gameConfigHostBackup;

/*
 * NAME :		patchCameraSpeed
 * 
 * DESCRIPTION :
 * 			Patches in-game camera speed setting to max out at 300%.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchCameraSpeed()
{
	const u16 SPEED = 0x100;
	char buffer[16];

	// Check if the value is the default max of 64
	// This is to ensure that we only write here when
	// we're in game and the patch hasn't already been applied
	if (CAMERA_SPEED_PATCH_OFF1 == 0x40)
	{
		CAMERA_SPEED_PATCH_OFF1 = SPEED;
		CAMERA_SPEED_PATCH_OFF2 = SPEED+1;
	}

	// Patch edit profile bar
	if (uiGetActive() == UI_ID_EDIT_PROFILE)
	{
		void * editProfile = (void*)UI_POINTERS[30];
		if (editProfile)
		{
			// get cam speed element
			void * camSpeedElement = (void*)*(u32*)(editProfile + 0xC0);
			if (camSpeedElement)
			{
				// update max value
				*(u32*)(camSpeedElement + 0x78) = SPEED;

				// get current value
				float value = *(u32*)(camSpeedElement + 0x70) / 64.0;

				// render
				sprintf(buffer, "%.0f%%", value*100);
				gfxScreenSpaceText(240,   166,   1, 1, 0x80000000, buffer, -1, 1);
				gfxScreenSpaceText(240-1, 166-1, 1, 1, 0x80FFFFFF, buffer, -1, 1);
			}
		}
	}
}

/*
 * NAME :		patchAnnouncements
 * 
 * DESCRIPTION :
 * 			Patches in-game announcements to work in all gamemodes.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchAnnouncements()
{
	u32 addrValue = ANNOUNCEMENTS_CHECK_PATCH;
	if (config.enableGamemodeAnnouncements && addrValue == 0x907E01A9)
		ANNOUNCEMENTS_CHECK_PATCH = 0x241E0000;
	else if (!config.enableGamemodeAnnouncements && addrValue == 0x241E0000)
		ANNOUNCEMENTS_CHECK_PATCH = 0x907E01A9;
}

/*
 * NAME :		patchResurrectWeaponOrdering_HookWeaponStripMe
 * 
 * DESCRIPTION :
 * 			Invoked during the resurrection process, when the game wishes to remove all weapons from the given player.
 * 			Before we continue to remove the player's weapons, we backup the list of equipped weapons.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchResurrectWeaponOrdering_HookWeaponStripMe(Player * player)
{
	// backup currently equipped weapons
	if (player->IsLocal) {
		weaponOrderBackup[player->LocalPlayerIndex][0] = playerGetLocalEquipslot(player->LocalPlayerIndex, 0);
		weaponOrderBackup[player->LocalPlayerIndex][1] = playerGetLocalEquipslot(player->LocalPlayerIndex, 1);
		weaponOrderBackup[player->LocalPlayerIndex][2] = playerGetLocalEquipslot(player->LocalPlayerIndex, 2);
	}

	// call hooked WeaponStripMe function after backup
	((void (*)(Player*))0x005e2e68)(player);
}

/*
 * NAME :		patchResurrectWeaponOrdering_HookGiveMeRandomWeapons
 * 
 * DESCRIPTION :
 * 			Invoked during the resurrection process, when the game wishes to give the given player a random set of weapons.
 * 			After the weapons are randomly assigned to the player, we check to see if the given weapons are the same as the last equipped weapon backup.
 * 			If they contain the same list of weapons (regardless of order), then we force the order of the new set of weapons to match the backup.
 * 
 * 			Consider the scenario:
 * 				Player dies with 								Fusion, B6, Magma Cannon
 * 				Player is assigned 							B6, Fusion, Magma Cannon
 * 				Player resurrects with  				Fusion, B6, Magma Cannon
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchResurrectWeaponOrdering_HookGiveMeRandomWeapons(Player* player, int weaponCount)
{
	int i, j, matchCount = 0;

	// call hooked GiveMeRandomWeapons function first
	((void (*)(Player*, int))0x005f7510)(player, weaponCount);

	// then try and overwrite given weapon order if weapons match equipped weapons before death
	if (player->IsLocal) {

		// restore backup if they match (regardless of order) newly assigned weapons
		for (i = 0; i < 3; ++i) {
			int backedUpSlotValue = weaponOrderBackup[player->LocalPlayerIndex][i];
			for (j = 0; j < 3; ++j) {
				if (backedUpSlotValue == playerGetLocalEquipslot(player->LocalPlayerIndex, j)) {
					matchCount++;
					DPRINTF("WEAPON ORDERING MATCH %d (backup:%d => new:%d)\n", backedUpSlotValue, i, j);
				}
			}
		}

		// if we found a match, set
		if (matchCount == 3) {
			DPRINTF("RESTORING BACKED UP WEAPON ORDER\n");

			// set equipped weapon in order
			for (i = 0; i < 3; ++i) {
				playerSetLocalEquipslot(player->LocalPlayerIndex, i, weaponOrderBackup[player->LocalPlayerIndex][i]);
			}

			// equip first slot weapon
			playerEquipWeapon(player, weaponOrderBackup[player->LocalPlayerIndex][0]);
		}
	}
}

/*
 * NAME :		patchResurrectWeaponOrdering
 * 
 * DESCRIPTION :
 * 			Installs necessary hooks such that when respawning with same weapons,
 * 			they are equipped in the same order.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchResurrectWeaponOrdering(void)
{
	if (!isInGame())
		return;

	HOOK_JAL(0x005e2b2c, &patchResurrectWeaponOrdering_HookWeaponStripMe);
	HOOK_JAL(0x005e2b48, &patchResurrectWeaponOrdering_HookGiveMeRandomWeapons);
}

/*
 * NAME :		patchGameSettingsLoad_Save
 * 
 * DESCRIPTION :
 * 			Saves the given value to the respective game setting.
 * 			I think this does validation depending on the input type then saves.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 			a0:					Points to the general settings area
 * 			offset0:			Offset to game setting
 * 			offset1:			Offset to game setting input type handler?
 * 			value:				Value to save
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchGameSettingsLoad_Save(void * a0, int offset0, int offset1, int value)
{
	u32 step1 = *(u32*)((u32)a0 + offset0);
	u32 step2 = *(u32*)(step1 + 0x58);
	u32 step3 = *(u32*)(step2 + offset1);
	((void (*)(u32, int))step3)(step1, value);
}

/*
 * NAME :		patchGameSettingsLoad_Hook
 * 
 * DESCRIPTION :
 * 			Called when loading previous game settings into create game.
 * 			Reloads Survivor correctly.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchGameSettingsLoad_Hook(void * a0, void * a1)
{
	int index = 0;

	// Load normal
	((void (*)(void *, void *))GAMESETTINGS_LOAD_FUNC)(a0, a1);

	// Get gametype
	index = ((int (*)(int))GAMESETTINGS_GET_INDEX_FUNC)(5);
	int gamemode = ((int (*)(void *, int))GAMESETTINGS_GET_VALUE_FUNC)(a1, index);

	// Handle each gamemode separately
	switch (gamemode)
	{
		case GAMERULE_DM:
		{
			// Save survivor
			patchGameSettingsLoad_Save(a0, 0x100, 0xA4, lastSurvivor);
			break;
		}
		case GAMERULE_CTF:
		{
			// Save crazy mode
			patchGameSettingsLoad_Save(a0, 0x10C, 0xA4, !lastCrazyMode);
			break;
		}
	}

	if (gamemode != GAMERULE_CQ)
	{
		// respawn timer
		GAMESETTINGS_RESPAWN_TIME2 = lastRespawnTime; //*(u8*)0x002126DC;
	}

	if (GAMESETTINGS_RESPAWN_TIME2 < 0)
		GAMESETTINGS_RESPAWN_TIME2 = lastRespawnTime;
}

/*
 * NAME :		patchGameSettingsLoad
 * 
 * DESCRIPTION :
 * 			Patches game settings load so it reloads Survivor correctly.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchGameSettingsLoad()
{
	if (GAMESETTINGS_LOAD_PATCH == 0x0C1CBBDE)
	{
		GAMESETTINGS_LOAD_PATCH = 0x0C000000 | ((u32)&patchGameSettingsLoad_Hook >> 2);
	}
}

/*
 * NAME :		patchPopulateCreateGame_Hook
 * 
 * DESCRIPTION :
 * 			Patches create game populate setting to add respawn timer.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchPopulateCreateGame_Hook(void * a0, int settingsCount, u32 * settingsPtrs)
{
	u32 respawnTimerPtr = 0x012B35D8;
	int i = 0;

	// Check if already loaded
	for (; i < settingsCount; ++i)
	{
		if (settingsPtrs[i] == respawnTimerPtr)
			break;
	}

	// If not loaded then append respawn timer
	if (i == settingsCount)
	{
		++settingsCount;
		settingsPtrs[i] = respawnTimerPtr;
	}

	// Populate
	((void (*)(void *, int, u32 *))GAMESETTINGS_BUILD_FUNC)(a0, settingsCount, settingsPtrs);
}

/*
 * NAME :		patchPopulateCreateGame
 * 
 * DESCRIPTION :
 * 			Patches create game populate setting to add respawn timer.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchPopulateCreateGame()
{
	// Patch function pointer
	if (GAMESETTINGS_BUILD_PTR == GAMESETTINGS_BUILD_FUNC)
	{
		GAMESETTINGS_BUILD_PTR = (u32)&patchPopulateCreateGame_Hook;
	}

	// Patch default respawn timer
	GAMESETTINGS_RESPAWN_TIME = lastRespawnTime;
}

/*
 * NAME :		patchCreateGame_Hook
 * 
 * DESCRIPTION :
 * 			Patches create game save settings to save respawn timer.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
u64 patchCreateGame_Hook(void * a0)
{
	// Save respawn timer if not survivor
	lastSurvivor = GAMESETTINGS_SURVIVOR;
	lastRespawnTime = GAMESETTINGS_RESPAWN_TIME2;
	lastCrazyMode = GAMESETTINGS_CRAZYMODE;

	// Load normal
	return ((u64 (*)(void *))GAMESETTINGS_CREATE_FUNC)(a0);
}

/*
 * NAME :		patchCreateGame
 * 
 * DESCRIPTION :
 * 			Patches create game save settings to save respawn timer.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchCreateGame()
{
	// Patch function pointer
	if (GAMESETTINGS_CREATE_PATCH == 0x0C1C2D50)
	{
		GAMESETTINGS_CREATE_PATCH = 0x0C000000 | ((u32)&patchCreateGame_Hook >> 2);
	}
}

/*
 * NAME :		patchWideStats_Hook
 * 
 * DESCRIPTION :
 * 			Copies over game mode ranks into respective slot in medius stats.
 * 			This just ensures that the ranks in the database always match up with the rank appearing in game.
 * 			Since, after login, the game tracks its own copies of ranks regardless of the ranks stored in the accounts wide stats.
 * 			This causes problems with custom gamemodes, which ignore rank changes as they are handled separately by the server.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchWideStats_Hook(void)
{
	int* mediusStats = (int*)0x001722C0;
	int* stats = *(int**)0x0017277c;

	if (stats) {
		// store each respective gamemode rank in player medius stats
		mediusStats[1] = stats[17]; // CQ
		mediusStats[2] = stats[24]; // CTF
		mediusStats[3] = stats[11]; // DM
		mediusStats[4] = stats[31]; // KOTH
		mediusStats[5] = stats[38]; // JUGGY
	}
}

/*
 * NAME :		patchWideStats
 * 
 * DESCRIPTION :
 * 			Writes hook into the callback of MediusGetLadderStatsWide.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchWideStats(void)
{
	*(u32*)0x0015C0EC = 0x08000000 | ((u32)&patchWideStats_Hook >> 2);
}

/*
 * NAME :		patchKillStealing_Hook
 * 
 * DESCRIPTION :
 * 			Filters out hits when player is already dead.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int patchKillStealing_Hook(Player * target, Moby * damageSource, u64 a2)
{
	// if player is already dead return 0
	if (target->Health <= 0)
		return 0;

	// pass through
	return ((int (*)(Player*,Moby*,u64))0x005DFF08)(target, damageSource, a2);
}

/*
 * NAME :		patchKillStealing
 * 
 * DESCRIPTION :
 * 			Patches who hit me on weapon hit with patchKillStealing_Hook.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchKillStealing()
{
	// 
	if (KILL_STEAL_WHO_HIT_ME_PATCH == 0x0C177FC2)
	{
		KILL_STEAL_WHO_HIT_ME_PATCH = 0x0C000000 | ((u32)&patchKillStealing_Hook >> 2);
		KILL_STEAL_WHO_HIT_ME_PATCH2 = KILL_STEAL_WHO_HIT_ME_PATCH;
	}
}

/*
 * NAME :		patchAggTime
 * 
 * DESCRIPTION :
 * 			Sets agg time to user configured value.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchAggTime(void)
{
	int aggTime = 30 + (config.playerAggTime * 5);
	u16* currentAggTime = (u16*)0x0015ac04;

	// only update on change
	if (*currentAggTime == aggTime)
		return;

	*currentAggTime = aggTime;
	void * connection = netGetDmeServerConnection();
	if (connection)
		netSetSendAggregationInterval(connection, 0, aggTime);
}

/*
 * NAME :		patchFrameSkip
 * 
 * DESCRIPTION :
 * 			Patches frame skip to always be off.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchFrameSkip()
{
	int addrValue = FRAME_SKIP_WRITE0;
	
	if (config.disableFramelimiter && addrValue == 0xAF848859)
	{
		FRAME_SKIP_WRITE0 = 0;
		FRAME_SKIP = 0;
	}
	else if (!config.disableFramelimiter && addrValue == 0)
	{
		FRAME_SKIP_WRITE0 = 0xAF848859;
	}
}

/*
 * NAME :		patchWeaponShotNetSendFlag
 * 
 * DESCRIPTION :
 * 			Patches weapon shot to be sent over TCP instead of UDP.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchWeaponShotNetSendFlag(void)
{
	/*
	u32* ptr = (u32*)0x00627AB4;
	if (*ptr == 0x906407F8) {
		// change to reliable
		*ptr = 0x24040000 | 0x40;

		// get rid of additional 3 packets sent
		// since its reliable we don't need redundancy
		*(u32*)0x0060F474 = 0;
		*(u32*)0x0060F4C4 = 0;
	}
	*/

	// send fusion shot reliably
	if (*(u32*)0x003FCE8C == 0x910407F8)
		*(u32*)0x003FCE8C = 0x24040040;
}

/*
 * NAME :		patchStateUpdate_Hook
 * 
 * DESCRIPTION :
 * 			Performs some extra logic to optimize sending state updates.
 * 			State updates are sent by default every 4 frames.
 * 			Every 16 frames a full state update is sent. This sends additional data as position and player state.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int patchStateUpdate_Hook(void * a0, void * a1)
{
	int v0 = ((int (*)(void*,void*))0x0061e130)(a0, a1);
	Player * p = (Player*)((u32)a0 - 0x2FEC);

	// when we're dead we don't really need to send the state very often
	// so we'll only send it every second
	// and when we do we'll send a full update (including position and player state)
	if (p->Health <= 0)
	{
		// only send every 60 frames
		int tick = *(int*)((u32)a0 + 0x1D8);
		if (tick % 60 != 0)
			return 0;

		// set to 1 to force full state update
		*(u8*)((u32)p + 0x31cf) = 1;
	}
	else
	{
		// set to 1 to force full state update
		*(u8*)((u32)p + 0x31cf) = 1;
	}

	return v0;
}

/*
 * NAME :		patchStateUpdate
 * 
 * DESCRIPTION :
 * 			Patches state update get size function to call our hooked method.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchStateUpdate(void)
{
	if (*(u32*)0x0060eb80 == 0x0C18784C)
		*(u32*)0x0060eb80 = 0x0C000000 | ((u32)&patchStateUpdate_Hook >> 2);
}

/*
 * NAME :		runCorrectPlayerChargebootRotation
 * 
 * DESCRIPTION :
 * 			Forces all remote player's rotations while chargebooting to be in the direction of their velocity.
 * 			This fixes some movement lag caused when a player's rotation is desynced from their chargeboot direction.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void runCorrectPlayerChargebootRotation(void)
{
	int i;
	VECTOR t;
	Player** players = playerGetAll();

	if (!isInGame())
		return;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player* p = players[i];

		// only apply to remote players chargebooting
		if (p && !p->IsLocal && p->PlayerState == PLAYER_STATE_CHARGE) {

			// get horizontal velocity
			vector_copy(t, p->Velocity);
			t[2] = 0;
			float speed = vector_length(t);

			// apply threshold to prevent small velocity from affecting rotation
			if (speed > 0.05) {

				// convert velocity to yaw
				float yaw = atan2f(t[1] / speed, t[0] / speed);

				// get delta between angles
				float dt = clampAngle(yaw - p->PlayerRotation[2]);
				
				// if delta is small, ignore
				if (fabsf(dt) > 0.001) {

					// apply delta rotation to hero turn function
					// at interpolation speed of 1/5 second
					((void (*)(Player*, float, float, float))0x005e4850)(p, 0, 0, dt * 0.01666 * 5);
				}
			}
		}
	}
}

/*
 * NAME :		runSendGameUpdate
 * 
 * DESCRIPTION :
 * 			Sends the current game info to the server.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int runSendGameUpdate(void)
{
	static int lastGameUpdate = 0;
	static int newGame = 0;
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	int gameTime = gameGetTime();
	int i;
	void * connection = netGetLobbyServerConnection();

	// skip if not online, in lobby, or the game host
	if (!connection || !gameSettings || !gameAmIHost())
	{
		lastGameUpdate = -GAME_UPDATE_SENDRATE;
		newGame = 1;
		return 0;
	}

	// skip if time since last update is less than sendrate
	if ((gameTime - lastGameUpdate) < GAME_UPDATE_SENDRATE)
		return 0;

	// update last sent time
	lastGameUpdate = gameTime;

	// construct
	patchStateContainer.GameStateUpdate.RoundNumber = 0;
	patchStateContainer.GameStateUpdate.TeamsEnabled = gameOptions->GameFlags.MultiplayerGameFlags.Teamplay;

	// copy over client ids
	memcpy(patchStateContainer.GameStateUpdate.ClientIds, gameSettings->PlayerClients, sizeof(patchStateContainer.GameStateUpdate.ClientIds));

	// reset some stuff whenever we enter a new game
	if (newGame)
	{
		memset(patchStateContainer.GameStateUpdate.TeamScores, 0, sizeof(patchStateContainer.GameStateUpdate.TeamScores));
		memset(patchStateContainer.CustomGameStats.Payload, 0, sizeof(patchStateContainer.CustomGameStats.Payload));
		newGame = 0;
	}

	// 
	if (isInGame())
	{
		memset(patchStateContainer.GameStateUpdate.TeamScores, 0, sizeof(patchStateContainer.GameStateUpdate.TeamScores));

		for (i = 0; i < GAME_SCOREBOARD_ITEM_COUNT; ++i)
		{
			ScoreboardItem * item = GAME_SCOREBOARD_ARRAY[i];
			if (item)
				patchStateContainer.GameStateUpdate.TeamScores[item->TeamId] = item->Value;
		}
	}

	// copy teams over
	memcpy(patchStateContainer.GameStateUpdate.Teams, gameSettings->PlayerTeams, sizeof(patchStateContainer.GameStateUpdate.Teams));

	return 1;
}

/*
 * NAME :		runEnableSingleplayerMusic
 * 
 * DESCRIPTION :
 * 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Troy "Agent Moose" Pruitt
 */
void runEnableSingleplayerMusic(void)
{
	static int FinishedConvertingTracks = 0;
	static int AddedTracks = 0;

	if (!config.enableSingleplayerMusic || !musicIsLoaded())
		return;

	u32 NewTracksLocation = 0x001CF940;
	if (!FinishedConvertingTracks || *(u32*)NewTracksLocation == 0)
	{
		AddedTracks = 0;
		int MultiplayerSectorID = *(u32*)0x001CF85C;
		int Stack = 0x0023ad00; // Original Location is 0x23ac00, but moving it somewhat fixes a bug with the SectorID.  But also, unsure if this breaks anything yet.
		int Sector = 0x001CE470;
		int a;
		int Offset = 0;

		// Zero out stack by the appropriate heap size (0x2a0 in this case)
		// This makes sure we get the correct values we need later on.
		memset((u32*)Stack, 0, 0x2A0);

		// Loop through each Sector
		for(a = 0; a < 12; a++)
		{
			Offset += 0x18;
			int MapSector = *(u32*)(Sector + Offset);
			// Check if Map Sector is not zero
			if (MapSector != 0)
			{
				internal_wadGetSectors(MapSector, 1, Stack);
				int SectorID = *(u32*)(Stack + 0x4);

				// BUG FIX AREA: If Stack is set to 0x23ac00, you need to add SectorID != 0x1DC1BE to if statement.
				// The bug is: On first load, the SectorID isn't what I need it to be,
				// the internal_wadGetSectors function doesn't update it quick enough for some reason.
				// the following if statement fixes it

				// make sure SectorID doesn't match 0x1dc1be, if so:
				// - Subtract 0x18 from offset and -1 from loop.
				if (SectorID != 0x0)
				{
					DPRINTF("Sector: 0x%X\n", MapSector);
					DPRINTF("Sector ID: 0x%X\n", SectorID);

					// do music stuffs~
					// Get SP 2 MP Offset for current SectorID.
					int SP2MP = SectorID - MultiplayerSectorID;
					// Remember we skip the first track because it is the start of the sp track, not the body of it.
					int b = 0;
					int Songs = Stack + 0x18;
					// while current song doesn't equal zero, then convert.
					// if it does equal zero, that means we reached the end of the list and we move onto the next batch of tracks.
					do
					{
						// Left Audio
						int StartingSong = *(u32*)(Songs + b);
						// Right Audio
						int EndingSong = *(u32*)((u32)(Songs + b) + 0x8);
						// Convert Left/Right Audio
						int ConvertedSong_Start = SP2MP + StartingSong;
						int ConvertedSong_End = SP2MP + EndingSong;
						// Apply newly Converted tracks
						*(u32*)(NewTracksLocation) = ConvertedSong_Start;
						*(u32*)(NewTracksLocation + 0x08) = ConvertedSong_End;
						NewTracksLocation += 0x10;
						// If on DreadZone Station, and first song, add 0x20 instead of 0x20
						// This fixes an offset bug.
						if (a == 0 && b == 0)
						{
							b += 0x28;
						}
						else
						{
							b += 0x20;
						}
						AddedTracks++;
					}
					while (*(u32*)(Songs + b) != 0);
				}
				else
				{
					Offset -= 0x18;
					a--;
				}
			}
			else
			{
				a--;
			}
		}
		// Zero out stack to finish the job.
		memset((u32*)Stack, 0, 0x2A0);

		FinishedConvertingTracks = 1;
		DPRINTF("AddedTracks: %d\n", AddedTracks);
	};
	

	int DefaultMultiplayerTracks = 0x0d; // This number will never change
	int StartingTrack = *(u8*)0x0021EC08;
	int AllTracks = DefaultMultiplayerTracks + AddedTracks;
	// Fun fact: The math for TotalTracks is exactly how the assembly is.  Didn't mean to do it that way.  (Minus the AddedTracks part)
	int TotalTracks = (DefaultMultiplayerTracks - StartingTrack + 1) + AddedTracks;
	int MusicDataPointer = *(u32*)0x0021DA24; // This is more than just music data pointer, but it's what Im' using it for.
	int CurrentTrack = *(u16*)0x00206990;
	// If not in main lobby, game lobby, ect.
	if(MusicDataPointer != 0x01430700){
		// if Last Track doesn't equal TotalTracks
		if(*(u32*)0x0021EC0C != TotalTracks){
			int MusicFunctionData = MusicDataPointer + 0x28A0D4;
			*(u16*)MusicFunctionData = AllTracks;
		}
	}

	// If in game
	if(isInGame())
	{
		int TrackDuration = *(u32*)0x002069A4;
		if (*(u32*)0x002069A0 <= 0)
		{
			/*
				This part: (CurrentTrack != -1 && *(u32*)0x020698C != 0)
				fixes a bug when switching tracks, and running the command
				to set your own track or stop a track.
			*/
			if ((CurrentTrack > DefaultMultiplayerTracks * 2) && (CurrentTrack != -1 && *(u32*)0x020698C != 0) && (TrackDuration <= 0x3000))
			{
				// This techinally cues track 1 (the shortest track) with no sound to play.
				// Doing this lets the current playing track to fade out.
				musicTransitionTrack(0,0,0,0);
			}
		}
	}
}

/*
 * NAME :		runGameStartMessager
 * 
 * DESCRIPTION :
 * 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void runGameStartMessager(void)
{
	GameSettings * gameSettings = gameGetSettings();
	if (!gameSettings)
		return;

	// in staging
	if (uiGetActive() == UI_ID_GAME_LOBBY)
	{
		// check if game started
		if (!sentGameStart && gameSettings->GameLoadStartTime > 0)
		{
			// check if host
			if (gameAmIHost())
			{
				netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GAME_LOBBY_STARTED, 0, gameSettings);
			}
			
			sentGameStart = 1;
		}
	}
	else
	{
		sentGameStart = 0;
	}
}

int checkStateCondition(const PlayerStateCondition_t * condition, int localState, int remoteState)
{
	switch (condition->Type)
	{
		case PLAYERSTATECONDITION_REMOTE_EQUALS: // check remote is and local isn't
		{
			return condition->StateId == remoteState;
		}
		case PLAYERSTATECONDITION_LOCAL_EQUALS: // check local is and remote isn't
		{
			return condition->StateId == localState;
		}
		case PLAYERSTATECONDITION_LOCAL_OR_REMOTE_EQUALS: // check local or remote is
		{
			return condition->StateId == remoteState || condition->StateId == localState;
		}
	}

	return 0;
}

/*
 * NAME :		runPlayerStateSync
 * 
 * DESCRIPTION :
 * 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void runPlayerStateSync(void)
{
	const int stateForceCount = sizeof(stateForceRemoteConditions) / sizeof(PlayerStateCondition_t);
	const int stateSkipCount = sizeof(stateSkipRemoteConditions) / sizeof(PlayerStateCondition_t);
	int gameTime = gameGetTime();
	Player ** players = playerGetAll();
	int i,j;

	if (!isInGame() || !config.enablePlayerStateSync)
		return;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player* p = players[i];
		if (p && !playerIsLocal(p))
		{
			// get remote state
			int localState = p->PlayerState;
			int remoteState = *(int*)((u32)p + 0x3a80);
			if (RemoteStateTimeStart[i].CurrentRemoteState != remoteState)
			{
				RemoteStateTimeStart[i].CurrentRemoteState = remoteState;
				RemoteStateTimeStart[i].TimeRemoteStateLastChanged = gameTime;
			}
			int remoteStateTicks = ((gameTime - RemoteStateTimeStart[i].TimeRemoteStateLastChanged) / 1000) * 60;

			// force onto local state
			PlayerVTable* vtable = playerGetVTable(p);
			if (!playerIsDead(p) && vtable && remoteState != localState)
			{
				int pStateTimer = p->timers.state;
				int skip = 0;

				// iterate each condition
				// if one is true, skip to the next player
				for (j = 0; j < stateSkipCount; ++j)
				{
					const PlayerStateCondition_t* condition = &stateSkipRemoteConditions[j];
					if (pStateTimer >= condition->TicksSince)
					{
						if (checkStateCondition(condition, localState, remoteState))
						{
							DPRINTF("%d skipping remote player %08x (%d) state (%d) timer:%d\n", j, (u32)p, p->PlayerId, remoteState, pStateTimer);
							skip = 1;
							break;
						}
					}
				}

				// go to next player
				if (skip)
					continue;

				// iterate each condition
				// if one is true, then force the remote state onto the local player
				for (j = 0; j < stateForceCount; ++j)
				{
					const PlayerStateCondition_t* condition = &stateForceRemoteConditions[j];
					if (pStateTimer >= condition->TicksSince
							&& (condition->MaxTicks <= 0 || remoteStateTicks < condition->MaxTicks)
							&& (gameTime - RemoteStateTimeStart[i].TimeLastRemoteStateForced) > 500)
					{
						if (checkStateCondition(condition, localState, remoteState))
						{
							if (condition->MaxTicks > 0 && remoteState != condition->StateId) {
								DPRINTF("%d changing remote player %08x (%d) state ticks to %d (from %d) state:%d\n", j, (u32)p, p->PlayerId, condition->MaxTicks, p->timers.state, localState);
								p->timers.state = condition->MaxTicks;
							} else {
								DPRINTF("%d changing remote player %08x (%d) state to %d (from %d) timer:%d\n", j, (u32)p, p->PlayerId, remoteState, localState, pStateTimer);
								vtable->UpdateState(p, remoteState, 1, 1, 1);
								p->timers.state = remoteStateTicks;
							}
							RemoteStateTimeStart[i].TimeLastRemoteStateForced = gameTime;
							break;
						}
					}
				}
			}
		}
	}
}

/*
 * NAME :		onGameStartMenuBack
 * 
 * DESCRIPTION :
 * 			Called when the user selects 'Back' in the in game start menu
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onGameStartMenuBack(long a0)
{
	// call start menu back callback
	((void (*)(long))0x00560E30)(a0);

	// open config
	if (netGetLobbyServerConnection())
		configMenuEnable();
}

/*
 * NAME :		patchStartMenuBack_Hook
 * 
 * DESCRIPTION :
 * 			Changes the BACK button text to our patch config text.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Troy "Agent Moose" Pruitt
 */
void patchStartMenuBack_Hook(long a0, u64 a1, u64 a2, u8 a3)
{
    // Force Start Menu to swap "Back" Option with "Patch Config"
    /*
        a0: Unknown
        a1: String
        a2: Function Pointer
        a3: Unknown
    */
    ((void (*)(long, const char*, void*, u8))0x005600F8)(a0, patchConfigStr, &onGameStartMenuBack, a3);
}

/*
 * NAME :		hookedProcessLevel
 * 
 * DESCRIPTION :
 * 		 	Function hook that the game will invoke when it is about to start processing a newly loaded level.
 * 			The argument passed is the address of the function to call to start processing the level.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
u64 hookedProcessLevel()
{
	u64 r = ((u64 (*)(void))0x001579A0)();

	// ensure that the game is loading and not exiting
	GameSettings * gs = gameGetSettings();
	if (gs && gs->GameStartTime < 0)
	{
		// Start at the first game module
		GameModule * module = GLOBAL_GAME_MODULES_START;

		// call gamerules level load
		grLoadStart();
	
		// pass event to modules
		while (module->GameEntrypoint || module->LobbyEntrypoint)
		{
			if (module->State > GAMEMODULE_OFF && module->LoadEntrypoint)
				module->LoadEntrypoint(module, &config, &gameConfig, &patchStateContainer);

			++module;
		}
	}

	return r;
}

/*
 * NAME :		patchProcessLevel
 * 
 * DESCRIPTION :
 * 			Installs hook at where the game starts the process a newly loaded level.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchProcessLevel(void)
{
	// jal hookedProcessLevel
	*(u32*)0x00157D38 = 0x0C000000 | (u32)&hookedProcessLevel / 4;
}

/*
 * NAME :		sendGameDataBlock
 * 
 * DESCRIPTION :
 * 			Sends a block of data over to the server as a GameDataBlock.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int sendGameDataBlock(short offset, char endOfList, void* buffer, int size)
{
	int i = 0;
	struct GameDataBlock data;

	data.Offset = offset;
	data.EndOfList = endOfList;

	// send in chunks
	while (i < size)
	{
		short len = (size - i);
		if (len > sizeof(data.Payload))
			len = sizeof(data.Payload);

		data.Length = len;
		memcpy(data.Payload, (char*)buffer + i, len);
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_SEND_GAME_DATA, sizeof(struct GameDataBlock), &data);
		i += len;
		data.Offset += len;
	}

	// return written bytes
	return i;
}

/*
 * NAME :		sendGameData
 * 
 * DESCRIPTION :
 * 			Sends all game data, including stats, to the server when the game ends.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void sendGameData(void)
{
  GameOptions* gameOptions = gameGetOptions();
  GameSettings* gameSettings = gameGetSettings();
  GameData* gameData = gameGetData();
	short offset = 0;

	// ensure in game/staging
	if (!gameSettings)
		return;
	
	DPRINTF("sending stats... ");
	offset += sendGameDataBlock(offset, 0, gameData, sizeof(GameData));
	offset += sendGameDataBlock(offset, 0, &patchStateContainer.GameSettingsAtStart, sizeof(GameSettings));
	offset += sendGameDataBlock(offset, 0, gameSettings, sizeof(GameSettings));
	offset += sendGameDataBlock(offset, 0, gameOptions, sizeof(GameOptions));
	offset += sendGameDataBlock(offset, 0, &patchStateContainer.CustomGameStats, sizeof(CustomGameModeStats_t));
	offset += sendGameDataBlock(offset, 1, &patchStateContainer.GameStateUpdate, sizeof(UpdateGameStateRequest_t));
	DPRINTF("done.\n");
}

/*
 * NAME :		processSendGameData
 * 
 * DESCRIPTION :
 * 			Logic that determines when to send game data to the server.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int processSendGameData(void)
{
	static int state = 0;
  GameSettings* gameSettings = gameGetSettings();
	int send = 0;

	// ensure in game/staging
	if (!gameSettings)
		return 0;
	
	if (isInGame())
	{
		// move game settings
		if (state == 0)
		{
			memcpy(&patchStateContainer.GameSettingsAtStart, gameSettings, sizeof(GameSettings));
			state = 1;
		}

		// game has ended
		if (state == 1 && gameGetFinishedExitTime() && gameAmIHost())
			send = 1;
	}
	else if (state > 0)
	{
		// host leaves the game
		if (state == 1 && gameAmIHost())
			send = 1;

		state = 0;
	}

	// 
	if (send)
		state = 2;

	return send;
}

/*
 * NAME :		runFpsCounter
 * 
 * DESCRIPTION :
 * 			Displays an fps counter on the top right screen.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void runFpsCounter(void)
{
	char buf[16];
	static int lastGameTime = 0;
	static int tickCounter = 0;
	static float lastFps = 0;

	// initialize time
	if (tickCounter == 0 && lastGameTime == 0)
		lastGameTime = gameGetTime();
	
	// update fps every 60 ticks/frames
	++tickCounter;
	if (tickCounter >= 60)
	{
		int currentTime = gameGetTime();
		lastFps = tickCounter / ((currentTime - lastGameTime) / (float)TIME_SECOND);
		lastGameTime = currentTime;
		tickCounter = 0;
	}

	// render if enabled
	if (config.enableFpsCounter)
	{			
		snprintf(buf, 16, "%.2f", lastFps);
		gfxScreenSpaceText(SCREEN_WIDTH - 5, 5, 0.75, 0.75, 0x80FFFFFF, buf, -1, 2);
	}
}

int hookCheckHostStartGame(void* a0)
{
	// call base
	int v0 = ((int (*)(void*))0x00757660)(a0);

	if (v0 && mapOverrideResponse < 0)
	{
		v0 = 0;
		showNoMapPopup = 1;
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_REQUEST_MAP_OVERRIDE, 0, NULL);
	}

	return v0;
}

/*
 * NAME :		runCheckGameMapInstalled
 * 
 * DESCRIPTION :
 * 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void runCheckGameMapInstalled(void)
{
	int i;
	GameSettings* gs = gameGetSettings();
	if (!gs || isInGame())
		return;

	// install start game hook
	if (*(u32*)0x00759580 == 0x0C1D5D98)
		*(u32*)0x00759580 = 0x0C000000 | ((u32)&hookCheckHostStartGame >> 2);

	int clientId = gameGetMyClientId();
	if (mapOverrideResponse < 0)
	{
		for (i = 1; i < GAME_MAX_PLAYERS; ++i)
		{
			if (gs->PlayerClients[i] == clientId && gs->PlayerStates[i] == 6)
			{
				gameSetClientState(i, 0);
				showNoMapPopup = 1;
				netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_REQUEST_MAP_OVERRIDE, 0, NULL);
			}
		}
	}
}

/*
 * NAME :		processGameModules
 * 
 * DESCRIPTION :
 * 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void processGameModules()
{
	// Start at the first game module
	GameModule * module = GLOBAL_GAME_MODULES_START;

	// Game settings
	GameSettings * gamesettings = gameGetSettings();

	// Iterate through all the game modules until we hit an empty one
	while (module->GameEntrypoint || module->LobbyEntrypoint)
	{
		// Ensure we have game settings
		if (gamesettings)
		{
			// Check the module is enabled
			if (module->State > GAMEMODULE_OFF)
			{
				// If in game, run game entrypoint
				if (isInGame())
				{
					// Check if the game hasn't ended
					// We also give the module a second after the game has ended to
					// do some end game logic
					if (!gameHasEnded() || gameGetTime() < (gameGetFinishedExitTime() + TIME_SECOND))
					{
						// Invoke module
						if (module->GameEntrypoint)
							module->GameEntrypoint(module, &config, &gameConfig, &patchStateContainer);
					}
				}
				else if (isInMenus())
				{
					// Invoke lobby module if still active
					if (module->LobbyEntrypoint)
					{
						module->LobbyEntrypoint(module, &config, &gameConfig, &patchStateContainer);
					}
				}
			}

		}
		// If we aren't in a game then try to turn the module off
		// ONLY if it's temporarily enabled
		else if (module->State == GAMEMODULE_TEMP_ON)
		{
			module->State = GAMEMODULE_OFF;
		}
		else if (module->State == GAMEMODULE_ALWAYS_ON)
		{
			// Invoke lobby module if still active
			if (isInMenus() && module->LobbyEntrypoint)
			{
				module->LobbyEntrypoint(module, &config, &gameConfig, &patchStateContainer);
			}
		}

		++module;
	}
}

/*
 * NAME :		onSetTeams
 * 
 * DESCRIPTION :
 * 			Called when the server requests the client to change the lobby's teams.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int onSetTeams(void * connection, void * data)
{
	int i, j;
  ChangeTeamRequest_t request;
	u32 seed;
	char teamByClientId[GAME_MAX_PLAYERS];

	// move message payload into local
	memcpy(&request, data, sizeof(ChangeTeamRequest_t));

	// move seed
	memcpy(&seed, &request.Seed, 4);

	//
	memset(teamByClientId, -1, sizeof(teamByClientId));

#if DEBUG
	printf("pool size: %d\npool: ", request.PoolSize);
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		printf("%d=%d,", i, request.Pool[i]);
	printf("\n");
#endif

	// get game settings
	GameSettings* gameSettings = gameGetSettings();
	if (gameSettings)
	{
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			int clientId = gameSettings->PlayerClients[i];
			if (clientId >= 0)
			{
				int teamId = teamByClientId[clientId];
				if (teamId < 0)
				{
					if (request.PoolSize == 0)
					{
						teamId = 0;
					}
					else
					{
						// psuedo random
						sha1(&seed, 4, &seed, 4);

						// get pool index from rng
						int teamPoolIndex = seed % request.PoolSize;
						DPRINTF("pool info pid:%d poolIndex:%d poolSize:%d\n", i, teamPoolIndex, request.PoolSize);

						// set team
						teamId = request.Pool[teamPoolIndex];

						// remove element from pool
						if (request.PoolSize > 0)
						{
							for (j = teamPoolIndex+1; j < request.PoolSize; ++j)
								request.Pool[j-1] = request.Pool[j];
							request.PoolSize -= 1;
						}
					}

					// set client id team
					teamByClientId[clientId] = teamId;
				}

				// set team
				DPRINTF("setting pid:%d to %d\n", i, teamId);
				gameSettings->PlayerTeams[i] = teamId;
			}
		}
	}

	return sizeof(ChangeTeamRequest_t);
}

/*
 * NAME :		onSetLobbyClientPatchConfig
 * 
 * DESCRIPTION :
 * 			Called when the server broadcasts a lobby client's patch config.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int onSetLobbyClientPatchConfig(void * connection, void * data)
{
	int i;
  SetLobbyClientPatchConfigRequest_t request;
	GameSettings* gs = gameGetSettings();

	// move message payload into local
	memcpy(&request, data, sizeof(SetLobbyClientPatchConfigRequest_t));

	if (request.PlayerId >= 0 && request.PlayerId < GAME_MAX_PLAYERS) {
		DPRINTF("recieved %d config\n", request.PlayerId);

		
		memcpy(&lobbyPlayerConfigs[request.PlayerId], &request.Config, sizeof(PatchConfig_t));
	}

	return sizeof(SetLobbyClientPatchConfigRequest_t);
}

/*
 * NAME :		onSetRanks
 * 
 * DESCRIPTION :
 * 			Called when the server requests the client to change the lobby's ranks.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
/*
int onSetRanks(void * connection, void * data)
{
	int i, j;

	// move message payload into local
	memcpy(&lastSetRanksRequest, data, sizeof(ServerSetRanksRequest_t));
	hasSetRanks = -1;

	return sizeof(ServerSetRanksRequest_t);
}
*/

/*
 */
#if SCR_PRINT

#define SCRPRINT_RINGSIZE (16)
#define SCRPRINT_BUFSIZE  (96)
int scrPrintTop = 0, scrPrintBottom = 0;
int scrPrintStepTicker = 60 * 5;
char scrPrintRingBuf[SCRPRINT_RINGSIZE][SCRPRINT_BUFSIZE];

int scrPrintHook(int fd, char* buf, int len)
{
	// add to our ring buf
	int n = len < SCRPRINT_BUFSIZE ? len : SCRPRINT_BUFSIZE;
	memcpy(scrPrintRingBuf[scrPrintTop], buf, n);
	scrPrintRingBuf[scrPrintTop][n] = 0;
	scrPrintTop = (scrPrintTop + 1) % SCRPRINT_RINGSIZE;

	// loop bottom to end of ring
	if (scrPrintBottom == scrPrintTop)
		scrPrintBottom = (scrPrintTop + 1) % SCRPRINT_RINGSIZE;

	return ((int (*)(int, char*, int))0x00127168)(fd, buf, len);
}

void handleScrPrint(void)
{
	int i = scrPrintTop;
	float x = 15, y = SCREEN_HEIGHT - 15;

	// hook
	*(u32*)0x00123D38 = 0x0C000000 | ((u32)&scrPrintHook >> 2);

	if (scrPrintTop != scrPrintBottom)
	{
		// draw
		do
		{
			i--;
			if (i < 0)
				i = SCRPRINT_RINGSIZE - 1;

			gfxScreenSpaceText(x+1, y+1, 0.7, 0.7, 0x80000000, scrPrintRingBuf[i], -1, 0);
			gfxScreenSpaceText(x, y, 0.7, 0.7, 0x80FFFFFF, scrPrintRingBuf[i], -1, 0);

			y -= 12;
		}
		while (i != scrPrintBottom);

		// move bottom to top
		if (scrPrintStepTicker == 0)
		{
			scrPrintBottom = (scrPrintBottom + 1) % SCRPRINT_RINGSIZE;
			scrPrintStepTicker = 60 * 5;
		}
		else
		{
			scrPrintStepTicker--;
		}
	}
}
#endif

/*
 * NAME :		onOnlineMenu
 * 
 * DESCRIPTION :
 * 			Called every ui update in menus.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onOnlineMenu(void)
{

	// call normal draw routine
	((void (*)(void))0x00707F28)();
	
	// 
	lastMenuInvokedTime = gameGetTime();

	//
	if (!hasInitialized)
	{
		padEnableInput();
		onConfigInitialize();
		memset(lobbyPlayerConfigs, 0, sizeof(lobbyPlayerConfigs));
		hasInitialized = 1;
	}

	// 
	if (hasInitialized == 1)
	{
		uiShowOkDialog("System", "Patch has been successfully loaded.");
		hasInitialized = 2;
	}

	// map loader
	onMapLoaderOnlineMenu();

#if COMP
	// run comp patch logic
	runCompMenuLogic();
#endif

	// settings
	onConfigOnlineMenu();

	// 
	if (showNoMapPopup)
	{
		if (mapOverrideResponse == -1)
		{
			uiShowOkDialog("Custom Maps", "You have not installed the map modules.");
		}
		else
		{
#if MAPDOWNLOADER
			char buf[32];
			sprintf(buf, "Would you like to download the map now?", dataCustomMaps.items[(int)gameConfig.customMapId]);
			
			//uiShowOkDialog("Custom Maps", buf);
			if (uiShowYesNoDialog("Required Map Update", buf) == 1)
			{
				ClientInitiateMapDownloadRequest_t msg = {
					.MapId = (int)gameConfig.customMapId
				};
				netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_INITIATE_DOWNLOAD_MAP_REQUEST, sizeof(ClientInitiateMapDownloadRequest_t), &msg);
			}
#else
			char buf[32];
			sprintf(buf, "Please install %s to play.", dataCustomMaps.items[(int)gameConfig.customMapId]);
			uiShowOkDialog("Custom Maps", buf);
#endif
		}

		showNoMapPopup = 0;
	}
}

/*
int sampleCount = 0;
int isSampling = 0;
int deltaTotal = 0;
int deltaCount = 0;
int counts[200];
const int SAMPLE_SIZE = 250;

int dot_callback(void * connection, void * data)
{
	int time = *(int*)data;
	int delta = gameGetTime() - time;

	//printf("delta %d\n", delta);
	counts[delta]++;
	deltaTotal += delta;
	deltaCount++;

	return sizeof(int);
}

int dot_callback2(void * connection, void * data)
{
	int time = *(int*)data;
	int delta = gameGetTime() - time;

	printf("delta %d\n", delta);

	return sizeof(int);
}

void dot(void)
{

	netInstallCustomMsgHandler(101, &dot_callback);
	netInstallCustomMsgHandler(102, &dot_callback2);

	int i = 0;
	int gameTime = gameGetTime();
	GameSettings* gs = gameGetSettings();
	if (!gs)
		return;

	void * dmeConnection = netGetDmeServerConnection();
	if (!dmeConnection)
		return;

	if (!isSampling && padGetButtonDown(0, PAD_L1) > 0) {
		printf("sending %d samples\n", SAMPLE_SIZE);
		isSampling = 1;
		sampleCount = 0;
		deltaCount = 0;
		deltaTotal = 0;
		memset(counts, 0, sizeof(counts));
	}
	else if (!isSampling && padGetButtonDown(0, PAD_R1) > 0) {
		netBroadcastCustomAppMessage(NET_LATENCY_CRITICAL, dmeConnection, 102, sizeof(gameTime), &gameTime);
	}

	if (isSampling) {
		if (sampleCount >= SAMPLE_SIZE) {
			if (isSampling < 200) {
				isSampling++;
				return;
			}
			isSampling = 0;

			float avgDt = (float)deltaTotal / (float)deltaCount;
			int minDt = (int)avgDt;
			int maxDt = (int)avgDt;
			for (i = 0; i < 200; ++i) {
				if (i < minDt && counts[i] > 0)
					minDt = i;
				if (i > maxDt && counts[i] > 0)
					maxDt = i;
			}

			printf("received %d samples with average latency %f, min:%d max:%d\n", deltaCount, avgDt, minDt, maxDt);
			for (i = 0; i < 200; ++i) {
				if (counts[i] > 0) {
					printf("\t%d: %d\n", i, counts[i]);
				}
			}
		} else {
			netBroadcastCustomAppMessage(NET_LATENCY_CRITICAL, dmeConnection, 101, sizeof(gameTime), &gameTime);
			++sampleCount;
		}
	}
}
*/

/*
 * NAME :		main
 * 
 * DESCRIPTION :
 * 			Applies all patches and modules.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int main (void)
{
	int i;
	
	// Call this first
	dlPreUpdate();

	// auto enable pad input to prevent freezing when popup shows
	if (lastMenuInvokedTime > 0 && gameGetTime() - lastMenuInvokedTime > TIME_SECOND)
	{
		padEnableInput();
		lastMenuInvokedTime = 0;
	}

	//
	if (!hasInitialized)
	{
		DPRINTF("patch loaded\n");
		onConfigInitialize();
		hasInitialized = 1;

		if (isInGame()) {
			uiShowPopup(0, "Patch has been successfully loaded.");
			hasInitialized = 2;
		}
	}

	// invoke exception display installer
	if (*(u32*)EXCEPTION_DISPLAY_ADDR != 0)
	{
		if (!hasInstalledExceptionHandler)
		{
			((void (*)(void))EXCEPTION_DISPLAY_ADDR)();
			hasInstalledExceptionHandler = 1;
		}
		
		// change display to match progressive scan resolution
		if (IS_PROGRESSIVE_SCAN)
		{
			*(u16*)(EXCEPTION_DISPLAY_ADDR + 0x9F4) = 0x0083;
			*(u16*)(EXCEPTION_DISPLAY_ADDR + 0x9F8) = 0x210E;
		}
		else
		{
			*(u16*)(EXCEPTION_DISPLAY_ADDR + 0x9F4) = 0x0183;
			*(u16*)(EXCEPTION_DISPLAY_ADDR + 0x9F8) = 0x2278;
		}
	}

#if SCR_PRINT
	handleScrPrint();
#endif

	// install net handlers
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_REQUEST_TEAM_CHANGE, &onSetTeams);
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_SET_LOBBY_CLIENT_PATCH_CONFIG_REQUEST, &onSetLobbyClientPatchConfig);
	//netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_SET_RANKS, &onSetRanks);

	// Run map loader
	runMapLoader();

#if COMP
	// run comp patch logic
	runCompLogic();
#endif

	//dot();
	
	// 
	runCheckGameMapInstalled();

	// Run game start messager
	runGameStartMessager();

	// Run sync player state
	runPlayerStateSync();

	// 
	runCorrectPlayerChargebootRotation();

	// 
	runFpsCounter();

	// Run add singleplayer music
	runEnableSingleplayerMusic();

	// Patch camera speed
	patchCameraSpeed();

	// Patch announcements
	patchAnnouncements();

	// Patch create game settings load
	patchGameSettingsLoad();

	// Patch populate create game
	patchPopulateCreateGame();

	// Patch save create game settings
	patchCreateGame();

	// Patch frame skip
	patchFrameSkip();

	// Patch weapon shot to be sent reliably
	patchWeaponShotNetSendFlag();

	// Patch state update to run more optimized
	patchStateUpdate();

	// Patch process level call
	patchProcessLevel();

	// Patch kill stealing
	patchKillStealing();

	// Patch resurrect weapon ordering
	patchResurrectWeaponOrdering();

	// 
	//patchWideStats();

	// 
	patchAggTime();

	// 
	patchStateContainer.UpdateCustomGameStats = processSendGameData();

	// 
	patchStateContainer.UpdateGameState = runSendGameUpdate();

	// config update
	onConfigUpdate();

	// in game stuff
	if (isInGame())
	{
		// reset when in game
		hasSendReachedEndScoreboard = 0;

	#if DEBUG
		if (padGetButtonDown(0, PAD_L3 | PAD_R3) > 0)
		{
			gameEnd(0);
		}
	#endif

		//
		grGameStart();

		// this lets guber events that are < 5 seconds old be processed (original is 1.2 seconds)
		//GADGET_EVENT_MAX_TLL = 5 * TIME_SECOND;

		// put hacker ray in weapon select
		GameSettings * gameSettings = gameGetSettings();
		if (gameSettings && gameSettings->GameRules == GAMERULE_CQ)
		{
			// put hacker ray in weapon select
			*(u32*)0x0038A0DC = WEAPON_ID_HACKER_RAY;

			// disable/enable press circle to equip hacker ray
			*(u32*)0x005DE870 = config.disableCircleToHackerRay ? 0x24040000 : 0x00C0202D;
		}

		// this disables filtering out fusion shots where the player is facing the opposite direction
		// in other words, a player may appear to shoot behind them but it's just lag/desync
		FUSION_SHOT_BACKWARDS_BRANCH = 0x1000005F;

		// close config menu on transition to lobby
		if (lastGameState != 1)
			configMenuDisable();

		// patch lod
		if (*(u32*)0x005930B8 == 0x02C3B020)
		{
			*(u32*)0x005930B8 = 0x08000000 | ((u32)&_correctTieLod >> 2);
		}

		// correct lod
		switch (config.levelOfDetail)
		{
			case 0: // low
			{
				_lodScale = 0.2;
				SHRUB_RENDER_DISTANCE = 50;
				*DRAW_SHADOW_FUNC = 0x03E00008;
				*(DRAW_SHADOW_FUNC + 1) = 0;
				break;
			}
			case 1: // normal
			{
				_lodScale = 1.0;
				SHRUB_RENDER_DISTANCE = 500;
				*DRAW_SHADOW_FUNC = 0x27BDFF90;
				*(DRAW_SHADOW_FUNC + 1) = 0xFFB30038;
				break;
			}
			case 2: // high
			{
				_lodScale = 10.0;
				SHRUB_RENDER_DISTANCE = 5000;
				*DRAW_SHADOW_FUNC = 0x27BDFF90;
				*(DRAW_SHADOW_FUNC + 1) = 0xFFB30038;
				break;
			}
		}

		// Hook game start menu back callback
		if (*(u32*)0x005605D4 == 0x0C15803E)
		{
			*(u32*)0x005605D4 = 0x0C000000 | ((u32)&patchStartMenuBack_Hook / 4);
		}

		// patch red and brown as last two color codes
		*(u32*)0x00391978 = COLOR_CODE_EX1;
		*(u32*)0x0039197C = COLOR_CODE_EX2;

		// if survivor is enabled then set the respawn time to -1
		GameOptions* gameOptions = gameGetOptions();
	  if (gameOptions && gameOptions->GameFlags.MultiplayerGameFlags.Survivor)
			gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0xFF;

		// trigger config menu update
		onConfigGameMenu();

#if MAPEDITOR
		// trigger map editor update
		onMapEditorGameUpdate();
#endif

		lastGameState = 1;
	}
	else if (isInMenus())
	{
		//
		grLobbyStart();

		// Hook menu loop
		if (*(u32*)0x00594CBC == 0)
			*(u32*)0x00594CB8 = 0x0C000000 | ((u32)(&onOnlineMenu) / 4);

		// send patch game config on create game
		GameSettings * gameSettings = gameGetSettings();
		if (gameSettings && gameSettings->GameLoadStartTime < 0)
		{
			// if host and just entered staging, send patch game config
			if (gameAmIHost() && !isInStaging)
			{
				// copy over last game config as host
				memcpy(&gameConfig, &gameConfigHostBackup, sizeof(PatchGameConfig_t));

				// send
				configTrySendGameConfig();
			}

			// try and apply ranks
			/*
			if (hasSetRanks && hasSetRanks != gameSettings->PlayerCount)
			{
				for (i = 0; i < GAME_MAX_PLAYERS; ++i)
				{
					int accountId = lastSetRanksRequest.AccountIds[i];
					if (accountId >= 0)
					{
						for (j = 0; j < GAME_MAX_PLAYERS; ++j)
						{
							if (gameSettings->PlayerAccountIds[j] == accountId)
							{
								gameSettings->PlayerRanks[j] = lastSetRanksRequest.Ranks[i];
							}
						}
					}
				}

				hasSetRanks = gameSettings->PlayerCount;
			}
			*/

			isInStaging = 1;
		}
		else
		{
			isInStaging = 0;
			//hasSetRanks = 0;
		}

		// send game reached end scoreboard
		if (!hasSendReachedEndScoreboard)
		{
			if (gameSettings && gameSettings->GameStartTime > 0 && uiGetActive() == 0x15C && gameAmIHost())
			{
				hasSendReachedEndScoreboard = 1;
				netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_GAME_LOBBY_REACHED_END_SCOREBOARD, 0, NULL);
			}
			else if (!gameSettings)
			{
				// disable if not in a lobby (must be in a lobby to send)
				hasSendReachedEndScoreboard = 1;
			}
		}

		// patch server hostname
		/*
		if (0)
		{
			char * muisServerHostname = (char*)0x001B1ECD;
			char * serverHostname = (char*)0x004BF4F0;
			if (!gameSettings && strlen(muisServerHostname) > 0)
			{
				for (i = 0; i < 32; ++i)
				{
					char c = muisServerHostname[i];
					if (c < 0x20)
						c = '.';
					serverHostname[i] = c;
				}
			}
		}
		*/

		// patch red and brown as last two color codes
		*(u32*)0x004C8A68 = COLOR_CODE_EX1;
		*(u32*)0x004C8A6C = COLOR_CODE_EX2;
		
		// close config menu on transition to lobby
		if (lastGameState != 0)
			configMenuDisable();

		lastGameState = 0;
	}

	// Process spectate
	if (config.enableSpectate)
		processSpectate();

#if FREECAM
	processFreecam();
#endif

	// Process game modules
	processGameModules();

	//
	if (patchStateContainer.UpdateGameState) {
		patchStateContainer.UpdateGameState = 0;
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_SET_GAME_STATE, sizeof(UpdateGameStateRequest_t), &patchStateContainer.GameStateUpdate);
	}

	// 
	if (patchStateContainer.UpdateCustomGameStats) {
		patchStateContainer.UpdateCustomGameStats = 0;
		sendGameData();
	}

	// Call this last
	dlPostUpdate();

	return 0;
}
