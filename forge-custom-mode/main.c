/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		FORGE CUSTOM MODE.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/string.h>
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
#include "module.h"
#include "messageid.h"
#include "include/game.h"
#include "include/stats.h"

struct CGMCustomMapExData MapData = {};
char HasMapData = 0;
struct CgmMapConfig* mapConfig = (struct CgmMapConfig*)(EXTRA_CODE_SEG_PTR + 0x10);
long timeGameStarted = 0;
long timeGameEnded = 0;

void applyTeamType(enum CgmTeamType teamType);

//--------------------------------------------------------------------------
int hasMapConfig(void)
{
  return mapConfig && mapConfig->Magic == MAP_CONFIG_MAGIC;
}

//--------------------------------------------------------------------------
void setEndGameScoreboard(PatchGameConfig_t * gameConfig)
{
	u32 * uiElements = (u32*)(*(u32*)(0x011C7064 + 4*18) + 0xB0);
  char buf[32];
	int i;
  GameOptions* gameOptions = gameGetOptions();
  GameSettings* gameSettings = gameGetSettings();

  // write column headers (starts at 17)
  for (i = 0; i < MAX_SCOREBOARD_STATS; ++i)
  {
    UiTextElement_t* textElement = (UiTextElement_t*)uiElements[18 + i];
	  safe_strcpy(textElement->Text, Stats.PlayerStatNames[i], sizeof(Stats.PlayerStatNames[i]));
  }

  // write team names/scores
  if (gameOptions->GameFlags.MultiplayerGameFlags.Teamplay)
  {
    for (i = 0; i < GAME_MAX_PLAYERS; ++i)
    {
      int placement = Stats.TeamPlacements[i];
      if (placement < 0 || placement > 3)
        continue;

      int elemIdx = 1 + placement;

      // set score
      statsFormatValue(buf, sizeof(buf), Stats.TeamScores[i], Stats.TeamScoreType);
      safe_strcpy(((UiTextElement_t*)uiElements[elemIdx])->Text, buf, sizeof(buf));

      char *teamName = ((char* (*)(int))0x006F5728)(i);
      safe_strcpy(((UiTextElement_t*)uiElements[elemIdx])->PAD_10 + 0x08, teamName, 33);
    }
  }

	// rows
	int* pids = (int*)(uiElements[0] - 0x9C);
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		// match scoreboard player row to their respective dme id
		int pid = pids[i];
		char* name = (char*)(uiElements[7 + i] + 0x18);
		if (pid < 0 || name[0] == 0)
			continue;

    int c;
    for (c = 0; c < MAX_SCOREBOARD_STATS; ++c)
    {
      UiTextElement_t* textElement = (UiTextElement_t*)uiElements[22 + (i*4) + c];

      // empty stat
      if (!Stats.PlayerStatNames[c][0])
      {
        textElement->Text[0] = 0;
        continue;
      }

      // set points
      int value = Stats.PlayerStatValues[pid][c];
      statsFormatValue(buf, sizeof(buf), value, Stats.PlayerStatTypes[c]);
      safe_strcpy(textElement->Text, buf, sizeof(buf));
    }
	}
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(PatchStateContainer_t * gameState)
{
	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions || !gameSettings || gameSettings->GameLoadStartTime <= 0)
		return;

  // read custom map settings
  if (!HasMapData) {
    if (gameState->ReadExtraDataFunc(&MapData, sizeof(MapData)) > 0) {
      HasMapData = 1;
    }
  }

	// apply game setting overrides
	gameSettings->GameRules = MapData.GameRule;

  // disable stats
  if (!MapData.TrackBaseStats)
    gameSetIsGameRanked(0);

  // set respawn algo
  switch (MapData.GameRule)
  {
    case GAMERULE_CQ: gameOptions->GameFlags.MultiplayerGameFlags.SpawnType = 0; break;
    case GAMERULE_CTF: gameOptions->GameFlags.MultiplayerGameFlags.SpawnType = 2; break;
    default: gameOptions->GameFlags.MultiplayerGameFlags.SpawnType = 3; break;
  }

  if (MapData.RadarBlips != -1) gameOptions->GameFlags.MultiplayerGameFlags.RadarBlips = MapData.RadarBlips;
  if (MapData.SpecialPickups != -1) gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickups = MapData.SpecialPickups > 0;
  if (MapData.SpecialPickups != -1) gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickupsRandom = MapData.SpecialPickups == 2;
	if (MapData.SpawnWithChargeboots != -1) gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = MapData.SpawnWithChargeboots;
	if (MapData.AutospawnWeapons != -1) gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = MapData.AutospawnWeapons;
	if (MapData.UnlimitedAmmo != -1) gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = MapData.UnlimitedAmmo;
	if (MapData.Timelimit != -1) gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = MapData.Timelimit;
	if (MapData.RespawnTime != -1) gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = MapData.RespawnTime;

	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = MapData.GameRule == GAMERULE_JUGGY;
  if (MapData.KillsToWin != -1) gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = MapData.KillsToWin;
	if (MapData.JuggernautVis != -1) gameOptions->GameFlags.MultiplayerGameFlags.JuggernautCanInvisible = MapData.JuggernautVis != 2;
	if (MapData.JuggernautVis != -1) gameOptions->GameFlags.MultiplayerGameFlags.JuggernautVisibleOnlyOnHit = MapData.JuggernautVis == 0;
	if (MapData.JuggernautHealing != -1) gameOptions->GameFlags.MultiplayerGameFlags.JuggernautHealing = MapData.JuggernautHealing;

  gameOptions->GameFlags.MultiplayerGameFlags.Flags = MapData.GameRule == GAMERULE_CTF;
  if (MapData.CapsToWin != -1) gameOptions->GameFlags.MultiplayerGameFlags.CapsToWin = MapData.CapsToWin;
  if (MapData.CrazyMode != -1) gameOptions->GameFlags.MultiplayerGameFlags.CrazyMode = MapData.CrazyMode;
  if (MapData.FlagReturn != -1) gameOptions->GameFlags.MultiplayerGameFlags.FlagReturn = MapData.FlagReturn;
  if (MapData.VehicleCarry != -1) gameOptions->GameFlags.MultiplayerGameFlags.FlagVehicleCarry = MapData.VehicleCarry;
  if (MapData.GrCtfHalftime != -1) gameState->GameConfig->grHalfTime = MapData.GrCtfHalftime;
  if (MapData.GrCtfOvertime != -1) gameState->GameConfig->grOvertime = MapData.GrCtfOvertime;

  gameOptions->GameFlags.MultiplayerGameFlags.Hills = MapData.GameRule == GAMERULE_KOTH;
  if (MapData.HillTimeToWin != -1) gameOptions->GameFlags.MultiplayerGameFlags.HillTimeToWin = MapData.HillTimeToWin;
  if (MapData.MovingHillTime != -1) gameOptions->GameFlags.MultiplayerGameFlags.HillMovingTime = MapData.MovingHillTime;
  if (MapData.HillSharing != -1) gameOptions->GameFlags.MultiplayerGameFlags.HillSharing = MapData.HillSharing;
  if (MapData.HillArmor != -1) gameOptions->GameFlags.MultiplayerGameFlags.HillArmor = MapData.HillArmor;
  
  gameOptions->GameFlags.MultiplayerGameFlags.Nodes = MapData.GameRule == GAMERULE_CQ;
  if (MapData.GameRule == GAMERULE_CQ)
  {
    gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
    gameOptions->GameFlags.MultiplayerGameFlags.CapsToWin = 0;
    gameOptions->GameFlags.MultiplayerGameFlags.HillTimeToWin = 0;
  }

  if (MapData.BoltsToWin != -1) gameOptions->GameFlags.MultiplayerGameFlags.BoltsToWin = MapData.BoltsToWin;
	if (MapData.NodeType != -1) gameOptions->GameFlags.MultiplayerGameFlags.NodeType = MapData.NodeType;
	if (MapData.SpecialRules != -1) gameOptions->GameFlags.MultiplayerGameFlags.Lockdown = MapData.SpecialRules == 1;
	if (MapData.SpecialRules != -1) gameOptions->GameFlags.MultiplayerGameFlags.Homenodes = MapData.SpecialRules == 2;
	if (MapData.UpgradeTimer != -1) gameOptions->GameFlags.MultiplayerGameFlags.UpgradeTimer = MapData.UpgradeTimer;
	if (MapData.VoteTime != -1) gameOptions->GameFlags.MultiplayerGameFlags.VoteTime = MapData.VoteTime;
	gameOptions->GameFlags.MultiplayerGameFlags.UNK_0F = MapData.Turrets != -1 ? MapData.Turrets : 1;
	gameOptions->GameFlags.MultiplayerGameFlags.UNK_11 = MapData.TeleporterUpgrade != -1 ? MapData.TeleporterUpgrade : 1;
  gameState->GameConfig->grCqDisableTurrets = 0; // defer to gameflag
  if (MapData.GrCqPersistentCapture != -1) gameState->GameConfig->grCqPersistentCapture = MapData.GrCqPersistentCapture;
  if (MapData.GrCqDisableUpgrades != -1) gameState->GameConfig->grCqDisableUpgrades = MapData.GrCqDisableUpgrades;
  
  if (MapData.Vehicles != -1)
  {
    gameOptions->GameFlags.MultiplayerGameFlags.Vehicles = MapData.Vehicles;
    gameOptions->GameFlags.MultiplayerGameFlags.Puma = MapData.Vehicles;
    gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = MapData.Vehicles;
    gameOptions->GameFlags.MultiplayerGameFlags.Hovership = MapData.Vehicles;
    gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = MapData.Vehicles;
  }

	if (MapData.Survivor != -1)
  {
    gameOptions->GameFlags.MultiplayerGameFlags.Survivor = MapData.Survivor;
    gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
  }
  
  if (MapData.GadgetsMask != -1) gameOptions->WeaponFlags.Raw = MapData.GadgetsMask;
  if (MapData.RespawnTime != -1 || MapData.Survivor != -1) gameState->GameConfig->grRespawnOverride = 0;
  
  // patch
  if (MapData.GrDamageCooldown != -1) gameState->GameConfig->grNoInvTimer = !MapData.GrDamageCooldown;
  if (MapData.GrHealthbars != -1) gameState->GameConfig->grHealthBars = MapData.GrHealthbars;
  if (MapData.GrHealthboxes != -1) gameState->GameConfig->grNoHealthBoxes = MapData.GrHealthboxes;
  if (MapData.GrInstantDeath != -1) gameState->GameConfig->grInstantDeath = MapData.GrInstantDeath;
  if (MapData.GrNametags != -1) gameState->GameConfig->grNoNames = !MapData.GrNametags;
  if (MapData.GrRadarShortDistance != -1) gameState->GameConfig->grRadarShortDistance = MapData.GrRadarShortDistance;
  if (MapData.GrRadarShortShared != -1) gameState->GameConfig->grFogOfWarRadar = MapData.GrRadarShortShared;
  if (MapData.GrSpawnImmunity != -1) gameState->GameConfig->grNoSpawnImmunity = !MapData.GrSpawnImmunity;
  if (MapData.GrV2s != -1) gameState->GameConfig->grV2s = MapData.GrV2s;
  if (MapData.GrVampire != -1) gameState->GameConfig->grVampire = MapData.GrVampire;
  if (MapData.GrWeaponPacks != -1) gameState->GameConfig->grNoPacks = !MapData.GrWeaponPacks;
  if (MapData.GrWeaponPickups != -1) gameState->GameConfig->grNoPickups = !MapData.GrWeaponPickups;

  // party
  if (MapData.PrChargebootForever != -1) gameState->GameConfig->prChargebootForever = MapData.PrChargebootForever;
  if (MapData.PrHeadbutt != -1) gameState->GameConfig->prHeadbutt = MapData.PrHeadbutt;
  if (MapData.PrHeadbuttFriendlyFire != -1) gameState->GameConfig->prHeadbuttFriendlyFire = MapData.PrHeadbuttFriendlyFire;
  if (MapData.PrRotatingWeapons != -1) gameState->GameConfig->prRotatingWeapons = MapData.PrRotatingWeapons;

  // apply team type
  applyTeamType(MapData.TeamType);
}

//--------------------------------------------------------------------------
void modeUpdateStats(struct CgmStats* stats)
{
  if (!stats)
    return;

  memcpy(&Stats, stats, sizeof(Stats));

  if (gameAmIHost())
    statsBroadcastSync();
}

//--------------------------------------------------------------------------
void mapUpdateGameState(PatchStateContainer_t * gameState)
{
  if (!hasMapConfig())
    return;

  if (!mapConfig->MapFunctions.UpdateGameState)
    return;

  // reset payload
  if (gameState->UpdateCustomGameStats && gameState->CustomGameStats)
  {
    struct CgmCustomGameStats *sGameData = (struct CgmCustomGameStats *)gameState->CustomGameStats->Payload;
    memset(sGameData, 0, sizeof(struct CgmCustomGameStats));
    GameData* gameData = gameGetData();
    sGameData->RuntimeMs = (timeGameEnded > 0 ? (timeGameEnded - timeGameStarted) : (timerGetSystemTime() - timeGameStarted)) / SYSTEM_TIME_TICKS_PER_MS;
    sGameData->MinTeamsForRank = MapData.MinTeamsForRank;
    sGameData->MinTeamsForStats = MapData.MinTeamsForStats;
    safe_strcpy(sGameData->Name, MapData.GameModeName, sizeof(sGameData->Name));
    safe_strcpy(sGameData->SharedRankCode, MapData.SharedRankCode, sizeof(sGameData->SharedRankCode));

    // tell server that both are disabled
    if (!MapData.TrackCustomStats)
    {
      sGameData->MinTeamsForRank = GAME_MAX_PLAYERS + 1;
      sGameData->MinTeamsForStats = GAME_MAX_PLAYERS + 1;
    }

    // block custom stats
    if (!MapData.TrackCustomStats)
      gameState->UpdateCustomGameStats = 0;
  }

  // pass to map
  mapConfig->MapFunctions.UpdateGameState(gameState);
}

//--------------------------------------------------------------------------
void mapTickFrame(PatchStateContainer_t * gameState)
{
  if (!hasMapConfig())
    return;

  if (!mapConfig->MapFunctions.TickFrame)
    return;

  // pass to map
  mapConfig->MapFunctions.TickFrame(gameState);
}

//--------------------------------------------------------------------------
void mapTickGame(PatchStateContainer_t * gameState)
{
  if (!hasMapConfig())
    return;

  if (!mapConfig->MapFunctions.TickGame)
    return;

  // pass to map
  mapConfig->MapFunctions.TickGame(gameState);
}

//--------------------------------------------------------------------------
void setModeFunctions(void)
{
  if (!isInGame() || !hasMapConfig())
    return;

  mapConfig->ModeFunctions.UpdateStats = modeUpdateStats;
}

//--------------------------------------------------------------------------
void gameUpdateTick(struct GameModule * module, PatchStateContainer_t * gameState)
{
	if (!gameGetSettings() || !isInGame())
		return;

  mapTickGame(gameState);
}

//--------------------------------------------------------------------------
void gameFrameTick(struct GameModule * module, PatchStateContainer_t * gameState)
{
	if (!gameGetSettings() || !isInGame())
		return;

  if (gameHasEnded())
    timeGameEnded = timerGetSystemTime();

  mapTickFrame(gameState);
	mapUpdateGameState(gameState);
}

//--------------------------------------------------------------------------
void lobbyStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	int i;
	int activeId = uiGetActive();
	Player** players = playerGetAll();
	static int initializedScoreboard = 0;

	// set game settings in staging when load countdown begins
	switch (activeId)
	{
		case 0x15C:
		{
			if (initializedScoreboard)
				break;

			setEndGameScoreboard(gameState->GameConfig);
			initializedScoreboard = 1;

			// patch rank computation to keep rank unchanged for base mode
			POKE_U32(0x0077ACE4, 0x4600BB06);
			break;
		}
		case UI_ID_GAME_LOBBY:
		{
			setLobbyGameOptions(gameState);
			break;
		}
	}
}

//--------------------------------------------------------------------------
void loadStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
  // reload map data and force cgm game settings
  HasMapData = 0;
  setLobbyGameOptions(gameState);
  timeGameStarted = timerGetSystemTime();
  statsInit();
}

//--------------------------------------------------------------------------
void start(struct GameModule * module, PatchStateContainer_t * gameState, enum GameModuleContext context)
{
  setModeFunctions();

  switch (context)
  {
    case GAMEMODULE_LOBBY: lobbyStart(module, gameState); break;
    case GAMEMODULE_LOAD: loadStart(module, gameState); break;
    case GAMEMODULE_GAME_FRAME: gameFrameTick(module, gameState); break;
    case GAMEMODULE_GAME_UPDATE: gameUpdateTick(module, gameState); break;
  }
}
