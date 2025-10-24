/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Survival Torval.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/math.h>
#include <libdl/area.h>
#include <libdl/math3d.h>
#include <libdl/random.h>
#include <libdl/collision.h>
#include <libdl/stdio.h>
#include <libdl/spawnpoint.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/hud.h>
#include <libdl/ui.h>
#include <libdl/radar.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include "module.h"
#include "messageid.h"
#include "game.h"
#include "mob.h"
#include "pathfind.h"
#include "maputils.h"
#include "torval.h"
#include "upgrade.h"
#include "drop.h"
#include "hackerorb.h"
//#include "mover.h"
//#include "controller.h"
//#include "messager.h"
//#include "dummy.h"

void mobInit(void);
void mobTick(void);
void configInit(void);
void pathTick(void);
void stackableInit(void);
void stackableTick(void);

void stackableOnMobKilled(Moby* moby, int killedByPlayerId, int killedByWeaponId);

void frameTick(void);
void mapOnMobKilled(Moby* moby, int killedByPlayerId, int killedByWeaponId);
int mapCanSpawnMobs(void);
int mapGetResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes);

int zombieCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int leviathanCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int reaperCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int reactorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int tremorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int swarmerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int executioner2Create(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);

char LocalPlayerStrBuffer[GAME_MAX_LOCALS][64];

typedef struct HudBoss_CommonData { // 0x38
	/* 0x00 */ int iShown;
	/* 0x04 */ int iState;
	/* 0x08 */ float fHP;
	/* 0x0c */ float fDisplayHP;
	/* 0x10 */ unsigned int iColor;
	/* 0x14 */ unsigned int iColor2;
	/* 0x18 */ int iIcon;
	/* 0x1c */ float fHideX;
	/* 0x20 */ float fShown;
	/* 0x24 */ char bShown;
	/* 0x25 */ char bFancy;
	/* 0x28 */ int iDelay;
	/* 0x2c */ char bUpdate;
	/* 0x30 */ float fPulse;
	/* 0x34 */ int iFillMode;
} HudBoss_CommonData_t;

// set by mode
extern SurvivalBakedConfig_t bakedConfig;
struct SurvivalMapConfig MapConfig __attribute__((section(".config"))) = {
  .Magic = MAP_CONFIG_MAGIC,
	.State = NULL,
  .BakedConfig = &bakedConfig,
  .OnFrameTickFunc = &frameTick,
  .OnMobKilledFunc = &mapOnMobKilled,
  .CanSpawnMobsFunc = &mapCanSpawnMobs,
};

//--------------------------------------------------------------------------
int mapSpawnMob(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags)
{
  // increment # mobs to spawn
  if (MapConfig.State && spawnFromUID < 0) {
    MapConfig.State->RoundMaxMobCount += 1;
  }

  if (!gameAmIHost()) return;

  struct MobConfig* config = &MapConfig.DefaultSpawnParams[spawnParamsIdx].Config;

  // spawn
  if (MapConfig.ModeCreateMobFunc)
    return MapConfig.ModeCreateMobFunc(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
  else
    return MapConfig.OnMobCreateFunc(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
}

//--------------------------------------------------------------------------
void mapOnMobKilled(Moby* moby, int killedByPlayerId, int killedByWeaponId)
{
  stackableOnMobKilled(moby, killedByPlayerId, killedByWeaponId);
}

//--------------------------------------------------------------------------
int mapCanSpawnMobs(void)
{
  bakedConfig.SpawnDistanceFactor = 0.2;
  return 1;
}

//--------------------------------------------------------------------------
void mapReturnPlayersToMap(void)
{
  int i;
  VECTOR p,r,o;

  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* player = playerGetFromSlot(i);
    if (!player || !player->SkinMoby) continue;

    // if we're under the map, teleport back up
    if (player->PlayerPosition[2] < (gameGetDeathHeight() + 1)) {
      
      // use player start
      if (bakedSpawnGetFirst(BAKED_SPAWNPOINT_PLAYER_START, p, r)) {
        vector_fromyaw(o, (player->PlayerId / (float)GAME_MAX_PLAYERS) * MATH_TAU - MATH_PI);
        vector_scale(o, o, 2.5);
        vector_add(p, p, o);
        playerSetPosRot(player, p, r);
        playerSetHealth(player, maxf(0, player->Health - player->MaxHealth*0.5));
      }
    }
  }
}

//--------------------------------------------------------------------------
void mobForceIntoMapBounds(Moby* moby)
{
  if (!moby)
    return;
    
  int i;
  VECTOR min = { 200, 260, 0, 0 };
  VECTOR max = { 400, 500, 200, 0 };
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  for (i = 0; i < 3; ++i) {
    if (moby->Position[i] < min[i]) {
      moby->Position[i] = min[i];
      pvars->MobVars.Respawn = 1;
      break;
    }
    else if (moby->Position[i] > max[i]) {
      moby->Position[i] = max[i];
      pvars->MobVars.Respawn = 1;
      break;
    }
  }
}

//--------------------------------------------------------------------------
int mapPathCanBeSkippedForTarget(Moby* moby)
{
  return 1;
}

//--------------------------------------------------------------------------
int createMob(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config)
{
  switch (spawnParamsIdx)
  {
    case MOB_SPAWN_PARAM_LEVIATHAN:
    {
      return leviathanCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_REAPER:
    {
      return reaperCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_REACTOR:
    {
      return reactorCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    // case MOB_SPAWN_PARAM_TREMOR:
    // {
    //   return tremorCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    // }
    case MOB_SPAWN_PARAM_EXECUTIONER:
    {
      return executioner2Create(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_NORMAL:
    {
      return zombieCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_SWARMER:
    {
      return swarmerCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    default:
    {
      DPRINTF("unhandled create spawnParamsIdx %d\n", spawnParamsIdx);
      break;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------
void addBlip(Moby* moby, int type, int team, int life)
{
  if (!moby) return;

  // add blip
  int blipId = radarGetBlipIndex(moby);
  if (blipId >= 0)
  {
    RadarBlip * blip = radarGetBlips() + blipId;
    blip->X = moby->Position[0];
    blip->Y = moby->Position[1];
    blip->Life = life;
    blip->Type = type;
    blip->Team = team;
  }
}

//--------------------------------------------------------------------------
void randomizeWeaponPickups(void)
{
  int i,j;
  GameOptions* gameOptions = gameGetOptions();
  char wepCounts[9];
  char wepEnabled[17];
  int pickupCount = 0;
  int pickupOptionCount = 0;
  memset(wepEnabled, 0, sizeof(wepEnabled));
  memset(wepCounts, 0, sizeof(wepCounts));

  if (gameOptions->WeaponFlags.DualVipers) { wepEnabled[2] = 1; pickupOptionCount++; }
  if (gameOptions->WeaponFlags.MagmaCannon) { wepEnabled[3] = 1; pickupOptionCount++; }
  if (gameOptions->WeaponFlags.Arbiter) { wepEnabled[4] = 1; pickupOptionCount++; }
  if (gameOptions->WeaponFlags.FusionRifle) { wepEnabled[5] = 1; pickupOptionCount++; }
  if (gameOptions->WeaponFlags.MineLauncher) { wepEnabled[6] = 1; pickupOptionCount++; }
  if (gameOptions->WeaponFlags.B6) { wepEnabled[7] = 1; pickupOptionCount++; }
  if (gameOptions->WeaponFlags.Holoshield) { wepEnabled[16] = 1; pickupOptionCount++; }
  if (gameOptions->WeaponFlags.Flail) { wepEnabled[12] = 1; pickupOptionCount++; }
  if (gameOptions->WeaponFlags.Chargeboots && gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots == 0) { wepEnabled[13] = 1; pickupOptionCount++; }

  if (pickupOptionCount > 0) {
    Moby* moby = mobyListGetStart();
    Moby* mEnd = mobyListGetEnd();

    while (moby < mEnd) {
      if (moby->OClass == MOBY_ID_WEAPON_PICKUP && moby->PVar) {
        
        int target = pickupCount / pickupOptionCount;
        int gadgetId = 1;
        if (target < 3) {
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
        ((void (*)(Moby*, int))0x0043A370)(moby, gadgetId);

        ++pickupCount;
      }

      ++moby;
    }
  }
}

//--------------------------------------------------------------------------
void updateBossMeter(void)
{
  static float lastHealth = -1;
  HudBoss_CommonData_t* hudBossData = (HudBoss_CommonData_t*)0x00310400;
  int i;
  Moby* bossMoby = NULL;

  if (MapConfig.State)
    bossMoby = MapConfig.State->BossMoby;

  // show/hide boss meter
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    PlayerHUDFlags* hud = hudGetPlayerFlags(i);
    if (hud) {
      hud->Flags.Meter = bossMoby ? 1 : 0;
    }
  }

  if (!bossMoby) return;
  struct MobPVar* pvars = (struct MobPVar*)bossMoby->PVar;
  float health = pvars->MobVars.Health / pvars->MobVars.Config.MaxHealth;

  // update meter and icon
  hudBossData->fHP = health;

  // refresh on health change
  if (lastHealth != health) {
    lastHealth = health;
    hudBossData->bUpdate = 1;
  }

  // set boss image to reactor sprite
  u32 id = hudPanelGetElement((void*)0x222b18, 6);
  struct HUDWidgetRectangleObject* bossImgRectObject = (struct HUDWidgetRectangleObject*)hudCanvasGetObject(hudGetCanvas(4), id);
  if (bossImgRectObject) ((void (*)(u32, u32))0x005ca3e8)(bossImgRectObject, 0x75AF + 3);
}

//--------------------------------------------------------------------------
void onBeforeUpdateHeroes(void)
{
  ((void (*)())0x005ce1d8)();
}

//--------------------------------------------------------------------------
void onBeforeUpdateHeroes2(u32 a0)
{
  ((void (*)(u32))0x0059b320)(a0);
}

//--------------------------------------------------------------------------
void frameTick(void)
{
  sboxFrameTick();
  //messagerFrameUpdate();
}

//--------------------------------------------------------------------------
int mapConsiderMobSpawnPoint(struct MobSpawnParams* mobSpawnParams, VECTOR position, float yaw, Player* targetPlayer)
{
  if (!targetPlayer || !targetPlayer->PlayerMoby) return 1;

  // check if we have a path to
  // if not, don't spawn here
  if (!pathHasRouteFromTo(pathGetClosestNodeIdx(position), pathTargetCacheGetClosestNodeIdx(targetPlayer->PlayerMoby))) {
    return 0;
  }

  return 1;
}

//--------------------------------------------------------------------------
struct GuberMoby* mapGetGuber(Moby* moby)
{
  if (!moby) return NULL;

  switch (moby->OClass)
  {
    case MOBY_ID_HACKER_ORB: return hackerorbGetGuber(moby);
  }
  
  return 0;
}

//--------------------------------------------------------------------------
int mapHandleGuberEvent(Moby* moby, GuberEvent* event)
{
  if (!moby || !event || !isInGame())
    return 0;

  switch (moby->OClass)
  {
    //case CONTROLLER_OCLASS: return controllerHandleEvent(moby, event);
    //case MOVER_OCLASS: return moverHandleEvent(moby, event);
    case MOBY_ID_HACKER_ORB: return hackerorbHandleEvent(moby, event);
    //case DUMMY_OCLASS: return dummyHandleEvent(moby, event);
  }

  return 0;
}

//--------------------------------------------------------------------------
void mapInstallMobyFunctions(MobyFunctions* mobyFunctions)
{
  //if (!baseGetGuberFunc) baseGetGuberFunc = mobyFunctions->GetGuberObject;
  //if (!baseHandleGuberEventFunc) baseHandleGuberEventFunc = mobyFunctions->MobyEventHandler;

  mobyFunctions->GetGuberObject = &mapGetGuber;
  mobyFunctions->GetMobyInterface = NULL;
  mobyFunctions->MobyEventHandler = &mapHandleGuberEvent;
}

//--------------------------------------------------------------------------
void initialize(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  MapConfig.Magic = MAP_CONFIG_MAGIC;
  MapConfig.WeaponPickupCooldownFactor = 0.65;
  MapConfig.OnUnhandledGetGuberFunc = mapGetGuber;
  MapConfig.OnUnhandledGuberEventFunc = mapHandleGuberEvent;
  MapConfig.ConsiderMobSpawnPointFunc = mapConsiderMobSpawnPoint;

  mapApplyFixes();
  mboxInit();
  mobInit();
  configInit();
  upgradeInit();
  dropInit();
  sboxInit();
  stackableInit();
  //moverInit();
  //controllerInit();
  //messagerInit();
  //dummyInit();
  hackerorbInit();
  randomizeWeaponPickups();
  MapConfig.OnMobCreateFunc = &createMob;

  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

  // only have gate collision on when processing players
  HOOK_JAL(0x003bd854, &onBeforeUpdateHeroes);
  //HOOK_J(0x003bd864, &onAfterUpdateHeroes);
  HOOK_JAL(0x0051f648, &onBeforeUpdateHeroes2);
  //HOOK_J(0x0051f78c, &onAfterUpdateHeroes2);
  
  DPRINTF("path %08X end %08X\n", (u32)&MOB_PATHFINDING_PATHS, (u32)&MOB_PATHFINDING_PATHS + (MOB_PATHFINDING_PATHS_MAX_PATH_LENGTH * MOB_PATHFINDING_NODES_COUNT * MOB_PATHFINDING_NODES_COUNT));

  initialized = 1;
}

/*
 * NAME :		main
 * 
 * DESCRIPTION :
 * 			Entrypoint.
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
  if (!isInGame() && !isSceneLoadedNotYetInGame())
		return 0;

  dlPreUpdate();

  // init
  initialize();

  if (!isInGame()) return;

  //
  if (MapConfig.ClientsReady || !netGetDmeServerConnection())
  {
    mboxSpawn();
    sboxSpawn();
    //moverStart();
    //controllerStart();
    //dummyStart();
  }

  mobTick();
  pathTick();
  upgradeTick();
  dropTick();
  stackableTick();
  mapReturnPlayersToMap();
  //updateBossMeter();

  if (MapConfig.State) {
    MapConfig.State->MapBaseComplexity = MAP_BASE_COMPLEXITY;
  }
  
  // 
  if (MapConfig.State) {
    addBlip(MapConfig.State->BigAl, 14, TEAM_YELLOW, 31);

    Moby* vendorMoby = MapConfig.State->Vendor;
    while (vendorMoby && vendorMoby->OClass == MOBY_ID_WEAPON_VENDOR) {
      addBlip(vendorMoby, 4, TEAM_GREEN, 31);
      ++vendorMoby;
    }
      
    // enable prestige if round % 25
    Moby* prestigeMachineMoby = MapConfig.State->PrestigeMachine;
    if (prestigeMachineMoby) {
      int enabled = MapConfig.State->RoundEndTime && ((MapConfig.State->RoundNumber + 1) % 25) == 0;
        
      if (enabled) {
        if (prestigeMachineMoby->CollActive < 0) {
          pushSnack(0, "Prestige Machine Activated", 120);
        }
        prestigeMachineMoby->DrawDist = 64;
        prestigeMachineMoby->CollActive = 0;
      } else {
        prestigeMachineMoby->DrawDist = 0;
        prestigeMachineMoby->CollActive = -1;
      }
    }
  }

#if DEBUG1
  static int tpPlayerToSpawn = 0;
  if (!tpPlayerToSpawn) {
    tpPlayerToSpawn = 1;

    Player* localPlayer = playerGetFromSlot(0);
    if (localPlayer && localPlayer->SkinMoby) {
      VECTOR pStart = { 521.06, 533, 434, 0 };
      VECTOR pRotStart = { 0, 0, 0, 0 };
      playerSetPosRot(localPlayer, pStart, pRotStart);
    }
  }
#endif

  dlPostUpdate();
	return 0;
}
