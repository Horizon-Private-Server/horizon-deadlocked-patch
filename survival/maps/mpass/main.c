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
#include <libdl/hud.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include "module.h"
#include "messageid.h"
#include "game.h"
#include "maputils.h"
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
void mobTick(void);
void configInit(void);
void pathTick(void);

int zombieCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);
int swarmerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);
int tremorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);
int reactorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);
int reaperCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int freeAgent, struct MobConfig *config);

char LocalPlayerStrBuffer[2][48];

int aaa = 0;

extern Moby* reactorActiveMoby;
Moby* TeleporterMoby = NULL;
int StatuesActivated = 0;

// spawn
VECTOR pStart = { 658.3901, 828.0401, 499.7961, 0 };
VECTOR pRotStart = { 0, 0, MATH_PI, 0 };

// set by mode
struct SurvivalMapConfig MapConfig __attribute__((section(".config"))) = {
	.State = NULL,
  .BakedConfig = NULL
};

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

// gate locations
VECTOR GateLocations[] = {
  { 628.93, 839.62, 502.28, 6.5 }, { 628.93, 819.62, 502.28, 6 },
  { 617.8, 751.91, 508.7, 6.5 }, { 633.8, 751.91, 508.7, 4 },
  { 464.9497, 759.2397, 509.65, 7 }, { 455.0503, 749.3402, 509.65, 2 },
  { 453.8, 824.04, 510.24, 6.5 }, { 453.8, 810.04, 510.24, 4 },
  { 452.52, 900.23, 504.52, 15 }, { 452.52, 880.23, 504.52, 3 },
  { 531.7302, 894.3903, 510.07, 8 }, { 541.6298, 904.2898, 510.07, 3 },
  { 573.8002, 935.2998, 512.69, 9 }, { 573.8002, 917.2998, 510.31, 4 },
  { 579.5502, 895.7498, 508.3, 8 }, { 589.4498, 885.8502, 508.3, 6 },
};
const int GateLocationsCount = sizeof(GateLocations)/sizeof(VECTOR);

// blessings
const char * INTERACT_BLESSING_MESSAGE = "\x11 %s";
const char* BLESSING_NAMES[] = {
  [BLESSING_ITEM_QUAD_JUMP]     "Blessing of the Hare",
  [BLESSING_ITEM_LUCK]          "Blessing of the Clover",
  [BLESSING_ITEM_INF_CBOOT]     "Blessing of the Bull",
  [BLESSING_ITEM_ELEM_IMMUNITY] "Blessing of Corrosion",
  [BLESSING_ITEM_HEALTH_REGEN]  "Blessing of Vitality",
  [BLESSING_ITEM_AMMO_REGEN]    "Blessing of the Hunt",
  [BLESSING_ITEM_THORNS]        "Blessing of the Rose",
};

const int BLESSINGS[] = {
  BLESSING_ITEM_QUAD_JUMP,
  BLESSING_ITEM_LUCK,
  BLESSING_ITEM_INF_CBOOT,
  // BLESSING_ITEM_ELEM_IMMUNITY,
  BLESSING_ITEM_THORNS,
  BLESSING_ITEM_HEALTH_REGEN,
  BLESSING_ITEM_AMMO_REGEN,
};

const VECTOR BLESSING_POSITIONS[BLESSING_ITEM_COUNT] = {
  [BLESSING_ITEM_QUAD_JUMP]     { 449.3509, 829.1361, 404.1362, (-90) * MATH_DEG2RAD },
  [BLESSING_ITEM_LUCK]          { 439.1009, 811.3826, 404.1362, (-30) * MATH_DEG2RAD },
  [BLESSING_ITEM_INF_CBOOT]     { 418.6009, 811.3826, 404.1362, (30) * MATH_DEG2RAD },
  // [BLESSING_ITEM_ELEM_IMMUNITY] { 408.3509, 829.1361, 404.1362, (90) * MATH_DEG2RAD },
  [BLESSING_ITEM_THORNS]        { 408.3509, 829.1361, 404.1362, (90) * MATH_DEG2RAD },
  [BLESSING_ITEM_HEALTH_REGEN]  { 418.6009, 846.8896, 404.1362, (150) * MATH_DEG2RAD },
  [BLESSING_ITEM_AMMO_REGEN]    { 439.1009, 846.8896, 404.1362, (-150) * MATH_DEG2RAD },
};

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

//--------------------------------------------------------------------------
void updateBossMeter(void)
{
  static float lastHealth = -1;
  HudBoss_CommonData_t* hudBossData = (HudBoss_CommonData_t*)0x00310400;
  int i;

  // show/hide boss meter
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    PlayerHUDFlags* hud = hudGetPlayerFlags(i);
    if (hud) {
      hud->Flags.Meter = reactorActiveMoby ? 1 : 0;
    }
  }

  if (!reactorActiveMoby) return;
  struct MobPVar* pvars = (struct MobPVar*)reactorActiveMoby->PVar;
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
  if (bossImgRectObject) ((void (*)(u32, u32))0x005ca3e8)(bossImgRectObject, 0x75AF);
}

//--------------------------------------------------------------------------
void updateBlessingTotems(void)
{
  int local, i;
  int count = sizeof(BLESSINGS)/sizeof(int);
  VECTOR dt;
  static char buf[4][64];

  if (!MapConfig.State) return;

  for (local = 0; local < GAME_MAX_LOCALS; ++local) {
    Player* player = playerGetFromSlot(local);
    if (!player) continue;

    struct SurvivalPlayer* playerData = &MapConfig.State->PlayerStates[player->PlayerId];

    for (i = 0; i < count; ++i) {
      int blessing = BLESSINGS[i];
      if (playerData->State.ItemBlessing == blessing) continue;

      vector_subtract(dt, player->PlayerPosition, (float*)BLESSING_POSITIONS[blessing]);
      if (vector_sqrmag(dt) < (4*4)) {
        
				// draw help popup
        snprintf(buf[local], sizeof(buf[local]), INTERACT_BLESSING_MESSAGE, BLESSING_NAMES[blessing]);
				uiShowPopup(local, buf[local]);
        playerData->MessageCooldownTicks = 2;

        if (padGetButtonDown(local, PAD_CIRCLE) > 0) {
          playerData->State.ItemBlessing = blessing;
        }
        break;
      }
    }
  }
}

//--------------------------------------------------------------------------
void updateBlessingTeleporter(void)
{
  if (!TeleporterMoby) return;

  if (StatuesActivated == statueSpawnPositionRotationsCount && !reactorActiveMoby) {
    mobySetState(TeleporterMoby, 1, -1);
    TeleporterMoby->DrawDist = 64;
    TeleporterMoby->UpdateDist = 64;
    TeleporterMoby->CollActive = 0;
  } else {
    mobySetState(TeleporterMoby, 2, -1);
    TeleporterMoby->DrawDist = 0;
    TeleporterMoby->UpdateDist = 0;
    TeleporterMoby->CollActive = -1;
  }
}

//--------------------------------------------------------------------------
void mapReturnPlayersToMap(void)
{
  int i;
  VECTOR p;

  // if we're in between rounds, don't return
  if (!MapConfig.State) return;
  if (MapConfig.State->RoundEndTime) return;
  if (!TeleporterMoby) return;
  if (TeleporterMoby->State == 1) return;

  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* player = playerGetFromSlot(i);
    if (!player || !player->SkinMoby) continue;

    // if we're under the map, then we must be dead or in the bunker
    // teleport back up either way
    if (player->PlayerPosition[2] < 460) {
      vector_fromyaw(p, (player->PlayerId / (float)GAME_MAX_PLAYERS) * MATH_TAU - MATH_PI);
      vector_scale(p, p, 2.5);
      vector_add(p, p, pStart);
      playerSetPosRot(player, p, pRotStart);
    }
  }
}

//--------------------------------------------------------------------------
void mobForceIntoMapBounds(Moby* moby)
{
  int i;
  VECTOR min = { 370, 700, 495, 0 };
  VECTOR max = { 730, 970, 530, 0 };
	struct MobPVar* pvars = (struct MobPVar*)moby->PVar;

  if (!moby)
    return;
    
  if (moby->Position[2] < min[2])
    pvars->MobVars.Respawn = 1;

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
    case MOB_SPAWN_PARAM_TREMOR:
    {
      return tremorCreate(spawnParamsIdx, position, yaw, spawnFromUID, freeAgent, config);
    }
    case MOB_SPAWN_PARAM_ACID:
    {
      return zombieCreate(spawnParamsIdx, position, yaw, spawnFromUID, freeAgent, config);
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

  // find teleporters
  TeleporterMoby = mobyFindNextByOClass(mobyListGetStart(), MOBY_ID_TELEPORT_PAD);
  DPRINTF("teleporter %08X\n", (u32)TeleporterMoby);

  // enable replaying dialog
  //POKE_U32(0x004E3E2C, 0);

  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

  // only have gate collision on when processing players
  HOOK_JAL(0x003bd854, &onBeforeUpdateHeroes);
  //HOOK_J(0x003bd864, &onAfterUpdateHeroes);
  HOOK_JAL(0x0051f648, &onBeforeUpdateHeroes2);
  //HOOK_J(0x0051f78c, &onAfterUpdateHeroes2);
  
  // have moby 0x1BC6 use glowColor for particle color
  POKE_U32(0x004171D8, 0x8FA80070);
  POKE_U32(0x00417200, 0x8D080060);

  // enable teleporter for everyone
  POKE_U32(0x003DFD20, 0x10000006);
  POKE_U32(0x003DFD6C, 0x00000000);

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
    bigalSpawn();
    statueSpawn();
    gateSpawn(GateLocations, GateLocationsCount);
  }

  mobTick();
  pathTick();
  updateBossMeter();
  updateBlessingTotems();
  updateBlessingTeleporter();
  mapReturnPlayersToMap();

  if (MapConfig.State) {
    MapConfig.State->MapBaseComplexity = MAP_BASE_COMPLEXITY;
  }

#if DEBUG
  static int tpPlayerToSpawn = 0;

  if (!tpPlayerToSpawn) {
    tpPlayerToSpawn = 1;
    Player* localPlayer = playerGetFromSlot(0);

    if (localPlayer && localPlayer->SkinMoby) {
      playerSetPosRot(localPlayer, pStart, pRotStart);
    }
  }
#endif

#if DEBUG
  dlPreUpdate();
  // if (padGetButtonDown(0, PAD_LEFT) > 0) {
  //   --aaa;
  //   //mobyPlaySoundByClass(aaa, 0, playerGetFromSlot(0)->PlayerMoby, MOBY_ID_ROBOT_ZOMBIE);
  //   //playDialog(aaa, 1);
  //   DPRINTF("%d 0x%x\n", aaa, aaa);
  // } else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
  //   ++aaa;
  //   //mobyPlaySoundByClass(aaa, 0, playerGetFromSlot(0)->PlayerMoby, MOBY_ID_ROBOT_ZOMBIE);
  //   //playDialog(aaa, 1);
  //   DPRINTF("%d 0x%x\n", aaa, aaa);
  // }

  /*
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
  */

  dlPostUpdate();
#endif

  dlPostUpdate();
	return 0;
}
