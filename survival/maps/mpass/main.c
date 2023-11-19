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
#include "mpass.h"
#include "bigal.h"
#include "gate.h"

Moby* gateCreate(VECTOR start, VECTOR end, float height);
void gateInit(void);
void gateSpawn(VECTOR gateData[], int count);
void gateSetCollision(int collActive);
void mobInit(void);
void configInit(void);
void pathTick(void);

int zombieCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);
int swarmerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);
int tremorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);
int reactorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);
int reaperCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);

char LocalPlayerStrBuffer[2][48];

int aaa = 0;

// set by mode
struct SurvivalMapConfig MapConfig __attribute__((section(".config"))) = {
	.State = NULL,
  .BakedConfig = NULL
};

// gate locations
VECTOR GateLocations[] = {
  { 628.93, 839.62, 502.28, 6.5 }, { 628.93, 819.62, 502.28, 4 },
  { 617.8, 751.91, 508.7, 6.5 }, { 633.8, 751.91, 508.7, 2 },
  { 464.9497, 759.2397, 509.65, 7 }, { 455.0503, 749.3402, 509.65, 1 },
  { 453.8, 824.04, 510.24, 6.5 }, { 453.8, 810.04, 510.24, 4 },
  { 452.52, 900.23, 504.52, 15 }, { 452.52, 880.23, 504.52, 3 },
  { 531.7302, 894.3903, 510.07, 8 }, { 541.6298, 904.2898, 510.07, 3 },
  { 573.8002, 935.2998, 512.69, 9 }, { 573.8002, 917.2998, 510.31, 2 },
  { 579.5502, 895.7498, 508.3, 8 }, { 589.4498, 885.8502, 508.3, 4 },
};
const int GateLocationsCount = sizeof(GateLocations)/sizeof(VECTOR);

//--------------------------------------------------------------------------
void mobForceIntoMapBounds(Moby* moby)
{
  int i;
  VECTOR min = { 370, 700, 470, 0 };
  VECTOR max = { 730, 970, 530, 0 };

  if (!moby)
    return;
    
  for (i = 0; i < 3; ++i) {
    if (moby->Position[i] < min[i])
      moby->Position[i] = min[i];
    else if (moby->Position[i] > max[i])
      moby->Position[i] = max[i];
  }
}

//--------------------------------------------------------------------------
int mapPathCanBeSkippedForTarget(Moby* moby)
{
  return 1;
}

//--------------------------------------------------------------------------
int createMob(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config)
{
  switch (spawnParamsIdx)
  {
    case MOB_SPAWN_PARAM_REACTOR:
    {
      return reactorCreate(spawnParamsIdx, position, yaw, spawnFromUID, freeAgent, config);
    }
    case MOB_SPAWN_PARAM_REAPER:
    {
      return reaperCreate(spawnParamsIdx, position, yaw, spawnFromUID, freeAgent, config);
    }
    case MOB_SPAWN_PARAM_RUNNER:
    {
      return tremorCreate(spawnParamsIdx, position, yaw, spawnFromUID, freeAgent, config);
    }
    case MOB_SPAWN_PARAM_NORMAL:
    {
      return zombieCreate(spawnParamsIdx, position, yaw, spawnFromUID, freeAgent, config);
    }
    case MOB_SPAWN_PARAM_SWARMER:
    {
      return swarmerCreate(spawnParamsIdx, position, yaw, spawnFromUID, freeAgent, config);
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
void initialize(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  MapConfig.Magic = MAP_CONFIG_MAGIC;

  gateInit();
  mboxInit();
  mobInit();
  configInit();
  bigalInit();
  statueInit();
  MapConfig.OnMobCreateFunc = &createMob;

  // only have gate collision on when processing players
  HOOK_JAL(0x003bd854, &onBeforeUpdateHeroes);
  //HOOK_J(0x003bd864, &onAfterUpdateHeroes);
  HOOK_JAL(0x0051f648, &onBeforeUpdateHeroes2);
  //HOOK_J(0x0051f78c, &onAfterUpdateHeroes2);
  
  // have moby 0x1BC6 use glowColor for particle color
  POKE_U32(0x004171D8, 0x8FA80070);
  POKE_U32(0x00417200, 0x8D080060);

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
    bigalSpawn();
    statueSpawn();
    gateSpawn(GateLocations, GateLocationsCount);
  }

  pathTick();

  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

#if DEBUG
  static int tpPlayerToSpawn = 0;

  if (!tpPlayerToSpawn) {
    tpPlayerToSpawn = 1;
    Player* localPlayer = playerGetFromSlot(0);

    if (localPlayer && localPlayer->SkinMoby) {
      VECTOR pStart = { 658.3901, 828.0401, 499.7961, 0 };
      VECTOR pRotStart = { 0, 0, MATH_PI, 0 };
      playerSetPosRot(localPlayer, pStart, pRotStart);
    }
  }
#endif

#if DEBUG
  dlPreUpdate();
  if (padGetButtonDown(0, PAD_LEFT) > 0) {
    --aaa;
    DPRINTF("%d\n", aaa);
  }
  else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    ++aaa;
    DPRINTF("%d\n", aaa);
  }

  /*
  static int handle = 0;
  if (padGetButtonDown(0, PAD_L1 | PAD_L3) > 0) {
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
  else if (padGetButtonDown(0, PAD_L1 | PAD_R3) > 0) {
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
  */

  dlPostUpdate();
#endif

  dlPostUpdate();
	return 0;
}
