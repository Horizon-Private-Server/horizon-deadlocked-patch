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
#include "orxon.h"
#include "upgrade.h"
#include "drop.h"

Moby* gateCreate(VECTOR start, VECTOR end, float height);
void gateSetCollision(int collActive);
void powerNodeUpdate(Moby* moby);
void gateInit(void);
void gateSpawn(VECTOR gateData[], int count);
void gasTick(void);
void mobInit(void);
void mobTick(void);
void configInit(void);
void pathTick(void);

int isMobyInGasArea(Moby* moby);

int zombieCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int executionerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int tremorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);

int aaa = 47;


char LocalPlayerStrBuffer[2][48];

// set by mode
extern SurvivalBakedConfig_t bakedConfig;
struct SurvivalMapConfig MapConfig __attribute__((section(".config"))) = {
  .Magic = MAP_CONFIG_MAGIC,
	.State = NULL,
  .BakedConfig = &bakedConfig,
};

// gate locations
VECTOR GateLocations[] = {
  { 383.47, 564.51, 430.44, 6.5 }, { 383.47, 550.51, 430.44, 1 },
  { 425.35, 564.51, 430.44, 6.5 }, { 425.35, 550.51, 430.44, 2 },
  { 425.46, 652.23, 430.44, 6.5 }, { 425.46, 638.23, 430.44, 1 },
  { 383.24, 652.23, 430.44, 6.5 }, { 383.24, 638.23, 430.44, 2 },
  { 359.28, 614.63, 430.44, 6.5 }, { 373.28, 614.63, 430.44, 3 },
  { 359.28, 576.26, 430.44, 6.5 }, { 373.28, 576.26, 430.44, 1 },
  { 470.88, 607.19, 439.14, 9 }, { 470.88, 593.19, 439.14, 6 },
  { 488.89, 523.54, 430.11, 6.5 }, { 488.89, 509.54, 430.11, 4 },
  { 488.89, 690.17, 430.11, 6.5 }, { 488.89, 676.17, 430.11, 4 },
  { 553.5787, 618.6273, 430.11, 6.5 }, { 563.3413, 608.5927, 430.11, 0 },
  { 587.5181, 599.4265, 430.11, 6.5 }, { 598.042, 590.1935, 430.11, 0 },
};
const int GateLocationsCount = sizeof(GateLocations)/sizeof(VECTOR);

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
  if (!moby || !moby->PVar)
    return 1;

  struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  // can't skip if target is in gas
  if (pvars->MobVars.Target && isMobyInGasArea(pvars->MobVars.Target))
    return 0;

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
void initialize(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  MapConfig.Magic = MAP_CONFIG_MAGIC;
  MapConfig.WeaponPickupCooldownFactor = 1;

  gateInit();
  wraithInit();
  //surgeInit();
  mboxInit();
  mobInit();
  configInit();
  upgradeInit();
  dropInit();
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

  mobTick();
  pathTick();
  upgradeTick();
  dropTick();

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

  gasTick();
  dlPostUpdate();
	return 0;
}

void nodeUpdate(Moby* moby)
{
  static int initialized = 0;
  if (!initialized) {
    DPRINTF("node %08X bolt:%08X\n", (u32)moby, *(u32*)(moby->PVar + 0xC));
    initialized = 1;
  }

  // init
  initialize();
  
  // enable cq
  GameOptions* gameOptions = gameGetOptions();
  if (gameOptions) {
    gameOptions->GameFlags.MultiplayerGameFlags.NodeType = 0;
    //gameOptions->GameFlags.MultiplayerGameFlags.Lockdown = 1;
    gameOptions->GameFlags.MultiplayerGameFlags.Nodes = 1;
  }

  //
  powerNodeUpdate(moby);

  // disable deleting node if not CQ
  POKE_U32(0x003D16DC, 0x1000001D);

  // disable node captured popup
  POKE_U32(0x003D2E6C, 0);

  // increase number of turns to turn on power
  POKE_U32(0x003D2530, 0x3C014000);

  // call base node base update
  ((void (*)(Moby*))0x003D13C0)(moby);

  if (MapConfig.ClientsReady || !netGetDmeServerConnection())
  {
    static int asd = 0;
    if (!asd) {
      asd = 1;
    }

    mboxSpawn();
    wraithSpawn();
    bboxSpawn();
    //surgeSpawn();
    gateSpawn(GateLocations, GateLocationsCount);
  }
}
