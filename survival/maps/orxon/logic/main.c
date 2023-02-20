/***************************************************
 * FILENAME :		logic.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Survival V2's Mining Facility.
 * 		Handles logic for gaseous part of map.
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
#include "../../../include/game.h"
#include "../../../include/mob.h"

Moby* gateCreate(VECTOR start, VECTOR end, float height);
void powerNodeUpdate(Moby* moby);
void gateInit(void);
void gateSpawn(VECTOR gateData[], int count);
void gasTick(void);
void mobInit(void);
void configInit(void);

int zombieCreate(VECTOR position, float yaw, int spawnFromUID, struct MobConfig *config);
int executionerCreate(VECTOR position, float yaw, int spawnFromUID, struct MobConfig *config);
int tremorCreate(VECTOR position, float yaw, int spawnFromUID, struct MobConfig *config);

int aaa = 2;


char LocalPlayerStrBuffer[2][48];

// set by mode
struct SurvivalMapConfig MapConfig __attribute__((section(".config"))) = {
	.State = NULL,
  .BakedConfig = NULL
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
int createMob(VECTOR position, float yaw, int spawnFromUID, struct MobConfig *config)
{
  switch (config->MobType)
  {
    case MOB_TANK:
    {
      return executionerCreate(position, yaw, spawnFromUID, config);
    }
    case MOB_RUNNER:
    {
      return tremorCreate(position, yaw, spawnFromUID, config);
    }
    case MOB_NORMAL:
    case MOB_ACID:
    case MOB_FREEZE:
    case MOB_EXPLODE:
    case MOB_GHOST:
    {
      return zombieCreate(position, yaw, spawnFromUID, config);
    }
    default:
    {
      DPRINTF("unhandled create mobtype %d\n", config->MobType);
      break;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------
void initialize(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  gateInit();
  mboxInit();
  mobInit();
  configInit();
  MapConfig.OnMobCreateFunc = &createMob;

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
	int i;
	if (!isInGame())
		return;

  dlPreUpdate();

  // init
  initialize();

  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

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
    DPRINTF("node %08X\n", (u32)moby);
    initialized = 1;
  }

  // init
  initialize();
  
  // enable cq
  GameOptions* gameOptions = gameGetOptions();
  if (gameOptions) {
    gameOptions->GameFlags.MultiplayerGameFlags.NodeType = 0;
    //gameOptions->GameFlags.MultiplayerGameFlags.Lockdown = 1;
    gameOptions->GameFlags.MultiplayerGameFlags.UNK_09 = 1;
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
    mboxSpawn();
    gateSpawn(GateLocations, GateLocationsCount);
  }
}
