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
#include "gate.h"
#include "game.h"
#include "npc.h"
#include "messager.h"
#include "spawner.h"
#include "controller.h"
#include "mover.h"
#include "mob.h"
#include "shared.h"
#include "pathfind.h"
#include "hub.h"

void mobInit(void);
void mobTick(void);
void configInit(void);
void spVendorInit(void);
void spVendorTick(void);

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
void mapGiveAmmo(void)
{
  // if any weapon ran out of ammo, return back to max
  int i;
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* player = playerGetFromSlot(i);
    if (!player || !player->GadgetBox) continue;

    int j;
    for (j = WEAPON_SLOT_VIPERS; j < WEAPON_SLOT_COUNT; ++j) {
      int gadgetId = weaponSlotToId(j);
      if (player->GadgetBox->Gadgets[gadgetId].Level >= 0 && player->GadgetBox->Gadgets[gadgetId].Ammo <= 0) {
        player->GadgetBox->Gadgets[gadgetId].Ammo = playerGetWeaponMaxAmmo(player->GadgetBox, gadgetId);
      }
    }
  }
}

//--------------------------------------------------------------------------
void mapReturnPlayersToMap(void)
{
  int i;
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
void mapRespawnPlayersOnStart(void)
{
  static int init = 0;

  if (init) return;

  // respawn all players
  Player** players = playerGetAll();
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player || !player->PlayerMoby || !player->pNetPlayer) continue;
    
    // respawn player
    playerGetSpawnpoint(player, player->PlayerPosition, player->PlayerRotation, 1);
    vector_copy(player->PlayerMoby->Position, player->PlayerPosition);
    if (!player->IsLocal) {
      memset((void*)((u32)player->pNetPlayer + 0x38), 0, 0xAD0 - 0x38);
      player->pNetPlayer->lastActiveSeqNum = -1;
    }
  }

  init = 1;
}

//--------------------------------------------------------------------------
void mapOnFrameTick(void)
{
  dlPreUpdate();

  messagerFrameUpdate();

  dlPostUpdate();
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

  mobInit();
  configInit();
  spVendorInit();
  spawnerInit();
  moverInit();
  controllerInit();
  gateInit();
  npcInit();
  messagerInit();
  MapConfig.OnMobCreateFunc = &createMob;
  MapConfig.OnMobUpdateFunc = &mapOnMobUpdate;
  MapConfig.OnMobKilledFunc = &mapOnMobKilled;
  MapConfig.OnFrameTickFunc = &mapOnFrameTick;

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

  // init
  initialize();

  //
  mapRespawnPlayersOnStart();
  if (MapConfig.ClientsReady || !netGetDmeServerConnection())
  {
    spawnerStart();
    moverStart();
    controllerStart();
    gateStart();
    npcStart();
  }

  mobTick();
  spVendorTick();
  for (i = 0; i < PathsCount; ++i) pathTick(&Paths[i]);
  mapReturnPlayersToMap();
  mapGiveAmmo();

  if (MapConfig.State) {
    MapConfig.State->MapBaseComplexity = MAP_BASE_COMPLEXITY;
    MapConfig.State->OnHubWorld = 1;
  }
  
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

	return 0;
}
