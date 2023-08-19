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
#include <libdl/collision.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/radar.h>
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

#define PASSWORD_BUFFER									((char*)0x0017233C)

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
int readLocalGlobalVersion(void);

extern int mapsRemoteGlobalVersion;
extern int mapsLocalGlobalVersion;

void resetFreecam(void);
void processFreecam(void);

#if COMP
void runCompMenuLogic(void);
void runCompLogic(void);
#endif

#if TEST
void runTestLogic(void);
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
int lastClientType = -1;
int lastAccountId = -1;
char mapOverrideResponse = 1;
char showNeedLatestMapsPopup = 0;
char showNoMapPopup = 0;
char showMiscPopup = 0;
char miscPopupTitle[32];
char miscPopupBody[64];
const char * patchConfigStr = "PATCH CONFIG";
char weaponOrderBackup[2][3] = { {0,0,0}, {0,0,0} };
float lastFps = 0;
int renderTimeMs = 0;
float averageRenderTimeMs = 0;
int updateTimeMs = 0;
float averageUpdateTimeMs = 0;
int timeLocalPlayerPickedUpFlag[2] = {0,0};
int localPlayerHasFlag[2] = {0,0};
int redownloadCustomModeBinaries = 0;
ServerSetRanksRequest_t lastSetRanksRequest;

extern int dlIsActive;

int lastLodLevel = 2;
const int lodPatchesPotato[][2] = {
	{ 0x0050e318, 0x03E00008 }, // disable corn
	{ 0x0050e31c, 0x00000000 },
	{ 0x0041d400, 0x03E00008 }, // disable small weapon explosion
	{ 0x0041d404, 0x00000000 },
	{ 0x003F7154, 0x00000000 }, // disable b6 ball shadow
	{ 0x003A18F0, 0x24020000 }, // disable b6 particles
	{ 0x0042EA50, 0x24020000 }, // disable mag particles
	{ 0x0043C150, 0x24020000 }, // disable mag shells
	{ 0x003A18F0, 0x24020000 }, // disable fusion shot particles
	{ 0x0042608C, 0x00000000 }, // disable jump pad blur effect
};

const int lodPatchesNormal[][2] = {
	{ 0x0050e318, 0x27BDFE40 }, // enable corn
	{ 0x0050e31c, 0x7FB40100 },
	{ 0x0041d400, 0x27BDFF00 }, // enable small weapon explosion
	{ 0x0041d404, 0x7FB00060 },
	{ 0x003F7154, 0x0C140F40 }, // enable b6 ball shadow
	{ 0x003A18F0, 0x0C13DC80 }, // enable b6 particles
	{ 0x0042EA50, 0x0C13DC80 }, // enable mag particles
	{ 0x0043C150, 0x0C13DC80 }, // enable mag shells
	{ 0x003A18F0, 0x0C13DC80 }, // enable fusion shot particles
	{ 0x0042608C, 0x0C131194 }, // enable jump pad blur effect
};


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
int isUnloading __attribute__((section(".config"))) = 0;
PatchConfig_t config __attribute__((section(".config"))) = {
	.framelimiter = 2,
	.enableGamemodeAnnouncements = 0,
	.enableSpectate = 0,
	.enableSingleplayerMusic = 0,
	.levelOfDetail = 2,
	.enablePlayerStateSync = 0,
	.enableAutoMaps = 0,
	.enableFpsCounter = 0,
	.disableCircleToHackerRay = 0,
	.playerAggTime = 0,
  .playerFov = 0,
  .preferredGameServer = 0,
  .enableSingleTapChargeboot = 0
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
				}
			}
		}

		// if we found a match, set
		if (matchCount == 3) {
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
 * NAME :		patchLevelOfDetail
 * 
 * DESCRIPTION :
 * 			Sets the level of detail.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchLevelOfDetail(void)
{
	int i = 0;

	// only apply in game
	if (!isInGame()) {
		lastLodLevel = -1;
		return;
	}

	// patch lod
	if (*(u32*)0x005930B8 == 0x02C3B020)
	{
		*(u32*)0x005930B8 = 0x08000000 | ((u32)&_correctTieLod >> 2);
	}

  if (lastClientType == CLIENT_TYPE_NORMAL) {

    // force lod on certain maps
    int lod = config.levelOfDetail;
    if (lastClientType == CLIENT_TYPE_NORMAL) {
      switch (gameConfig.customMapId)
      {
        case CUSTOM_MAP_CANAL_CITY:
        {
          lod = 0; // always potato on canal city
          break;
        }
      }
    }

    // correct lod
    int lodChanged = lod != lastLodLevel;
    switch (lod)
    {
      case 0: // potato
      {
        _lodScale = 0.2;
        SHRUB_RENDER_DISTANCE = 50;
        *DRAW_SHADOW_FUNC = 0x03E00008;
        *(DRAW_SHADOW_FUNC + 1) = 0;

        if (lodChanged)
        {
          // set terrain and tie render distance
          POKE_U16(0x00223158, 120);
          *(float*)0x002230F0 = 120 * 1024;

          for (i = 0; i < sizeof(lodPatchesPotato) / (2 * sizeof(int)); ++i)
            POKE_U32(lodPatchesPotato[i][0], lodPatchesPotato[i][1]);
        }
        break;
      }
      case 1: // low
      {
        _lodScale = 0.2;
        SHRUB_RENDER_DISTANCE = 50;
        *DRAW_SHADOW_FUNC = 0x03E00008;
        *(DRAW_SHADOW_FUNC + 1) = 0;

        if (lodChanged)
        {
          // set terrain and tie render distance
          POKE_U16(0x00223158, 480);
          *(float*)0x002230F0 = 480 * 1024;

          for (i = 0; i < sizeof(lodPatchesNormal) / (2 * sizeof(int)); ++i)
            POKE_U32(lodPatchesNormal[i][0], lodPatchesNormal[i][1]);

          // disable jump pad blur
          POKE_U32(0x0042608C, 0);
        }
        break;
      }
      case 2: // normal
      {
        _lodScale = 1.0;
        SHRUB_RENDER_DISTANCE = 500;
        *DRAW_SHADOW_FUNC = 0x27BDFF90;
        *(DRAW_SHADOW_FUNC + 1) = 0xFFB30038;
        POKE_U16(0x00223158, 960);
        *(float*)0x002230F0 = 960 * 1024;

        if (lodChanged)
        {
          // set terrain and tie render distance
          POKE_U16(0x00223158, 960);
          *(float*)0x002230F0 = 960 * 1024;

          for (i = 0; i < sizeof(lodPatchesNormal) / (2 * sizeof(int)); ++i)
            POKE_U32(lodPatchesNormal[i][0], lodPatchesNormal[i][1]);
        }
        break;
      }
      case 3: // high
      {
        _lodScale = 10.0;
        SHRUB_RENDER_DISTANCE = 5000;
        *DRAW_SHADOW_FUNC = 0x27BDFF90;
        *(DRAW_SHADOW_FUNC + 1) = 0xFFB30038;

        for (i = 0; i < sizeof(lodPatchesNormal) / (2 * sizeof(int)); ++i)
          POKE_U32(lodPatchesNormal[i][0], lodPatchesNormal[i][1]);
        break;
      }
    }
  }

	// backup lod
	lastLodLevel = config.levelOfDetail;
}

/*
 * NAME :		patchCameraShake
 * 
 * DESCRIPTION :
 * 			Disables camera shake.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchCameraShake(void)
{
	if (!isInGame())
		return;

	if (config.disableCameraShake)
	{
		POKE_U32(0x004b14a0, 0x03E00008);
		POKE_U32(0x004b14a4, 0);
	}
	else
	{
		POKE_U32(0x004b14a0, 0x34030470);
		POKE_U32(0x004b14a4, 0x27BDFF70);
	}
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
int patchGameSettings_OpenPasswordInputDialog(void * a0, char * title, char * value, int a3, int maxLength, int t1, int t2, int t3, int t4)
{
	strncpy((char*)value, PASSWORD_BUFFER, maxLength);

	return internal_uiInputDialog(a0, title, value, a3, maxLength, t1, t2, t3, t4);
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

	// enable pw input prompt when password is true
	POKE_U32(0x0072E4A0, 0x0000182D);
	HOOK_JAL(0x0072e510, &patchGameSettings_OpenPasswordInputDialog);
	
	// allow user to enable/disable password
	POKE_U32(0x0072C408, 0x0000282D);
	

	// password
	if (strlen(PASSWORD_BUFFER) > 0) {

		// set default to on
		patchGameSettingsLoad_Save(a0, 0xC4, 0xA4, 1);
		DPRINTF("pw %s\n", PASSWORD_BUFFER);
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
 * NAME :		patchComputePoints_Hook
 * 
 * DESCRIPTION :
 * 			The game uses one function to compute points for each player at the end of the game.
 *      These points are used to determine MVP, assign more/less MMR, etc.
 *      For competitive reasons, 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int patchComputePoints_Hook(int playerIdx)
{
  GameData* gameData = gameGetData();
  int basePoints = ((int (*)(int))0x00625510)(playerIdx);
  int newPoints = basePoints;

  if (gameData) {
    
    // saves are worth half a cap (10 pts)
    newPoints += gameData->PlayerStats.CtfFlagsSaved[playerIdx] * 5;

  }

  //DPRINTF("points for %d => base:%d ours:%d\n", playerIdx, basePoints, newPoints);

  return newPoints;
}

/*
 * NAME :		patchComputePoints
 * 
 * DESCRIPTION :
 * 			Hooks all calls to ComputePoints.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchComputePoints(void)
{
  if (!isInGame())
    return;

  HOOK_JAL(0x006233f0, &patchComputePoints_Hook);
  HOOK_JAL(0x006265a4, &patchComputePoints_Hook);
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
 * NAME :		writeFov
 * 
 * DESCRIPTION :
 * 			Replaces game's SetFov function. Hook installed by patchFov().
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void writeFov(int cameraIdx, int a1, int a2, u32 ra, float fov, float f13, float f14, float f15)
{
  static float lastFov = 0;

  GameCamera* camera = cameraGetGameCamera(cameraIdx);
  if (!camera)
    return;
  
  // save last fov
  // or reuse last if fov passed is 0
  if (fov > 0)
    lastFov = fov;
  else if (lastFov > 0)
    fov = lastFov;
  else
    fov = lastFov = camera->fov.ideal;

  // apply our fov modifier
  // only if not scoping with sniper
  if (ra != 0x003FB4E4) fov += (config.playerFov / 10.0) * 1;

  if (a2 > 2) {
    if (a2 != 3) return;
    camera->fov.limit = f15;
    camera->fov.changeType = a2;
    camera->fov.ideal = fov;
    camera->fov.state = 1;
    camera->fov.gain = f13;
    camera->fov.damp = f14;
    return;
  }
  else if (a2 < 1) {
    if (a2 != 0) return;
    camera->fov.ideal = fov;
    camera->fov.changeType = 0;
    camera->fov.state = 1;
    return;
  }

  if (a1 == 0) {
    camera->fov.ideal = fov;
    camera->fov.changeType = 0;
  }
  else {
    camera->fov.changeType = a2;
    camera->fov.init = camera->fov.actual;
    camera->fov.timer = (short)a2;
    camera->fov.timerInv = 1.0 / (float)a2;
  }

  camera->fov.state = 1;
}

/*
 * NAME :		initFovHook
 * 
 * DESCRIPTION :
 * 			Called when FOV is initialized.
 *      Ensures custom FOV is applied on initialization.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void initFovHook(int cameraIdx)
{
  // call base
  ((void (*)(int))0x004ae9e0)(cameraIdx);

  GameCamera* camera = cameraGetGameCamera(cameraIdx);
  writeFov(0, 0, 3, 0, camera->fov.ideal, 0.05, 0.2, 0);
}

/*
 * NAME :		patchFov
 * 
 * DESCRIPTION :
 * 			Installs SetFov override hook.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchFov(void)
{
  static int ingame = 0;
  static int lastFov = 0;
	if (!isInGame()) {
		ingame = 0;
    return;
  }

  // replace SetFov function
  HOOK_J(0x004AEA90, &writeFov);
  POKE_U32(0x004AEA94, 0x03E0382D);
  
  HOOK_JAL(0x004B25C0, &initFovHook);

  // initialize fov at start of game
  if (!ingame || lastFov != config.playerFov) {
    GameCamera* camera = cameraGetGameCamera(0);
    if (!camera)
      return;

    writeFov(0, 0, 3, 0, 0, 0.05, 0.2, 0);
    lastFov = config.playerFov;
    ingame = 1;
  }
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
	static int disableByAuto = 0;
	static int autoDisableDelayTicks = 0;
	
	int addrValue = FRAME_SKIP_WRITE0;
  int dzoClient = CLIENT_TYPE_DZO == PATCH_POINTERS_CLIENT; // force framelimiter off for dzo
	int disableFramelimiter = config.framelimiter == 2 || dzoClient;
	int totalTimeMs = renderTimeMs + updateTimeMs;
	float averageTotalTimeMs = averageRenderTimeMs + averageUpdateTimeMs; 

	if (!dzoClient && config.framelimiter == 1) // auto
	{
		// already disabled, to re-enable must have instantaneous high total time
		if (disableByAuto && totalTimeMs > 15.0) {
			autoDisableDelayTicks = 30; // 1 seconds before can disable again
			disableByAuto = 0; 
		} else if (disableByAuto && averageTotalTimeMs > 14.0) {
			autoDisableDelayTicks = 60; // 2 seconds before can disable again
			disableByAuto = 0; 
		}

		// not disabled, to disable must have average low total time
		// and must not have just enabled it
		else if (autoDisableDelayTicks == 0 && !disableByAuto && averageTotalTimeMs < 12.5)
			disableByAuto = 1;

		// decrement disable delay
		if (autoDisableDelayTicks > 0)
			--autoDisableDelayTicks;

		// set framelimiter
		disableFramelimiter = disableByAuto;
	}

	// re-enable framelimiter if last fps is really low
	if (disableFramelimiter && addrValue == 0xAF848859)
	{
		FRAME_SKIP_WRITE0 = 0;
		FRAME_SKIP = 0;
	}
	else if (!disableFramelimiter && addrValue == 0)
	{
		FRAME_SKIP_WRITE0 = 0xAF848859;
	}
}

/*
 * NAME :		handleWeaponShotDelayed
 * 
 * DESCRIPTION :
 * 			Forces all incoming weapon shot events to happen immediately.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void handleWeaponShotDelayed(Player* player, char a1, int a2, short a3, char t0, struct tNW_GadgetEventMessage * message)
{
	if (player && message && message->GadgetEventType == 8) {
		int delta = a2 - gameGetTime();

		// client is not holding correct weapon on our screen
		// haven't determined a way to fix this yet but
		if (player->Gadgets[0].id != message->GadgetId) {
			//DPRINTF("remote gadgetevent %d from weapon %d but player holding %d\n", message->GadgetEventType, message->GadgetId, player->Gadgets[0].id);
			playerEquipWeapon(player, message->GadgetId);
		}

		// set weapon shot event time to now if its in the future
		// because the client is probably lagging behind
		if (player->Gadgets[0].id == message->GadgetId && (delta > 0 || delta < -TIME_SECOND)) {
			a2 = gameGetTime();
		}
	}

	((void (*)(Player*, char, int, short, char, struct tNW_GadgetEventMessage*))0x005f0318)(player, a1, a2, a3, t0, message);
}

/*
 * NAME :		getFusionShotDirection
 * 
 * DESCRIPTION :
 * 			Ensures that the fusion shot will hit its target.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
u128 getFusionShotDirection(Player* target, int a1, u128 vFrom, u128 vTo, Moby* targetMoby)
{
  VECTOR from, to;

	// move from registers to memory
  vector_write(from, vFrom);
  vector_write(to, vTo);

  // if we have a valid player target
	// then we want to check that the given to from will hit
	// if it doesn't then we want to force it to hit
  if (target)
  {
    // if the shot didn't hit the target then we want to force the shot to hit the target
    int hit = CollLine_Fix(from, to, 2, NULL, 0);
    if (!hit || CollLine_Fix_GetHitMoby() != target->PlayerMoby) {
      vTo = ((u128 (*)(Moby*, Player*))0x003f7968)(targetMoby, target);
    }
  }

	// call base get direction
  return ((u128 (*)(Player*, int, u128, u128, float))0x003f9fc0)(target, a1, vFrom, vTo, 0.0);
}

/*
 * NAME :		patchFusionReticule
 * 
 * DESCRIPTION :
 * 			Enables/disables the fusion reticule.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchFusionReticule(void)
{
  static int patched = -1;
  if (!isInGame()) {
    patched = -1; // reset
    return;
  }

  // don't allow reticule when disabled by host
  if (gameConfig.grNoSniperHelpers)
    return;

  // already patched
  if (patched == config.enableFusionReticule)
    return;

  if (config.enableFusionReticule) {
    POKE_U32(0x003FAFA4, 0x8203266D);
    POKE_U32(0x003FAFA8, 0x1060003E);

    POKE_U32(0x003FAEE0, 0);
  } else {
    POKE_U32(0x003FAFA4, 0x24020001);
    POKE_U32(0x003FAFA8, 0x1062003E);

    POKE_U32(0x003FAEE0, 0x10400070);
  }

  patched = config.enableFusionReticule;
}

struct FlagPVars
{
	VECTOR BasePosition;
	short CarrierIdx;
	short LastCarrierIdx;
	short Team;
	char UNK_16[6];
	int TimeFlagDropped;
};

void customFlagLogic(Moby* flagMoby)
{
	VECTOR t;
	int i;
	Player** players = playerGetAll();
	int gameTime = gameGetTime();
	GameOptions* gameOptions = gameGetOptions();
	if (!isInGame())
		return;
		
	// validate flag
	if (!flagMoby)
		return;

	// get pvars
	struct FlagPVars* pvars = (struct FlagPVars*)flagMoby->PVar;
	if (!pvars)
		return;

	// somehow a player is holding the flag without being the carrier
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player* player = players[i];
		if (player && player->HeldMoby == flagMoby && pvars->CarrierIdx != player->PlayerId) {
			player->HeldMoby = 0;
		}
	}
	
	// 
	if (flagMoby->State != 1)
		return;

	// flag is currently returning
	if (flagIsReturning(flagMoby))
		return;

	// flag is currently being picked up
	if (flagIsBeingPickedUp(flagMoby))
		return;
	
	// return to base if flag has been idle for 40 seconds
	if ((pvars->TimeFlagDropped + (TIME_SECOND * 40)) < gameTime && !flagIsAtBase(flagMoby)) {
		flagReturnToBase(flagMoby, 0, 0xFF);
		return;
	}

	// return to base if flag landed on bad ground
	if (!flagIsOnSafeGround(flagMoby)) {
		flagReturnToBase(flagMoby, 0, 0xFF);
		return;
	}

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player* player = players[i];
		if (!player || !player->IsLocal)
			continue;

		// wait 3 seconds for last carrier to be able to pick up again
		if ((pvars->TimeFlagDropped + 3*TIME_SECOND) > gameTime && i == pvars->LastCarrierIdx)
			continue;

		// only allow actions by living players
		if (playerIsDead(player) || player->Health <= 0)
			continue;

		// something to do with vehicles
		// not sure exactly when this case is true
		if (((int (*)(Player*))0x00619b58)(player))
			continue;

		// skip player if they've only been alive for < 3 seconds
		if (player->timers.timeAlive <= 180)
			continue;

		// skip player if in vehicle
		if (player->Vehicle)
			continue;

		// skip if player state is in vehicle and critterMode is on
		if (player->Camera && player->PlayerState == PLAYER_STATE_VEHICLE && player->Camera->camHeroData.critterMode)
			continue;

		// skip if player is on teleport pad
		if (player->Ground.pMoby && player->Ground.pMoby->OClass == MOBY_ID_TELEPORT_PAD)
			continue;
		
		// player must be within 2 units of flag
		vector_subtract(t, flagMoby->Position, player->PlayerPosition);
		float sqrDistance = vector_sqrmag(t);
		if (sqrDistance > (2*2))
			continue;

		// player is on different team than flag and player isn't already holding flag
		if (player->Team != pvars->Team) {
			if (!player->HeldMoby) {
				flagPickup(flagMoby, i);
				player->HeldMoby = flagMoby;
				return;
			}
		}
		// player is on same team so attempt to return flag
		else if (gameOptions->GameFlags.MultiplayerGameFlags.FlagReturn) {
			vector_subtract(t, pvars->BasePosition, flagMoby->Position);
			float sqrDistanceToBase = vector_sqrmag(t);
			if (sqrDistanceToBase > 0.1) {
				flagReturnToBase(flagMoby, 0, i);
				return;
			}
		}
	}

	//0x00619b58
}

/*
 * NAME :		onRemoteClientPickedUpFlag
 * 
 * DESCRIPTION :
 * 			Handles when a remote client sends the CUSTOM_MSG_ID_FLAG_PICKED_UP message.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int onRemoteClientPickedUpFlag(void * connection, void * data)
{
	int i;
	ClientPickedUpFlag_t msg;
	memcpy(&msg, data, sizeof(msg));

	// ignore if not in game
	if (!isInGame())
		return;

	// get remote player or ignore message
	Player* remotePlayer = playerGetAll()[msg.PlayerId];
	if (!remotePlayer)
		return;

	// check if client picked up our flag
	DPRINTF("remote player %d picked up flag %X at %d\n", msg.PlayerId, msg.FlagClass, msg.GameTime);
  for (i = 0; i < 2; ++i) {
    Player* localPlayer = playerGetFromSlot(i);
    if (!localPlayer || !localPlayerHasFlag[i])
      continue;

    if (remotePlayer->HeldMoby && remotePlayer->HeldMoby->OClass == msg.FlagClass) {

      // reassign to us if they picked up before us
      if (msg.GameTime < timeLocalPlayerPickedUpFlag[i]) {
        flagPickup(remotePlayer->HeldMoby, localPlayer->PlayerId);
        localPlayer->HeldMoby = remotePlayer->HeldMoby;
        remotePlayer->HeldMoby = NULL;
        DPRINTF("local player %d flag force pickup case A\n", i);
      }
      // reassign to us if they picked up at same time but are later player id
      // we need some kind of priority ordering for this case
      else if (msg.GameTime == timeLocalPlayerPickedUpFlag[i] && msg.PlayerId > localPlayer->PlayerId) {
        flagPickup(remotePlayer->HeldMoby, localPlayer->PlayerId);
        localPlayer->HeldMoby = remotePlayer->HeldMoby;
        remotePlayer->HeldMoby = NULL;
        DPRINTF("local player %d flag force pickup case B\n", i);
      }
    }
  }

	return sizeof(ClientPickedUpFlag_t);
}

/*
 * NAME :		runFlagPickupFix
 * 
 * DESCRIPTION :
 * 			Ensures that player's don't "lag through" the flag.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void runFlagPickupFix(void)
{
	VECTOR t;
	int i = 0;
	Player** players = playerGetAll();

	if (!isInGame()) {
		// reset and exit
		timeLocalPlayerPickedUpFlag[0] = timeLocalPlayerPickedUpFlag[1] = 0;
		localPlayerHasFlag[0] = localPlayerHasFlag[1] = 0;
		return;
	}

#if DEBUG
  // drop flag
  if (padGetButtonDown(0, PAD_UP | PAD_LEFT) > 0) {
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      if (players[i]) {
        playerDropFlag(players[i], 0);
        playerDropFlag(players[i], 1);
        DPRINTF("force drop %d\n", i);
      }
    }
  }
#endif

	// issue with this fix is that now two people can pick up the flag
	// at the same time, causing both of them to hold the flag (one is invis)
	// so to fix this we have to broadcast when we pick the flag up, with the game time
	// and if a client receives this message and it tells them another person picked up the flag before them
	// then we drop our flag
	void * dmeConnection = netGetDmeServerConnection();
	if (dmeConnection) {

		netInstallCustomMsgHandler(CUSTOM_MSG_ID_FLAG_PICKED_UP, &onRemoteClientPickedUpFlag);

		// update local player(s) flag time and picked up status
		for (i = 0; i < 2; ++i) {
			Player* localPlayer = playerGetFromSlot(i);
			if (localPlayer) {
				if (localPlayer->HeldMoby) {
					short heldMobyOClass = localPlayer->HeldMoby->OClass;
					if (heldMobyOClass == MOBY_ID_BLUE_FLAG || heldMobyOClass == MOBY_ID_RED_FLAG || heldMobyOClass == MOBY_ID_GREEN_FLAG || heldMobyOClass == MOBY_ID_ORANGE_FLAG) {
						
						// just picked up flag
						if (!localPlayerHasFlag[i]) {
							timeLocalPlayerPickedUpFlag[i] = gameGetTime();

							DPRINTF("local player %d picked up flag %X at %d\n", localPlayer->PlayerId, localPlayer->HeldMoby->OClass, gameGetTime());

							// broadcast to other clients
							ClientPickedUpFlag_t msg;
							msg.GameTime = gameGetTime();
							msg.PlayerId = localPlayer->PlayerId;
							msg.FlagClass = localPlayer->HeldMoby->OClass;
							netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, dmeConnection, CUSTOM_MSG_ID_FLAG_PICKED_UP, sizeof(ClientPickedUpFlag_t), &msg);
						}
						
						localPlayerHasFlag[i] = 1;
					}
					else {
						localPlayerHasFlag[i] = 0;
					}
				}
				else {
					localPlayerHasFlag[i] = 0;
				}
			}
		}
	}

  // set pickup cooldown to 0.5 seconds
  //POKE_U16(0x00418aa0, 500);

	// disable normal flag update code
	POKE_U32(0x00418858, 0x03E00008);
	POKE_U32(0x0041885C, 0x0000102D);

	// run custom flag update on flags
	GuberMoby* gm = guberMobyGetFirst();
  while (gm)
  {
    if (gm->Moby)
    {
      switch (gm->Moby->OClass)
      {
        case MOBY_ID_BLUE_FLAG:
        case MOBY_ID_RED_FLAG:
        case MOBY_ID_GREEN_FLAG:
        case MOBY_ID_ORANGE_FLAG:
        {
					customFlagLogic(gm->Moby);
					break;
				}
			}
		}
    gm = (GuberMoby*)gm->Guber.Prev;
	}

	return;
	/*
	// iterate guber mobies
	// finding each flag
	// we want to check if we're the master
	// if we're not and within some reasonable distance then 
	// just run the flag update manually
  GuberMoby* gm = guberMobyGetFirst();
  while (gm)
  {
    if (gm->Moby)
    {
      switch (gm->Moby->OClass)
      {
        case MOBY_ID_BLUE_FLAG:
        case MOBY_ID_RED_FLAG:
        case MOBY_ID_GREEN_FLAG:
        case MOBY_ID_ORANGE_FLAG:
        {
					int flagUpdateRan = 0;

					// ensure no one is master
					void * master = masterGet(gm->Guber.Id.UID);
					if (master)
						masterDelete(master);

					// detect if flag is currently held
					for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
						Player* player = players[i];
						if (player && player->HeldMoby == gm->Moby) {
							flagUpdateRan = 1;
							break;
						}
					}

					if (!flagUpdateRan) {
						for (i = 0; i < 2; ++i) {
							// get local player
							Player* localPlayer = playerGetFromSlot(i);
							if (localPlayer) {
								// get distance from local player to flag
								vector_subtract(t, gm->Moby->Position, localPlayer->PlayerPosition);
								float sqrDist = vector_sqrmag(t);

								if (sqrDist < (5 * 5))
								{
									// run update
									((void (*)(Moby*, u32))0x00418858)(gm->Moby, 0);
									flagUpdateRan = 1;

									// we only need to run it once per client
									// even if both locals are near the flag
									break;
								}
							}
						}
					}

					// run flag update as host if no one else is nearby
					if (gameAmIHost() && !flagUpdateRan) {
						((void (*)(Moby*, u32))0x00418858)(gm->Moby, 0);
					}
          break;
        }
      }
    }
    gm = (GuberMoby*)gm->Guber.Prev;
  }
	*/
}

/*
 * NAME :		patchSingleTapChargeboot
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
void patchSingleTapChargeboot(void)
{
  int i;
  int patch = 0;
  if (!isInGame()) return;

  // 
  for (i = 0; i < 2; ++i) {
    Player* p = playerGetFromSlot(i);
    if (!p)
      continue;
    if (!p->Paddata)
      continue;

    u32 padData = *(u32*)((u32)p->Paddata + 0x1A4);

    // chargeDownTimer
    // when non-zero, game is expecting another L2 tap to cboot
    // write value here when R2 is tapped
    if (config.enableSingleTapChargeboot && (padData & 0x2)) {
      POKE_U16((u32)p + 0x2E50, 20);
      patch = 1;
    }
  }

  if (patch) {
    POKE_U32(0x0060DC48, 0x24030002);
  } else {
    POKE_U32(0x0060DC48, 0x8C8301A4);
  }
}

/*
 * NAME :		patchFlagCaptureMessage_Hook
 * 
 * DESCRIPTION :
 * 			Replaces default flag capture popup text with custom one including
 *      the name of the player who captured the flag.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchFlagCaptureMessage_Hook(int localPlayerIdx, int msgStringId, GuberEvent* event)
{
  static char buf[64];
  int teams[4] = {0,0,0,0};
  int teamCount = 0;
  int i;
  GameSettings* gs = gameGetSettings();

  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    int team = gs->PlayerTeams[i];
    if (team >= 0 && teams[team] == 0) {
      teams[team]++;
      teamCount++;
    }
  }

  // flag captured
  if (gs && teamCount < 3 && event && event->NetEvent.EventID == 1 && event->NetEvent.NetData[0] == 1) {

    // pid of player who captured the flag
    int pid = event->NetEvent.NetData[1];
    if (pid >= 0 && pid < GAME_MAX_PLAYERS)
    {
      snprintf(buf, 64, "%s has captured the flag!", gs->PlayerNames[pid]);
      uiShowPopup(localPlayerIdx, buf);
      return;
    }
  }

  // call underlying GUI PrintPrompt
  ((void (*)(int, int))0x00540190)(localPlayerIdx, msgStringId);
}

/*
 * NAME :		patchFlagCaptureMessage
 * 
 * DESCRIPTION :
 * 			Hooks patchFlagCaptureMessage_Hook
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchFlagCaptureMessage(void)
{
  if (!isInGame()) return;

  // moves some logic around in a loop to free up an extra instruction slot
  // so that we can pass a pointer to the guber event to our custom message handler
  POKE_U32(0x00418808, 0x92242FEA);
  POKE_U32(0x00418830, 0x263131F0);
  HOOK_JAL(0x00418814, &patchFlagCaptureMessage_Hook);
  POKE_U32(0x00418810, 0x8FA60068);
}

/*
 * NAME :		runFixB6EatOnDownSlope
 * 
 * DESCRIPTION :
 * 			B6 shots are supposed to do full damage when a player is hit on the ground.
 * 			But the ground detection fails when walking/cbooting down slopes.
 * 			This allows players to take only partial damage in some circumstances.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void runFixB6EatOnDownSlope(void)
{
  int i = 0;

  while (i < 2)
  {
    Player* p = playerGetFromSlot(i);
    if (p) {
      // determine height delta of player from ground
      // player is grounded if "onGood" or height delta from ground is less than 0.3 units
      float hDelta = p->PlayerPosition[2] - *(float*)((u32)p + 0x250 + 0x78);
      int grounded = hDelta < 0.3 || *(int*)((u32)p + 0x250 + 0xA0) != 0;

      // store grounded in unused part of player data
      POKE_U32((u32)p + 0x30C, grounded);
    }

    ++i;
  }

  // patch b6 grounded damage check to read our isGrounded state
  POKE_U16(0x003B56E8, 0x30C);
}

/*
 * NAME :		patchWeaponShotLag
 * 
 * DESCRIPTION :
 * 			Patches weapon shots to be less laggy.
 * 			This is always a work-in-progress.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchWeaponShotLag(void)
{
	if (!isInGame())
		return;

	// send all shots reliably
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

	// patches fusion shot so that remote shots aren't given additional "shake"
	// ie, remote shots go in the same direction that is sent by the source client
	POKE_U32(0x003FA28C, 0);

	// patches function that converts fusion shot start and end positions
	// to a direction to always hit target player if there is a target
	POKE_U32(0x003fa5c0, 0x0240402D);
	HOOK_JAL(0x003fa5c8, &getFusionShotDirection);

	// patch all weapon shots to be shot on remote as soon as they arrive
	// instead of waiting for the gametime when they were shot on the remote
	// since all shots happen immediately (none are sent ahead-of-time)
	// and this only happens when a client's timebase is desync'd
	HOOK_JAL(0x0062ac60, &handleWeaponShotDelayed);

	// this disables filtering out fusion shots where the player is facing the opposite direction
	// in other words, a player may appear to shoot behind them but it's just lag/desync
	FUSION_SHOT_BACKWARDS_BRANCH = 0x1000005F;

	// send fusion shot reliably
	if (*(u32*)0x003FCE8C == 0x910407F8)
		*(u32*)0x003FCE8C = 0x24040040;
		
	// fix b6 eating on down slope
	runFixB6EatOnDownSlope();
}

/*
 * NAME :		patchRadarScale
 * 
 * DESCRIPTION :
 * 			Changes the radar scale and zoom.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchRadarScale(void)
{
	u32 frameSizeValue = 0;
	u32 minimapBorderColor = 0;
	u32 minimapTeamBorderColorInstruction = 0x0000282D;
	float scale = 1;
	if (!isInGame())
		return;

	// adjust expanded frame size
	if (!config.minimapScale) {
		frameSizeValue = 0x460C6300;
		minimapBorderColor = 0x80969696;
		minimapTeamBorderColorInstruction = 0x0200282D;
	}
	
	// read expanded animation time (0=shrunk, 1=expanded)
	float frameExpandedT = *(float*)0x0030F750;
	float smallScale = 1 + (config.minimapSmallZoom / 5.0);
	float bigScale = 1 + (config.minimapBigZoom / 5.0);
	scale = lerpf(smallScale, bigScale, frameExpandedT);
	
	// set radar frame size during animation
	POKE_U32(0x00556900, frameSizeValue);
	// remove border 
	POKE_U32(0x00278C90, minimapBorderColor);
	// remove team border
	POKE_U32(0x00556D08, minimapTeamBorderColorInstruction);

	// set minimap scale
	*(float*)0x0038A320 = 500 * scale; // when idle
	*(float*)0x0038A324 = 800 * scale; // when moving

	// set blip scale
	*(float*)0x0038A300 = 0.04 / scale;
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
 * NAME :		onClientReady
 * 
 * DESCRIPTION :
 * 			Handles when a client loads the level.
 *      Sets a bit indicating the client has loaded and then checks if all clients have loaded.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onClientReady(int clientId)
{
  GameSettings* gs = gameGetSettings();
  int i;
  int bit = 1 << clientId;

  // set ready
  patchStateContainer.AllClientsReady = 0;
  patchStateContainer.ClientsReadyMask |= bit;
  DPRINTF("received client ready from %d\n", clientId);

  if (!gs)
    return;

  // check if all clients are ready
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    bit = 1 << i;
    if (gs->PlayerClients[i] >= 0 && (patchStateContainer.ClientsReadyMask & bit) == 0)
      return;
  }

  patchStateContainer.AllClientsReady = 1;
  DPRINTF("all clients ready\n");
}

/*
 * NAME :		onClientReadyRemote
 * 
 * DESCRIPTION :
 * 			Receives when another client has loaded the level and passes it to the onClientReady handler.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int onClientReadyRemote(void * connection, void * data)
{
  int clientId;
  memcpy(&clientId, data, sizeof(clientId));
	onClientReady(clientId);

	return sizeof(clientId);
}

/*
 * NAME :		sendClientReady
 * 
 * DESCRIPTION :
 * 			Broadcasts to all other clients in lobby that this client has finished loading the level.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void sendClientReady(void)
{
  int clientId = gameGetMyClientId();
  int bit = 1 << clientId;
  if (patchStateContainer.ClientsReadyMask & bit)
    return;

	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_CLIENT_READY, 4, &clientId);
  DPRINTF("send client ready\n");

	// locally
  onClientReady(clientId);
}

/*
 * NAME :		runClientReadyMessager
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
void runClientReadyMessager(void)
{
  GameSettings* gs = gameGetSettings();

  if (!gs)
  {
    patchStateContainer.AllClientsReady = 0;
    patchStateContainer.ClientsReadyMask = 0;
  }
  else if (isInGame())
  {
    sendClientReady();
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
  GameData * gameData = gameGetData();
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
	patchStateContainer.GameStateUpdate.Version = 1;

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

    if (gameSettings->GameRules == GAMERULE_JUGGY) {
      for (i = 0; i < GAME_MAX_PLAYERS; ++i)
      {
        int team = gameSettings->PlayerTeams[i];
        if (team >= 0 && team < GAME_MAX_PLAYERS)
          patchStateContainer.GameStateUpdate.TeamScores[team] = gameData->PlayerStats.Kills[i];
      }
    } else {
      for (i = 0; i < GAME_SCOREBOARD_ITEM_COUNT; ++i)
      {
        ScoreboardItem * item = GAME_SCOREBOARD_ARRAY[i];
        if (item)
          patchStateContainer.GameStateUpdate.TeamScores[item->TeamId] = item->Value;
      }
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
  static int Loading = 0;

  // there's an crash issue when loading sp music
  // in survival
  // so reject it then
  if (gameConfig.customModeId == CUSTOM_MODE_SURVIVAL && !isInMenus())
    return;

  // indicate to user we're loading sp music
  // running uiRunCallbacks triggers our vsync hook and reinvokes this method
  // while it is still looping
  if (Loading)
  {
    gfxScreenSpaceBox(0.3, 0.45, 0.4, 0.1, 0x80000000);
    gfxScreenSpaceText(0.5 * SCREEN_WIDTH, 0.5 * SCREEN_HEIGHT, 1, 1, 0x80FFFFFF, "Loading Music...", 14 + ((gameGetTime()/1000)%3), 4);
    return;
  }

	if (!config.enableSingleplayerMusic || !musicIsLoaded())
		return;

  Loading = 1;
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
      // let the game handle net and rendering stuff
      // this prevents the user from lagging out if the disc
      // takes forever to seek
      uiRunCallbacks();

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
	if (isInGame())
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

  Loading = 0;
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
			
#if DEBUG
			redownloadCustomModeBinaries = 1;
#endif

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
  // enable singleplayer music
  runEnableSingleplayerMusic();

	u64 r = ((u64 (*)(void))0x001579A0)();

	// Start at the first game module
	GameModule * module = GLOBAL_GAME_MODULES_START;

  // increase wait for players to 50 seconds
  POKE_U32(0x0021E1E8, 50 * 60);

	// call gamerules level load
	grLoadStart();

	// pass event to modules
	while (module->GameEntrypoint || module->LobbyEntrypoint)
	{
		if (module->State > GAMEMODULE_OFF && module->LoadEntrypoint)
			module->LoadEntrypoint(module, &config, &gameConfig, &patchStateContainer);

		++module;
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

	// send in chunks
	while (i < size)
	{
		short len = (size - i);
		if (len > sizeof(data.Payload)) {
			len = sizeof(data.Payload);
	    data.EndOfList = 0;
    } else {
	    data.EndOfList = endOfList;
    }

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
  int hasCustomGameData = patchStateContainer.CustomGameStatsSize > 0;
  int header[] = {
    0x1337C0DE,
    PATCH_GAME_STATS_VERSION
  };

	// ensure in game/staging
	if (!gameSettings)
		return;
	
	DPRINTF("sending stats... ");
	offset += sendGameDataBlock(offset, 0, header, sizeof(header));
	offset += sendGameDataBlock(offset, 0, gameData, sizeof(GameData)); uiRunCallbacks();
	offset += sendGameDataBlock(offset, 0, &patchStateContainer.GameSettingsAtStart, sizeof(GameSettings)); uiRunCallbacks();
	offset += sendGameDataBlock(offset, 0, gameSettings, sizeof(GameSettings)); uiRunCallbacks();
	offset += sendGameDataBlock(offset, 0, gameOptions, sizeof(GameOptions)); uiRunCallbacks();
	offset += sendGameDataBlock(offset, !hasCustomGameData, &patchStateContainer.GameStateUpdate, sizeof(UpdateGameStateRequest_t));
  if (hasCustomGameData) {
    uiRunCallbacks();
	  offset += sendGameDataBlock(offset, 1, &patchStateContainer.CustomGameStats, patchStateContainer.CustomGameStatsSize);
  }
  
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
	char buf[64];
	static int lastGameTime = 0;
	static int tickCounter = 0;

	if (!isInGame())
		return;

	// initialize time
	if (tickCounter == 0 && lastGameTime == 0)
		lastGameTime = gameGetTime();
	
	// update fps every 60 frames
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
		if (averageRenderTimeMs > 0) {
			snprintf(buf, 64, "EE: %.1fms GS: %.1fms FPS: %.2f", averageUpdateTimeMs, averageRenderTimeMs, lastFps);
		} else {
			snprintf(buf, 64, "FPS: %.2f", lastFps);
		}
		
		gfxScreenSpaceText(SCREEN_WIDTH - 5, 5, 0.75, 0.75, 0x80FFFFFF, buf, -1, 2);
	}
}

int hookCheckHostStartGame(void* a0)
{
	GameSettings* gs = gameGetSettings();

	// call base
	int v0 = ((int (*)(void*))0x00757660)(a0);

	// success
	if (v0) {

		// verify we have map
		if (mapOverrideResponse < 0) {
			showNoMapPopup = 1;
			netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_REQUEST_MAP_OVERRIDE, 0, NULL);
			return 0;
		}

    // if survival
    // verify we have the latest maps
    if (gameConfig.customModeId == CUSTOM_MODE_SURVIVAL && mapsLocalGlobalVersion != mapsRemoteGlobalVersion) {
      readLocalGlobalVersion();
      if (mapsLocalGlobalVersion != mapsRemoteGlobalVersion) {
        showNeedLatestMapsPopup = 1;
        return 0;
      }
    }

		// wait for download to finish
		if (dlIsActive) {
			showMiscPopup = 1;
			strncpy(miscPopupTitle, "System", 32);
			strncpy(miscPopupBody, "Please wait for the download to finish.", 64);
			return 0;
		}

		// if training, verify we're the only player in the lobby
		if (gameConfig.customModeId == CUSTOM_MODE_TRAINING && gs && gs->PlayerCount != 1) {
			showMiscPopup = 1;
			strncpy(miscPopupTitle, "Training", 32);
			strncpy(miscPopupBody, "Too many players to start.", 64);
			return 0;
		}
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
	if (!gs || !isInMenus())
		return;

	// install start game hook
	if (*(u32*)0x00759580 == 0x0C1D5D98)
		*(u32*)0x00759580 = 0x0C000000 | ((u32)&hookCheckHostStartGame >> 2);

	int clientId = gameGetMyClientId();
  for (i = 1; i < GAME_MAX_PLAYERS; ++i)
  {
    if (gs->PlayerClients[i] == clientId && gs->PlayerStates[i] == 6)
    {
      // need map
      if (mapOverrideResponse < 0)
      {
        gameSetClientState(i, 0);
        showNoMapPopup = 1;
        netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_REQUEST_MAP_OVERRIDE, 0, NULL);
      }
      else if (gameConfig.customModeId == CUSTOM_MODE_SURVIVAL && mapsLocalGlobalVersion != mapsRemoteGlobalVersion) {
        readLocalGlobalVersion();
        if (mapsLocalGlobalVersion != mapsRemoteGlobalVersion) {
          gameSetClientState(i, 0);
          showNeedLatestMapsPopup = 1;
        }
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

						// set team
						teamId = request.Pool[teamPoolIndex];

						DPRINTF("pool info pid:%d poolIndex:%d poolSize:%d team:%d\n", i, teamPoolIndex, request.PoolSize, teamId);

						// remove element from pool
						if (request.PoolSize > 0)
						{
							for (j = teamPoolIndex+1; j < request.PoolSize; ++j)
								request.Pool[j-1] = request.Pool[j];
							request.PoolSize -= 1;

#if DEBUG
							printf("pool after shift ");
							for (j = 0; j < request.PoolSize; ++j)
								printf("%d=%d ", j, request.Pool[j]);
							printf("\n");
#endif
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
 * NAME :		padMappedPadHooked
 * 
 * DESCRIPTION :
 * 			Replaces game's built in Pad_MappedPad function
 *      which remaps input masks.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int padMappedPadHooked(int padMask, int a1)
{
  if (config.enableSingleTapChargeboot && padMask == 8) return 0x3; // R1 -> L2 or R2 (cboot)

  // not third person
  if (*(char*)(0x00171de0 + a1) != 0) {
    if (padMask == 0x08) return 1; // R1 -> L2 (cboot)
    if (padMask < 9) {
      if (padMask == 4) return 1; // L1 -> L2 (ADS)
    } else {
      if (padMask == 0x10 && config.enableSingleTapChargeboot) return 0x10; // Triangle -> Triangle (quick select)
      if (padMask == 0x10) return 0x12; // Triangle -> Triangle or R2 (quick select)
      if (padMask == 0x40) return 0x44; // Cross -> Cross or L1 (jump)
    }
  }

  return padMask;
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
int onSetRanks(void * connection, void * data)
{
	int i, j;

	// move message payload into local
	memcpy(&lastSetRanksRequest, data, sizeof(ServerSetRanksRequest_t));

	return sizeof(ServerSetRanksRequest_t);
}

/*
 * NAME :		getCustomGamemodeRankNumber
 * 
 * DESCRIPTION :
 * 			Returns the player rank number for the given player based off the current gamemode or custom gamemode.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int getCustomGamemodeRankNumber(int offset)
{
	GameSettings* gs = gameGetSettings();
	int pid = offset / 4;
	float rank = gs->PlayerRanks[pid];
	int i;

	if (gameConfig.customModeId != CUSTOM_MODE_NONE && lastSetRanksRequest.Enabled)
	{
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			if (lastSetRanksRequest.AccountIds[i] == gs->PlayerAccountIds[pid])
			{
				rank = lastSetRanksRequest.Ranks[i];
				break;
			}
		}
	}

	return ((int (*)(float))0x0077B8A0)(rank);
}

/*
 * NAME :		patchStagingRankNumber
 * 
 * DESCRIPTION :
 * 			Patches the staging menu's player rank menu elements to read the rank from our custom function.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void patchStagingRankNumber(void)
{
	if (!isInMenus())
		return;
	
	HOOK_JAL(0x0075AC3C, &getCustomGamemodeRankNumber);
	POKE_U32(0x0075AC40, 0x0060202D);
}

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
 * NAME :		drawHook
 * 
 * DESCRIPTION :
 * 			Logs time it takes to render a frame.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void drawHook(u64 a0)
{
	static int renderTimeCounterMs = 0;
	static int frames = 0;
	static long ticksIntervalStarted = 0;

	long t0 = timerGetSystemTime();
	((void (*)(u64))0x004c3240)(a0);
	long t1 = timerGetSystemTime();

	renderTimeMs = (t1-t0) / SYSTEM_TIME_TICKS_PER_MS;

	renderTimeCounterMs += renderTimeMs;
	frames++;

	// update every 500 ms
	if ((t1 - ticksIntervalStarted) > (SYSTEM_TIME_TICKS_PER_MS * 500))
	{
		averageRenderTimeMs = renderTimeCounterMs / (float)frames;
		renderTimeCounterMs = 0;
		frames = 0;
		ticksIntervalStarted = t1;
	}
}

/*
 * NAME :		updateHook
 * 
 * DESCRIPTION :
 * 			Logs time it takes to run an update.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void updateHook(void)
{
	static int updateTimeCounterMs = 0;
	static int frames = 0;
	static long ticksIntervalStarted = 0;

	long t0 = timerGetSystemTime();
	((void (*)(void))0x005986b0)();
	long t1 = timerGetSystemTime();

	updateTimeMs = (t1-t0) / SYSTEM_TIME_TICKS_PER_MS;

	updateTimeCounterMs += updateTimeMs;
	frames++;

	// update every 500 ms
	if ((t1 - ticksIntervalStarted) > (SYSTEM_TIME_TICKS_PER_MS * 500))
	{
		averageUpdateTimeMs = updateTimeCounterMs / (float)frames;
		updateTimeCounterMs = 0;
		frames = 0;
		ticksIntervalStarted = t1;
	}
}

int onCustomModeDownloadInitiated(void * connection, void * data)
{
	DPRINTF("requested mode binaries started %d\n", dlIsActive);
	redownloadCustomModeBinaries = 0;
	return 0;
}

void sendClientType(void)
{
  static int sendCounter = -1;

  // get
  int currentClientType = PATCH_POINTERS_CLIENT;
  int accountId = *(int*)0x00172194;
  void* lobbyConnection = netGetLobbyServerConnection();

  // if we're not logged in, update last account id to -1
  if (accountId < 0) {
    lastAccountId = accountId;
  }

  // if we're not connected to the server
  // which can happen as we transition from lobby to game server
  // or vice-versa, or when we've logged out/disconnected
  // reset sendCounter, indicating it's time to recheck if we need to send the client type
  if (!lobbyConnection) {
    sendCounter = -1;
    return;
  }

  if (sendCounter > 0) {
    // tick down until we hit 0
    --sendCounter;
  } else if (sendCounter < 0) {

    // if relogged in
    // or client type has changed
    // trigger send in 60 ticks
    if (lobbyConnection && accountId > 0 && (lastAccountId != accountId || lastClientType != currentClientType)) {
      sendCounter = 60;
    }

  } else {

    lastClientType = currentClientType;
    lastAccountId = accountId;

    // keep trying to send until it succeeds
    if (netSendCustomAppMessage(NET_DELIVERY_CRITICAL, lobbyConnection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_SET_CLIENT_TYPE, sizeof(int), &currentClientType) != 0) {
      // failed, wait another second
      sendCounter = 60;
    } else {
      // success
      sendCounter = -1;
      DPRINTF("sent %d %d\n", gameGetTime(), currentClientType);
    }
  }

}

void runPayloadDownloadRequester(void)
{
	GameModule * module = GLOBAL_GAME_MODULES_START;
	GameSettings* gs = gameGetSettings();
	if (!gs || !isInMenus()) {
		
    if (dlIsActive == 201) {
      dlIsActive = 0;
    }
    
    return;
  }

	// don't make any requests when the config menu is active
	if (gameAmIHost() && isConfigMenuActive)
		return;

	if (!dlIsActive) {
		// redownload when set mode doesn't match downloaded one
		if (!redownloadCustomModeBinaries && gameConfig.customModeId && (module->State == 0 || module->ModeId != gameConfig.customModeId)) {
			redownloadCustomModeBinaries = 1;
			DPRINTF("mode id %d != %d\n", gameConfig.customModeId, module->ModeId);
		}

		// redownload when set map doesn't match downloaded one
		if (!redownloadCustomModeBinaries && module->State && module->MapId != gameConfig.customMapId) {
			redownloadCustomModeBinaries = 1;
			DPRINTF("map id %d != %d\n", gameConfig.customMapId, module->MapId);
		}
    
    // redownload if training mode changed
    if (!redownloadCustomModeBinaries && module->State && module->ModeId == CUSTOM_MODE_TRAINING && gameConfig.trainingConfig.type != module->Arg3) {
			redownloadCustomModeBinaries = 1;
			DPRINTF("training type id %d != %d\n", gameConfig.trainingConfig.type, module->Arg3);
    }

		// disable when module id doesn't match mode
		// unless mode is forced (negative mode)
		if (redownloadCustomModeBinaries == 1 || (module->State && !gameConfig.customModeId && module->ModeId >= 0)) {
			DPRINTF("disabling module map:%d mode:%d\n", module->MapId, module->ModeId);
			module->State = 0;
			memset((void*)(u32)0x000F0000, 0, 0xF000);
		}

		if (redownloadCustomModeBinaries == 1) {
			netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_REQUEST_CUSTOM_MODE_PATCH, 0, &dlIsActive);
			redownloadCustomModeBinaries = 2;
			DPRINTF("requested mode binaries\n");
		}
	}
}

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
  static int hasDevGameConfig = 0;
  char buf[64];

	// call normal draw routine
	((void (*)(void))0x00707F28)();
	
	// 
	lastMenuInvokedTime = gameGetTime();

	//
	if (!hasInitialized)
	{
		padEnableInput();
		onConfigInitialize();
    PATCH_DZO_INTEROP_FUNCS = 0;
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

  // check if dev item is newly enabled
  // and we're host
  if (!netGetDmeServerConnection()) {
    hasDevGameConfig = 0;
  }
  else if (!isConfigMenuActive) {
    int lastHasDevConfig = hasDevGameConfig;
    hasDevGameConfig = gameConfig.drFreecam != 0;
    if (gameAmIHost() && hasDevGameConfig && !lastHasDevConfig) {
      uiShowOkDialog("Warning", "By enabling a dev rule this game will not count towards stats.");
    }
  }

	if (showNoMapPopup)
	{
		if (mapOverrideResponse == -1)
		{
			uiShowOkDialog("Custom Maps", "You have not installed the map modules.");
		}
		else
		{
#if MAPDOWNLOADER
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
			sprintf(buf, "Please install %s to play.", dataCustomMaps.items[(int)gameConfig.customMapId]);
			uiShowOkDialog("Custom Maps", buf);
#endif
		}

		showNoMapPopup = 0;
	}

	// 
	if (showNeedLatestMapsPopup)
	{
    sprintf(buf, "Please download the latest custom maps to play. %d %d", mapsLocalGlobalVersion, mapsRemoteGlobalVersion);
    uiShowOkDialog("Custom Maps", buf);

		showNeedLatestMapsPopup = 0;
	}

	if (showMiscPopup)
	{
		uiShowOkDialog(miscPopupTitle, miscPopupBody);
		showMiscPopup = 0;
	}
}

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

  //
  //if (padGetButtonDown(0, PAD_L1 | PAD_UP) > 0) {
  //  POKE_U32(0x0036D664, gameGetTime() + (TIME_SECOND * 1020));
  //}

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

  // write patch pointers
  POKE_U32(PATCH_POINTERS + 0, &config);
  POKE_U32(PATCH_POINTERS + 4, &gameConfig);
  //POKE_U32(PATCH_POINTERS + 8, &patchStateContainer);

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
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_SET_RANKS, &onSetRanks);
	netInstallCustomMsgHandler(CUSTOM_MSG_RESPONSE_CUSTOM_MODE_PATCH, &onCustomModeDownloadInitiated);
	netInstallCustomMsgHandler(CUSTOM_MSG_CLIENT_READY, &onClientReadyRemote);

	// Run map loader
	runMapLoader();

#if COMP
	// run comp patch logic
	runCompLogic();
#endif

#if TEST
	// run test patch logic
	runTestLogic();
#endif

  //
  sendClientType();

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
  if (isInMenus()) {
	  runEnableSingleplayerMusic();
  }

	// detects when to download a new custom mode patch
	runPayloadDownloadRequester();

  // sends client ready state to others in lobby when we load the level
  // ensures our local copy of who is ready is reset when loading a new lobby
  runClientReadyMessager();

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

	// Patch shots to be less laggy
	patchWeaponShotLag();

	// Ensures that player's don't lag through the flag
	runFlagPickupFix();

  //
  patchFlagCaptureMessage();

	// Patch state update to run more optimized
	patchStateUpdate();

	// Patch radar scale
	patchRadarScale();

	// Patch process level call
	patchProcessLevel();

	// Patch kill stealing
	patchKillStealing();

	// Patch resurrect weapon ordering
	patchResurrectWeaponOrdering();

	// Patch camera shake
	patchCameraShake();

  //
  patchFusionReticule();

  //
  patchSingleTapChargeboot();

	// 
	//patchWideStats();

  // 
  patchComputePoints();

  // 
  patchFov();

	// 
	//patchAggTime();

	//
	patchLevelOfDetail();

	// 
	patchStateContainer.UpdateCustomGameStats = processSendGameData();

	// 
	patchStateContainer.UpdateGameState = runSendGameUpdate();

	// config update
	onConfigUpdate();

#if MAPINFO
  static Moby* waterMoby = NULL;
  if (!isInGame()) {
    waterMoby = NULL;
  } else {
    waterMoby = mobyFindNextByOClass(mobyListGetStart(), MOBY_ID_WATER);
    if (waterMoby && waterMoby->PVar) {
      DPRINTF("water %08X height %f\n", (u32)waterMoby, ((float*)waterMoby->PVar)[19]);
    }
  }
#endif

	// in game stuff
	if (isInGame())
	{
		// hook render function
		HOOK_JAL(0x004A84B0, &updateHook);
		HOOK_JAL(0x004C3A94, &drawHook);

    // hook Pad_MappedPad
    HOOK_J(0x005282d8, &padMappedPadHooked);
    POKE_U32(0x005282dc, 0);

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

		// close config menu on transition to lobby
		if (lastGameState != 1)
			configMenuDisable();

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
		// render ms isn't important in the menus so assume best case scenario of 0ms
		renderTimeMs = 0;
		averageRenderTimeMs = 0;
		updateTimeMs = 0;
		averageUpdateTimeMs = 0;

		//
		grLobbyStart();

		// 
		patchStagingRankNumber();

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
  else
    PATCH_POINTERS_SPECTATE = 0;

  // Process freecam
  if (isInGame() && gameConfig.drFreecam) {
	  processFreecam();
  } else {
    resetFreecam();
  }

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
