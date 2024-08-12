/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Custom map logic for Survival V2's Mountain Pass.
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
#include "upgrade.h"
#include "drop.h"
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
void blessingsInit(void);
void blessingsTick(void);

int zombieCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int swarmerCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int tremorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int reactorCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);
int reaperCreate(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config);

char LocalPlayerStrBuffer[2][64];
char HasEquippedNewBlessing[GAME_MAX_PLAYERS];

int aaa = 0;

extern Moby* reactorActiveMoby;
Moby* TeleporterMoby = NULL;
int StatuesActivated = 0;
int BlessingSlotsUnlocked = 0;

// spawn
VECTOR pStart = { 658.3901, 828.0401, 499.7961, 0 };
VECTOR pRotStart = { 0, 0, MATH_PI, 0 };

// set by mode
extern SurvivalBakedConfig_t bakedConfig;
struct SurvivalMapConfig MapConfig __attribute__((section(".config"))) = {
  .Magic = MAP_CONFIG_MAGIC,
	.State = NULL,
  .BakedConfig = &bakedConfig,
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
  [BLESSING_ITEM_MULTI_JUMP]      "Blessing of the Hare",
  [BLESSING_ITEM_LUCK]            "Blessing of the Clover",
  [BLESSING_ITEM_BULL]            "Blessing of the Bull",
  //[BLESSING_ITEM_ELEM_IMMUNITY] "Blessing of Corrosion",
  [BLESSING_ITEM_THORNS]          "Blessing of the Rose",
  [BLESSING_ITEM_HEALTH_REGEN]    "Blessing of Vitality",
  [BLESSING_ITEM_AMMO_REGEN]      "Blessing of the Hunt",
};

const char* BLESSING_DESCRIPTIONS[BLESSING_ITEM_COUNT] = {
  [BLESSING_ITEM_MULTI_JUMP]      "Suddenly the walls seem.. shorter...",
  [BLESSING_ITEM_LUCK]            "Vox appreciates your business...",
  [BLESSING_ITEM_BULL]            "You feel as though you could charge forever...",
  [BLESSING_ITEM_THORNS]          "You feel.. protected...",
  [BLESSING_ITEM_HEALTH_REGEN]    "The nanomites inside you begin to change...",
  [BLESSING_ITEM_AMMO_REGEN]      "The nanomites inside your weapons begin to change...",
};

const int BLESSINGS[] = {
  BLESSING_ITEM_MULTI_JUMP,
  BLESSING_ITEM_LUCK,
  BLESSING_ITEM_BULL,
  BLESSING_ITEM_THORNS,
  BLESSING_ITEM_HEALTH_REGEN,
  BLESSING_ITEM_AMMO_REGEN,
  // BLESSING_ITEM_ELEM_IMMUNITY,
};

const int BLESSING_TEXIDS[] = {
  [BLESSING_ITEM_MULTI_JUMP]  111 - 3,
  [BLESSING_ITEM_LUCK]  112 - 3,
  [BLESSING_ITEM_BULL]  113 - 3,
  [BLESSING_ITEM_ELEM_IMMUNITY]  114 - 3,
  [BLESSING_ITEM_HEALTH_REGEN]  115 - 3,
  [BLESSING_ITEM_AMMO_REGEN]  116 - 3,
  [BLESSING_ITEM_THORNS]  117 - 3,
};

VECTOR BLESSING_POSITIONS[BLESSING_ITEM_COUNT] = {
  [BLESSING_ITEM_MULTI_JUMP]     { 449.3509, 829.1361, 404.1362, (-90) * MATH_DEG2RAD },
  [BLESSING_ITEM_LUCK]          { 439.1009, 811.3826, 404.1362, (-30) * MATH_DEG2RAD },
  [BLESSING_ITEM_BULL]     { 418.6009, 811.3826, 404.1362, (30) * MATH_DEG2RAD },
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
int onHasPlayerUsedTeleporter(Moby* moby, Player* player)
{
	// pointer to player is in $s1
	asm volatile (
    ".set noreorder;\n"
		"move %0, $s1"
		: : "r" (player)
	);

  // only filter for the top teleporter moby
  if (moby != TeleporterMoby) return 0;

  return HasEquippedNewBlessing[player->PlayerId];
}

//--------------------------------------------------------------------------
void playerSetPlayerHasEquippedNewBlessing(Player* player, int hasEntered)
{
  HasEquippedNewBlessing[player->PlayerId] = hasEntered;
}

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
void drawBlessingTotemQuads(void)
{
  int count = sizeof(BLESSINGS)/sizeof(int);
  int i;

  for (i = 0; i < count; ++i) {
    int blessing = BLESSINGS[i];

    // draw
    VECTOR p = {0,0,3,0};
    vector_add(p, BLESSING_POSITIONS[blessing], p);
    //HOOK_JAL(0x205b64dc, 0x004e4d70);
    gfxDrawBillboardQuad(vector_read(p), 1, 0x80FFFFFF, 0x80 << 24, -MATH_PI / 2, BLESSING_TEXIDS[blessing], 1, 0);
    //HOOK_JAL(0x205b64dc, 0x004c4200);
  }
}

//--------------------------------------------------------------------------
void updateBlessingTotems(void)
{
  int local, i, j;
  int count = sizeof(BLESSINGS)/sizeof(int);
  VECTOR dt;
  static char buf[4][64];

  if (!MapConfig.State) return;

  for (i = 0; i < count; ++i) {
    int blessing = BLESSINGS[i];

    // handle pickup
    for (local = 0; local < GAME_MAX_LOCALS; ++local) {
      Player* player = playerGetFromSlot(local);
      if (!player) continue;

      struct SurvivalPlayer* playerData = &MapConfig.State->PlayerStates[player->PlayerId];
      if (!playerData->State.BlessingSlots) continue;
      if (playerHasBlessing(player->PlayerId, blessing)) continue;

      vector_subtract(dt, player->PlayerPosition, (float*)BLESSING_POSITIONS[blessing]);
      if (vector_sqrmag(dt) < (4*4)) {
        
				// draw help popup
        snprintf(buf[local], sizeof(buf[local]), INTERACT_BLESSING_MESSAGE, BLESSING_NAMES[blessing]);
				uiShowPopup(local, buf[local]);
        playerData->MessageCooldownTicks = 2;

        if (padGetButtonDown(local, PAD_CIRCLE) > 0) {
          for (j = 0; j < playerData->State.BlessingSlots;)
            if (playerData->State.ItemBlessings[j++] == BLESSING_ITEM_NONE) break;
          playerData->State.ItemBlessings[j-1] = blessing;
          pushSnack(local, BLESSING_DESCRIPTIONS[blessing], TPS * 2);
          playerSetPlayerHasEquippedNewBlessing(player, 1);
        }
        break;
      }
    }
  }
}

//--------------------------------------------------------------------------
int statuesAreActivated(void)
{
  return StatuesActivated == statueSpawnPositionRotationsCount;
}

//--------------------------------------------------------------------------
void updateUnlockedBlessingSlots(void)
{
  static int unlockedThisRound = 0;
  static int lastRoundNumber = 0;
  static int roundWasSpecial = 0;
  if (!MapConfig.State) return;

#if DEBUG
  BlessingSlotsUnlocked = 3;
#endif

  if (BlessingSlotsUnlocked < 3 && MapConfig.State->RoundCompleteTime && !unlockedThisRound && roundWasSpecial && statuesAreActivated()) {
    BlessingSlotsUnlocked += 1;
    unlockedThisRound = 1;
    DPRINTF("unlocked blessing slots %d\n", BlessingSlotsUnlocked);
  }

  // force
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    MapConfig.State->PlayerStates[i].State.BlessingSlots = BlessingSlotsUnlocked;
  }

  if (lastRoundNumber != MapConfig.State->RoundNumber) {
    lastRoundNumber = MapConfig.State->RoundNumber;
    roundWasSpecial = MapConfig.State->RoundIsSpecial;
    unlockedThisRound = 0;
  }
}

//--------------------------------------------------------------------------
void updateBlessingTeleporter(void)
{
  if (!TeleporterMoby) return;

  gfxRegisterDrawFunction((void**)0x0022251C, &drawBlessingTotemQuads, TeleporterMoby);

  if (statuesAreActivated() && !reactorActiveMoby && MapConfig.State->RoundCompleteTime) {
    mobySetState(TeleporterMoby, 1, -1);
    TeleporterMoby->DrawDist = 64;
    TeleporterMoby->UpdateDist = 64;
    TeleporterMoby->CollActive = 0;
  } else {
#if !DEBUG
    mobySetState(TeleporterMoby, 2, -1);
    TeleporterMoby->DrawDist = 0;
    TeleporterMoby->UpdateDist = 0;
    TeleporterMoby->CollActive = -1;
#endif
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

    // reset so they can enter next time the teleporter appears
    playerSetPlayerHasEquippedNewBlessing(player, 0);
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
int createMob(int spawnParamsIdx, VECTOR position, float yaw, int spawnFromUID, int spawnFlags, struct MobConfig *config)
{
  switch (spawnParamsIdx)
  {
    case MOB_SPAWN_PARAM_REACTOR:
    {
      return reactorCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_REAPER:
    {
      return reaperCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_TREMOR:
    {
      return tremorCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_ACID:
    {
      return zombieCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_NORMAL:
    {
      return zombieCreate(spawnParamsIdx, position, yaw, spawnFromUID, spawnFlags, config);
    }
    case MOB_SPAWN_PARAM_SWARMER:
    case MOB_SPAWN_PARAM_GIANT_SWARMER:
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
  VECTOR p = {582.43,792.27,501.0247,0};
  VECTOR r = {0,0,(75 + 90) * MATH_DEG2RAD,0};
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
  mboxInit();
  mobInit();
  configInit();
  bigalInit();
  statueInit();
  upgradeInit();
  dropInit();
  bboxInit();
  blessingsInit();
  MapConfig.OnMobCreateFunc = &createMob;

  memset(HasEquippedNewBlessing, 0, sizeof(HasEquippedNewBlessing));

  // find teleporters
  TeleporterMoby = mobyFindNextByOClass(mobyListGetStart(), MOBY_ID_TELEPORT_PAD);
  DPRINTF("teleporter %08X\n", (u32)TeleporterMoby);

  // enable replaying dialog
  //POKE_U32(0x004E3E2C, 0);

  // change DrawBillboardQuad to use frame tex
  HOOK_JAL(0x005b64dc, 0x004e4d70);

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
  HOOK_JAL(0x003dfd18, &onHasPlayerUsedTeleporter);
  POKE_U32(0x003dfd1c, 0x0240202D);
  strncpy(uiMsgString(0x25f9), "The gods have blessed you plenty.", 34);
  //POKE_U32(0x003DFD20, 0x10000006);
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
    bboxSpawn();
    gateSpawn(GateLocations, GateLocationsCount);
  }

  mobTick();
  pathTick();
  upgradeTick();
  dropTick();
  blessingsTick();
  updateUnlockedBlessingSlots();
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
  // static int soundTestMobyClass = 0;
  // short class = *(short*)(0x00249E30 + (2 * soundTestMobyClass));
  // if (padGetButtonDown(0, PAD_LEFT) > 0) {
  //   --aaa;

  //   mobyPlaySoundByClass(aaa, 0, playerGetFromSlot(0)->PlayerMoby, class);
  //   //playDialog(aaa, 1);

  //   DPRINTF("aaa:%d_0x%x class:0x%04x\n", aaa, aaa, class);
  // } else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
  //   ++aaa;

  //   mobyPlaySoundByClass(aaa, 0, playerGetFromSlot(0)->PlayerMoby, class);
  //   //playDialog(aaa, 1);

  //   DPRINTF("aaa:%d_0x%x class:0x%04x\n", aaa, aaa, class);
  // } else if (padGetButtonDown(0, PAD_UP) > 0) {
  //   soundTestMobyClass += 1;
  //   class = *(short*)(0x00249E30 + (2 * soundTestMobyClass));
  //   aaa = 0;

  //   mobyPlaySoundByClass(aaa, 0, playerGetFromSlot(0)->PlayerMoby, class);

  //   DPRINTF("aaa:%d_0x%x class:0x%04x\n", aaa, aaa, class);
  // } else if (padGetButtonDown(0, PAD_DOWN) > 0) {
  //   soundTestMobyClass -= 1;
  //   class = *(short*)(0x00249E30 + (2 * soundTestMobyClass));
  //   aaa = 0;

  //   mobyPlaySoundByClass(aaa, 0, playerGetFromSlot(0)->PlayerMoby, class);

  //   DPRINTF("aaa:%d_0x%x class:0x%04x\n", aaa, aaa, class);
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
