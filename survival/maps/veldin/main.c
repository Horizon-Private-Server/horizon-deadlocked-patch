/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Survival Veldin.
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
#include "veldin.h"
#include "upgrade.h"
#include "drop.h"

Moby* gateCreate(VECTOR start, VECTOR end, float height);
void gateSetCollision(int collActive);
void gateInit(void);
void gateSpawn(VECTOR gateData[], int count);
void mobInit(void);
void mobTick(void);
void configInit(void);
void pathTick(void);

int zombieCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int reaperCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int tremorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);

int aaa = 0;
char LocalPlayerStrBuffer[2][64];

// set by mode
extern SurvivalBakedConfig_t bakedConfig;
struct SurvivalMapConfig MapConfig __attribute__((section(".config"))) = {
  .Magic = MAP_CONFIG_MAGIC,
	.State = NULL,
  .BakedConfig = &bakedConfig,
};

// spawn
VECTOR pStart = { 188.15, 450.96, 87.93756, 0 };
VECTOR pRotStart = { 0, 0, -MATH_PI/2, 0 };

// gate locations
VECTOR GateLocations[] = {
  { 224.5227, 414.6316, 83.76, 8 }, { 223.5353, 400.6664, 83.76, 0 },
};
const int GateLocationsCount = sizeof(GateLocations)/sizeof(VECTOR);

//--------------------------------------------------------------------------
void mobForceIntoMapBounds(Moby* moby)
{
  if (!moby)
    return;

  if (moby->Position[0] < 125)
    moby->Position[0] = 125;
  else if (moby->Position[0] > 300)
    moby->Position[0] = 300;
  
  if (moby->Position[1] < 300)
    moby->Position[1] = 300;
  else if (moby->Position[1] > 550)
    moby->Position[1] = 550;

  if (moby->Position[2] < 0)
    moby->Position[2] = 0;
  else if (moby->Position[2] > 200)
    moby->Position[2] = 200;
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
    case MOB_SPAWN_PARAM_REAPER:
    {
      return reaperCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_TREMOR:
    {
      return tremorCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_NORMAL:
    {
      return zombieCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
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
void mapReturnPlayersToMap(void)
{
  int i;
  VECTOR p;

  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* player = playerGetFromSlot(i);
    if (!player || !player->SkinMoby) continue;

    // if we're under the map, then tp back up to spawn
    if (player->PlayerPosition[2] < 76) {
      vector_fromyaw(p, (player->PlayerId / (float)GAME_MAX_PLAYERS) * MATH_TAU - MATH_PI);
      vector_scale(p, p, 2.5);
      vector_add(p, p, pStart);
      playerSetPosRot(player, p, pRotStart);
      playerSetHealth(player, player->Health - (player->MaxHealth*0.5));
    }
  }
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
  VECTOR p = {211.01,397.66,80.00006,0};
  VECTOR r = {0,0,(48.983 + 90) * MATH_DEG2RAD,0};
  bboxCreate(p, r);

  spawned = 1;
}

//--------------------------------------------------------------------------
void initialize(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  MapConfig.Magic = MAP_CONFIG_MAGIC;
  MapConfig.WeaponPickupCooldownFactor = 0.5;

  gateInit();
  mboxInit();
  mobInit();
  configInit();
  upgradeInit();
  dropInit();
  bboxInit();
  MapConfig.OnMobCreateFunc = &createMob;

  // only have gate collision on when processing players
  HOOK_JAL(0x003bd854, &onBeforeUpdateHeroes);
  //HOOK_J(0x003bd864, &onAfterUpdateHeroes);
  HOOK_JAL(0x0051f648, &onBeforeUpdateHeroes2);
  //HOOK_J(0x0051f78c, &onAfterUpdateHeroes2);

  DPRINTF("path %08X end %08X\n", (u32)&MOB_PATHFINDING_PATHS, (u32)&MOB_PATHFINDING_PATHS + (MOB_PATHFINDING_PATHS_MAX_PATH_LENGTH * MOB_PATHFINDING_NODES_COUNT * MOB_PATHFINDING_NODES_COUNT));

  initialized = 1;
}


SoundDef def =
{
	0.0,	// MinRange
	50.0,	// MaxRange
	100,		// MinVolume
	10000,		// MaxVolume
	0,			// MinPitch
	0,			// MaxPitch
	0,			// Loop
	0x10,		// Flags
	95,    // 98, 95, 
	3			  // Bank
};


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
    bboxSpawn();
    gateSpawn(GateLocations, GateLocationsCount);
  }

  mobTick();
  pathTick();
  upgradeTick();
  dropTick();
  mapReturnPlayersToMap();

  if (MapConfig.State) {
    MapConfig.State->MapBaseComplexity = MAP_BASE_COMPLEXITY;
  }
  
  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

#if DEBUG
  dlPreUpdate();
  Player* localPlayer = playerGetFromSlot(0);
  // if (padGetButtonDown(0, PAD_LEFT) > 0) {
  //   --aaa;
  //   DPRINTF("%d\n", aaa);
  // }
  // else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
  //   ++aaa;
  //   DPRINTF("%d\n", aaa);
  // }

  static int handle = 0;
  if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    aaa += 1;
    def.Index = aaa;
    if (handle)
      soundKillByHandle(handle);
    int id = soundPlay(&def, 0, playerGetFromSlot(0)->PlayerMoby, 0, 0x400);
    if (id >= 0)
      handle = soundCreateHandle(id);
    else
      handle = 0;
    DPRINTF("%d\n", aaa);
  }
  else if (padGetButtonDown(0, PAD_LEFT) > 0) {
    aaa -= 1;
    def.Index = aaa;
    if (handle)
      soundKillByHandle(handle);
    int id = soundPlay(&def, 0, playerGetFromSlot(0)->PlayerMoby, 0, 0x400);
    if (id >= 0)
      handle = soundCreateHandle(id);
    else
      handle = 0;
    DPRINTF("%d\n", aaa);
  }

  dlPostUpdate();
#endif

  dlPostUpdate();
	return 0;
}
