/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Survival V2's Orxon.
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
#include <libdl/math3d.h>
#include <libdl/collision.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include "module.h"
#include "messageid.h"
#include "game.h"
#include "mob.h"
#include "pathfind.h"
#include "maputils.h"
#include "orxon.h"
#include "upgrade.h"
#include "drop.h"

Moby* gateCreate(VECTOR start, VECTOR end, float height);
void gateSetCollision(int collActive);
void powerNodeUpdate(Moby* moby);
void gateInit(void);
void gateSpawn(VECTOR gateData[], int count);
void mobInit(void);
void mobTick(void);
void configInit(void);
void pathTick(void);
void stackableInit(void);
void stackableTick(void);

void frameTick(void);

int zombieCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int executionerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int tremorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int swarmerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);

int aaa = 47;


char LocalPlayerStrBuffer[2][64];

// set by mode
extern SurvivalBakedConfig_t bakedConfig;
struct SurvivalMapConfig MapConfig __attribute__((section(".config"))) = {
  .Magic = MAP_CONFIG_MAGIC,
	.State = NULL,
  .BakedConfig = &bakedConfig,
  .OnFrameTickFunc = &frameTick
};

// gate locations
VECTOR GateLocations[] = {
  { 383.47, 564.51, 430.44, 6.5 }, { 383.47, 550.51, 430.44, 1 },
  { 425.35, 564.51, 430.44, 6.5 }, { 425.35, 550.51, 430.44, 2 },
  { 425.46, 652.23, 430.44, 6.5 }, { 425.46, 638.23, 430.44, 1 },
  { 383.24, 652.23, 430.44, 6.5 }, { 383.24, 638.23, 430.44, 2 },
  { 359.28, 614.63, 430.44, 6.5 }, { 373.28, 614.63, 430.44, 3 },
  { 359.28, 576.26, 430.44, 6.5 }, { 373.28, 576.26, 430.44, 1 },
};
const int GateLocationsCount = sizeof(GateLocations)/sizeof(VECTOR);

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
      if (bakedSpawnGetFirst(BAKED_SPAWNPOINT_PLAYER_START, p, r)) {
        vector_fromyaw(o, (player->PlayerId / (float)GAME_MAX_PLAYERS) * MATH_TAU - MATH_PI);
        vector_scale(o, o, 2.5);
        vector_add(p, p, o);
        playerSetPosRot(player, p, r);
      }
    }
  }
}

//--------------------------------------------------------------------------
void mobForceIntoMapBounds(Moby* moby)
{
  if (!moby)
    return;

  if (moby->Position[0] < 100)
    moby->Position[0] = 100;
  // prevent mob from entering gas zone
  else if (moby->Position[0] > 524.4)
    moby->Position[0] = 524.4;
  
  if (moby->Position[1] < 400)
    moby->Position[1] = 400;
  else if (moby->Position[1] > 800)
    moby->Position[1] = 800;

  if (moby->Position[2] < 400)
    moby->Position[2] = 400;
  else if (moby->Position[2] > 500)
    moby->Position[2] = 500;
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
    case MOB_SPAWN_PARAM_TITAN:
    {
      return executionerCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_TREMOR:
    {
      return tremorCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_GHOST:
    case MOB_SPAWN_PARAM_EXPLOSION:
    case MOB_SPAWN_PARAM_ACID:
    case MOB_SPAWN_PARAM_FREEZE:
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
void onBeforeUpdateHeroes(void)
{
  gateSetCollision(1);

  ((void (*)())0x005ce1d8)();
}

//--------------------------------------------------------------------------
void onBeforeUpdateHeroes2(u32 a0)
{
  gateSetCollision(1);

  ((void (*)(u32))0x0059b320)(a0);
}

//--------------------------------------------------------------------------
void bboxSpawn(void)
{
  static int spawned = 0;
  if (spawned)
    return;

  // spawn
  VECTOR p = {322.91,539.6399,433.9998,0};
  VECTOR r = {0,0,(-45 + 90) * MATH_DEG2RAD,0};
  bboxCreate(p, r);

  spawned = 1;
}

//--------------------------------------------------------------------------
void frameTick(void)
{
  sboxFrameTick();
}

//--------------------------------------------------------------------------
void initialize(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  MapConfig.Magic = MAP_CONFIG_MAGIC;
  MapConfig.WeaponPickupCooldownFactor = 1;

  gateInit();
  mboxInit();
  mobInit();
  configInit();
  upgradeInit();
  dropInit();
  bboxInit();
  sboxInit();
  stackableInit();
  MapConfig.OnMobCreateFunc = &createMob;

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
	if (!isInGame())
		return 0;

  dlPreUpdate();

  // init
  initialize();

  //
  if (MapConfig.ClientsReady || !netGetDmeServerConnection())
  {
    mboxSpawn();
    //bboxSpawn();
    sboxSpawn();
    gateSpawn(GateLocations, GateLocationsCount);
  }

  mobTick();
  pathTick();
  upgradeTick();
  dropTick();
  stackableTick();
  mapReturnPlayersToMap();

  if (MapConfig.State) {
    MapConfig.State->MapBaseComplexity = MAP_BASE_COMPLEXITY;
  }
  
  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

#if DEBUG
  static int tpPlayerToSpawn = 0;
  if (!tpPlayerToSpawn) {
    tpPlayerToSpawn = 1;

    Player* localPlayer = playerGetFromSlot(0);
    if (localPlayer && localPlayer->SkinMoby) {
      VECTOR pStart = { 328.6, 544.85, 434, 0 };
      VECTOR pRotStart = { 0, 0, 0, 0 };
      playerSetPosRot(localPlayer, pStart, pRotStart);
    }
  }
#endif

  dlPostUpdate();
	return 0;
}
