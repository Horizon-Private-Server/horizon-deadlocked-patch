/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		SURVIVAL.
 * 
 * NOTES :
 * 		Each offset is determined per app id.
 * 		This is to ensure compatibility between versions of Deadlocked/Gladiator.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/weapon.h>
#include <libdl/hud.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/dialog.h>
#include <libdl/ui.h>
#include <libdl/stdio.h>
#include <libdl/color.h>
#include <libdl/stdlib.h>
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/random.h>
#include <libdl/net.h>
#include <libdl/sound.h>
#include <libdl/dl.h>
#include <libdl/utils.h>
#include <libdl/string.h>
#include <libdl/collision.h>
#include <libdl/radar.h>
#include "module.h"
#include "common.h"
#include "messageid.h"
#include "config.h"
#include "include/mob.h"
#include "include/bubble.h"
#include "include/game.h"
#include "include/stats.h"
#include "include/utils.h"

#define SPAWNPOINT_NEAR_BUFFER_SIZE         (3)

const char * SURVIVAL_ROUND_COMPLETE_MESSAGE = "Round %d Complete!";
const char * SURVIVAL_ROUND_START_MESSAGE = "Round %d";
const char * SURVIVAL_START_NEXT_ROUND_TIMER_MESSAGE = "\x1d   Start Round";
const char * SURVIVAL_VOTE_NEXT_ROUND_TIMER_MESSAGE = "\x1d   Vote To Start Round (%d/%d)";
const char * SURVIVAL_VOTED_NEXT_ROUND_TIMER_MESSAGE = "Waiting For Players (%d/%d)";
const char * SURVIVAL_HOST_SKIP_VOTE_NEXT_ROUND_TIMER_MESSAGE = "\x1d   Skip Waiting For Players (%d/%d)";
const char * SURVIVAL_GAME_OVER = "GAME OVER";
const char * SURVIVAL_REVIVE_MESSAGE = "Revive %s"; //"\x1c (DOWN) Revive %s";
const char * SURVIVAL_OPEN_WEAPONS_MESSAGE = "\x12 Manage Mods";
const char * SURVIVAL_INTERACT_BANK_BALANCE_MESSAGE = "Balance \x0A%s\x08";
const char * SURVIVAL_INTERACT_BANK_INTERACT_MESSAGE = "\x11 Deposit \x13 Withdraw";
const char * SURVIVAL_PRESTIGE_WEAPON_MESSAGE = "\x11 Prestige [\x0E%s\x08]";
const char * SURVIVAL_PRESTIGE_WEAPON_NEED_V10_MESSAGE = "Your weapon is not powerful enough";
const char * SURVIVAL_PRESTIGE_WEAPON_MAXED_MESSAGE = "Your weapon is too powerful";

const char * ALPHA_MODS[] = {
  "",
  "Speed Mod",
  "Ammo Mod",
  "Aiming Mod",
  "Impact Mod",
  "Area Mod",
  "XP Mod",
  "Jackpot Mod",
  "Nanoleech Mod"
};

const char WEAPON_PRESTIGE_PREFIX[WEAPON_PRESTIGE_MAX+1] = {
  [0] '\x08',
  [1] '\x09',
  [2] '\x0A',
  [3] '\x0B',
  [4] '\x0F',
  [5] '\x0E',
};

const u8 UPGRADEABLE_WEAPONS[] = {
  WEAPON_ID_VIPERS,
  WEAPON_ID_MAGMA_CANNON,
  WEAPON_ID_ARBITER,
  WEAPON_ID_FUSION_RIFLE,
  WEAPON_ID_MINE_LAUNCHER,
  WEAPON_ID_B6,
  WEAPON_ID_OMNI_SHIELD,
  WEAPON_ID_FLAIL
};

#if FIXEDTARGET
Moby* FIXEDTARGETMOBY = NULL;
#endif

char LocalPlayerStrBuffer[2][64];

int BoltCounts[GAME_MAX_LOCALS] = {};

int Initialized = 0;
int InitializeDelay = 0;
int FirstTimeInitialized = 0;

struct SurvivalState State;
//struct SurvivalMapConfig* mapConfig = (struct SurvivalMapConfig*)0x01EF0010;
struct SurvivalMapConfig* mapConfig = (struct SurvivalMapConfig*)(EXTRA_CODE_SEG_PTR + 0x10);

struct SurvivalSnackItem snackItems[SNACK_ITEM_MAX_COUNT] = {};
int snackItemsCount = 0;

int defaultSpawnParamsCooldowns[MAX_MOB_SPAWN_PARAMS] = {};
u8 mobPlaySoundCooldownTicks[MAX_MOB_SPAWN_PARAMS][MOBS_PLAY_SOUND_COOLDOWN_MAX_SOUNDIDS] = {};

PatchConfig_t* playerConfig = NULL;

int playerStates[GAME_MAX_PLAYERS] = {};
int playerStateTimers[GAME_MAX_PLAYERS] = {};

SoundDef TestSoundDef =
{
  0.0,	  // MinRange
  50.0,	  // MaxRange
  10,		  // MinVolume
  1000,		// MaxVolume
  0,			// MinPitch
  0,			// MaxPitch
  0,			// Loop
  0x10,		// Flags
  0xF5,		// Index
  3			  // Bank
};

struct CustomDzoCommandSurvivalDrawHud
{
  char RoundCompleteMessage[64];
  char RoundStartMessage[64];
  int Tokens;
  int EnemiesAlive;
  int CurrentRoundNumber;
  int HeldItem;
  int StartRoundTimer;
  float XpPercent;
  char HasRoundCompleteMessage;
  char HasDblPoints;
  char HasDblXp;
  char WaitingForHost;
  int Timer;
  Moby* BossMoby;
  int BossIconId;
} dzoDrawHudCmd;

struct CustomDzoCommandSurvivalDrawReviveMsg
{
  VECTOR Position;
  int Seconds;
  int PlayerIdx;
};

void _getLocalBolts(void);
void dzoDrawReviveMsg(int playerId, VECTOR wsPosition, int seconds);
void resetRoundState(void);

//--------------------------------------------------------------------------
void dzoDrawReviveMsg(int playerId, VECTOR wsPosition, int seconds)
{
  struct CustomDzoCommandSurvivalDrawReviveMsg cmd;
  if (!PATCH_DZO_INTEROP_FUNCS)
    return;

  vector_copy(cmd.Position, wsPosition);
  cmd.Seconds = seconds;
  cmd.PlayerIdx = playerId;
  PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_SURVIVAL_DRAW_REVIVE_MSG, sizeof(cmd), &cmd);
}

//--------------------------------------------------------------------------
void updateDzoHud(void)
{
  if (!PATCH_DZO_INTEROP_FUNCS)
    return;

  int gameTime = gameGetTime();
  int bossIconId = 0;
  Player* player = playerGetFromSlot(0);

  u32 bossIconHudId = hudPanelGetElement((void *)0x222b18, 6);
	struct HUDWidgetRectangleObject *bossImgRectObject = (struct HUDWidgetRectangleObject *)hudCanvasGetObject(hudGetCanvas(4), bossIconHudId);
  if (bossImgRectObject) bossIconId = bossImgRectObject->TextureId;
  
  dzoDrawHudCmd.BossIconId = bossIconId;
  dzoDrawHudCmd.BossMoby = State.BossMoby;
  dzoDrawHudCmd.Tokens = State.LocalPlayerState->State.CurrentTokens;
  dzoDrawHudCmd.HeldItem = -1;
  dzoDrawHudCmd.CurrentRoundNumber = State.RoundNumber;
  dzoDrawHudCmd.EnemiesAlive = (State.RoundMaxMobCount - State.MobStats.TotalSpawnedThisRound) + State.MobStats.TotalAlive + State.MobStats.TotalSpawning; //State.MobStats.TotalAlive;
  dzoDrawHudCmd.StartRoundTimer = State.RoundEndTime - gameTime;
  dzoDrawHudCmd.WaitingForHost = State.RoundCompleteTime && State.RoundEndTime < 0;
  dzoDrawHudCmd.HasDblPoints = State.LocalPlayerState->IsDoublePoints;
  dzoDrawHudCmd.HasDblXp = State.LocalPlayerState->IsDoubleXP;
  dzoDrawHudCmd.XpPercent = State.LocalPlayerState->State.XP / (float)getXpForNextToken(player, State.LocalPlayerState->State.TotalTokens);
  dzoDrawHudCmd.Timer = gameTime - State.InitializedTime;
  PATCH_DZO_INTEROP_FUNCS->SendCustomCommandToClient(CUSTOM_DZO_CMD_ID_SURVIVAL_DRAW_HUD, sizeof(dzoDrawHudCmd), &dzoDrawHudCmd);

  // reset
  dzoDrawHudCmd.HasRoundCompleteMessage = 0;
  dzoDrawHudCmd.StartRoundTimer = 0;
  dzoDrawHudCmd.RoundStartMessage[0] = 0;
}

//--------------------------------------------------------------------------
void setPlayerEXP(int localPlayerIndex, float expPercent)
{
  Player* player = playerGetFromSlot(localPlayerIndex);
  if (!player || !player->PlayerMoby)
    return;

  u32 canvasId = localPlayerIndex; //hudGetCurrentCanvas() + localPlayerIndex;
  void* canvas = hudGetCanvas(canvasId);
  if (!canvas)
    return;

  struct HUDWidgetRectangleObject* expBar = (struct HUDWidgetRectangleObject*)hudCanvasGetObject(canvas, 0xF000010A);
  if (!expBar) {
    //DPRINTF("no exp bar %d canvas:%d\n", localPlayerIndex, canvasId);
    return;
  }
  
  // set exp bar
  expBar->iFrame.ScaleX = 0.2275 * clamp(expPercent, 0, 1);
  expBar->iFrame.PositionX = 0.406 + (expBar->iFrame.ScaleX / 2);
  expBar->iFrame.PositionY = 0.0546875;
  expBar->iFrame.Color = hudGetTeamColor(player->Team, 2);
  expBar->Color1 = hudGetTeamColor(player->Team, 0);
  expBar->Color2 = hudGetTeamColor(player->Team, 2);
  expBar->Color3 = hudGetTeamColor(player->Team, 0);

  // make sure dzo gets xp bar
  if (PATCH_INTEROP->Client == CLIENT_TYPE_DZO) {
    *(float*)0x002ADC4C = 0.2275 * clamp(expPercent, 0, 1);
  }
  
  struct HUDWidgetTextObject* healthText = (struct HUDWidgetTextObject*)hudCanvasGetObject(canvas, 0xF000010E);
  if (!healthText) {
    //DPRINTF("no health text %d\n", localPlayerIndex);
    return;
  }
  
  // set health string
  snprintf(State.PlayerStates[player->PlayerId].HealthBarStrBuf, sizeof(State.PlayerStates[player->PlayerId].HealthBarStrBuf), "%d/%d", (int)player->Health, (int)player->MaxHealth);
  healthText->ExternalStringMemory = State.PlayerStates[player->PlayerId].HealthBarStrBuf;
}

//--------------------------------------------------------------------------
void setPlayerWeaponsMenu(int localPlayerIndex)
{
  static int has[GAME_MAX_LOCALS] = {0};
  Player* player = playerGetFromSlot(localPlayerIndex);
  if (!player || !player->PlayerMoby)
    return;

  u32 startCanvas = hudGetCurrentCanvas();
  u32 canvasId = localPlayerIndex; //hudGetCurrentCanvas() + localPlayerIndex;
  void* canvas = hudGetCanvas(canvasId);
  if (!canvas)
    return;

  int addrs[] = {
    0x00222C40,
    0x00222C48,
  };

  int isOpen = hudCanvasGetObject(canvas, hudPanelGetElement((void*)addrs[0], 0)) != NULL;
  if (!isOpen) {
    has[localPlayerIndex] = 0;
    return;
  } else if (has[localPlayerIndex]) {
    return;
  }

  hudSetCurrentCanvas(canvasId);

  int i,j;
  for (j = 0; j < 2; ++j) {
    int addr = addrs[j];
      
    for (i = -1; i < 256; ++i) {
      u32 id = hudPanelGetElement((void*)addr, i);
      struct HUDFrameObject* frame = (struct HUDFrameObject*)hudCanvasGetObject(canvas, id);

      if (frame) {
        
        float sx,sy,px,py;
        hudElementGetScale(id, &sx, &sy);
        hudElementGetPosition(id, &px, &py);

        // squish vertically
        sy *= 0.5;
        py *= 0.5;
        if (j == 1) {
          if (i == 0) {
            sy *= 2;
          }

          py += 0.11;
        }

        hudElementSetScale(id, sx, sy);
        hudElementSetPosition(id, px, py);
      }
    }
  }

  hudSetCurrentCanvas(startCanvas);
  has[localPlayerIndex] = 1;
}

//--------------------------------------------------------------------------
void popSnack(void)
{
  memmove(&snackItems[0], &snackItems[1], sizeof(struct SurvivalSnackItem) * (SNACK_ITEM_MAX_COUNT-1));
  memset(&snackItems[SNACK_ITEM_MAX_COUNT-1], 0, sizeof(struct SurvivalSnackItem));

  if (snackItemsCount)
    --snackItemsCount;
}

//--------------------------------------------------------------------------
void pushSnack(char * str, int ticksAlive, int localPlayerIdx)
{
  while (snackItemsCount >= SNACK_ITEM_MAX_COUNT) {
    popSnack();
  }

  // clamp
  if (snackItemsCount < 0)
    snackItemsCount = 0;

  // special case if ticks is 0
  // want to add only if there are no other snacks in the queue
  // as we assume this case is used for snacks drawn once a frame, but called every frame
  // and we don't want to fill up the queue with them
  if (ticksAlive <= 0 && snackItemsCount > 0)
    return;

  escapePrintfPercent(snackItems[snackItemsCount].Str, sizeof(snackItems[snackItemsCount].Str), str);
  snackItems[snackItemsCount].TicksAlive = ticksAlive;
  snackItems[snackItemsCount].DisplayForLocalPlayerIdx = localPlayerIdx;
  ++snackItemsCount;
}

//--------------------------------------------------------------------------
void drawSnack(void)
{
  if (snackItemsCount <= 0)
    return;

  // draw
  //uiShowPopup(0, snackItems[0].Str);
  char* a = uiMsgString(0x2400);
  safe_strcpy(a, snackItems[0].Str, sizeof(snackItems[0].Str));

  if (snackItems[0].DisplayForLocalPlayerIdx <= 0)
    uiShowLowerPopup(0, 0x2400);
  if (snackItems[0].DisplayForLocalPlayerIdx < 0 || snackItems[0].DisplayForLocalPlayerIdx == 1)
    uiShowLowerPopup(1, 0x2400);

  snackItems[0].TicksAlive--;

  // remove when dead
  if (snackItems[0].TicksAlive <= 0)
    popSnack();
}

//--------------------------------------------------------------------------
int shouldDrawHud(void)
{
  PlayerHUDFlags* hudFlags = hudGetPlayerFlags(0);
  return hudFlags && hudFlags->Flags.Raw != 0;
}

//--------------------------------------------------------------------------
void drawRoundMessage(const char * message, float scale, int yPixelsOffset)
{
  float x = 0.5;
  float y = 0.16;

  // move to dzo
  dzoDrawHudCmd.HasRoundCompleteMessage = 1;
  safe_strcpy(dzoDrawHudCmd.RoundCompleteMessage, message, sizeof(dzoDrawHudCmd.RoundCompleteMessage));

  // draw message
  y *= SCREEN_HEIGHT;
  x *= SCREEN_WIDTH;
  gfxScreenSpaceText(x, y + 5 + yPixelsOffset, scale, scale * 1.5, 0x80FFFFFF, message, -1, 1);
}

//--------------------------------------------------------------------------
void drawTimer(int time)
{
  char buf[32];

  if (State.LocalPlayerState->IsInWeaponsMenu) return;
  if (time < 0) time = 0;
  snprintf(buf, sizeof(buf), "%02d:%02d", time / TIME_MINUTE, (time % TIME_MINUTE) / TIME_SECOND);
  gfxScreenSpaceText(31, 211, 0.9, 0.9, 0x40000000, buf, -1, 1);
  gfxScreenSpaceText(30, 210, 0.9, 0.9, 0x80E0E0E0, buf, -1, 1);
}

//--------------------------------------------------------------------------
void openWeaponsMenu(int localPlayerIndex)
{
  ((void (*)(int, int))0x00544748)(localPlayerIndex, 4);
  ((void (*)(int, int))0x005415c8)(localPlayerIndex, 2);

  ((void (*)(int))0x00543e10)(localPlayerIndex); // hide player
  int s = ((int (*)(int))0x00543648)(localPlayerIndex); // get hud enter state
  ((void (*)(int))0x005c2370)(s); // swapto
}

//--------------------------------------------------------------------------
Moby * mobyGetRandomHealthbox(void)
{
  Moby* results[10];
  int count = 0;
  Moby* start = mobyListGetStart();
  Moby* end = mobyListGetEnd();

  while (start < end && count < 10) {
    if (start->OClass == MOBY_ID_HEALTH_BOX_MULT && !mobyIsDestroyed(start))
      results[count++] = start;

    ++start;
  }

  if (count > 0)
    return results[rand(count)];

  return NULL;
}

//--------------------------------------------------------------------------
Player * playerGetRandom(void)
{
  int r = rand(GAME_MAX_PLAYERS);
  int i = 0, c = 0;
  Player ** players = playerGetAll();

  do {
    // make sure player is alive and not in the jump pad state
    Player* player = players[i];
    if (player && !playerIsDead(player) && player->Health > 0 && player->PlayerState != PLAYER_STATE_MOON_JUMP)
      ++c;
    
    ++i;
    if (i == GAME_MAX_PLAYERS) {
      if (c == 0)
        return NULL;
      i = 0;
    }
  } while (c < r);

  return players[i-1];
}

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
  // pass to map
  if (hasMapConfig() && playerGetRes(player, outPos, outRot, firstRes))
    return;

  // pass to base if we don't have a player start
  playerGetSpawnpoint(player, outPos, outRot, firstRes);
}

//--------------------------------------------------------------------------
void mobyEmptyDrawCallback(Moby* moby)
{

}

//--------------------------------------------------------------------------
void mobyRemoveDrawFunctions(Moby* moby)
{
  struct DrawFunction {
    void* pCallback;
    Moby* pMoby;
    void* pUNK_C;
    void* pUNK_10;
  };

  int count = *(int*)0x00222574;
  struct DrawFunction* pDrawFuncs = (struct DrawFunction*)0x0023e700;

  int i;
  for (i = 0; i < count; ++i) {
    struct DrawFunction* pDrawFunc = pDrawFuncs + i;
    if (pDrawFunc->pMoby == moby) {
      pDrawFunc->pCallback = &mobyEmptyDrawCallback;
    }
  }
}

//--------------------------------------------------------------------------
void mobyRemoveDamages(Moby* moby)
{
  struct DrawFunction {
    void* pCallback;
    Moby* pMoby;
    void* pUNK_C;
    void* pUNK_10;
  };

  MobyColDamage* damageTable = (MobyColDamage*)0x0023F980;
  int i;
  for (i = 0; i < 0x40; ++i) {
    if (damageTable[i].Damager == moby) {
      memset(&damageTable[i], 0, sizeof(MobyColDamage));
    }
  }
}

//--------------------------------------------------------------------------
void onMobyDestroyedCleanupAnimLayers(Moby* moby)
{
  // call base
  ((void (*)(Moby*))0x004fb480)(moby);
  moby->CollCnt = 0;

  // remove any persistent draw calls
  mobyRemoveDrawFunctions(moby);
}

//--------------------------------------------------------------------------
void onMobySpawnedInitInstance(Moby* moby, int oclass, int a2)
{
  // init moby instance
  ((void (*)(Moby*, int, int))0x004f7330)(moby, oclass, a2);

  // remove any MobyCollDamage where moby is source
  mobyRemoveDamages(moby);
}

//--------------------------------------------------------------------------
int spawnPointGetNearestTo(VECTOR point, VECTOR out, float minDist)
{
  VECTOR t;
  int i;
  float bestPointDist = 100000;
  float minDistSqr = minDist * minDist;
  int* spIndices = NULL;
  int spCount = mapConfig->Functions.GetSpawnPointsFunc(&spIndices);
  if (!spCount || !spIndices) return 0;

  for (i = 0; i < spCount; ++i) {
    SpawnPoint* sp = spawnPointGet(spIndices[i]);
    vector_subtract(t, (float*)&sp->M0[12], point);
    float d = vector_sqrmag(t);
    if (d >= minDistSqr) {
      // randomize order a little
      d += randRange(0, 15 * 15);
      if (d < bestPointDist) {
        vector_copy(out, (float*)&sp->M0[12]);
        vector_fromyaw(t, randRadian());
        vector_scale(t, t, 3);
        vector_add(out, out, t);
        bestPointDist = d;
      }
    }
  }

  return spCount > 0;
}

//--------------------------------------------------------------------------
int spawnPointGetNearToPlayer(struct MobSpawnParams* mob, VECTOR out, float minDist)
{
  VECTOR t;
  int i,j;
  int found = 0;
  float bestPointDistSqr[SPAWNPOINT_NEAR_BUFFER_SIZE] = {100000,100000,100000};
  int bestPoints[SPAWNPOINT_NEAR_BUFFER_SIZE] = {-1,-1,-1};
  float minDistSqr = minDist * minDist;
  VECTOR cuboidSpaceP = {randRange(-1, 1),randRange(-1, 1),0.1,0};
  int* spIndices = NULL;
  int spCount = mapConfig->Functions.GetSpawnPointsFunc(&spIndices);
  if (!spCount || !spIndices) return 0;

  // pick random player
  Player* player = playerGetRandom();
  if (!player) return 0;

  for (i = 0; i < spCount; ++i) {
    int spIdx = spIndices[i];
    SpawnPoint* sp = spawnPointGet(spIdx);

    // get closest sqr dist to player
    VECTOR p;
    vector_apply(p, cuboidSpaceP, sp->M0);
    vector_subtract(t, p, player->PlayerPosition);
    float d = vector_sqrmag(t);

    // randomize order a little
    d += randRange(0, 0.5 * minDistSqr);

    if (d >= minDistSqr) {
      if (found < SPAWNPOINT_NEAR_BUFFER_SIZE) {
        bestPoints[found] = spIdx;
        bestPointDistSqr[found] = d;
        found++;
      } else {
        // replace largest if closer
        int largestIdx = 0;
        float largestIdxSqrDist = bestPointDistSqr[0];
        for (j = 1; j < SPAWNPOINT_NEAR_BUFFER_SIZE; ++j) {
          if (bestPointDistSqr[j] > largestIdxSqrDist) {
            largestIdx = j;
            largestIdxSqrDist = bestPointDistSqr[j];
          }
        }

        if (d < largestIdxSqrDist) {
          bestPoints[largestIdx] = spIdx;
          bestPointDistSqr[largestIdx] = d;
        }
      }
    }
  }

  // pick random from best points
  if (found)
  {
    int pick = rand(found);
    int idx = bestPoints[pick];
    SpawnPoint* sp = spawnPointGet(idx);
    vector_apply(out, cuboidSpaceP, sp->M0);

    // let map decide if spawn point is valid
    if (mapConfig->Functions.ConsiderMobSpawnPointFunc && !mapConfig->Functions.ConsiderMobSpawnPointFunc(mob, out, 0, player))
      return 0;

    //vector_copy(out, (float*)&sp->M0[12]);
    //vector_fromyaw(t, randRadian());
    //vector_scale(t, t, 3);
    //vector_add(out, out, t);
  }
  
  return found;
}

//--------------------------------------------------------------------------
int spawnGetRandomPoint(VECTOR out, struct MobSpawnParams* mob) {
  // harder difficulty, better chance mob spawns near you
  float r = randRange(0, 1) / State.Difficulty;
  float demonBellFactor = 1;
  float spawnDistanceFactor = getSpawnDistanceMultiplier();

  if (State.DemonBellCount > 0) {
    demonBellFactor = lerpf(1, 0.33, powf(State.RoundDemonBellCount / (float)State.DemonBellCount, 2));
  }

#if QUICK_SPAWN
  r = MOB_SPAWN_NEAR_PLAYER_PROBABILITY;
#endif

  // spawn on player
  if (r <= MOB_SPAWN_AT_PLAYER_PROBABILITY && (mob->SpawnType & SPAWN_TYPE_ON_PLAYER)) {
    Player * targetPlayer = playerGetRandom();
    if (targetPlayer) {
      vector_write(out, randVectorRange(-1, 1));
      out[2] = 0;
      vector_add(out, out, targetPlayer->PlayerPosition);
      return 1;
    }
  }

  // spawn near healthbox
  if (r <= MOB_SPAWN_NEAR_HEALTHBOX_PROBABILITY && (mob->SpawnType & SPAWN_TYPE_NEAR_HEALTHBOX)) {
    Moby* hb = mobyGetRandomHealthbox();
    if (hb)
      return spawnPointGetNearestTo(hb->Position, out, 10);
  }

  // spawn near player
  if (r <= MOB_SPAWN_NEAR_PLAYER_PROBABILITY && (mob->SpawnType & SPAWN_TYPE_NEAR_PLAYER)) {
    return spawnPointGetNearToPlayer(mob, out, 30 * demonBellFactor * spawnDistanceFactor);
  }

  // spawn semi near player
  if (r <= MOB_SPAWN_SEMI_NEAR_PLAYER_PROBABILITY && (mob->SpawnType & SPAWN_TYPE_SEMI_NEAR_PLAYER)) {
    return spawnPointGetNearToPlayer(mob, out, 60 * demonBellFactor * spawnDistanceFactor);
  }

  // spawn
  return spawnPointGetNearToPlayer(mob, out, 100 * demonBellFactor * spawnDistanceFactor);
}

//--------------------------------------------------------------------------
int spawnCanSpawnMob(struct MobSpawnParams* mob, int spawnParamIdx)
{
  return (State.RoundNumber + 1) >= mob->MinRound
      && mob->Probability > 0
      && (!mob->SpecialRoundOnly || State.RoundIsSpecial)
      && (mob->MaxSpawnedAtOnce <= 0 || State.MobStats.NumAlive[spawnParamIdx] < mob->MaxSpawnedAtOnce)
      && (mob->MaxSpawnedPerRound <= 0 || State.MobStats.NumSpawnedThisRound[spawnParamIdx] < mob->MaxSpawnedPerRound)
      ;
}

//--------------------------------------------------------------------------
void populateSpawnArgsFromConfig(struct MobSpawnEventArgs* output, struct MobConfig* config, int spawnParamsIdx, int isBaseConfig, int spawnFlags)
{
  GameSettings* gs = gameGetSettings();
  if (!gs)
    return;

  float damage = config->Damage;
  float speed = config->Speed;
  float health = config->Health;
  float difficulty = getCurrentDifficulty();

  // scale config by round
  if (isBaseConfig) {
    //printf("1 %d damage:%f speed:%f health:%f\n", spawnParamsIdx, damage, speed, health);
    //damage = damage * powf(1 + (MOB_BASE_DAMAGE_SCALE * config->DamageScale * DIFFICULTY_FACTOR * State.Difficulty * randRange(0.5, 1.2)), 2);
    //speed = speed * powf(1 + (MOB_BASE_SPEED_SCALE * config->SpeedScale * DIFFICULTY_FACTOR * State.Difficulty * randRange(0.5, 1.2)), 2);
    
    damage = damage * (1 + (MOB_BASE_DAMAGE_SCALE * config->DamageScale * difficulty));
    speed = speed * (1 + (MOB_BASE_SPEED_SCALE * config->SpeedScale * difficulty));
    health = health * powf(1 + (MOB_BASE_HEALTH_SCALE * config->HealthScale * difficulty), 2);
    //printf("2 %d:%f damage:%f speed:%f health:%f\n", spawnParamsIdx, difficulty, damage, speed, health);
  }

  // enforce max values
  if (config->MaxDamage > 0 && damage > config->MaxDamage)
    damage = config->MaxDamage;
  if (config->MaxSpeed > 0 && speed > config->MaxSpeed)
    speed = config->MaxSpeed;
  if (config->MaxHealth > 0 && health > config->MaxHealth)
    health = config->MaxHealth;
  
  //printf("3 %d damage:%f speed:%f health:%f\n", spawnParamsIdx, damage, speed, health);

  output->SpawnParamsIdx = spawnParamsIdx;
  output->Bolts = config->Bolts;
  output->Xp = config->Xp;
  output->StartHealth = health;
  output->Bangles = (u16)config->Bangles;
  output->Damage = (u16)damage;
  output->AttackRadiusEighths = (u8)(config->AttackRadius * 8);
  output->HitRadiusEighths = (u8)(config->HitRadius * 8);
  output->CollRadiusEighths = (u8)(config->CollRadius * 8);
  output->SpeedEighths = (u16)(speed * 8);
  output->ReactionTickCount = (u8)config->ReactionTickCount;
  output->AttackCooldownTickCount = (u16)config->AttackCooldownTickCount;
  output->DamageCooldownTickCount = (u16)config->DamageCooldownTickCount;
  output->MobAttribute = config->MobAttribute;
  output->Behavior = config->Behavior;
}

//--------------------------------------------------------------------------
struct MobSpawnParams* spawnGetRandomMobParams(int * mobIdx)
{
  int i;

  if (!mapConfig->DefaultSpawnParams)
    return NULL;

  if (State.RoundIsSpecial && mapConfig->SpecialRoundParams) {
    struct SurvivalSpecialRoundParam* params = &mapConfig->SpecialRoundParams[State.RoundSpecialIdx];
    for (i = 0; i < params->SpawnParamCount; ++i) {
      int spawnParamIdx = params->SpawnParamIds[i];
      struct MobSpawnParams* mob = &mapConfig->DefaultSpawnParams[spawnParamIdx];
      if (spawnCanSpawnMob(mob, spawnParamIdx) && randRange(0,1) <= mob->Probability) {
        if (mobIdx)
          *mobIdx = spawnParamIdx;
        return mob;
      }
    }

    return NULL;
  }

  for (i = 0; i < mapConfig->DefaultSpawnParamsCount; ++i) {
    struct MobSpawnParams* mob = &mapConfig->DefaultSpawnParams[i];
    if (spawnCanSpawnMob(mob, i) && randRange(0,1) <= mob->Probability) {
      if (mobIdx)
        *mobIdx = i;
      return mob;
    }
  }

  // for (i = 0; i < mapConfig->DefaultSpawnParamsCount; ++i) {
  //   struct MobSpawnParams* mob = &mapConfig->DefaultSpawnParams[i];
  //   DPRINTF("CANT SPAWN round:%d mobIdx:%d mobMinRound:%d mobCost:%d mobProb:%f\n", State.RoundNumber, i, mob->MinRound, mob->Cost, mob->Probability);
  // }
  
  return NULL;
}

//--------------------------------------------------------------------------
int spawnRandomMob(void) {

  // disable spawning during pause
  if (survivalIsPaused())
    return 0;

  VECTOR sp;
  int mobIdx = 0;
  struct MobSpawnParams* mob = spawnGetRandomMobParams(&mobIdx);
  if (mob) {
    int cooldownTicks = mob->CooldownTicks + (mob->CooldownOffsetPerRoundFactor * State.RoundNumber);
    if (cooldownTicks < 0) cooldownTicks = 0;

    // skip if cooldown not 0
    int cooldown = defaultSpawnParamsCooldowns[mobIdx];
    if (cooldown > 0) {
      return 0;
    }
    else if (State.RoundIsSpecial) {
      defaultSpawnParamsCooldowns[mobIdx] = lerpf(2 * TPS, cooldownTicks, mapConfig->SpecialRoundParams[State.RoundSpecialIdx].SpawnRateFactor);
      //DPRINTF("%d => %d\n", mobIdx, defaultSpawnParamsCooldowns[mobIdx]);
    }
    else {
      defaultSpawnParamsCooldowns[mobIdx] = cooldownTicks;
    }

    // try and spawn
    // run it a few times in case we get an unlucky spawn attempt
    int count = 0;
    while (count < 3) {
      if (spawnGetRandomPoint(sp, mob)) {
        if (mobCreate(mobIdx, sp, 0, -1, 0, &mob->Config)) {
          return 1;
        }

        break;
      }
      ++count;
    }
    // if (spawnGetRandomPoint(sp, mob)) {
    //   if (mobCreate(mobIdx, sp, 0, -1, 0, &mob->Config)) {
    //     return 1;
    //   } else { DPRINTF("failed to create mob\n"); }
    // } else { DPRINTF("failed to get random spawn point\n"); }
  } else { DPRINTF("failed to get random mob params\n"); }

  // no more budget left
  return 0;
}

//--------------------------------------------------------------------------
void customBangelizeWeapons(Moby* weaponMoby, int weaponId, int weaponLevel)
{
  switch (weaponId)
  {
    case WEAPON_ID_VIPERS:
    {
      weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? 0 : 1;
      break;
    }
    case WEAPON_ID_MAGMA_CANNON:
    {
      weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 4 : 3) : 0x31;
      break;
    }
    case WEAPON_ID_ARBITER:
    {
      weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 3 : 1) : 0xC;
      break;
    }
    case WEAPON_ID_FUSION_RIFLE:
    {
      weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 2 : 1) : 6;
      break;
    }
    case WEAPON_ID_MINE_LAUNCHER:
    {
      weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 0xC : 0) : 0xF;
      break;
    }
    case WEAPON_ID_B6:
    {
      weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 6 : 4) : 7;
      break;
    }
    case WEAPON_ID_OMNI_SHIELD:
    {
      weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 0xC : 1) : 0xF;
      break;
    }
    case WEAPON_ID_FLAIL:
    {
      if (weaponMoby->PVar) {
        weaponMoby = *(Moby**)((u32)weaponMoby->PVar + 0x33C);
        if (weaponMoby)
          weaponMoby->Bangles = weaponLevel < VENDOR_MAX_WEAPON_LEVEL ? (weaponLevel ? 3 : 1) : 0x1F;
      }
      break;
    }
  }
}

//--------------------------------------------------------------------------
// TODO: Move this into the code segment, overwriting GuiMain_GetGadgetVersionName at (0x00541850)
char * customGetGadgetVersionName(int localPlayerIndex, int weaponId, int showWeaponLevel, int capitalize, int minLevel)
{
  Player* p = playerGetFromSlot(localPlayerIndex);
  char* buf = (char*)(0x2F9D78 + localPlayerIndex*0x40);
  short* gadgetDef = ((short* (*)(int, int))0x00627f48)(weaponId, 0);
  int level = 0;
  int strIdOffset = capitalize ? 3 : 4;
  if (p && p->GadgetBox) {
    level = p->GadgetBox->Gadgets[weaponId].Level;
  }

  if (level >= 9)
    strIdOffset = capitalize ? 12 : 10;

  char* str = uiMsgString(gadgetDef[strIdOffset]);
  int prestige = State.PlayerStates[p->PlayerId].State.WeaponPrestige[weaponIdToSlot(weaponId)];

  if (level < 9 && level >= minLevel) {
    snprintf(buf, 0x40, "%c%s V%d", WEAPON_PRESTIGE_PREFIX[prestige], str, level+1);
    return buf;
  } else {
    snprintf(buf, 0x40, "%c%s", WEAPON_PRESTIGE_PREFIX[prestige], str);
    return buf;
  }
}

//--------------------------------------------------------------------------
void customMineMobyUpdate(Moby* moby)
{
  // handle auto destructing v10 child mines after N seconds
  // and auto destroy v10 child mines if they came from a remote client
  u32 pvar = (u32)moby->PVar;
  if (pvar) {
    int mLayer = *(int*)(pvar + 0x200);
    Player * p = playerGetFromIndex(*(short*)(pvar + 0xC0));
    int createdLocally = p && p->IsLocal;
    int gameTime = gameGetTime();
    int timeCreated = *(int*)(&moby->Rotation[3]);

    // set initial time created
    if (timeCreated == 0) {
      timeCreated = gameTime;
      *(int*)(&moby->Rotation[3]) = timeCreated;
    }

    // don't spawn child mines if mine was created by remote client
    if (!createdLocally) {
      *(int*)(pvar + 0x200) = 2;
    } else if (p && mLayer > 0 && (gameTime - timeCreated) > (5 * TIME_SECOND)) {
      *(int*)(&moby->Rotation[3]) = gameTime;
      ((void (*)(Moby*, Player*, int))0x003C90C0)(moby, p, 0);
      return;
    }
  }
  
  // call base
  ((void (*)(Moby*))0x003C6C28)(moby);
}

//--------------------------------------------------------------------------
void onPlayerWithdrawnBankBox(int playerId, int bolts)
{
  if (!State.Bankbox) return;

  struct BankBoxPVar* pvars = (struct BankBoxPVar*)State.Bankbox->PVar;
  pvars->TotalBolts -= bolts;
  pvars->BoltsWithdrawnThisRound += bolts;
  State.PlayerStates[playerId].State.Bolts += bolts;
  playPaidSound(playerGetFromIndex(playerId));
}

//--------------------------------------------------------------------------
int onPlayerWithdrawnBankBoxRemote(void * connection, void * data)
{
  SurvivalPlayerWithdrawnBankBoxMessage_t * message = (SurvivalPlayerWithdrawnBankBoxMessage_t*)data;
  onPlayerWithdrawnBankBox(message->PlayerId, message->Amount);

  return sizeof(SurvivalPlayerWithdrawnBankBoxMessage_t);
}

//--------------------------------------------------------------------------
void playerWithdrawnBankBox(int playerId, int bolts)
{
  SurvivalPlayerWithdrawnBankBoxMessage_t message;

  // send out
  message.PlayerId = playerId;
  message.Amount = bolts;
  netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_WITHDRAWN_BANK_BOX, sizeof(SurvivalPlayerWithdrawnBankBoxMessage_t), &message);

  // set locally
  onPlayerWithdrawnBankBox(message.PlayerId, message.Amount);
}

//--------------------------------------------------------------------------
void onPlayerInteractBankBox(int playerId, int deposit, int amount)
{
  if (!State.Bankbox) return;

  struct BankBoxPVar* pvars = (struct BankBoxPVar*)State.Bankbox->PVar;
  
  if (deposit) {
    pvars->TotalBolts += amount;
    pvars->BoltsDepositThisRound += amount;
  } else if (gameAmIHost() && pvars->TotalBolts > 0) {
    int withdraw = pvars->TotalBolts;
    if (withdraw > BANK_BOX_AMOUNT)
      withdraw = BANK_BOX_AMOUNT;
    playerWithdrawnBankBox(playerId, withdraw);
  }
}

//--------------------------------------------------------------------------
int onPlayerInteractBankBoxRemote(void * connection, void * data)
{
  SurvivalPlayerInteractBankBoxMessage_t * message = (SurvivalPlayerInteractBankBoxMessage_t*)data;
  onPlayerInteractBankBox(message->PlayerId, message->Deposit, message->Amount);

  return sizeof(SurvivalPlayerInteractBankBoxMessage_t);
}

//--------------------------------------------------------------------------
void playerInteractBankBox(Player* player, int deposit)
{
  SurvivalPlayerInteractBankBoxMessage_t message;

  int amount = BANK_BOX_AMOUNT;
  if (deposit && State.PlayerStates[player->PlayerId].State.Bolts < amount) {
    amount = State.PlayerStates[player->PlayerId].State.Bolts;
  }

  // send out
  message.PlayerId = player->PlayerId;
  message.Deposit = deposit;
  message.Amount = amount;
  if (deposit) State.PlayerStates[player->PlayerId].State.Bolts -= amount;
  netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_INTERACT_BANK_BOX, sizeof(SurvivalPlayerInteractBankBoxMessage_t), &message);

  // set locally
  onPlayerInteractBankBox(message.PlayerId, message.Deposit, message.Amount);
}

//--------------------------------------------------------------------------
void onPlayerPrestigeWeapon(int playerId, int weaponId, int prestigeId)
{
  Player* p = playerGetFromIndex(playerId);
  if (!p)
    return;

  GadgetBox* gBox = p->GadgetBox;
  gBox->Gadgets[weaponId].Level = -1;
  playerGiveWeapon(gBox, weaponId, 0, 1);
  if (p->Gadgets[0].pMoby && p->Gadgets[0].id == weaponId)
    customBangelizeWeapons(p->Gadgets[0].pMoby, weaponId, 0);

  // stats
  int wepSlotId = weaponIdToSlot(weaponId);
  State.PlayerStates[playerId].State.WeaponPrestige[wepSlotId] = prestigeId;

  // play upgrade sound
  playUpgradeSound(p);

#if LOG_STATS2
  DPRINTF("%d (%08X) weapon %d prestiged to %d\n", playerId, (u32)p, weaponId, prestigeId);
#endif
}

//--------------------------------------------------------------------------
int onPlayerPrestigeWeaponRemote(void * connection, void * data)
{
  SurvivalWeaponPrestigeMessage_t * message = (SurvivalWeaponPrestigeMessage_t*)data;
  onPlayerPrestigeWeapon(message->PlayerId, message->WeaponId, message->PrestigeId);

  return sizeof(SurvivalWeaponPrestigeMessage_t);
}

//--------------------------------------------------------------------------
int playerPrestigeWeapon(Player* player, int weaponId)
{
  SurvivalWeaponPrestigeMessage_t message;

  int slotId = weaponIdToSlot(weaponId);
  if (slotId <= 0) return 0;

  int nextPrestige = State.PlayerStates[player->PlayerId].State.WeaponPrestige[slotId] + 1;
  if (!canPrestigePlayerWeapon(player, weaponId, nextPrestige, NULL)) return 0;

  // send out
  message.PlayerId = player->PlayerId;
  message.WeaponId = weaponId;
  message.PrestigeId = nextPrestige;
  netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_WEAPON_PRESTIGE, sizeof(SurvivalWeaponPrestigeMessage_t), &message);

  // set locally
  onPlayerPrestigeWeapon(message.PlayerId, message.WeaponId, message.PrestigeId);

  // some weapons stay in their v10 state until they are re-equipped
  player->ChangeWeaponHeldId = WEAPON_ID_WRENCH;
  return 1;
}

//--------------------------------------------------------------------------
void onPlayerUpgradeWeapon(int playerId, int weaponId, int level)
{
  Player* p = playerGetFromIndex(playerId);
  if (!p)
    return;

  GadgetBox* gBox = p->GadgetBox;
  gBox->Gadgets[weaponId].Level = -1;
  playerGiveWeapon(gBox, weaponId, level, 1);
  if (p->Gadgets[0].pMoby && p->Gadgets[0].id == weaponId)
    customBangelizeWeapons(p->Gadgets[0].pMoby, weaponId, level);

  // stats
  int wepSlotId = weaponIdToSlot(weaponId);
  if (level > State.PlayerStates[playerId].State.BestWeaponLevel[wepSlotId])
    State.PlayerStates[playerId].State.BestWeaponLevel[wepSlotId] = level;

  // play upgrade sound
  playUpgradeSound(p);

  // set exp bar to max
  // moved to map
  // if (level == VENDOR_MAX_WEAPON_LEVEL)
  //   gBox->Gadgets[weaponId].Experience = level;

#if LOG_STATS2
  DPRINTF("%d (%08X) weapon %d upgraded to %d\n", playerId, (u32)p, weaponId, level);
#endif
}

//--------------------------------------------------------------------------
int onPlayerUpgradeWeaponRemote(void * connection, void * data)
{
  SurvivalWeaponUpgradeMessage_t * message = (SurvivalWeaponUpgradeMessage_t*)data;
  onPlayerUpgradeWeapon(message->PlayerId, message->WeaponId, message->Level);

  return sizeof(SurvivalWeaponUpgradeMessage_t);
}

//--------------------------------------------------------------------------
void playerUpgradeWeapon(Player* player, int weaponId)
{
  SurvivalWeaponUpgradeMessage_t message;
  GadgetBox* gBox = player->GadgetBox;

  // send out
  message.PlayerId = player->PlayerId;
  message.WeaponId = weaponId;
  message.Level = gBox->Gadgets[weaponId].Level + 1;
  netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_WEAPON_UPGRADE, sizeof(SurvivalWeaponUpgradeMessage_t), &message);

  // set locally
  onPlayerUpgradeWeapon(message.PlayerId, message.WeaponId, message.Level);
}

//--------------------------------------------------------------------------
void mapUpgradePlayerWeaponHandler(int playerId, int weaponId)
{
  Player* player = playerGetFromIndex(playerId);
  if (!player || !player->PlayerMoby || !player->IsLocal)
    return;
  
  if (!player->GadgetBox || player->GadgetBox->Gadgets[weaponId].Level >= 9)
    return;

  playerUpgradeWeapon(player, weaponId);
}

//--------------------------------------------------------------------------
void onPlayerRevive(int playerId, int fromPlayerId)
{
  int useDeadPos = 0;

  if (fromPlayerId >= 0 && playerId != fromPlayerId)
    State.PlayerStates[fromPlayerId].State.Revives++;
  
  State.PlayerStates[playerId].IsDead = 0;
  State.PlayerStates[playerId].State.TimesRevived++;
  State.PlayerStates[playerId].State.TimesRevivedSinceRoundStart++;
  Player* player = playerGetFromIndex(playerId);
  if (!playerIsValid(player))
    return;

  // backup current position/rotation
  VECTOR deadPos, deadPosDown, deadRot;
  vector_copy(deadPos, player->PlayerPosition);
  vector_copy(deadPosDown, player->PlayerPosition);
  vector_copy(deadRot, player->PlayerRotation);
  deadPos[2] += 0.5;
  deadPosDown[2] -= 1;

  // respawn
  PlayerVTable* vtable = playerGetVTable(player);
  if (vtable)
    vtable->UpdateState(player, PLAYER_STATE_IDLE, 1, 1, 1);
  getResurrectPoint(player, player->PlayerPosition, player->PlayerRotation, 0);
  playerSetPosRot(player, player->PlayerPosition, player->PlayerRotation);
  playerSetHealth(player, player->MaxHealth);

  // only reset back to dead pos if player died on ground
  if (CollLine_Fix(deadPos, deadPosDown, COLLISION_FLAG_IGNORE_DYNAMIC, player->PlayerMoby, NULL)) {
    int colId = CollLine_Fix_GetHitCollisionId() & 0xF;
    if (colId == 0xF || colId == 0x7 || colId == 0x9 || colId == 0xA)
      useDeadPos = 1;
  }

  if (useDeadPos) {
    playerSetPosRot(player, deadPos, deadRot);

    // spawn explosion to push zombies back
    spawnExplosion(deadPos, 5, 0x80008000);
    mobReactToExplosionAt(fromPlayerId, deadPos, 1, 8);
  }

  player->timers.acidTimer = 0;
  player->timers.collOff = 0;
  player->timers.freezeTimer = 1; // triggers the game to handle resetting movement speed on 0
  player->timers.invincibilityTimer = TPS * 3;

  // pass to map
  if (hasMapConfig() && mapConfig->Functions.OnPlayerRevivedFunc)
    mapConfig->Functions.OnPlayerRevivedFunc(player, playerGetFromIndex(fromPlayerId));
}

//--------------------------------------------------------------------------
int onPlayerReviveRemote(void * connection, void * data)
{
  SurvivalReviveMessage_t * message = (SurvivalReviveMessage_t*)data;
  onPlayerRevive(message->PlayerId, message->FromPlayerId);

  return sizeof(SurvivalReviveMessage_t);
}

//--------------------------------------------------------------------------
void playerRevive(Player* player, int fromPlayerId)
{
  SurvivalReviveMessage_t message;

  // send out
  message.PlayerId = player->PlayerId;
  message.FromPlayerId = fromPlayerId;
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_REVIVE_PLAYER, sizeof(SurvivalReviveMessage_t), &message);

  // set locally
  onPlayerRevive(message.PlayerId, message.FromPlayerId);
}

//--------------------------------------------------------------------------
void onSetPlayerDead(int playerId, char isDead)
{
  int totalTicks = PLAYER_BASE_REVIVE_TICKS - (PLAYER_REVIVE_COST_PER_REVIVE_TICKS * State.PlayerStates[playerId].State.TimesRevivedSinceRoundStart);
  if (totalTicks < PLAYER_MIN_REVIVE_TICKS) totalTicks = PLAYER_MIN_REVIVE_TICKS;

  State.PlayerStates[playerId].IsDead = isDead;
  State.PlayerStates[playerId].ReviveCooldownTicks = isDead ? totalTicks : 0;
#if LOG_STATS2
  DPRINTF("%d died (%d)\n", playerId, isDead);
#endif
}

//--------------------------------------------------------------------------
int onSetPlayerDeadRemote(void * connection, void * data)
{
  SurvivalSetPlayerDeadMessage_t * message = (SurvivalSetPlayerDeadMessage_t*)data;
  onSetPlayerDead(message->PlayerId, message->IsDead);

  return sizeof(SurvivalSetPlayerDeadMessage_t);
}

//--------------------------------------------------------------------------
void setPlayerDead(Player* player, char isDead)
{
  SurvivalSetPlayerDeadMessage_t message;

  // send out
  message.PlayerId = player->PlayerId;
  message.IsDead = isDead;
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_DIED, sizeof(SurvivalSetPlayerDeadMessage_t), &message);

  // set locally
  onSetPlayerDead(message.PlayerId, message.IsDead);
}

//--------------------------------------------------------------------------
void onSetPlayerWeaponMods(int playerId, int weaponId, u8* mods)
{
  int i;
  Player* p = playerGetFromIndex(playerId);
  if (!p)
    return;

  GadgetBox* gBox = p->GadgetBox;
  for (i = 0; i < 10; ++i)
    gBox->Gadgets[weaponId].AlphaMods[i] = mods[i];
    
#if LOG_STATS2
  DPRINTF("%d set %d mods\n", playerId, weaponId);
#endif
}

//--------------------------------------------------------------------------
int onSetPlayerWeaponModsRemote(void * connection, void * data)
{
  SurvivalSetWeaponModsMessage_t * message = (SurvivalSetWeaponModsMessage_t*)data;
  onSetPlayerWeaponMods(message->PlayerId, message->WeaponId, message->Mods);

  return sizeof(SurvivalSetPlayerDeadMessage_t);
}

//--------------------------------------------------------------------------
void setPlayerWeaponMods(Player* player, int weaponId, int* mods)
{
  int i;
  SurvivalSetWeaponModsMessage_t message;

  // send out
  message.PlayerId = player->PlayerId;
  message.WeaponId = (u8)weaponId;
  for (i = 0; i < 10; ++i)
    message.Mods[i] = (u8)mods[i];
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_WEAPON_MODS, sizeof(SurvivalSetWeaponModsMessage_t), &message);

  // set locally
  onSetPlayerWeaponMods(message.PlayerId, message.WeaponId, message.Mods);
}

//--------------------------------------------------------------------------
void onSetPlayerStats(int playerId, struct SurvivalPlayerState* stats)
{
  memcpy(&State.PlayerStates[playerId].State, stats, sizeof(struct SurvivalPlayerState));
}

//--------------------------------------------------------------------------
int onSetPlayerStatsRemote(void * connection, void * data)
{
  SurvivalSetPlayerStatsMessage_t * message = (SurvivalSetPlayerStatsMessage_t*)data;
  onSetPlayerStats(message->PlayerId, &message->Stats);

  return sizeof(SurvivalSetPlayerStatsMessage_t);
}

//--------------------------------------------------------------------------
void sendPlayerStats(int playerId)
{
  SurvivalSetPlayerStatsMessage_t message;

  // send out
  message.PlayerId = playerId;
  memcpy(&message.Stats, &State.PlayerStates[playerId].State, sizeof(struct SurvivalPlayerState));
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_STATS, sizeof(SurvivalSetPlayerStatsMessage_t), &message);
}

//--------------------------------------------------------------------------
void onSetPlayerDoublePoints(char isActive[GAME_MAX_PLAYERS], int timeOfDoublePoints[GAME_MAX_PLAYERS])
{
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    State.PlayerStates[i].IsDoublePoints = isActive[i];
    State.PlayerStates[i].TimeOfDoublePoints = timeOfDoublePoints[i];
  }
}

//--------------------------------------------------------------------------
int onSetPlayerDoublePointsRemote(void * connection, void * data)
{
  SurvivalSetPlayerDoublePointsMessage_t * message = (SurvivalSetPlayerDoublePointsMessage_t*)data;
  onSetPlayerDoublePoints(message->IsActive, message->TimeOfDoublePoints);

  return sizeof(SurvivalSetPlayerDoublePointsMessage_t);
}

//--------------------------------------------------------------------------
void sendDoublePoints(void)
{
  int i;
  SurvivalSetPlayerDoublePointsMessage_t message;

  // build active list
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    message.IsActive[i] = State.PlayerStates[i].IsDoublePoints;
    message.TimeOfDoublePoints[i] = State.PlayerStates[i].TimeOfDoublePoints;
  }

  // send out
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_DOUBLE_POINTS, sizeof(SurvivalSetPlayerDoublePointsMessage_t), &message);
}

//--------------------------------------------------------------------------
void setDoublePoints(int isActive)
{
  int i;
  SurvivalSetPlayerDoublePointsMessage_t message;

  // build active list
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* p = playerGetFromIndex(i);
    if (p) {
      State.PlayerStates[i].TimeOfDoublePoints = gameGetTime();
      State.PlayerStates[i].IsDoublePoints = isActive;
    }

    message.IsActive[i] = State.PlayerStates[i].IsDoublePoints;
    message.TimeOfDoublePoints[i] = State.PlayerStates[i].TimeOfDoublePoints;
  }

  // send out
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_DOUBLE_POINTS, sizeof(SurvivalSetPlayerDoublePointsMessage_t), &message);

  // locally
  onSetPlayerDoublePoints(message.IsActive, message.TimeOfDoublePoints);
}

//--------------------------------------------------------------------------
void onSetPlayerDoubleXP(char isActive[GAME_MAX_PLAYERS], int timeOfDoubleXP[GAME_MAX_PLAYERS])
{
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    State.PlayerStates[i].IsDoubleXP = isActive[i];
    State.PlayerStates[i].TimeOfDoubleXP = timeOfDoubleXP[i];
  }
}

//--------------------------------------------------------------------------
int onSetPlayerDoubleXPRemote(void * connection, void * data)
{
  SurvivalSetPlayerDoubleXPMessage_t * message = (SurvivalSetPlayerDoubleXPMessage_t*)data;
  onSetPlayerDoubleXP(message->IsActive, message->TimeOfDoubleXP);

  return sizeof(SurvivalSetPlayerDoubleXPMessage_t);
}

//--------------------------------------------------------------------------
void sendDoubleXP(void)
{
  int i;
  SurvivalSetPlayerDoubleXPMessage_t message;

  // build active list
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    message.IsActive[i] = State.PlayerStates[i].IsDoubleXP;
    message.TimeOfDoubleXP[i] = State.PlayerStates[i].TimeOfDoubleXP;
  }

  // send out
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_DOUBLE_XP, sizeof(SurvivalSetPlayerDoubleXPMessage_t), &message);
}

//--------------------------------------------------------------------------
void setDoubleXP(int isActive)
{
  int i;
  SurvivalSetPlayerDoubleXPMessage_t message;

  // build active list
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* p = playerGetFromIndex(i);
    if (p) {
      State.PlayerStates[i].TimeOfDoubleXP = gameGetTime();
      State.PlayerStates[i].IsDoubleXP = isActive;
    }

    message.IsActive[i] = State.PlayerStates[i].IsDoubleXP;
    message.TimeOfDoubleXP[i] = State.PlayerStates[i].TimeOfDoubleXP;
  }

  // send out
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_DOUBLE_XP, sizeof(SurvivalSetPlayerDoubleXPMessage_t), &message);

  // locally
  onSetPlayerDoubleXP(message.IsActive, message.TimeOfDoubleXP);
}

//--------------------------------------------------------------------------
void onSetFreeze(char isActive)
{
  State.Freeze = isActive;
  if (isActive)
    State.TimeOfFreeze = gameGetTime();
}

//--------------------------------------------------------------------------
int onSetFreezeRemote(void * connection, void * data)
{
  SurvivalSetFreezeMessage_t * message = (SurvivalSetFreezeMessage_t*)data;
  onSetFreeze(message->IsActive);

  return sizeof(SurvivalSetFreezeMessage_t);
}

//--------------------------------------------------------------------------
void setFreeze(int isActive)
{
  SurvivalSetFreezeMessage_t message;

  // send out
  message.IsActive = isActive;
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_SET_FREEZE, sizeof(SurvivalSetFreezeMessage_t), &message);

  // locally
  onSetFreeze(isActive);
}

//--------------------------------------------------------------------------
void onPlayerCastNextRoundVote(int clientId, int value)
{
  if (!State.VoteForNextRound.IsActive)
    return;

  voteCast(&State.VoteForNextRound, clientId, value);
}

//--------------------------------------------------------------------------
int onPlayerCastVoteRemote(void * connection, void * data)
{
  SurvivalPlayerCastVote_t message;

  memcpy(&message, data, sizeof(SurvivalPlayerCastVote_t));
  switch (message.Ballot)
  {
    case 1: onPlayerCastNextRoundVote(message.ClientId, message.Value); break;
    default: DPRINTF("recv vote from %d for invalid ballot %d\n", message.ClientId, message.Ballot); break;
  }

  return sizeof(SurvivalPlayerCastVote_t);
}

//--------------------------------------------------------------------------
void playerCastNextRoundVote(void)
{
  SurvivalPlayerCastVote_t message;

  // send out
  message.Ballot = 1;
  message.ClientId = gameGetMyClientId();
  message.Value = 1;
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_CAST_VOTE, sizeof(message), &message);

  // locally
  onPlayerCastNextRoundVote(message.ClientId, message.Value);
}

//--------------------------------------------------------------------------
void onPlayerItemAcquired(int playerId, int itemId, int currentCount)
{
  DPRINTF("onPlayerItemAcquired(%d, %d, %d)\n", playerId, itemId, currentCount);
  State.PlayerStates[playerId].State.ItemCounts[itemId] = currentCount;

  // trigger acquire
  Player *player = playerGetFromIndex(playerId);
  if (!playerIsValid(player))
    return;

  passPlayerOnItemAcquiredToMap(player, itemId);
}

//--------------------------------------------------------------------------
int onPlayerItemAcquiredRemote(void * connection, void * data)
{
  SurvivalPlayerItemAcquireMessage_t message;

  memcpy(&message, data, sizeof(SurvivalPlayerItemAcquireMessage_t));
  onPlayerItemAcquired(message.PlayerId, message.ItemId, message.CurrentCount);
  return sizeof(SurvivalPlayerItemAcquireMessage_t);
}

//--------------------------------------------------------------------------
void sendPlayerItemAcquired(int playerId, int itemId)
{
  DPRINTF("sendPlayerItemAcquired(%d, %d)\n", playerId, itemId);
  SurvivalPlayerItemAcquireMessage_t message;

  // send out
  message.PlayerId = playerId;
  message.ItemId = itemId;
  message.CurrentCount = State.PlayerStates[playerId].State.ItemCounts[itemId];
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_ITEM_ACQUIRE, sizeof(message), &message);

  // locally
  onPlayerItemAcquired(message.PlayerId, message.ItemId, message.CurrentCount);
}

//--------------------------------------------------------------------------
void onPlayerItemConsumed(int playerId, int itemId, int currentCount)
{
  DPRINTF("onPlayerItemConsumed(%d, %d, %d)\n", playerId, itemId, currentCount);
  State.PlayerStates[playerId].State.ItemCounts[itemId] = currentCount;

  // trigger consume
  Player *player = playerGetFromIndex(playerId);
  if (!playerIsValid(player))
    return;

  passPlayerOnItemConsumedToMap(player, itemId);
}

//--------------------------------------------------------------------------
int onPlayerItemConsumedRemote(void * connection, void * data)
{
  SurvivalPlayerItemConsumeMessage_t message;

  memcpy(&message, data, sizeof(SurvivalPlayerItemConsumeMessage_t));
  onPlayerItemConsumed(message.PlayerId, message.ItemId, message.CurrentCount);
  return sizeof(SurvivalPlayerItemConsumeMessage_t);
}

//--------------------------------------------------------------------------
void sendPlayerItemConsumed(int playerId, int itemId)
{
  DPRINTF("sendPlayerItemConsumed(%d, %d)\n", playerId, itemId);
  SurvivalPlayerItemConsumeMessage_t message;

  // send out
  message.PlayerId = playerId;
  message.ItemId = itemId;
  message.CurrentCount = State.PlayerStates[playerId].State.ItemCounts[itemId];
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_PLAYER_ITEM_CONSUME, sizeof(message), &message);

  // locally
  onPlayerItemConsumed(message.PlayerId, message.ItemId, message.CurrentCount);
}

//--------------------------------------------------------------------------
void onSetRound50Time(int time)
{
  char buffer[64];
  
  snprintf(buffer, sizeof(buffer), "50 Rounds Completed %02d:%02d.%03d", time / TIME_MINUTE, (time % TIME_MINUTE) / TIME_SECOND, time % TIME_SECOND);
  pushSnack(buffer, TPS * 10, 0);
  State.Round50Time = time;
}

//--------------------------------------------------------------------------
int onSetRound50TimeRemote(void * connection, void * data)
{
  int time;
  memcpy(&time, data, sizeof(time));

  onSetRound50Time(time);
  return sizeof(time);
}

//--------------------------------------------------------------------------
void setRound50Time(int time)
{
  if (time <= 0) return;

  // send and set locally
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), -1, CUSTOM_MSG_SET_ROUND_50_TIME, sizeof(time), &time);
  onSetRound50Time(time);
}

//--------------------------------------------------------------------------
void checkForRound50Time(void)
{
  // update time took to beat 50 rounds
  if (State.RoundNumber == 50 && State.Round50Time == 0 && gameAmIHost()) {
    setRound50Time(gameGetTime() - State.InitializedTime);
  }
}

//--------------------------------------------------------------------------
void respawnDeadPlayers(void) {
  int i;

  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player * p = playerGetFromIndex(i);
    if (p && playerIsDead(p)) {
      State.PlayerStates[i].State.TimesRevivedSinceRoundStart = 0;
      
      // if player still has time left on their revive
      // just auto revive them
      if (State.PlayerStates[i].ReviveCooldownTicks) {
        playerRevive(p, -1);
      } else if (p->IsLocal) {
        playerRespawn(p);
      }
    }
    
    State.PlayerStates[i].IsDead = 0;
    State.PlayerStates[i].ReviveCooldownTicks = 0;
    //memset(State.PlayerStates[i].State.WeaponPrestige, 0, sizeof(State.PlayerStates[i].State.WeaponPrestige));
  }
}

//--------------------------------------------------------------------------
void setPlayerQuadCooldownTimer(Player * player) {
  player->timers.damageMuliplierTimer = 1200;
  player->DamageMultiplier = 4;
}

//--------------------------------------------------------------------------
void setPlayerShieldCooldownTimer(void) {
  
  Player* player = NULL;

  // pointer to player is in $s1
  asm volatile (
    ".set noreorder;"
    "move %0, $s1"
    : : "r" (player)
  );

  player->timers.armorLevelTimer = 1800;
  POKE_U32((u32)player + 0x2FB4, 3);
}

//--------------------------------------------------------------------------
void onV10MagDamageMoby(Moby* target, MobyColDamageIn* in)
{
  if (in->Damager) {
    VECTOR dt;
    vector_subtract(dt, target->Position, in->Damager->Position);
    float dist = vector_length(dt);
    float min = 0.8;
    //Player* damager = guberMobyGetPlayerDamager(in->Damager);
    //if (damager) min += 0.05 * playerGetWeaponAlphaModCount(damager->GadgetBox, WEAPON_ID_MAGMA_CANNON, ALPHA_MOD_AREA);

    float falloff = minf(1, maxf(min, minf(1, 1 - (dist / 32))));
    // float origDmg = in->DamageHp;
    in->DamageHp *= falloff;
  }

  mobyCollDamageDirect(target, in);
}

//--------------------------------------------------------------------------
void onEmpExplode(Moby* moby, VECTOR from, VECTOR to, u32 a3, u32 t0, u32 t1, u32 t2, u32 t3, float f12, float f13, float f14, float f15) {
  
  // int i;
  // Player** players = playerGetAll();
  // VECTOR dt;
  // Player* fromPlayer = guberMobyGetPlayerDamager(moby);

  // if (fromPlayer) {
  //   for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
  //     Player* player = players[i];
  //     if (!player || !player->SkinMoby) continue;

  //     vector_subtract(dt, player->PlayerPosition, to);
  //     if (vector_sqrmag(dt) < (ITEM_EMP_HEALTH_EFFECT_RADIUS*ITEM_EMP_HEALTH_EFFECT_RADIUS)) {
  //       if (playerIsDead(player)) {
  //         playerRevive(player, fromPlayer->PlayerId);
  //       } else {
  //         playerSetHealth(player, player->MaxHealth);
  //       }
  //     }
  //   }
  // }
  
  ((void (*)(Moby* moby, VECTOR from, VECTOR to, u32 a3, u32 t0, u32 t1, u32 t2, u32 t3, float f12, float f13, float f14, float f15))0x00420ac0)(
    moby, from, to, a3, t0, t1, t2, t3,
    f12, f13, f14, f15
  );
}

//--------------------------------------------------------------------------
int onEmpHitMoby(Moby* moby) {
  Player* player = guberMobyGetPlayerDamager(moby);
  return player != 0;
}

//--------------------------------------------------------------------------
void playerRewardXp(int playerId, int weaponId, int xp)
{
  Player* player = playerGetFromIndex(playerId);
  if (!player || !player->PlayerMoby) return;

  struct SurvivalPlayer* pState = &State.PlayerStates[playerId];
  float xpMultiplier = getXpMultiplier();

  // give xp
  pState->State.XP += xp * (pState->IsDoubleXP ? 2 : 1);
  u64 targetForNextToken = getXpForNextToken(player, pState->State.TotalTokens);

  // handle weapon xp
  if (weaponId > 1) {
    int xpCount = playerGetWeaponAlphaModCount(player->GadgetBox, weaponId, ALPHA_MOD_XP);
    pState->State.XP += xpCount * XP_ALPHAMOD_XP * xpMultiplier;
  }

  // give tokens
  while (pState->State.XP >= targetForNextToken)
  {
    pState->State.XP -= targetForNextToken;
    pState->State.TotalTokens += 1;
    pState->State.CurrentTokens += 1;
    targetForNextToken = getXpForNextToken(player, pState->State.TotalTokens);

    if (player->IsLocal) {
      sendPlayerStats(playerId);
    }
  }
}

//--------------------------------------------------------------------------
void onV10VipersHitSurface(Moby* moby)
{
  ((void (*)(Moby*))0x003C05D8)(moby);

  // explosion
  ((void (*)(float damage, float radius, VECTOR p, u32 damageFlags, Moby* moby, Moby* hitMoby))0x003c3a48)(1, 0.5, moby->Position, 0x801, moby, NULL);
}

//--------------------------------------------------------------------------
int playerIsSmashingFlail(Player* player)
{
  if (!player) return 0;

  // jump smash
  if (player->PlayerState == PLAYER_STATE_JUMP_ATTACK && player->WeaponHeldId == WEAPON_ID_FLAIL) {
    return 1;
  }
  
  // ground smash
  if (player->PlayerState == PLAYER_STATE_FLAIL_ATTACK && player->PlayerMoby && player->PlayerMoby->AnimSeqId == 0x33) {
    return 1;
  }
  
  return 0;
}

//--------------------------------------------------------------------------
void processPlayer(int pIndex) {
  VECTOR t;
  int i = 0, localPlayerIndex, heldWeapon, hasMessage = 0;
  char strBuf[32];
  Player* player = playerGetFromIndex(pIndex);
  struct SurvivalPlayer * playerData = &State.PlayerStates[pIndex];
  GameSettings* gs = gameGetSettings();

  if (!player || !player->PlayerMoby)
    return;

  int isDeadState = playerIsDead(player) || player->Health == 0;
  int actionCooldownTicks = decTimerU8(&playerData->ActionCooldownTicks);
  int messageCooldownTicks = decTimerU8(&playerData->MessageCooldownTicks);
  int reviveCooldownTicks = decTimerU16(&playerData->ReviveCooldownTicks);
  if (playerData->IsDead && reviveCooldownTicks > 0 && playerGetNumLocals() == 1) {
    int x,y;
    VECTOR pos = {0,0,1,0};
    vector_add(pos, player->PlayerPosition, pos);
    if (gfxWorldSpaceToScreenSpace(pos, &x, &y)) {
      snprintf(strBuf, sizeof(strBuf), "%02d", reviveCooldownTicks/60);
      gfxScreenSpaceText(x, y, 0.75, 0.75, 0x80FFFFFF, strBuf, -1, 4);
      dzoDrawReviveMsg(pIndex, pos, reviveCooldownTicks/60);
    }
  }

  // last hit
  if (player->Health != playerData->LastHealth) playerData->TicksSinceHealthChanged = 0;
  else playerData->TicksSinceHealthChanged += 1;
  playerData->LastHealth = player->Health;

  // set base max health
  player->MaxHealth = 50;

  // set base speed
  player->Speed = 1;

  // pass to map for final adjustments
  passPlayerUpdateToMap(player);

  // adjust speed of chargeboot stun
  if (player->PlayerState == PLAYER_STATE_JUMP_BOUNCE) {
    *(float*)((u32)player + 0x25C4) = player->Speed;
  } else {
    *(float*)((u32)player + 0x25C4) = 1.0;
  }
  
  // update state timers
  if (playerStates[pIndex] != player->PlayerState)
    playerStateTimers[pIndex] = 0;
  else
    playerStateTimers[pIndex] += 1;
  playerStates[pIndex] = player->PlayerState;

  if (player->IsLocal) {
    
    GadgetBox* gBox = player->GadgetBox;
    localPlayerIndex = player->LocalPlayerIndex;
    heldWeapon = player->WeaponHeldId;

    // set max xp
    u32 xp = playerData->State.XP;
    u32 nextXp = getXpForNextToken(player, playerData->State.TotalTokens);
    if (playerGetNumLocals() > 1) setPlayerWeaponsMenu(localPlayerIndex);
    setPlayerEXP(localPlayerIndex, xp / (float)nextXp);

    // find closest mob
    playerData->MinSqrDistFromMob = 1000000;
    playerData->MaxSqrDistFromMob = 0;
    GuberMoby* gm = guberMobyGetFirst();
    
    while (gm)
    {
      if (gm->Moby && mobyIsMob(gm->Moby))
      {
        vector_subtract(t, player->PlayerPosition, gm->Moby->Position);
        float sqrDist = vector_sqrmag(t);
        if (sqrDist < playerData->MinSqrDistFromMob)
          playerData->MinSqrDistFromMob = sqrDist;
        if (sqrDist > playerData->MaxSqrDistFromMob)
          playerData->MaxSqrDistFromMob = sqrDist;
      }
      
      gm = (GuberMoby*)gm->Guber.Prev;
    }

    // handle death
    if (isDeadState && !playerData->IsDead) {

      // increment DeathsByMob if we were killed by a mob
      // if (player->PlayerMoby->CollDamage >= 0) {
      //   MobyColDamage* damage = mobyGetDamage(player->PlayerMoby, 0xFFFFFF, 1);
      //   if (damage && mobyIsMob(damage->Damager)) {
      //     struct MobPVar* pvars = (struct MobPVar*)damage->Damager->PVar;
      //     if (pvars) {
      //       playerData->State.DeathsByMob[pvars->MobVars.SpawnParamsIdx] += 1;
      //     }
      //   }
      // }

      // pass to map
      passPlayerDiedToMap(player);

      // delay sending dead to give time for self revive
      // and to let the player die before ending the game
      ++playerData->PlayerDeadForTicks;
      if (playerData->PlayerDeadForTicks > TPS) {
        playerData->PlayerDeadForTicks = 0;
        setPlayerDead(player, 1);
      }
    } else if (playerData->IsDead && !isDeadState) {
      setPlayerDead(player, 0);
    }

    // force to last good position
    // if (isDeadState && player->timers.state > TPS) {
    // 	vector_subtract(t, player->PlayerPosition, player->Ground.lastGoodPos);
    // 	if (vector_sqrmag(t) > 1) {
    // 		playerSetPosRot(player, player->Ground.lastGoodPos, player->PlayerRotation);
    // 		player->Health = 0;
    // 		player->PlayerState = PLAYER_STATE_DEATH;
    // 	}
    // }

    // set experience to min of level and max level 
    for (i = WEAPON_ID_VIPERS; i <= WEAPON_ID_FLAIL; ++i) {
      // moved to map
      // if (gBox->Gadgets[i].Level >= 0) {
      //   gBox->Gadgets[i].Experience = gBox->Gadgets[i].Level < VENDOR_MAX_WEAPON_LEVEL ? gBox->Gadgets[i].Level : VENDOR_MAX_WEAPON_LEVEL;
      // }

      // embed prestige in weapon "modActiveWeapon" mod entry
      // only lower 8 bits are used by game
      int slot = weaponIdToSlot(i);
      if (slot > 0) {
        gBox->Gadgets[i].UNK_10 = (gBox->Gadgets[i].UNK_10 & 0xFF) | (playerData->State.WeaponPrestige[slot] << 8);
      }
    }

    // decrement flail ammo while spinning flail
    GameOptions* gameOptions = gameGetOptions();
    if (!gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo
      && player->PlayerState == PLAYER_STATE_FLAIL_ATTACK
      && player->timers.state >= 60) {
        if (gBox->Gadgets[WEAPON_ID_FLAIL].Ammo > 0) {
          if ((player->timers.state % 60) == 0) {
            ((void (*)(GadgetBox*, int, int))0x00627180)(gBox, WEAPON_ID_FLAIL, 1);
            //gBox->Gadgets[WEAPON_ID_FLAIL].Ammo -= 1;
          }
        } else {
          PlayerVTable* vtable = playerGetVTable(player);
          if (vtable && vtable->UpdateState)
            vtable->UpdateState(player, PLAYER_STATE_IDLE, 1, 1, 1);
        }
    }

    //
    if (actionCooldownTicks > 0 || player->timers.noInput) {
      if (messageCooldownTicks == 1) {
        hudHidePopup();
      }
      return;
    }

    // handle closing weapons menu
    if (playerData->IsInWeaponsMenu) {
      for (i = 0; i < sizeof(UPGRADEABLE_WEAPONS)/sizeof(u8); ++i)
        setPlayerWeaponMods(player, UPGRADEABLE_WEAPONS[i], gBox->Gadgets[UPGRADEABLE_WEAPONS[i]].AlphaMods);
      playerData->IsInWeaponsMenu = 0;
    }

    // handle big al logic
    if (State.BigAl && State.BigAl->Drawn && State.BigAl->DrawDist > 0) {

      // check distance
      vector_subtract(t, player->PlayerPosition, State.BigAl->Position);
      if (vector_sqrmag(t) < (BIG_AL_MAX_DIST * BIG_AL_MAX_DIST) && vector_innerproduct(t, player->CameraForward) < -0.9) {
        
        // draw help popup
        uiShowPopup(localPlayerIndex, SURVIVAL_OPEN_WEAPONS_MESSAGE);
        hasMessage = 1;
        playerData->MessageCooldownTicks = 2;

        if (padGetButtonDown(localPlayerIndex, PAD_TRIANGLE) > 0) {
          openWeaponsMenu(localPlayerIndex);
          playerData->ActionCooldownTicks = WEAPON_MENU_COOLDOWN_TICKS;
          playerData->IsInWeaponsMenu = 1;
        }
      }
    }

    // handle bank logic
    if (State.Bankbox && State.Bankbox->Drawn && State.Bankbox->DrawDist > 0) {

      struct BankBoxPVar* bboxPvars = (struct BankBoxPVar*)State.Bankbox->PVar;

      // check distance
      vector_subtract(t, player->PlayerPosition, State.Bankbox->Position);
      if (vector_sqrmag(t) < (BANK_BOX_MAX_DIST * BANK_BOX_MAX_DIST) /* && vector_innerproduct(t, player->CameraForward) < -0.6 */) {
        
        // draw help popup
        char buf[32];
        char costBuf[32];
        uiPrintCommaNumber(costBuf, sizeof(costBuf), bboxPvars->TotalBolts, 0);
        snprintf(buf, sizeof(buf), SURVIVAL_INTERACT_BANK_BALANCE_MESSAGE, costBuf);
        snprintf(LocalPlayerStrBuffer[localPlayerIndex], sizeof(LocalPlayerStrBuffer[localPlayerIndex]), "%s %s", SURVIVAL_INTERACT_BANK_INTERACT_MESSAGE, buf);
        uiShowPopup(localPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
        hasMessage = 1;
        playerData->MessageCooldownTicks = 2;

        if (padGetButton(localPlayerIndex, PAD_SQUARE) > 0 && bboxPvars->TotalBolts > 0) {
          playerInteractBankBox(player, 0);
          playerData->ActionCooldownTicks = PLAYER_BANK_BOX_COOLDOWN_TICKS;
        } else if (padGetButton(localPlayerIndex, PAD_CIRCLE) > 0 && playerData->State.Bolts > 0) {
          playerInteractBankBox(player, 1);
          playPaidSound(player);
          playerData->ActionCooldownTicks = PLAYER_BANK_BOX_COOLDOWN_TICKS;
        }
      }
    }

    // handle prestige logic
    if (State.PrestigeMachine && State.PrestigeMachine->Drawn && State.PrestigeMachine->DrawDist > 0) {

      // check distance
      vector_subtract(t, player->PlayerPosition, State.PrestigeMachine->Position);
      if (vector_sqrmag(t) < (PRESTIGE_MACHINE_MAX_DIST * PRESTIGE_MACHINE_MAX_DIST)) {
        
        int weaponId = player->WeaponHeldId;
        int slotId = weaponIdToSlot(weaponId);
        if (slotId > 0) {

          int nextPrestige = State.PlayerStates[player->PlayerId].State.WeaponPrestige[slotId] + 1;
          char* errMsg = "Cannot prestige weapon"; // generic error message if function doesn't provide one
          u32 cost = getPrestigePlayerWeaponCost(player, weaponId, nextPrestige);
          int canPrestige = canPrestigePlayerWeapon(player, weaponId, nextPrestige, &errMsg);
          char costBuf[32];
          uiPrintCommaNumber(costBuf, sizeof(costBuf), cost, 0);

          // draw help popup
          if (!canPrestige)
            snprintf(LocalPlayerStrBuffer[localPlayerIndex], sizeof(LocalPlayerStrBuffer[localPlayerIndex]), errMsg);
          else
            snprintf(LocalPlayerStrBuffer[localPlayerIndex], sizeof(LocalPlayerStrBuffer[localPlayerIndex]), SURVIVAL_PRESTIGE_WEAPON_MESSAGE, costBuf);

          uiShowPopup(localPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
          hasMessage = 1;
          playerData->MessageCooldownTicks = 2;

          if (canPrestige && padGetButtonDown(localPlayerIndex, PAD_CIRCLE) > 0 && playerData->State.Bolts >= cost) {
            if (playerPrestigeWeapon(player, weaponId)) {
              playerData->State.Bolts -= cost;
              playerData->ActionCooldownTicks = 10;
              playPaidSound(player);
              pushSnack("Your weapon seems more powerful...", 60, localPlayerIndex);
            }
          }
        }
      }
    }

    // handle revive logic
    int revivingPlayerId = -1;
    if (!isDeadState) {
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        if (i != pIndex) {

          // ensure player exists, is dead, and is on the same team
          struct SurvivalPlayer * otherPlayerData = &State.PlayerStates[i];
          Player * otherPlayer = playerGetFromIndex(i);
          if (!otherPlayer)
            continue;

          if (otherPlayer && otherPlayerData->IsDead && (otherPlayerData->ReviveCooldownTicks > 0 || (playerData->RevivingPlayerTicks > 0 && playerData->RevivingPlayerId == i))) {

            // check distance
            vector_subtract(t, player->PlayerPosition, otherPlayer->PlayerPosition);
            if (vector_sqrmag(t) < (PLAYER_REVIVE_MAX_DIST * PLAYER_REVIVE_MAX_DIST)) {

              // draw revive player popup
              // or draw how 
              if (playerData->RevivingPlayerId == -1) {
                snprintf(LocalPlayerStrBuffer[localPlayerIndex], sizeof(LocalPlayerStrBuffer[localPlayerIndex]), SURVIVAL_REVIVE_MESSAGE, gs->PlayerNames[i]);
                uiShowPopup(localPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
                hasMessage = 1;
                playerData->MessageCooldownTicks = 2;
              } else if (playerData->RevivingPlayerId == i) {
                float time = (PLAYER_TIME_TO_REVIVE_TICKS - playerData->RevivingPlayerTicks) / (float)TPS;
                snprintf(LocalPlayerStrBuffer[localPlayerIndex], sizeof(LocalPlayerStrBuffer[localPlayerIndex]), "Reviving %s\x08... %.2f", gs->PlayerNames[i], time);
                gfxHelperDrawText(SCREEN_WIDTH/2, SCREEN_HEIGHT-100, 0, 0, 1, 0x80FFFFFF, LocalPlayerStrBuffer[localPlayerIndex], -1, TEXT_ALIGN_TOPCENTER, COMMON_DZO_DRAW_NORMAL);
                //uiShowPopup(localPlayerIndex, LocalPlayerStrBuffer[localPlayerIndex]);
                //hasMessage = 1;
                //playerData->MessageCooldownTicks = 2;
              }

              // check for interaction pad
              // revision: have the player automatically revive by standing on top
              if (1 /*padGetButton(localPlayerIndex, PAD_DOWN) > 0*/) {

                // check that we are reviving this player or no one yet
                if (playerData->RevivingPlayerId == i || playerData->RevivingPlayerId == -1) {
                  //playerData->ActionCooldownTicks = PLAYER_REVIVE_COOLDOWN_TICKS;

                  // count ticks
                  if (playerData->RevivingPlayerId != i) playerData->RevivingPlayerTicks = 0;
                  playerData->RevivingPlayerTicks++;
                  revivingPlayerId = i;

                  // revive after n ticks have passed
                  if (playerData->RevivingPlayerTicks > PLAYER_TIME_TO_REVIVE_TICKS) {
                    playerRevive(otherPlayer, player->PlayerId);
                    playPaidSound(player);
                    playerData->RevivingPlayerTicks = 0;
                    revivingPlayerId = -1;
                  }

                  break;
                }
              }
            }
          }
        }
      }
    }

    // 
    playerData->RevivingPlayerId = revivingPlayerId;

    /*
    static int aaa = 1;
    static int handle = 0;
    if (padGetButtonDown(0, PAD_RIGHT) > 0) {
      aaa += 1;
      TestSoundDef.Index = aaa;
      if (handle)
        soundKillByHandle(handle);
      int id = soundPlay(&TestSoundDef, 0, playerGetFromSlot(0)->PlayerMoby, 0, 0x400);
      if (id >= 0)
        handle = soundCreateHandle(id);
      else
        handle = 0;
      DPRINTF("%d\n", aaa);
    }
    else if (padGetButtonDown(0, PAD_LEFT) > 0) {
      aaa -= 1;
      TestSoundDef.Index = aaa;
      if (handle)
        soundKillByHandle(handle);
      int id = soundPlay(&TestSoundDef, 0, playerGetFromSlot(0)->PlayerMoby, 0, 0x400);
      if (id >= 0)
        handle = soundCreateHandle(id);
      else
        handle = 0;
      DPRINTF("%d\n", aaa);
    }
    */

    if (!hasMessage && messageCooldownTicks == 1) {
      hudHidePopup();
    }
  } else {

    // bug where players on GREEN or higher will remain cranking bolt after finishing
    // so we'll check to see if their remote player state is no longer cranking
    // and we'll stop them
    int remoteState = *(int*)((u32)player + 0x3a80);
    int playerStateTimer = playerStateTimers[pIndex];
    if (player->PlayerState == PLAYER_STATE_BOLT_CRANK
     && remoteState != PLAYER_STATE_BOLT_CRANK
     && playerStateTimer > TPS*3) {

      PlayerVTable* vtable = playerGetVTable(player);
      vtable->UpdateState(player, remoteState, 1, 1, 1);
    }
  }

  // finally update speed to factor freeze
  if (player->timers.freezeTimer)
    player->Speed *= 0.65;
}

//--------------------------------------------------------------------------
int getRoundBonus(int roundNumber, int numPlayers)
{
  float boltMultiplier = getBoltMultiplier();
  float multiplier = State.RoundIsSpecial ? ROUND_SPECIAL_BONUS_MULTIPLIER : 1;
  int bonus = State.RoundNumber * ROUND_BASE_BOLT_BONUS * numPlayers;
  if (bonus > ROUND_MAX_BOLT_BONUS)
    return ROUND_MAX_BOLT_BONUS * multiplier * boltMultiplier;

  // give a round bonus for using the demon bells
  if (State.DemonBellCount > 0 && State.RoundDemonBellCount > 0) {
    multiplier += State.RoundDemonBellCount / (float)State.DemonBellCount;
  }

  return bonus * multiplier * boltMultiplier;
}

//--------------------------------------------------------------------------
void onRoundBegin(void)
{
  resetRoundState();
}

//--------------------------------------------------------------------------
int onRoundBeginRemote(void * connection, void * data)
{
  SurvivalRoundBeginMessage_t message;
  memcpy(&message, data, sizeof(message));
  onRoundBegin();

  return sizeof(message);
}

//--------------------------------------------------------------------------
void sendRoundBegin(void)
{
  SurvivalRoundBeginMessage_t message;

  // if round has already begun, ignore request
  if (!State.RoundCompleteTime)
    return;

  // don't allow beginning unless host
  if (!State.IsHost)
    return;

  // send out
  netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_ROUND_BEGIN, sizeof(message), &message);

  // set locally
  onRoundBegin();
}

//--------------------------------------------------------------------------
void onSetRoundComplete(int gameTime, int boltBonus)
{
  int i;
#if LOG_STATS2
  DPRINTF("round complete. zombies spawned %d/%d\n", State.MobStats.TotalSpawnedThisRound, State.RoundMaxMobCount);
#endif

  // 
  State.RoundEndTime = 0;
  State.RoundCompleteTime = gameTime;

  // begin vote
  voteBegin(&State.VoteForNextRound);

  // add bonus
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (!State.PlayerStates[i].IsDead) {
      State.PlayerStates[i].State.Bolts += boltBonus;
      State.PlayerStates[i].State.TotalBolts += boltBonus;
    }
  }

  // vest
  if (State.Bankbox) {
    struct BankBoxPVar* pvars = (struct BankBoxPVar*)State.Bankbox->PVar;
    int vestable = pvars->BoltsAtStartOfRound - pvars->BoltsWithdrawnThisRound;
    if (vestable < 0) vestable = 0;

#if LOG_STATS2
    DPRINTF("bank vested %d (- %d)=>%d (total %d=>%d)", pvars->BoltsAtStartOfRound, pvars->BoltsWithdrawnThisRound, (int)(vestable * (1 + BANK_VEST_FACTOR)), pvars->TotalBolts, pvars->TotalBolts + (int)(vestable * BANK_VEST_FACTOR));
#endif

    int vested = vestable * BANK_VEST_FACTOR;
    if (vested > BANK_MAX_VEST) vested = BANK_MAX_VEST;

    int totalBolts = pvars->TotalBolts + vested;
    if (totalBolts > BANK_MAX_BOLTS) totalBolts = BANK_MAX_BOLTS;
    pvars->TotalBolts = totalBolts;
  }

  respawnDeadPlayers();
}

//--------------------------------------------------------------------------
int onSetRoundCompleteRemote(void * connection, void * data)
{
  SurvivalRoundCompleteMessage_t * message = (SurvivalRoundCompleteMessage_t*)data;
  onSetRoundComplete(message->GameTime, message->BoltBonus);

  return sizeof(SurvivalRoundCompleteMessage_t);
}

//--------------------------------------------------------------------------
void setRoundComplete(void)
{
  SurvivalRoundCompleteMessage_t message;

  // don't allow overwriting existing outcome
  if (State.RoundCompleteTime)
    return;

  // don't allow changing outcome when not host
  if (!State.IsHost)
    return;

  // kill any remaining mobs
  mobNuke(-1);

  // send out
  GameSettings* gameSettings = gameGetSettings();
  message.GameTime = gameGetTime();
  message.BoltBonus = getRoundBonus(State.RoundNumber, gameSettings->PlayerCount);
  netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_ROUND_COMPLETE, sizeof(SurvivalRoundCompleteMessage_t), &message);

  // set locally
  onSetRoundComplete(message.GameTime, message.BoltBonus);
}

//--------------------------------------------------------------------------
void onSetRoundStart(int roundNumber, int gameTime)
{
  int i;

  // 
  State.RoundNumber = roundNumber;
  State.RoundEndTime = gameTime;
  State.RoundIsSpecial = 0;
  if (mapConfig->DefaultSpawnParams && mapConfig->SpecialRoundParamsCount > 0) {

    // find next special round
    int roundId = roundNumber + 1;
    for (i = 0; i < mapConfig->SpecialRoundParamsCount; ++i) {
      int minRound = mapConfig->SpecialRoundParams[i].MinRound;
      int repeatEvery = mapConfig->SpecialRoundParams[i].RepeatEveryNRounds;
      int repeatCount = mapConfig->SpecialRoundParams[i].RepeatCount;

      if (roundId >= minRound) {
        float iteration = (roundId - minRound) / (float)repeatEvery;
        if (iteration == ceilf(iteration) && (repeatCount <= 0 || iteration < repeatCount)) {
          State.RoundIsSpecial = 1;
          State.RoundSpecialIdx = i;
          break;
        }
      }
    }
  }

  // set best round for each player still in lobby
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = playerGetFromIndex(i);
    if (player && player->SkinMoby) {
      State.PlayerStates[i].State.BestRound = roundNumber;
    }
  }

  DPRINTF("round start %d\n", roundNumber);
}

//--------------------------------------------------------------------------
int onSetRoundStartRemote(void * connection, void * data)
{
  SurvivalRoundStartMessage_t * message = (SurvivalRoundStartMessage_t*)data;
  onSetRoundStart(message->RoundNumber, message->GameTime);

  return sizeof(SurvivalRoundStartMessage_t);
}

//--------------------------------------------------------------------------
void setRoundStart(int skip)
{
  SurvivalRoundStartMessage_t message;

  // don't allow overwriting existing outcome unless skip
  if (State.RoundEndTime && !skip)
    return;

  // don't allow changing outcome when not host
  if (!State.IsHost)
    return;

  // increment round if end
  int targetRound = State.RoundNumber;
  if (!State.RoundEndTime)
    targetRound += 1;

  // build message
  message.RoundNumber = targetRound;
  message.GameTime = gameGetTime();

  if (!skip) {
    // get transition time
    // if map returns negative, use post unlimited
    int postSpecialRoundUnlimited = State.RoundIsSpecial && mapConfig->SpecialRoundParams[State.RoundSpecialIdx].UnlimitedPostRoundTime;
    int transitionTime = getRoundTransitionTime(targetRound);
    if (postSpecialRoundUnlimited || transitionTime < 0)
      message.GameTime = -1;
    else
      message.GameTime += transitionTime;
  }

  // send out
  netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, netGetDmeServerConnection(), CUSTOM_MSG_ROUND_START, sizeof(SurvivalRoundStartMessage_t), &message);

  // set locally
  onSetRoundStart(message.RoundNumber, message.GameTime);
}

//--------------------------------------------------------------------------
void forcePlayerHUD(void)
{
  int i;

  // replace normal scoreboard with bolt counter
  for (i = 0; i < GAME_MAX_LOCALS; ++i)
  {
    if (shouldDrawHud()) {
      PlayerHUDFlags* hudFlags = hudGetPlayerFlags(i);
      if (hudFlags) {
        hudFlags->Flags.BoltCounter = 1;
        hudFlags->Flags.NormalScoreboard = 0;
      }
    }
  }

  // show exp
  POKE_U32(0x0054ffc0, 0x0000102D);
  POKE_U16(0x00550054, 0);
}

//--------------------------------------------------------------------------
void destroyOmegaPads(void)
{
  int c = 0;
  GuberMoby* gm = guberMobyGetFirst();
  while (gm)
  {
    Moby* moby = gm->Moby;
    if (moby && moby->OClass == MOBY_ID_PICKUP_PAD && moby->PVar) {
      if (*(u8*)moby->PVar == 5) {
        guberMobyDestroy(moby);
        ++c;
      }
    }

    gm = (GuberMoby*)gm->Guber.Prev;
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
#if LOG_STATS2
        DPRINTF("setting pickup at %08X to %d\n", (u32)moby, gadgetId);
#endif
        ((void (*)(Moby*, int))0x0043A370)(moby, gadgetId);

        ++pickupCount;
      }

      ++moby;
    }
  }
}

//--------------------------------------------------------------------------
int whoKilledMeHook(Player* player, Moby* moby, int b)
{
  if (!moby)
    return 0;

  // only allow mobs or special mobys
  if (mobyIsMob(moby) || moby->Bolts == -1) {
    //DPRINTF("%08X %f %d %d\n", (u32)moby, player->Health, player->PlayerState, playerIsDead(player));


    return ((int (*)(Player*, Moby*, int))0x005dff08)(player, moby, b);
  }

  return 0;
}

//--------------------------------------------------------------------------
void onPlayerRespawnStripWeapons(Player* player)
{
  if (!player || !player->GadgetBox) return;

  // don't penalize if round ends before full death
  int penalize = State.PlayerStates[player->PlayerId].ReviveCooldownTicks == 0;

  // derank weapons
  int i;
  for (i = WEAPON_SLOT_VIPERS; i < WEAPON_SLOT_COUNT; ++i) {
    int gadgetId = weaponSlotToId(i);
    int level = player->GadgetBox->Gadgets[gadgetId].Level;
    if (penalize && level > 0)
      player->GadgetBox->Gadgets[gadgetId].Level--;
    
    if (level > 0)
      player->GadgetBox->Gadgets[gadgetId].Ammo = playerGetWeaponMaxAmmo(player->GadgetBox, gadgetId);
  }

  player->Health = player->MaxHealth;
}

//--------------------------------------------------------------------------
int onMobyPlayDesiredSound(int sound, int a1, Moby* moby)
{
  // catch mobs
  if (mobyIsMob(moby)) {
    int midx = ((struct MobPVar*)moby->PVar)->MobVars.SpawnParamsIdx;
    int sidx = sound % MOBS_PLAY_SOUND_COOLDOWN_MAX_SOUNDIDS;
    if (mobPlaySoundCooldownTicks[midx][sidx]) return -1;
    mobPlaySoundCooldownTicks[midx][sidx] = MOBS_PLAY_SOUND_COOLDOWN;
  }

  // pass to mobyPlaySound
  return mobyPlaySound(sound, a1, moby);
}

//--------------------------------------------------------------------------
Moby* FindMobyOrSpawnBox(int oclass, int defaultToSpawnpointId)
{
  // find
  Moby* m = mobyFindNextByOClass(mobyListGetStart(), oclass);
  
  // if can't find moby then just spawn a beta box at a spawn point
  if (!m) {
    SpawnPoint* sp = spawnPointGet(defaultToSpawnpointId);

    //
    m = mobySpawn(MOBY_ID_BETA_BOX, 0);
    vector_copy(m->Position, &sp->M0[12]);

#if DEBUG
    printf("could not find oclass %04X... spawned box (%08X) at ", oclass, (u32)m);
    vector_print(&sp->M0[12]);
    printf("\n");
#endif
  }

  return m;
}

//--------------------------------------------------------------------------
void resetRoundState(void)
{
  int i;
  int gameTime = gameGetTime();

  // 
  State.RoundMaxMobCount = MAX_MOBS_BASE + (int)(MAX_MOBS_ROUND_WEIGHT * (1 + State.RoundNumber));
  State.RoundMaxSpawnedAtOnce = MAX_MOBS_ALIVE_REAL;
  State.RoundInitialized = 0;
  State.RoundStartTime = gameTime;
  State.RoundCompleteTime = 0;
  State.RoundEndTime = 0;
  State.RoundSpawnTicker = 0;
  State.RoundSpawnTickerCounter = 0;
  State.RoundNextSpawnTickerCounter = randRangeInt(MOB_SPAWN_BURST_MIN + (State.RoundNumber * MOB_SPAWN_BURST_MIN_INC_PER_ROUND), MOB_SPAWN_BURST_MAX + (State.RoundNumber * MOB_SPAWN_BURST_MAX_INC_PER_ROUND));

  State.MobStats.TotalAlive = 0;
  State.MobStats.TotalSpawnedThisRound = 0;
  memset(State.MobStats.NumAlive, 0, sizeof(State.MobStats.NumAlive));
  memset(State.MobStats.NumSpawnedThisRound, 0, sizeof(State.MobStats.NumSpawnedThisRound));

  // reset revive counters
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    State.PlayerStates[i].State.TimesRevivedSinceRoundStart = 0;
  }

  // end vote
  voteEnd(&State.VoteForNextRound);

  // reset bank
  if (State.Bankbox) {
    struct BankBoxPVar* bankPvars = (struct BankBoxPVar*)State.Bankbox->PVar;
    bankPvars->BoltsAtStartOfRound = bankPvars->TotalBolts;
    bankPvars->BoltsDepositThisRound = 0;
    bankPvars->BoltsWithdrawnThisRound = 0;
#if LOG_STATS2
    DPRINTF("bank vestable bolts this round:%d\n", bankPvars->BoltsAtStartOfRound);
#endif
  }

  // 
  if (State.RoundIsSpecial) {
    State.RoundMaxSpawnedAtOnce = mapConfig->SpecialRoundParams[State.RoundSpecialIdx].MaxSpawnedAtOnce;
    State.RoundMaxMobCount *= mapConfig->SpecialRoundParams[State.RoundSpecialIdx].SpawnCountFactor;

    // set max mob count to MaxSpawnedPerRound if every mob param has round limit
    // ie if the special round only has 1 mob param, and that mob is set to only spawn 1 per round
    // then set RoundMaxMobCount to 1 so that the round ends when that mob dies
    int maxPerRound = 0;
    for (i = 0; i < mapConfig->SpecialRoundParams[State.RoundSpecialIdx].SpawnParamCount; ++i) {
      int spawnParamIdx = mapConfig->SpecialRoundParams[State.RoundSpecialIdx].SpawnParamIds[i];
      if (mapConfig->DefaultSpawnParams[spawnParamIdx].MaxSpawnedPerRound <= 0) {
        maxPerRound = State.RoundMaxMobCount;
        break;
      }

      maxPerRound += mapConfig->DefaultSpawnParams[spawnParamIdx].MaxSpawnedPerRound;
    }

    State.RoundMaxMobCount = maxPerRound;
  }

  // 
  State.RoundInitialized = 1;
}

//--------------------------------------------------------------------------
float playerGetArbiterExplosionRadius(Player* player)
{
  if (!player || !player->GadgetBox) return 0;

  return playerGetWeaponAlphaModCount(player->GadgetBox, WEAPON_ID_ARBITER, ALPHA_MOD_AREA) * 0.5;
}

//--------------------------------------------------------------------------
int playerOnSpawnHoloshield(int playerId)
{
	// each player has 4 holoshields max
	Moby **cacheList = (Moby **)0x003360e0 + (playerId * 4);

	int i;
	int count = 0;
	for (i = 0; i < 4; ++i)
	{
		if (cacheList[i])
			++count;
	}

	// remove first and shift rest down if we've hit the max
	// to give room for the new one
	if (count == 4)
	{
		guberMobyDestroy(cacheList[0]);
		memmove(&cacheList[0], &cacheList[1], 3 * sizeof(Moby *));
		cacheList[3] = NULL;
	}

	return 0;
}

//--------------------------------------------------------------------------
float fusionGetBeamDistOffByAreaMods(int areaCount)
{
  // falloff sqrt(x * 0.5)
  return sqrtf(areaCount * 0.5);
}

//--------------------------------------------------------------------------
float fusionGetBeamHitDistSqr(u128 a0, u128 a1, u128 a2, void* a3)
{
  float sqrDist = ((float (*)(u128,u128,u128,void*))0x003fbb10)(a0, a1, a2, a3);

  // increase area of beam per area mod
  Player* player = *(Player**)(*(void**)((u32)a3 + 0x40) + 0x64);
  if (player && player->GadgetBox) {
    int count = playerGetWeaponAlphaModCount(player->GadgetBox, WEAPON_ID_FUSION_RIFLE, ALPHA_MOD_AREA);
    float dist = sqrtf(sqrDist);
    float off = fusionGetBeamDistOffByAreaMods(count);
    sqrDist = powf(maxf(0, dist - off), 2);
  }

  return sqrDist;
}

//--------------------------------------------------------------------------
void mapLoadResetState(PatchStateContainer_t* gameState)
{
  State.Bankbox = NULL;
  State.BigAl = NULL;
  State.BossMoby = NULL;
  State.PrestigeMachine = NULL;
  State.MysteryBoxMoby = NULL;
}

//--------------------------------------------------------------------------
void initialize(PatchStateContainer_t* gameState)
{
  static int waitingForClientsReady = 0;
  static int firstTime = 1;
  char hasTeam[10] = {0,0,0,0,0,0,0,0,0,0};
  int i, j;

  if (firstTime) {
    firstTime = 0;
    
    // clear state
    memset(&State, 0, sizeof(State));
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      memset(State.PlayerStates[i].State.ItemCounts, 0, sizeof(State.PlayerStates[i].State.ItemCounts));
    }
  }

  // disable randomize weapons on respawn
  POKE_U32(0x005E2B40, 0);
  HOOK_JAL(0x005e2b2c, &onPlayerRespawnStripWeapons);
  POKE_U32(0x005e2b48, 0);

  // Disable normal game ending
  *(u32*)0x006219B8 = 0;	// survivor (8)
  *(u32*)0x00620F54 = 0;	// time end (1)
  *(u32*)0x00621568 = 0;	// kills reached (2)
  *(u32*)0x006211A0 = 0;	// all enemies leave (9)
  *(u32*)0x006210D8 = 0;	// all enemies leave (9)

  HOOK_JAL(0x004f7780, &onMobyDestroyedCleanupAnimLayers);
  HOOK_JAL(0x004f72a4, &onMobySpawnedInitInstance);

  // spawn area mod explosion on each ricochet of the v10 vipers
  //HOOK_JAL(0x003C283C, &onV10VipersHitSurface);

  // disable holoshields from disappearing
  //*(u16*)0x00401478 = 2;

  // sniper shot radius
  POKE_U32(0x003FBC84, 0x3C024200);
  HOOK_JAL(0x003FBD3C, &fusionGetBeamHitDistSqr);
  HOOK_JAL(0x003FBDB8, &fusionGetBeamHitDistSqr);

	// let holo shoot always
	HOOK_JAL(0x00400A48, &playerOnSpawnHoloshield);
	POKE_U32(0x003CF04C, 0x0000102D);

  // disable team based holoshield toggling
  // enables players to shoot through eachothers shields
  POKE_U32(0x005A3830, 0x2402FFFF);
  POKE_U32(0x005A3834, 0x14500018);

  // force holoshield hit testing
  *(u32*)0x00401194 = 0;
  *(u32*)0x003FFDE8 = 0x1000000D;
  POKE_U32(0x003FFD98, 0x120000DD); // fix holo crash when owner leaves

  // Disables end game draw dialog
  *(u32*)0x0061fe84 = 0;

  // sets start of colored weapon icons to v10
  *(u32*)0x005420E0 = 0x2A020009;
  *(u32*)0x005420E4 = 0x10400005;

  // Removes MP check on HudAmmo weapon icon color (so v99 is pinkish)
  *(u32*)0x00542114 = 0;

  // Enable sniper to shoot through multiple enemies
  *(u32*)0x003FC2A8 = 0;

  // have sniper shot always shoot straight
  POKE_U32(0x003F935C, 0);

  // Disable sniper shot corn
  //*(u32*)0x003FC410 = 0;
  *(u32*)0x003FC5A8 = 0;

  // fix arbiter explosion radius
  POKE_U32(0x003F595C, 0);
  //POKE_U32(0x003F5760, 0x00028040);
  HOOK_JAL_OP(0x003F57F0, &playerGetArbiterExplosionRadius, 0x8E2400A8);

  // Fix v10 arb overlapping shots
  *(u32*)0x003F2E70 = 0x24020000;

  // hook when MobyPlayDesiredSound plays a sound for a moby
  // lets us reduce the # of mob sounds
  HOOK_JAL(0x004f790c, &onMobyPlayDesiredSound);
  HOOK_J(0x004fa800, &onMobyPlayDesiredSound);

  // disable timebase query percentile filter
  // always accept remote time
  //POKE_U32(0x01eabd60, 0);

  // Fix locals using same bolt count
  HOOK_J(0x00557C00, &_getLocalBolts);
  POKE_U32(0x00557C04, 0);

  // fix emp
  //POKE_U32(0x0042075C, 0x2C42010B);
  //POKE_U32(0x00420610, 0x24050001);
  //HOOK_JAL(0x00420634, &onEmpHitMoby);
  //HOOK_JAL(0x004209bc, &onEmpExplode);
  //HOOK_JAL(0x0041fb8c, &onEmpFired);

  // hook when v10 mag shot hits
  HOOK_JAL(0x0044B374, &onV10MagDamageMoby);

  // Change mine update function to ours
  u32 mineUpdateFunc = 0x003c6c28;
  u32* updateFuncs = (u32*)0x00249980;
  for (i = 0; i < 113; ++i) {
    if (*updateFuncs == mineUpdateFunc) { *updateFuncs = (u32)&customMineMobyUpdate; }
    updateFuncs++;
  }

  // Change bangelize weapons call to ours
  *(u32*)0x005DD890 = 0x0C000000 | ((u32)&customBangelizeWeapons >> 2);

  // Enable weapon version and v10 name variant in places that display weapon name
  *(u32*)0x00541850 = 0x08000000 | ((u32)&customGetGadgetVersionName >> 2);
  *(u32*)0x00541854 = 0;

  // patch who killed me to prevent damaging others
  *(u32*)0x005E07C8 = 0x0C000000 | ((u32)&whoKilledMeHook >> 2);
  *(u32*)0x005E11B0 = *(u32*)0x005E07C8;

  // patch quad/shield cooldown timer
  HOOK_JAL(0x004468D8, &setPlayerQuadCooldownTimer);
  POKE_U32(0x004468E4, 0);
  HOOK_JAL(0x00446948, &setPlayerShieldCooldownTimer);
  POKE_U32(0x00446954, 0);

  // disable guber event delay until createTime+relDispatchTime reached
  // when players desync, their net time falls behind everyone else's
  // causing events that they receive to be delayed for long periods of time
  // leading to even more desyncing issues
  // since survival can cause a lot of frame lag, especially for players on emu/dzo
  // this fix is required to ensure that important mob guber events trigger on everyone's screen
  POKE_U32(0x00611518, 0x24040000);

  // set default ammo for flail to 8
  //*(u8*)0x0039A3B4 = 8;

  // disable targeting players
  *(u32*)0x005F8A80 = 0x10A20002;
  *(u32*)0x005F8A84 = 0x0000102D;
  *(u32*)0x005F8A88 = 0x24440001;

  // clear if magic not valid
  if (mapConfig->Magic != MAP_CONFIG_MAGIC) {
    memset(mapConfig, 0, sizeof(struct SurvivalMapConfig));
    mapConfig->Magic = MAP_CONFIG_MAGIC;
    DPRINTF("clear\n");
  }

  // write map config
  mapConfig->State = &State;
  mapConfig->Functions.ModeSpawnGetRandomPointFunc = &spawnGetRandomPoint;
  mapConfig->Functions.ModeUpgradePlayerWeaponFunc = &mapUpgradePlayerWeaponHandler;
  mapConfig->Functions.ModePushSnackFunc = &pushSnack;
  mapConfig->Functions.ModePushBubbleFunc = &bubblePush;
  mapConfig->Functions.ModePopulateSpawnArgsFunc = &populateSpawnArgsFromConfig;
  mapConfig->Functions.ModeCreateMobFunc = &mobCreate;
  mapConfig->Functions.ModeMobNukeFunc = &mobNuke;
  mapConfig->Functions.ModeSetDoublePointsFunc = &setDoublePoints;
  mapConfig->Functions.ModeSetDoubleXPFunc = &setDoubleXP;
  mapConfig->Functions.ModeSetFreezeMobsFunc = &setFreeze;
  mapConfig->Functions.ModeRevivePlayerFunc = &playerRevive;
  mapConfig->Functions.ModeSendPlayerStatsFunc = &sendPlayerStats;
  mapConfig->Functions.ModeOnGuberEventFunc = &mobHandleEvent;
  mapConfig->Functions.ModeSendOnPlayerItemAcquiredFunc = &sendPlayerItemAcquired;
  mapConfig->Functions.ModeSendOnPlayerItemConsumedFunc = &sendPlayerItemConsumed;

  // custom damage cooldown time
  //POKE_U16(0x0060583C, DIFFICULTY_HITINVTIMERS[gameConfig->survivalConfig.difficulty]);
  //POKE_U16(0x0060582C, DIFFICULTY_HITINVTIMERS[gameConfig->survivalConfig.difficulty]);

  // Hook custom net events
  netInstallCustomMsgHandler(CUSTOM_MSG_ROUND_COMPLETE, &onSetRoundCompleteRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_ROUND_START, &onSetRoundStartRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_ROUND_BEGIN, &onRoundBeginRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_WEAPON_UPGRADE, &onPlayerUpgradeWeaponRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_REVIVE_PLAYER, &onPlayerReviveRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_DIED, &onSetPlayerDeadRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_WEAPON_MODS, &onSetPlayerWeaponModsRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_STATS, &onSetPlayerStatsRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_DOUBLE_POINTS, &onSetPlayerDoublePointsRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_DOUBLE_XP, &onSetPlayerDoubleXPRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SET_FREEZE, &onSetFreezeRemote);
  //netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_USE_ITEM, &onPlayerUseItemRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_MOB_UNRELIABLE_MSG, &mobOnUnreliableMsgRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_WEAPON_PRESTIGE, &onPlayerPrestigeWeaponRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_INTERACT_BANK_BOX, &onPlayerInteractBankBoxRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_WITHDRAWN_BANK_BOX, &onPlayerWithdrawnBankBoxRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_SET_ROUND_50_TIME, &onSetRound50TimeRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_CAST_VOTE, &onPlayerCastVoteRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_ITEM_ACQUIRE, &onPlayerItemAcquiredRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_ITEM_CONSUME, &onPlayerItemConsumedRemote);

  // set game over string
  safe_strcpy(uiMsgString(0x3477), SURVIVAL_GAME_OVER, strlen((char*)SURVIVAL_GAME_OVER)+1);
  safe_strcpy(uiMsgString(0x3153), SURVIVAL_REVIVE_MESSAGE, strlen((char*)SURVIVAL_REVIVE_MESSAGE)+1);

  // disable v2s and packs
  cheatsApplyNoV2s();
  cheatsApplyNoPacks();

  // change hud
  forcePlayerHUD();
  setPlayerEXP(0, 0);
  setPlayerEXP(1, 0);

  // set bolts to 0
  memset(BoltCounts, 0, sizeof(BoltCounts));

  // destroy omega mod pads
  destroyOmegaPads();

  // prevent player from doing anything
  padDisableInput();

  // 
  mobInitialize();

  //
  memset(playerStates, 0, sizeof(playerStates));
  memset(playerStateTimers, 0, sizeof(playerStateTimers));

  if (InitializeDelay) {
    --InitializeDelay;
    return;
  }

  // wait for all clients to be ready
  // or for 15 seconds
  if (!gameState->AllClientsReady && waitingForClientsReady < (5 * TPS)) {
    uiShowPopup(0, "Waiting For Players...");
    ++waitingForClientsReady;
    return;
  }

  // hide waiting for players popup
  hudHidePopup();

  //
  mapConfig->ClientsReady = 1;

  // re-enable input
  padEnableInput();

  bubbleInit();
  statsInit();

  memset(defaultSpawnParamsCooldowns, 0, sizeof(defaultSpawnParamsCooldowns));
  memset(snackItems, 0, sizeof(snackItems));

  // initialize player states
  State.LocalPlayerState = NULL;
  State.NumTeams = 0;
  State.ActivePlayerCount = 0;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player * p = playerGetFromIndex(i);
    State.PlayerStates[i].RevivingPlayerId = -1;

#if PAYDAY
    State.PlayerStates[i].State.CurrentTokens = 1000;
#endif

    if (p) {

      // is local
      State.PlayerStates[i].IsLocal = p->IsLocal;
      if (p->IsLocal && !State.LocalPlayerState)
        State.LocalPlayerState = &State.PlayerStates[i];
        
      // clear alpha mods
      if (p->GadgetBox && !FirstTimeInitialized)
        memset(p->GadgetBox->ModBasic, 0, sizeof(p->GadgetBox->ModBasic));

      if (!hasTeam[p->Team]) {
        State.NumTeams++;
        hasTeam[p->Team] = 1;
      }

#if PAYDAY
      p->GadgetBox->ModBasic[0] = 64;
      p->GadgetBox->ModBasic[1] = 64;
      p->GadgetBox->ModBasic[2] = 64;
      p->GadgetBox->ModBasic[3] = 64;
      p->GadgetBox->ModBasic[4] = 64;
      p->GadgetBox->ModBasic[5] = 64;
      p->GadgetBox->ModBasic[6] = 64;
      p->GadgetBox->ModBasic[7] = 64;
      State.PlayerStates[i].State.Bolts = 100000000;
#endif

      ++State.ActivePlayerCount;
    }
  }

  // initialize weapon data
  WeaponDefsData* gunDefs = weaponGetGunLevelDefs();
  for (i = 0; i < 7; ++i) {
    for (j = 0; j < 10; ++j) {
      gunDefs[i].Entries[j].MpLevelUpExperience = VENDOR_MAX_WEAPON_LEVEL;
    }
  }
  WeaponDefsData* flailDefs = weaponGetFlailLevelDefs();
  for (j = 0; j < 10; ++j) {
    flailDefs->Entries[j].MpLevelUpExperience = VENDOR_MAX_WEAPON_LEVEL;
  }

  // randomize weapon picks
  if (State.IsHost) {
    //randomizeWeaponPickups();
  }

  // find al
#if defined(BIGAL_OCLASS)
  State.BigAl = FindMobyOrSpawnBox(BIGAL_OCLASS, 2);
#else
  State.BigAl = FindMobyOrSpawnBox(0, 2);
#endif

  // find prestige machine
#if defined(PRESTIGEMACHINE_OCLASS)
  State.PrestigeMachine = FindMobyOrSpawnBox(PRESTIGEMACHINE_OCLASS, 2);
#else
  State.PrestigeMachine = FindMobyOrSpawnBox(0, 2);
#endif

  State.Bankbox = NULL;
  DPRINTF("big al %08X\n", (u32)State.BigAl);
  DPRINTF("prestige machine %08X\n", (u32)State.PrestigeMachine);

  // initialize state
  memset(&State.MobStats, 0, sizeof(State.MobStats));
  State.Freeze = 0;
  State.TimeOfFreeze = 0;
  State.RoundIsSpecial = 0;
  State.RoundSpecialIdx = 0;
  State.DropCooldownTicks = TPS; // try and stop drops from spawning immediately
  State.Difficulty = getDifficultyMultiplier();
  
  if (!FirstTimeInitialized) {
    State.InitializedTime = gameGetTime();
  }

#if STARTROUND
  State.RoundNumber = STARTROUND - 2;
  //State.RoundIsSpecial = MapConfig.
  //State.RoundSpecialIdx = 4;
  int xp[GAME_MAX_PLAYERS];

#if !PAYDAY
  for (j = 0; j < GAME_MAX_PLAYERS; ++j) {
    State.PlayerStates[j].State.Bolts = 10000;
    xp[j] = 1500;
  }
#endif

  // 
  for (i = 0; i < State.RoundNumber; ++i) {
    State.MobStats.TotalSpawned += MAX_MOBS_BASE + (int)(MAX_MOBS_ROUND_WEIGHT * (1 + i));
    
#if !PAYDAY
    for (j = 0; j < GAME_MAX_PLAYERS; ++j) {
      State.PlayerStates[j].State.Bolts *= 1.225;
      //State.PlayerStates[j].State.TotalBolts = State.PlayerStates[j].State.Bolts;
      xp[j] *= 1.125;
    }
#endif
  }

  for (j = 0; j < GAME_MAX_PLAYERS; ++j) {
    //playerRewardXp(j, -1, xp[j]);
    State.PlayerStates[j].State.XP = xp[j];
  }

  DPRINTF("Skipped to Round #%d with %d mobs spawned\n", State.RoundNumber + 1, State.MobStats.TotalSpawned);
#endif

  resetRoundState();

  // force initial delay on first round
  State.RoundSpawnTicker = TPS * 3;

  // scale default mob params by difficulty
  // for (i = 0; i < mapConfig->DefaultSpawnParamsCount; ++i)
  // {
  // 	struct MobConfig* config = &mapConfig->DefaultSpawnParams[i].Config;
  // 	config->Bolts /= State.Difficulty;
  // 	config->MaxHealth *= State.Difficulty;
  // 	config->Health *= State.Difficulty;
  // 	config->Damage *= State.Difficulty;
  // }

#if FIXEDTARGET
  FIXEDTARGETMOBY = mobySpawn(0xE7D, 0);
  FIXEDTARGETMOBY->Position[0] = 697.54;
  FIXEDTARGETMOBY->Position[1] = 445.12;
  FIXEDTARGETMOBY->Position[2] = 314.7968;
#endif

  Initialized = 1;
  FirstTimeInitialized = 1;
}

//--------------------------------------------------------------------------
void updateGameState(PatchStateContainer_t * gameState)
{
  int i;

  // kind of a hack but keep this value around so that when in game we can load it from the map
  // but still have it when we post stats after the game ends
  static int boltRankMult = 1;

  // game state update
  if (gameState->UpdateGameState)
  {
    gameState->GameStateUpdate.RoundNumber = State.RoundNumber + 1;
  }

  if (isInGame()) {
    boltRankMult = getBoltRankMultiplier();

    // compute round 50 completed time
    checkForRound50Time();

    // update custom game stats after each round
    static int roundCompleted = -1;
    if (roundCompleted != State.RoundNumber && gameAmIHost()) {
      gameState->UpdateCustomGameStats = 1;
      roundCompleted = State.RoundNumber;
    }
  }

  // stats
	if (gameState->UpdateCustomGameStats && gameState->CustomGameStats)
  {
    gameState->CustomGameStatsSize = sizeof(struct SurvivalGameData);
    struct SurvivalGameData* sGameData = (struct SurvivalGameData*)gameState->CustomGameStats->Payload;
    sGameData->RoundNumber = State.RoundNumber;
    sGameData->Version = 0x00000007;
    sGameData->Round50Time = State.Round50Time;

    // set per player stats
    for (i = 0; i < GAME_MAX_PLAYERS; ++i)
    {
      sGameData->Kills[i] = State.PlayerStates[i].State.Kills;
      sGameData->Revives[i] = State.PlayerStates[i].State.Revives;
      sGameData->TimesRevived[i] = State.PlayerStates[i].State.TimesRevived;
      sGameData->Points[i] = State.PlayerStates[i].State.TotalBolts * boltRankMult;
      sGameData->BestRound[i] = State.PlayerStates[i].State.BestRound;
    }
  }
}

//--------------------------------------------------------------------------
void gameStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
  GameSettings * gameSettings = gameGetSettings();
  GameOptions * gameOptions = gameGetOptions();
  Player* localPlayer = playerGetFromSlot(0);
  int i;
  char buffer[64];
  int gameTime = gameGetTime();
  float demonBellFactor = 0;

  if (State.DemonBellCount > 0) {
    demonBellFactor = 1 - powf(1 - (State.RoundDemonBellCount / (float)State.DemonBellCount), 2);
  }

  // first
  dlPreUpdate();

  // 
  updateGameState(gameState);

  // Ensure in game
  if (!gameSettings || !isInGame())
    return;

  // Determine if host
  State.IsHost = gameAmIHost();

  // 
  playerConfig = gameState->Config;

  if (!Initialized) {
    initialize(gameState);
    return;
  }

  // update pause state
  State.Paused = 0;
  if (gameSettings->PlayerCount == 1)
  {
    int pause = 1;
    for (i = 0; i < GAME_MAX_LOCALS; ++i) 
    {
      Player* player = playerGetFromSlot(i);
      if (!playerIsValid(player)) continue;
      if (!gameIsStartMenuOpen(i) || State.PlayerStates[player->PlayerId].IsInWeaponsMenu)
      {
        pause = 0;
        break;
      }
    }

    State.Paused = pause;
  }

#if STARTROUND
  static int startRoundDelay = 60 * 1;
  if (startRoundDelay == 0) {
    setRoundComplete();
    startRoundDelay--;
  } else if (startRoundDelay > 0) {
    --startRoundDelay;
  }
#endif

  // get local player data
  struct SurvivalPlayer* localPlayerData = &State.PlayerStates[localPlayer->PlayerId];

#if LOG_STATS
  static int statsTicker = 0;
  if (statsTicker <= 0) {
    DPRINTF("liveMobCount:%d totalSpawnedThisRound:%d roundNumber:%d roundSpawnTicker:%d\n", State.MobStats.TotalAlive, State.MobStats.TotalSpawnedThisRound, State.RoundNumber, State.RoundSpawnTicker);
    statsTicker = 60 * 15;
  } else {
    --statsTicker;
  }
#endif

#if DEBUG_SOUNDS
  {
    u16 * list = mobyGetLoadedMobyClassList();
    static int aaa = 0;
    static int bbb = 0;
    int mobyClass = list[bbb];
    if (padGetButtonDown(0, PAD_RIGHT) > 0) {
      aaa += 1;
      printf("%04X %d\n", mobyClass, aaa);
      mobyPlaySoundByClass(aaa, 0, localPlayer->PlayerMoby, mobyClass);
    } else if (padGetButtonDown(0, PAD_LEFT) > 0) {
      aaa -= 1;
      printf("%04X %d\n", mobyClass, aaa);
      mobyPlaySoundByClass(aaa, 0, localPlayer->PlayerMoby, mobyClass);
    } else if (padGetButtonDown(0, PAD_UP) > 0) {
      printf("%04X %d\n", mobyClass, aaa);
      mobyPlaySoundByClass(aaa, 0, localPlayer->PlayerMoby, mobyClass);
    } else if (padGetButtonDown(0, PAD_L1) > 0) {
      bbb -= 1;
      mobyClass = list[bbb];
      printf("%04X %d\n", mobyClass, aaa);
      mobyPlaySoundByClass(aaa, 0, localPlayer->PlayerMoby, mobyClass);
    } else if (padGetButtonDown(0, PAD_L2) > 0) {
      bbb += 1;
      mobyClass = list[bbb];
      printf("%04X %d\n", mobyClass, aaa);
      mobyPlaySoundByClass(aaa, 0, localPlayer->PlayerMoby, mobyClass);
    }
  }
#endif

#if MANUAL_SPAWN
  if (localPlayerHasInput())
  {
    
    // static int aaa = 0;
    // if (padGetButtonDown(0, PAD_RIGHT) > 0) {
    // 	aaa += 1;
    // 	DPRINTF("%d\n", aaa);
    // }
    // else if (padGetButtonDown(0, PAD_LEFT) > 0) {
    // 	aaa -= 1;
    // 	DPRINTF("%d\n", aaa);
    // }

    if (padGetButtonDown(0, PAD_DOWN) > 0) {
      static int manSpawnMobId = 0;

      //if (manSpawnMobId == 0) manSpawnMobId = 2;

      // force one mob type
      //manSpawnMobId = 0;
      //manSpawnMobId = 2;
      //manSpawnMobId = mapConfig->DefaultSpawnParamsCount - 2;
      //manSpawnMobId = mapConfig->DefaultSpawnParamsCount - 1;

      // skip invalid params
      while (mapConfig->DefaultSpawnParams[manSpawnMobId].Probability < 0) {
        manSpawnMobId = (manSpawnMobId + 1) % mapConfig->DefaultSpawnParamsCount;
      }

      // build spawn position
      VECTOR t;
      if (1 || !spawnGetRandomPoint(t, &mapConfig->DefaultSpawnParams[manSpawnMobId])) {
        VECTOR offset = {1,1,1,0};
        vector_scale(t, offset, mapConfig->DefaultSpawnParams[manSpawnMobId].Config.CollRadius*2);
        vector_add(t, t, localPlayer->PlayerPosition);
      }

      //DPRINTF("spawning mob type %d\n", manSpawnMobId);
      
      // spawn
      mobCreate(manSpawnMobId, t, 0, -1, 0, &mapConfig->DefaultSpawnParams[manSpawnMobId].Config);
      manSpawnMobId = (manSpawnMobId + 1) % mapConfig->DefaultSpawnParamsCount;
    }
    // else if (padGetButtonDown(0, PAD_L1 | PAD_RIGHT) > 0) {
    //   State.Freeze = 1;
    //   State.TimeOfFreeze = 0x6FFFFFFF;
    //   DPRINTF("freeze\n");
    // }
    // else if (padGetButtonDown(0, PAD_L1 | PAD_LEFT) > 0) {
    //   State.Freeze = 0;
    //   State.TimeOfFreeze = 0;
    //   DPRINTF("unfreeze\n");
    // }
  }
#endif

#if BENCHMARK
  {
    static int manSpawnMobId = 0;
    if (manSpawnMobId < MAX_MOBS_ALIVE)
    {
      VECTOR t = {396,606,434,0};
      
      t[0] += (manSpawnMobId % 8) * 2;
      t[1] += (manSpawnMobId / 8) * 2;

      int id = manSpawnMobId++ % mapConfig->DefaultSpawnParamsCount;
      mobCreate(id, t, 0, -1, 0, &mapConfig->DefaultSpawnParams[id].Config);
    }
  }
#endif

#if MANUAL_DROP_SPAWN
  if (localPlayerHasInput())
  {
    if (padGetButtonDown(0, PAD_UP) > 0) {
      static int manSpawnDropId = 0;
      static int manSpawnDropIdx = 0;

      int loop = 0;
      do {
        manSpawnDropId = (manSpawnDropId + 1) % mapConfig->ItemDefCount;
        loop++;
      } while (loop < mapConfig->ItemDefCount && mapConfig->ItemDefs[manSpawnDropId].DropChanceWeight <= 0);

      //manSpawnDropId = DROP_NUKE;
      VECTOR t;
      vector_copy(t, localPlayer->PlayerPosition);
      t[0] += 5;

      State.DropCooldownTicks = 0;
      if (mapConfig && mapConfig->Functions.CreateMobDropFunc)
        mapConfig->Functions.CreateMobDropFunc(t, manSpawnDropId, gameGetTime() + (30 * TIME_SECOND), localPlayer->Team);

      ++manSpawnDropIdx;
    }
  }
#endif

  // ticks
  mobTick();
  bubbleTick();
  statsTick();

  if (mapConfig && mapConfig->Functions.OnFrameTickFunc)
    mapConfig->Functions.OnFrameTickFunc();

  // tick down mob sound cooldown
  int j;
  for (i = 0; i < MAX_MOB_SPAWN_PARAMS; ++i) {
    for (j = 0; j < MOBS_PLAY_SOUND_COOLDOWN_MAX_SOUNDIDS; ++j) {
      if (mobPlaySoundCooldownTicks[i][j] > 0) {
        --mobPlaySoundCooldownTicks[i][j];
      }
    }
  }

  // draw hud stuff
  if (shouldDrawHud()) {

    if (!localPlayerData->IsInWeaponsMenu) {
      // draw round number
      char* roundStr = uiMsgString(0x25A9);
      gfxScreenSpaceText(31, 281, 0.7, 0.7, 0x40000000, roundStr, -1, 1);
      gfxScreenSpaceText(30, 280, 0.7, 0.7, 0x80E0E0E0, roundStr, -1, 1);
      snprintf(buffer, sizeof(buffer), "%d", State.RoundNumber + 1);
      gfxScreenSpaceText(31, 292, 1, 1, 0x40000000, buffer, -1, 1);
      gfxScreenSpaceText(30, 291, 1, 1, 0x8029E5E6, buffer, -1, 1);

      // draw double bolts
      if (localPlayer && State.PlayerStates[localPlayer->PlayerId].IsDoublePoints) {
        gfxScreenSpaceText(490+1, 40+1, 0.7, 0.7, 0x40000000, "x2", -1, 1);
        gfxScreenSpaceText(490+0, 40+0, 0.7, 0.7, 0x8029E5E6, "x2", -1, 1);
      }

      // draw double xp
      if (localPlayer && State.PlayerStates[localPlayer->PlayerId].IsDoubleXP) {
        gfxScreenSpaceText(208+1, 18+1, 0.7, 0.7, 0x40000000, "x2", -1, 1);
        gfxScreenSpaceText(208+0, 18+0, 0.7, 0.7, 0x8029E5E6, "x2", -1, 1);
      }

      // draw number of mobs spawned
      char* enemiesStr = "ENEMIES";
      gfxScreenSpaceText(31, 241, 0.7, 0.7, 0x40000000, enemiesStr, -1, 1);
      gfxScreenSpaceText(30, 240, 0.7, 0.7, 0x80E0E0E0, enemiesStr, -1, 1);
      //snprintf(buffer, sizeof(buffer), "%d", State.MobStats.TotalAlive + State.MobStats.TotalSpawning);
      snprintf(buffer, sizeof(buffer), "%d", (State.RoundMaxMobCount - State.MobStats.TotalSpawnedThisRound) + State.MobStats.TotalAlive + State.MobStats.TotalSpawning);
      gfxScreenSpaceText(31, 252, 1, 1, 0x40000000, buffer, -1, 1);
      gfxScreenSpaceText(30, 251, 1, 1, 0x8029E5E6, buffer, -1, 1);
      //printf("max:%d total:%d alive:%d spawning:%d\n", State.RoundMaxMobCount, State.MobStats.TotalSpawnedThisRound, State.MobStats.TotalAlive, State.MobStats.TotalSpawning);
    }

    // draw paused
    if (survivalIsPaused())
    {
      gfxHelperDrawText(SCREEN_WIDTH/2, 50, 0, 0, 2, 0x8080FFFF, "PAUSED", -1, TEXT_ALIGN_TOPCENTER, COMMON_DZO_DRAW_NORMAL);
    }

    // draw timer
    drawTimer(gameGetTime() - State.InitializedTime);

    // draw difficulty
    //float difficulty = getDifficultyFactor();
    //snprintf(buffer, sizeof(buffer), "%d%%", (int)(difficulty * 100));
    //gfxScreenSpaceText(15, SCREEN_HEIGHT - 10, 0.8, 0.8, 0x80FFFFFF, buffer, -1, TEXT_ALIGN_BOTTOMLEFT);

    // draw player specific values
    // TODO: add support for 3,4 local players
    for (i = 0; i < GAME_MAX_LOCALS; ++i) {
      Player* p = playerGetFromSlot(i);
      if (!p || !p->PlayerMoby) continue;
      if (gameIsStartMenuOpen(i)) continue;

      struct SurvivalPlayer* playerData = &State.PlayerStates[p->PlayerId];
      float x = 0, y = 0;

      if (playerData->IsInWeaponsMenu) continue;

      // draw dread tokens
      {
        x = 457;
        y = 54;
        transformToSplitscreenPixelCoordinates(i, &x, &y);
        snprintf(buffer, 32, "%d", playerData->State.CurrentTokens);
        gfxScreenSpaceText(x+2, y+8+2, 1, 1, 0x40000000, buffer, -1, 2);
        gfxScreenSpaceText(x,   y+8,   1, 1, 0x80C0C0C0, buffer, -1, 2);
        drawDreadTokenIcon(x+5, y, 32);
      }
    }
  }

#if DEBUG || TEST
  if (padGetButton(0, PAD_CROSS)) {
    *(float*)0x00347BD8 = 0.125;
    *(float*)(0x347AA0 + 0x2E20) = 125;
  }
#endif

#if FIXEDTARGET
  if (FIXEDTARGETMOBY && padGetButton(0, PAD_L1 | PAD_R1 | PAD_L2 | PAD_R2)) {
    vector_copy(FIXEDTARGETMOBY->Position, playerGetFromSlot(0)->PlayerPosition);
    printf("fixed target moved: ");
    vector_print(FIXEDTARGETMOBY->Position);
    printf("\n");
  }
#endif

  if (!State.GameOver)
  {
    // update bolt counter
    for (i = 0; i < GAME_MAX_LOCALS; ++i)
    {
      Player* lp = playerGetFromSlot(i);
      if (!lp) continue;

      BoltCounts[i] = State.PlayerStates[lp->PlayerId].State.Bolts;
    }

    // 
    State.ActivePlayerCount = 0;
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      if (playerIsValid(playerGetFromIndex(i)))
        State.ActivePlayerCount++;

      processPlayer(i);
    }
    drawSnack();

    // replace normal scoreboard with bolt counter
    forcePlayerHUD();

    // handle freeze and double bolts
    if (State.IsHost) {
      int dblPointsChanged = 0;
      int dblXPChanged = 0;

      // disable double bolts for players
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        if (State.PlayerStates[i].IsDoublePoints && gameTime >= (State.PlayerStates[i].TimeOfDoublePoints + DOUBLE_POINTS_DURATION)) {
          State.PlayerStates[i].IsDoublePoints = 0;
          dblPointsChanged = 1;
          DPRINTF("setting player %d dbl bolts to 0\n", i);
        }

        if (State.PlayerStates[i].IsDoubleXP && gameTime >= (State.PlayerStates[i].TimeOfDoubleXP + DOUBLE_XP_DURATION)) {
          State.PlayerStates[i].IsDoubleXP = 0;
          dblXPChanged = 1;
          DPRINTF("setting player %d dbl xp to 0\n", i);
        }
      }

      // disable freeze
      if (State.Freeze && gameTime >= (State.TimeOfFreeze + FREEZE_DROP_DURATION)) {
        DPRINTF("disabling freeze\n");
        setFreeze(0);
      }

      if (dblPointsChanged)
        sendDoublePoints();

      if (dblXPChanged)
        sendDoubleXP();
    }

    // handle game over
    if (State.IsHost && gameOptions->GameFlags.MultiplayerGameFlags.Survivor && gameTime > (State.InitializedTime + 5*TIME_SECOND))
    {
      int isAnyPlayerAlive = 0;

      // determine number of players alive
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        if (playerGetFromIndex(i) && !State.PlayerStates[i].IsDead) {
          isAnyPlayerAlive = 1;
        }
      }

      // if everyone has died, end game
      if (!isAnyPlayerAlive)
      {
        State.GameOver = 1;
        gameSetWinner(10, 1);
      }
    }

    if (State.RoundCompleteTime)
    {
      // draw round complete message
      if (gameTime < (State.RoundCompleteTime + ROUND_MESSAGE_DURATION_MS)) {
        snprintf(buffer, sizeof(buffer), SURVIVAL_ROUND_COMPLETE_MESSAGE, State.RoundNumber);
        drawRoundMessage(buffer, 1.5, 0);
      }

      // start round transition
      if (State.RoundEndTime == 0)
      {
        setRoundStart(0);
      }
      // check for auto end
      else if (State.RoundEndTime > 0 && gameTime > State.RoundEndTime)
      {
        sendRoundBegin();
      }
      else if (State.VoteForNextRound.IsActive)
      {
        int hasCastVote = voteIsCast(&State.VoteForNextRound, gameGetMyClientId());

        // vote ended, start round
        int voteResult = voteGetResult(&State.VoteForNextRound);
        if (voteResult) {
          setRoundStart(1);
        }

        // print vote status
        if (!voteResult) {
          const char* voteMsg = hasCastVote ? SURVIVAL_VOTED_NEXT_ROUND_TIMER_MESSAGE : SURVIVAL_VOTE_NEXT_ROUND_TIMER_MESSAGE;
          if (gameAmIHost() && hasCastVote) voteMsg = SURVIVAL_HOST_SKIP_VOTE_NEXT_ROUND_TIMER_MESSAGE;
          if (State.ActivePlayerCount <= 1) voteMsg = SURVIVAL_START_NEXT_ROUND_TIMER_MESSAGE;
          snprintf(dzoDrawHudCmd.RoundStartMessage, sizeof(dzoDrawHudCmd.RoundStartMessage), voteMsg, State.VoteForNextRound.NumVotes, State.VoteForNextRound.NumVotesRequired);
        }

        // draw timer if round transition has time limit
        if (State.RoundEndTime > 0) {
          int timerSec = State.RoundEndTime - gameTime;
          uiShowTimer(0, dzoDrawHudCmd.RoundStartMessage, (int)(timerSec * (60.0 / TIME_SECOND)));
          dzoDrawHudCmd.StartRoundTimer = timerSec;
        } else if (State.RoundEndTime < 0 && !gameIsAnyStartMenuOpen()) {
          gfxScreenSpaceText(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50, 1, 1, 0x80FFFFFF, dzoDrawHudCmd.RoundStartMessage, -1, TEXT_ALIGN_MIDDLECENTER);
        }

        // handle skip
        if (!hasCastVote && localPlayerHasInput() && padGetButtonDown(0, PAD_UP) > 0) {
          playerCastNextRoundVote();
        } else if (gameAmIHost() && hasCastVote && localPlayerHasInput() && padGetButtonDown(0, PAD_UP) > 0) {
          setRoundStart(1);
        }
      }
    }
    else
    {
      // draw round start message
      if (gameTime < (State.RoundStartTime + ROUND_MESSAGE_DURATION_MS)) {
        snprintf(buffer, sizeof(buffer), SURVIVAL_ROUND_START_MESSAGE, State.RoundNumber+1);
        drawRoundMessage(buffer, 1.5, 0);
        if (State.RoundIsSpecial) {
          drawRoundMessage(mapConfig->SpecialRoundParams[State.RoundSpecialIdx].Name, 1, 35);
        }
      }

#if !defined(DISABLE_SPAWNING)
      // host specific logic
      if (State.IsHost && (gameTime - State.RoundStartTime) > ROUND_START_DELAY_MS)
      {
        // decrement mob cooldowns
        for (i = 0; i < mapConfig->DefaultSpawnParamsCount; ++i) {
          if (defaultSpawnParamsCooldowns[i]) {
            defaultSpawnParamsCooldowns[i] -= 1;
          }
        }

        // reduce count per player to reduce lag
        int maxSpawn = State.RoundMaxSpawnedAtOnce;
        int canSpawn = 1;
        if (mapConfig && mapConfig->Functions.CanSpawnMobsFunc)
          canSpawn = mapConfig->Functions.CanSpawnMobsFunc();

        // handle spawning
        if (State.RoundSpawnTicker == 0) {
          if (State.MobStats.TotalAlive < maxSpawn && !State.Freeze && canSpawn) {
            if (State.MobStats.TotalSpawnedThisRound < State.RoundMaxMobCount) {
              if (spawnRandomMob()) {
#if QUICK_SPAWN
                State.RoundSpawnTicker = 10;
#else
                ++State.RoundSpawnTickerCounter;
                if (State.RoundSpawnTickerCounter > State.RoundNextSpawnTickerCounter)
                {
                  State.RoundSpawnTickerCounter = 0;
                  State.RoundNextSpawnTickerCounter = randRangeInt(MOB_SPAWN_BURST_MIN + (State.RoundNumber * MOB_SPAWN_BURST_MIN_INC_PER_ROUND), MOB_SPAWN_BURST_MAX + (State.RoundNumber * MOB_SPAWN_BURST_MAX_INC_PER_ROUND));
                  

                  // ticks to delay between spawning bursts
                  float maxBurstDelay = lerpf(MOB_SPAWN_BURST_MAX_DELAY, MOB_SPAWN_BURST_MIN_DELAY, clamp((State.RoundNumber / 20.0), 0, 1));
                  maxBurstDelay = lerpf(MOB_SPAWN_BURST_MAX_DELAY, 5, demonBellFactor);
                  State.RoundSpawnTicker = randRangeInt(minf(maxBurstDelay, MOB_SPAWN_BURST_MIN_DELAY), maxBurstDelay);

                  //DPRINTF("MIN:%f MAX:%f BELL:%f TICKER:%f\n", minf(maxBurstDelay, MOB_SPAWN_BURST_MIN_DELAY)/60.0, maxBurstDelay/60.0, demonBellFactor, State.RoundSpawnTicker/60.0);
                }
                else
                {
                  // ticks to delay between spawning mobs within a burst
                  State.RoundSpawnTicker = 1 + lerpf(60.0 / (State.RoundNumber+1), 0, demonBellFactor);
                }
#endif
              }
            } else {

              DPRINTF("finished spawning zombies...\n");
              State.RoundSpawnTicker = -1;
            }
          }
        } else if (State.RoundSpawnTicker < 0) {
          if (State.MobStats.TotalAlive < 0) {
            DPRINTF("%d\n", State.MobStats.TotalAlive);
          }
          // wait for all zombies to die
          if ((State.MobStats.TotalAlive + State.MobStats.TotalSpawning) == 0) {
            setRoundComplete();
          }
        } else {
          --State.RoundSpawnTicker;
        }
      }
#endif
    }
  }
  else
  {
    // end game
    if (State.GameOver == 1)
    {
      gameEnd(4);
      State.GameOver = 2;
    }
  }

  //
  updateDzoHud();

  // last
  dlPostUpdate();
  return;
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(PatchGameConfig_t * gameConfig)
{
  // set game options
  GameOptions * gameOptions = gameGetOptions();
  GameSettings* gameSettings = gameGetSettings();
  if (!gameOptions || gameSettings->GameLoadStartTime <= 0)
    return;

  // force deathmatch
  if (gameSettings->GameRules != GAMERULE_DM) {
    gameSettings->GameRules = GAMERULE_DM;
    gameOptions->GameFlags.MultiplayerGameFlags.Nodes = 0;
    gameOptions->GameFlags.MultiplayerGameFlags.Flags = 0;
    gameOptions->GameFlags.MultiplayerGameFlags.Hills = 0;
	  gameOptions->GameFlags.MultiplayerGameFlags.SpawnType = 3; // NORMAL SPAWNS
  }
	
  // apply options
  gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = 1;
  gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickups = 1;
  gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickupsRandom = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;

#if !DEBUG && !TEST
  gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 1;
  gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = 0;
#endif

  // no vehicles
  gameOptions->GameFlags.MultiplayerGameFlags.Vehicles = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Puma = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = 0;
  gameOptions->GameFlags.MultiplayerGameFlags.Hovership = 0;

  // enable all weapons
  gameOptions->WeaponFlags.Chargeboots = 1;
  gameOptions->WeaponFlags.DualVipers = 1;
  gameOptions->WeaponFlags.MagmaCannon = 1;
  gameOptions->WeaponFlags.Arbiter = 1;
  gameOptions->WeaponFlags.FusionRifle = 1;
  gameOptions->WeaponFlags.MineLauncher = 1;
  gameOptions->WeaponFlags.B6 = 1;
  gameOptions->WeaponFlags.Holoshield = 1;
  gameOptions->WeaponFlags.Flail = 1;

  // disable custom game rules
  if (gameConfig) {
    gameConfig->grNoHealthBoxes = 1;
    gameConfig->grNoInvTimer = 0;
    gameConfig->grNoPickups = 0;
    gameConfig->grV2s = 0;
    gameConfig->grVampire = 0;
    gameConfig->grHealthBars = 1;
    gameConfig->prChargebootForever = 0;
    gameConfig->prHeadbutt = 0;
	  gameConfig->grInstantDeath = 0;
    gameConfig->grCqPersistentCapture = 0;
    gameConfig->grCqDisableTurrets = 0;
    gameConfig->grCqDisableUpgrades = 0;
    gameConfig->grRespawnOverride = 0;
    gameConfig->grInstantDeath = 0;
    gameConfig->grNoSpawnImmunity = 0;
  }

  // force everyone to same team as host
  //for (i = 1; i < GAME_MAX_PLAYERS; ++i) {
  //	if (gameSettings->PlayerTeams[i] >= 0) {
  //		gameSettings->PlayerTeams[i] = gameSettings->PlayerTeams[0];
  //	}
  //}
}

//--------------------------------------------------------------------------
void setEndGameScoreboard(PatchGameConfig_t * gameConfig)
{
  u32 * uiElements = (u32*)(*(u32*)(0x011C7064 + 4*18) + 0xB0);
  int i;

  // column headers start at 17
  safe_strcpy((char*)(uiElements[19] + 0x60), "BOLTS", 6);
  safe_strcpy((char*)(uiElements[20] + 0x60), "DEATHS", 7);
  safe_strcpy((char*)(uiElements[21] + 0x60), "REVIVES", 8);

  // rows
  int* pids = (int*)(uiElements[0] - 0x9C);
  for (i = 0; i < GAME_MAX_PLAYERS; ++i)
  {
    // match scoreboard player row to their respective dme id
    int pid = pids[i];
    char* name = (char*)(uiElements[7 + i] + 0x18);
    if (pid < 0 || name[0] == 0)
      continue;

    struct SurvivalPlayerState* pState = &State.PlayerStates[pid].State;

    // set round number
    sprintf((char*)0x003D3AE0, "Survived %d Rounds!", State.RoundNumber);

    // set kills
    sprintf((char*)(uiElements[22 + (i*4) + 0] + 0x60), "%d", pState->Kills);

    // copy over deaths
    safe_strcpy((char*)(uiElements[22 + (i*4) + 2] + 0x60), (char*)(uiElements[22 + (i*4) + 1] + 0x60), 10);

    // set bolts
    sprintf((char*)(uiElements[22 + (i*4) + 1] + 0x60), "%ld", pState->TotalBolts);

    // set revives
    sprintf((char*)(uiElements[22 + (i*4) + 3] + 0x60), "%d", pState->Revives);
  }
}

//--------------------------------------------------------------------------
void waitForMapConfig(PatchStateContainer_t * gameState)
{
  //int* state = 0x0021e684;
  if (*(u16*)0x004a7dec != 0xA9B0) return;

  if (!mapConfig || mapConfig->Magic != MAP_CONFIG_MAGIC) {
    
    // prevent game from finishing loading
    //POKE_U32(0x004A7FD4, 0);
    //POKE_U32(0x004A7FDC, 0);
    //POKE_U32(0x004A7FE4, 0);
    //POKE_U32(0x004A82E8, 0);
  } else if (!hasMapConfig()) {

    // call map code to let it initialize
    ((void (*)(void))EXTRA_CODE_SEG_PTR)();

  } else if (gameState->AllClientsReady && *(u16*)0x0021ddb4 == 6) {

    // let game load
    //POKE_U32(0x0021e680, 15);
    //POKE_U32(0x0021e684, 15);
    //POKE_U32(0x0021ddb4, 15);
  }
}

//--------------------------------------------------------------------------
void lobbyStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
  int i;
  int activeId = uiGetActive();
  static int initializedScoreboard = 0;

  // send final local player stats to others
  if (Initialized == 1) {
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      if (State.PlayerStates[i].IsLocal) {
        sendPlayerStats(i);
      }
    }
    
    bubbleDeinit();

    Initialized = 2;
  }

  // disable ranking
  gameSetIsGameRanked(0);

  // 
  updateGameState(gameState);

  // scoreboard
  switch (activeId)
  {
    case 0x15C:
    {
      if (initializedScoreboard)
        break;

      setEndGameScoreboard(gameState->GameConfig);
      initializedScoreboard = 1;

      // patch rank computation to keep rank unchanged for base mode
      POKE_U32(0x0077ACE4, 0x4600BB06);
      break;
    }
    case UI_ID_GAME_LOBBY:
    {
      setLobbyGameOptions(gameState->GameConfig);
      break;
    }
  }
}

//--------------------------------------------------------------------------
void loadStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
  // reset initialized on load
  // enables level hopping
  Initialized = 0;
  InitializeDelay = TPS * 0.2;
  mapLoadResetState(gameState);

  setLobbyGameOptions(gameState->GameConfig);
  
  // point get resurrect point to ours
  *(u32*)0x00610724 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
  *(u32*)0x005e2d44 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
}

//--------------------------------------------------------------------------
void start(struct GameModule * module, PatchStateContainer_t * gameState, enum GameModuleContext context)
{
  waitForMapConfig(gameState);
  switch (context)
  {
    case GAMEMODULE_LOBBY: lobbyStart(module, gameState); break;
    case GAMEMODULE_LOAD: loadStart(module, gameState); break;
    case GAMEMODULE_GAME_FRAME: gameStart(module, gameState); break;
    case GAMEMODULE_GAME_UPDATE: break;
    default: break;
  }
}
