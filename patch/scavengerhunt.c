#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/team.h>
#include <libdl/ui.h>
#include <libdl/pad.h>
#include <libdl/utils.h>
#include <libdl/graphics.h>
#include <libdl/player.h>
#include <libdl/collision.h>
#include <libdl/color.h>
#include <libdl/net.h>
#include <libdl/time.h>
#include <libdl/random.h>
#include <libdl/time.h>
#include "messageid.h"
#include "module.h"
#include "config.h"

#define HBOLT_MOBY_OCLASS				    (0x3004)
#define HBOLT_PICKUP_RADIUS			    (3)
#define HBOLT_PARTICLE_COLOR        (0x0000FFFF)
#define HBOLT_SPRITE_COLOR          (0x00131341)
#define HBOLT_SCALE                 (0.5)

#define HBOLT_MIN_SPAWN_DELAY_TPS   (60 * 10)
#define HBOLT_MAX_SPAWN_DELAY_TPS   (60 * 20)
#define HBOLT_SPAWN_PROBABILITY     (0.25)

extern PatchConfig_t config;
extern PatchGameConfig_t gameConfig;
extern PatchStateContainer_t patchStateContainer;
float scavHuntSpawnFactor = 1;
float scavHuntSpawnTimerFactor = 1;
int scavHuntShownPopup = 0;
int scavHuntHasGotSettings = 0;
int scavHuntEnabled = 0;
int scavHuntInitialized = 0;
int scavHuntBoltSpawnCooldown = 0;

struct PartInstance {	
	char IClass;
	char Type;
	char Tex;
	char Alpha;
	u32 RGBA;
	char Rot;
	char DrawDist;
	short Timer;
	float Scale;
	VECTOR Position;
	int Update[8];
};

struct HBoltPVar {
	int DestroyAtTime;
	struct PartInstance* Particles[4];
};

//--------------------------------------------------------------------------
void scavHuntResetBoltSpawnCooldown(void)
{
  scavHuntBoltSpawnCooldown = randRangeInt(HBOLT_MIN_SPAWN_DELAY_TPS, HBOLT_MAX_SPAWN_DELAY_TPS) / scavHuntSpawnTimerFactor;
}

//--------------------------------------------------------------------------
int scavHuntRoll(void)
{
  float roll = (randRange(0, 1) / scavHuntSpawnFactor);
  DPRINTF("scavenger hunt rolled %f => %d\n", roll, roll <= HBOLT_SPAWN_PROBABILITY);
  return roll <= HBOLT_SPAWN_PROBABILITY;
}

//--------------------------------------------------------------------------
int scavHuntOnReceiveRemoteSettings(void* connection, void* data)
{
  ScavengerHuntSettingsResponse_t response;
  memcpy(&response, data, sizeof(response));

  scavHuntEnabled = response.Enabled;
  scavHuntSpawnFactor = maxf(response.SpawnFactor, 0.1);
  scavHuntSpawnTimerFactor = clamp(scavHuntSpawnFactor, 1, 30);
  DPRINTF("received scav hunt %d %f\n", scavHuntEnabled, scavHuntSpawnFactor);
}

//--------------------------------------------------------------------------
void scavHuntQueryForRemoteSettings(void)
{
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  // reset, server will update
  scavHuntEnabled = 0;
  scavHuntSpawnFactor = 1;

  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_REQUEST_SCAVENGER_HUNT_SETTINGS, 0, NULL);
}

//--------------------------------------------------------------------------
void scavHuntSendHorizonBoltPickedUpMessage(void)
{
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_PICKED_UP_HORIZON_BOLT, 0, NULL);
}

//--------------------------------------------------------------------------
struct PartInstance * scavHuntSpawnParticle(VECTOR position, u32 color, char opacity, int idx)
{
	u32 a3 = *(u32*)0x002218E8;
	u32 t0 = *(u32*)0x002218E4;
	float f12 = *(float*)0x002218DC;
	float f1 = *(float*)0x002218E0;

	return ((struct PartInstance* (*)(VECTOR, u32, char, u32, u32, int, int, int, float))0x00533308)(position, color, opacity, a3, t0, -1, 0, 0, f12 + (f1 * idx));
}

//--------------------------------------------------------------------------
void scavHuntDestroyParticle(struct PartInstance* particle)
{
	((void (*)(struct PartInstance*))0x005284d8)(particle);
}

//--------------------------------------------------------------------------
void scavHuntHBoltDestroy(Moby* moby)
{
  if (mobyIsDestroyed(moby)) return;
	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;

	// destroy particles
  int i;
	for (i = 0; i < 4; ++i) {
		if (pvars->Particles[i]) {
			scavHuntDestroyParticle(pvars->Particles[i]);
			pvars->Particles[i] = NULL;
		}
	}

  mobyDestroy(moby);
}

//--------------------------------------------------------------------------
void scavHuntHBoltPostDraw(Moby* moby)
{
	struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
  int opacity = 0x80;
	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;
	if (!pvars)
		return;

	// determine color
	u32 color = HBOLT_SPRITE_COLOR;

	// fade as we approach destruction
	int timeUntilDestruction = (pvars->DestroyAtTime - gameGetTime()) / TIME_SECOND;
	if (timeUntilDestruction < 1)
		timeUntilDestruction = 1;

	if (timeUntilDestruction < 10) {
		float speed = timeUntilDestruction < 3 ? 20.0 : 3.0;
		float pulse = (1 + sinf((gameGetTime() / 1000.0) * speed)) * 0.5;
		opacity = 32 + (pulse * 96);
	}

  opacity = opacity << 24;
  color = opacity | (color & HBOLT_SPRITE_COLOR);
  moby->PrimaryColor = color;

  HOOK_JAL(0x205b64dc, 0x004e4d70);
  gfxDrawBillboardQuad(vector_read(moby->Position), HBOLT_SCALE * 0.6, opacity, 0, -MATH_PI / 2, 3, 1, 0);
  gfxDrawBillboardQuad(vector_read(moby->Position), HBOLT_SCALE * 0.5, color, 0, -MATH_PI / 2, 3, 1, 0);
  HOOK_JAL(0x205b64dc, 0x004c4200);
}

//--------------------------------------------------------------------------
void scavHuntHBoltUpdate(Moby* moby)
{
  const float rotSpeeds[] = { 0.05, 0.02, -0.03, -0.1 };
	const int opacities[] = { 64, 32, 44, 51 };

	VECTOR t;
	int i;
	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;
	if (!pvars)
		return;

	// register draw event
	gfxRegisterDrawFunction((void**)0x0022251C, (void*)&scavHuntHBoltPostDraw, moby);

	// handle particles
	u32 color = colorLerp(0, HBOLT_PARTICLE_COLOR, 1.0 / 4);
	color |= 0x80000000;
	for (i = 0; i < 4; ++i) {
		struct PartInstance * particle = pvars->Particles[i];
		if (!particle) {
			pvars->Particles[i] = particle = scavHuntSpawnParticle(moby->Position, color, opacities[i], i);
		}

		// update
		if (particle) {
			particle->Rot = (int)((gameGetTime() + (i * 100)) / (TIME_SECOND * rotSpeeds[i])) & 0xFF;
		}
	}

	// handle pickup
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* p = playerGetFromSlot(i);
    if (!p || playerIsDead(p)) continue;

    vector_subtract(t, p->PlayerPosition, moby->Position);
    if (vector_sqrmag(t) < (HBOLT_PICKUP_RADIUS * HBOLT_PICKUP_RADIUS)) {
      uiShowPopup(0, "You found a Horizon Bolt!");
      mobyPlaySoundByClass(1, 0, moby, MOBY_ID_PICKUP_PAD);
      scavHuntSendHorizonBoltPickedUpMessage();
      scavHuntHBoltDestroy(moby);
      break;
    }
  }

	// handle auto destruct
	if (pvars->DestroyAtTime && gameGetTime() > pvars->DestroyAtTime) {
		scavHuntHBoltDestroy(moby);
	}
}

//--------------------------------------------------------------------------
void scavHuntSpawn(VECTOR position)
{
  Moby* moby = mobySpawn(HBOLT_MOBY_OCLASS, sizeof(struct HBoltPVar));
  if (!moby) return;

  moby->PUpdate = &scavHuntHBoltUpdate;
  vector_copy(moby->Position, position);
  moby->UpdateDist = -1;
  moby->ModeBits = MOBY_MODE_BIT_UNK_40;
  moby->ModeBits2 = 0x10;
  moby->CollData = NULL;
  moby->DrawDist = 0;
  moby->PClass = NULL;
  moby->MClass = 26;
  moby->DeathCnt = 0;
  moby->AnimSeq = NULL;
  moby->AnimLayers = NULL;
  moby->AnimSeqId = moby->LSeq = 0;
  
	// update pvars
	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;
	pvars->DestroyAtTime = gameGetTime() + (TIME_SECOND * 30);
	memset(pvars->Particles, 0, sizeof(pvars->Particles));

	mobySetState(moby, 0, -1);
  scavHuntResetBoltSpawnCooldown();
  mobyPlaySoundByClass(0, 0, moby, MOBY_ID_NODE_BASE);
	DPRINTF("hbolt spawned at %08X destroyAt:%d %04X\n", (u32)moby, pvars->DestroyAtTime, moby->ModeBits);
}

//--------------------------------------------------------------------------
void scavHuntSpawnRandomNearPosition(VECTOR position)
{
  // try 4 times to generate a random point near given position
  int i = 0;
  while (i < 4)
  {
    // generate random position
    VECTOR from, to = {0,0,-6,0}, p = {0,0,1,0};
    float theta = randRadian();
    float radius = randRange(5, 10);
    vector_fromyaw(from, theta);
    vector_scale(from, from, radius);
    from[2] = 3;
    vector_add(from, from,  position);
    vector_add(to, to, from);

    // snap to ground
    // and check if ground is walkable
    if (CollLine_Fix(from, to, COLLISION_FLAG_IGNORE_DYNAMIC, NULL, NULL)) {
      int colId = CollLine_Fix_GetHitCollisionId() & 0x0f;
      if (colId == 0xF || colId == 0x7 || colId == 0x9 || colId == 0xA) {
        vector_add(p, p, CollLine_Fix_GetHitPosition());
        scavHuntSpawn(p);
        break;
      }
    }

    ++i;
  }
}

//--------------------------------------------------------------------------
void scavHuntSpawnRandomNearPlayer(int pIdx)
{
  Player* player = NULL;
  if (pIdx < 0) player = playerGetFromSlot(0);
  else player = playerGetAll()[pIdx];
  if (!player) return;

  scavHuntSpawnRandomNearPosition(player->PlayerPosition);
}

//--------------------------------------------------------------------------
void scavHuntOnKillDeathMessage(void* killDeathMsg)
{
  ((void (*)(void*))0x00621CF8)(killDeathMsg);

  if (scavHuntBoltSpawnCooldown || !scavHuntRoll()) return;

  Player* localPlayer = playerGetFromSlot(0);
  if (!localPlayer) return;
  char killer = *(char*)(killDeathMsg + 0);
  char killed = *(char*)(killDeathMsg + 2);
  if (killed >= 0 && killed < GAME_MAX_PLAYERS && killer == localPlayer->PlayerId) {
    scavHuntSpawnRandomNearPlayer(killed);
  }
}

//--------------------------------------------------------------------------
void scavHuntCheckForSurvivalSpawnConditions(void)
{
  static int lastSurvivalRound = 0;
  static int survivalBoltsSpawnedThisRound = 0;
  static int checkForMobyCooldown = 0;
  static Moby* survivalBoltMoby = NULL;

  if (gameConfig.customModeId != CUSTOM_MODE_SURVIVAL) return;

  // reset number of bolts spawned this round
  if (patchStateContainer.GameStateUpdate.RoundNumber != lastSurvivalRound) {
    lastSurvivalRound = patchStateContainer.GameStateUpdate.RoundNumber;
    survivalBoltsSpawnedThisRound = 0;
  }

  // limit # of bolts per round to 3
  // find zombie and wait for them to die
  if (survivalBoltsSpawnedThisRound < 3) {
    if (!scavHuntBoltSpawnCooldown && !survivalBoltMoby) {
      if (checkForMobyCooldown) {
        checkForMobyCooldown--;
      } else {
        if (scavHuntRoll()) { survivalBoltMoby = mobyFindNextByOClass(mobyListGetStart(), MOBY_ID_ROBOT_ZOMBIE); }
        checkForMobyCooldown = 60 * 15;
      }
    } else if (survivalBoltMoby) {
      if (mobyIsDestroyed(survivalBoltMoby) || survivalBoltMoby->OClass != MOBY_ID_ROBOT_ZOMBIE) {
        survivalBoltMoby = NULL;
      } else if (survivalBoltMoby->PVar) {
        struct TargetVars* targetVars = *(struct TargetVars**)survivalBoltMoby->PVar;
        if (targetVars && targetVars->hitPoints <= 0) {
          scavHuntSpawnRandomNearPosition(survivalBoltMoby->Position);
          ++survivalBoltsSpawnedThisRound;
          survivalBoltMoby = NULL;
        } else if (!targetVars) {
          survivalBoltMoby = NULL;
        }
      }
    }
  }
}

//--------------------------------------------------------------------------
void scavHuntRun(void)
{
  GameSettings* gs = gameGetSettings();
  GameData* gameData = gameGetData();
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_RESPONSE_SCAVENGER_HUNT_SETTINGS, &scavHuntOnReceiveRemoteSettings);

  // if the hunt is live then show a first time popup on the online lobby
  if (isInMenus() && uiGetActive() == UI_ID_ONLINE_MAIN_MENU && !scavHuntShownPopup && !config.disableScavengerHunt && scavHuntEnabled) {
    uiShowOkDialog("Scavenger Hunt", "The Horizon Scavenger Hunt is live! Hunt for Horizon Bolts for a chance to win prizes! Join our discord for more info: discord.gg/horizonps");
    scavHuntShownPopup = 1;
  }

  // request settings on first download of the patch
  if (!scavHuntHasGotSettings) {
    scavHuntHasGotSettings = 1;
    scavHuntQueryForRemoteSettings();
  }

  // only continue if enabled and in game
  if (config.disableScavengerHunt || !isInGame() || !gs || !gameData) {
    scavHuntInitialized = 0;
    return;
  }

#if DEBUG
  if (padGetButtonDown(0, PAD_DOWN) > 0) {
    scavHuntSpawnRandomNearPlayer(0);
  }
#endif

  // disabled
  if (!scavHuntEnabled) { scavHuntShownPopup = 0; return; }
  if (scavHuntSpawnFactor <= 0) { scavHuntShownPopup = 0; return; }

  // reset cooldown on beginning of game
  if (!scavHuntInitialized) {
    scavHuntResetBoltSpawnCooldown();
    scavHuntInitialized = 1;
  }

  Player* localPlayer = playerGetFromSlot(0);
  if (!localPlayer) return;

  // we need at least 3 unique clients
  // unless playing survival
#if !DEBUG
  if (gameConfig.customModeId != CUSTOM_MODE_SURVIVAL) {
    int i;
    int uniqueClients = 0;
    char clientIds[GAME_MAX_PLAYERS] = {0,0,0,0,0,0,0,0,0,0};

    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      int clientId = gs->PlayerClients[i];
      if (clientId < 0) continue;

      if (clientIds[clientId]++ == 0) {
        uniqueClients += 1;
      }
    }

    if (uniqueClients < 3) return;
  }
#endif

  // hooks
  HOOK_JAL(0x00621c7c, &scavHuntOnKillDeathMessage);

  // use stats to detect when objective event happens
  static int lastFlagsCaptured = 0;
  static int lastNodesCaptured = 0;
  static int lastNodesSaved = 0;
  static int lastHillTime = 0;
  int flagsCaptured = gameData->PlayerStats.CtfFlagsCaptures[localPlayer->PlayerId];
  int nodesCaptured = gameData->PlayerStats.ConquestNodesCaptured[localPlayer->PlayerId];
  int nodesSaved = gameData->PlayerStats.ConquestNodeSaves[localPlayer->PlayerId];
  int hillTime = (int)(gameData->PlayerStats.KingHillHoldTime[localPlayer->PlayerId] / (60 / scavHuntSpawnTimerFactor));

  // decrement cooldown or check for event
  if (scavHuntBoltSpawnCooldown) {
    --scavHuntBoltSpawnCooldown;
  } else if (flagsCaptured > lastFlagsCaptured && scavHuntRoll()) {
    scavHuntSpawnRandomNearPlayer(localPlayer->PlayerId);
  } else if (nodesCaptured > lastNodesCaptured && scavHuntRoll()) {
    scavHuntSpawnRandomNearPlayer(localPlayer->PlayerId);
  } else if (nodesSaved > lastNodesSaved && scavHuntRoll()) {
    scavHuntSpawnRandomNearPlayer(localPlayer->PlayerId);
  } else if (hillTime > lastHillTime && scavHuntRoll()) {
    scavHuntSpawnRandomNearPlayer(localPlayer->PlayerId);
  }

  scavHuntCheckForSurvivalSpawnConditions();

  lastFlagsCaptured = flagsCaptured;
  lastNodesCaptured = nodesCaptured;
  lastNodesSaved = nodesSaved;
  lastHillTime = hillTime;
}
