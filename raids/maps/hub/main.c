/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Raids Hub.
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
#include "spawner.h"
#include "mob.h"
#include "shared.h"
#include "pathfind.h"
#include "hub.h"

void mobInit(void);
void mobTick(void);
void configInit(void);

char LocalPlayerStrBuffer[2][64];

// set by mode
extern RaidsBakedConfig_t bakedConfig;
struct RaidsMapConfig MapConfig __attribute__((section(".config"))) = {
  .Magic = MAP_CONFIG_MAGIC,
	.State = NULL,
  .BakedConfig = &bakedConfig,
};

//--------------------------------------------------------------------------
void mobForceIntoMapBounds(Moby* moby)
{

}

//--------------------------------------------------------------------------
int mapPathCanBeSkippedForTarget(struct PathGraph* path, Moby* moby)
{
  return 1;
}

//--------------------------------------------------------------------------
int createMob(struct MobCreateArgs* args)
{
  switch (args->SpawnParamsIdx)
  {
    case MOB_SPAWN_PARAM_NORMAL:
    {
      return zombieCreate(args);
    }
    case MOB_SPAWN_PARAM_SWARMER:
    {
      return swarmerCreate(args);
    }
    default:
    {
      DPRINTF("unhandled create spawnParamsIdx %d\n", args->SpawnParamsIdx);
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
    if (player->PlayerPosition[2] < gameGetDeathHeight()) {
      playerRespawn(player);
      playerSetHealth(player, player->MaxHealth);
    }
  }
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
void initialize(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  MapConfig.Magic = MAP_CONFIG_MAGIC;

  mobInit();
  configInit();
  spawnerInit();
  MapConfig.OnMobCreateFunc = &createMob;
  MapConfig.OnMobUpdateFunc = &mapOnMobUpdate;
  MapConfig.OnMobKilledFunc = &mapOnMobKilled;

  // only have gate collision on when processing players
  HOOK_JAL(0x003bd854, &onBeforeUpdateHeroes);
  //HOOK_J(0x003bd864, &onAfterUpdateHeroes);
  HOOK_JAL(0x0051f648, &onBeforeUpdateHeroes2);
  //HOOK_J(0x0051f78c, &onAfterUpdateHeroes2);

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
int main(void)
{
  int i;
	if (!isInGame())
		return 0;

  dlPreUpdate();

  // init
  initialize();

  //
  if (MapConfig.ClientsReady || !netGetDmeServerConnection())
  {

  }

  mobTick();
  for (i = 0; i < PathsCount; ++i) pathTick(&Paths[i]);
  mapReturnPlayersToMap();

  if (MapConfig.State) {
    MapConfig.State->MapBaseComplexity = MAP_BASE_COMPLEXITY;
  }
  
  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

#if DEBUG1
  dlPreUpdate();
  Player* localPlayer = playerGetFromSlot(0);
  
  static int handle = 0;
  static int aaa = 0;
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
