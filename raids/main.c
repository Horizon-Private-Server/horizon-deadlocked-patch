/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		RAIDS.
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

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
#include <libdl/stdlib.h>
#include <libdl/guber.h>
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/random.h>
#include <libdl/net.h>
#include <libdl/sound.h>
#include <libdl/dl.h>
#include <libdl/utils.h>
#include <libdl/collision.h>
#include <libdl/radar.h>
#include <libdl/music.h>
#include "module.h"
#include "messageid.h"
#include "config.h"
#include "common.h"
#include "include/game.h"
#include "include/hop.h"
#include "include/bank.h"
#include "include/loot.h"
#include "include/mob.h"
#include "include/bubble.h"
#include "include/stats.h"
#include "include/utils.h"

char LocalPlayerStrBuffer[2][64];
int Initialized = 0;
int FirstTimeInitialized = 0;

struct RaidsState State;
struct RaidsMapConfig* mapConfig = (struct RaidsMapConfig*)(EXTRA_CODE_SEG_PTR + 0x10);

struct RaidsSnackItem snackItems[SNACK_ITEM_MAX_COUNT] = {};
int snackItemsCount = 0;

PatchConfig_t* playerConfig = NULL;

float Difficulties[RAIDS_DIFFICULTY_COUNT] = {
  [RAIDS_DIFFICULTY_1STAR] 0,
  [RAIDS_DIFFICULTY_2STAR] 25.0,
  [RAIDS_DIFFICULTY_3STAR] 150.0,
  [RAIDS_DIFFICULTY_4STAR] 500.0,
  [RAIDS_DIFFICULTY_5STAR] 1250.0,
};

int Lives[RAIDS_DIFFICULTY_COUNT] = {
  [RAIDS_DIFFICULTY_1STAR] 2,
  [RAIDS_DIFFICULTY_2STAR] 2,
  [RAIDS_DIFFICULTY_3STAR] 2,
  [RAIDS_DIFFICULTY_4STAR] 2,
  [RAIDS_DIFFICULTY_5STAR] 2,
};

float VehicleHealthMultipliers[] = {
  [0] 2.0, // puma
  [1] 3.0, // hovership
  [2] 0.0,
  [3] 0.0,
  [4] 0.0,
  [5] 3.0, // landstalker
  [6] 1.0, // hoverbike
};

float VehicleDriverDamage[] = {
  [0] 50.0, // puma
  [1] 75.0, // hovership
  [2] 0.0,
  [3] 0.0,
  [4] 0.0,
  [5] 50.0, // landstalker
  [6] 50.0, // hoverbike
};

float VehiclePassengerDamage[] = {
  [0] 100.0, // puma
  [1] 120.0, // hovership
  [2] 0.0,
  [3] 0.0,
  [4] 0.0,
  [5] 120.0, // landstalker
  [6] 0.0, // hoverbike
};

//--------------------------------------------------------------------------
void respawnDeadPlayers(void) {
	int i;
	Player** players = playerGetAll();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		if (playerIsValid(p) && playerIsDead(p)) {
      playerRespawn(p);
		}
	}
}

//--------------------------------------------------------------------------
int onMissionFailedRemote(void * connection, void * data)
{
  State.MissionStatus = RAIDS_MISSION_FAILED;
  musicPlayTrack(0x9A, 0);
  DPRINTF("recv mission failed\n");
  return 0;
}

//--------------------------------------------------------------------------
int onUseLifeRemote(void * connection, void * data)
{
  respawnDeadPlayers();
  State.LivesLeft--;
  DPRINTF("recv use life\n");
  return 0;
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
void uiShowLowerPopup(int localPlayerIdx, int msgStringId)
{
	((void (*)(int, int, int))0x0054ea30)(localPlayerIdx, msgStringId, 0);
}

//--------------------------------------------------------------------------
void popSnack(void)
{
  memmove(&snackItems[0], &snackItems[1], sizeof(struct RaidsSnackItem) * (SNACK_ITEM_MAX_COUNT-1));
  memset(&snackItems[SNACK_ITEM_MAX_COUNT-1], 0, sizeof(struct RaidsSnackItem));

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

  strncpy(snackItems[snackItemsCount].Str, str, sizeof(snackItems[snackItemsCount].Str));
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
  strncpy(a, snackItems[0].Str, sizeof(snackItems[0].Str));

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
u64 missionCompleteGetBoltReward(void)
{
  Player* player = playerGetFromSlot(0);
  u64 bolts = State.PlayerStates[player->PlayerId].State.Bolts >> 2;
  bolts -= bolts % 1000;
  if (bolts < 10000) bolts = 10000;
  if (bolts > 100000) bolts = 100000;
  return bolts;
}

//--------------------------------------------------------------------------
float missionCompleteGetXpReward(void)
{
  Player* player = playerGetFromSlot(0);

  // xp is based on amount of xp earned during gameplay
  // clamp between 5000 and 100000
  float xp = 500 + (State.PlayerStates[player->PlayerId].State.Experience / 2);
  xp -= (long)xp % 100;
  if (xp < 1000) xp = 1000;
  if (xp > 10000) xp = 10000;
  return xp;
}

//--------------------------------------------------------------------------
void drawMissionCompleteMessage(void)
{
  if (hasPendingWorldHop()) return;
  if (State.MenuOpen) return;

  float x = SCREEN_WIDTH * 0.5, y = SCREEN_HEIGHT * 0.16;
  char strBuf[64];

  drawStars(x, y, 0, 0, 24, 6, 0x80008080, TEXT_ALIGN_MIDDLECENTER, State.DifficultyStars + 1);
  snprintf(strBuf, sizeof(strBuf), "%s", State.CurrentMapDef->Name);
  gfxHelperDrawText(x, y, 0, 30, 1.5, 0x80FFFFFF, strBuf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  int time = State.MissionCompleteTime - State.MissionStartTime;
  snprintf(strBuf, sizeof(strBuf), "Completed in %02d:%02d", time / TIME_MINUTE, (time % TIME_MINUTE) / TIME_SECOND);
  gfxHelperDrawText(x, y, 0, 50, 0.9, 0x80FFFFFF, strBuf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  //snprintf(strBuf, sizeof(strBuf), "\x0A+%'ld\x08 Bolts        \x0A+%'ld\x08 XP", missionCompleteGetBoltReward(), missionCompleteGetXpReward());
  //gfxHelperDrawText(x, y, 0, 65, 0.7, 0x80FFFFFF, strBuf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  snprintf(strBuf, sizeof(strBuf), "Press [UP] to open the Planet Select menu");
  gfxHelperDrawText(x, SCREEN_HEIGHT - 20, 0, 0, 0.7, 0x80FFFFFF, strBuf, -1, TEXT_ALIGN_BOTTOMCENTER, COMMON_DZO_DRAW_NORMAL);
}

//--------------------------------------------------------------------------
void drawTimer(int time)
{
  char buf[32];

  if (time < 0) time = 0;
  snprintf(buf, sizeof(buf), "%02d:%02d", time / TIME_MINUTE, (time % TIME_MINUTE) / TIME_SECOND);
  gfxHelperDrawText(SCREEN_WIDTH - 15, 105, 0, 0, 0.9, 0x80E0E0E0, buf, -1, TEXT_ALIGN_TOPRIGHT, COMMON_DZO_DRAW_NORMAL);
}

//--------------------------------------------------------------------------
int collisionIdIsWalkable(int collisionId)
{
  collisionId &= 0x0f;
  return collisionId == 0x03 || collisionId == 0x07 || collisionId == 0x09 || collisionId == 0x0A || collisionId == 0x0E || collisionId == 0x0F;
}

//--------------------------------------------------------------------------
void onMissionComplete(int cuboidIdx)
{
  int lootCount = 2;
  Player* player = playerGetFromSlot(0);
  VECTOR pos;
  VECTOR up = {0,0,5,0};

  // play challenge complete track
  musicPlayTrack(0x98, 0);

  // all players should be alive
  respawnDeadPlayers();

  // # of loot drops is based on duration of run
  // the longer the run the more drops
  // to equalize time invested in a run, vs the payout at the end
  // 0-9 min = 1 drop
  // 9-27 min = 2 drops
  // 27+ min = 3 drops
  // with some randomness
  float minutes = (State.MissionCompleteTime - State.MissionStartTime) / TIME_MINUTE;
  lootCount = (int)clamp(logf(minutes + randRange(0, 5)) / logf(3), 1, 3) + rand(3);

  SpawnPoint* cuboid = spawnPointGet(cuboidIdx);

  // spawn N loot drops near local player 0
  int i;
  for (i = 0; i < lootCount; ++i) {

    // try get random nearby pos above solid ground
    int r = 0;
    for (r = 0; r < 5; ++r) {

      if (cuboidIdx < 0) {
        vector_fromyaw(pos, randRadian());
        vector_scale(pos, pos, randRange(5, 15));
        vector_add(pos, pos, up);
        vector_add(pos, pos, player->PlayerPosition);
      } else {
        pos[0] = randRange(-1, 1);
        pos[1] = randRange(-1, 1);
        pos[2] = 1;
        vector_apply(pos, pos, cuboid->M0);
      }

      VECTOR to = {0,0,-50,0};
      vector_add(to, to, pos);
      if (CollLine_Fix(pos, to, COLLISION_FLAG_IGNORE_DYNAMIC, NULL, NULL)) {
        if (collisionIdIsWalkable(CollLine_Fix_GetHitCollisionId())) {
          // found spot above ground
          vector_copy(pos, CollLine_Fix_GetHitPosition());
          pos[2] += 1;
          break;
        }
      }
    }

    lootRequestFromMissionComplete(pos);
  }

  // 
  mapConfig->BankVTable->AddBolts(missionCompleteGetBoltReward());
  //bankAddXP(missionCompleteGetXpReward());

  // send time to server
  struct RaidsSetMissionCompleteRequest msg;
  void* connection = netGetLobbyServerConnection();
  if (!connection) return;

  msg.TimeMs = State.MissionCompleteTime - State.MissionStartTime;
  msg.Difficulty = State.DifficultyStars;
  strncpy(msg.MapFilename, State.CurrentMapDef->Filename, sizeof(msg.MapFilename));
  netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_RAIDS_SET_MISSION_COMPLETED_REQUEST, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
void onMissionFail(void)
{
  State.MissionStatus = RAIDS_MISSION_FAILED;
  musicPlayTrack(0x9A, 0);
  DPRINTF("recv mission failed\n");

  // explode all players
  int i;
  Player** players = playerGetAll();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (player) player->timers.explodeTimer = TPS;
  }
}

//--------------------------------------------------------------------------
void missionCheckForMissionFailed(void)
{
  if (!missionIsActive()) return;

  int failed = !State.OnHubWorld && State.ClientsReady && State.TicksWithNoLivingPlayers > TPS && !State.LivesLeft && State.ActivePlayerCount && !hasPendingWorldHop();
  if (!failed) return;

  // broadcast
  void* connection = netGetDmeServerConnection();
  if (connection) {
    netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, connection, CUSTOM_MSG_SET_MISSION_FAILED, 0, NULL);
  }

  onMissionFail();
}

//--------------------------------------------------------------------------
void missionUseLife(void)
{
  if (State.LivesLeft <= 0) return;
  if (missionIsActive()) {
    
    // broadcast
    void* connection = netGetDmeServerConnection();
    if (connection) {
      netBroadcastCustomAppMessage(NET_DELIVERY_CRITICAL, connection, CUSTOM_MSG_USE_LIFE, 0, NULL);
    }

    State.LivesLeft--;
  }

  respawnDeadPlayers();
}

//--------------------------------------------------------------------------
struct Guber* getGuber(Moby* moby)
{
  if (!moby) return NULL;

	//if (moby->OClass == UPGRADE_MOBY_OCLASS && moby->PVar)
	//	return moby->GuberMoby;
	
	return 0;
}

//--------------------------------------------------------------------------
int handleEvent(Moby* moby, GuberEvent* event)
{
	if (!moby || !event || !isInGame())
		return 0;

  if (mobyIsMob(moby)) return mobHandleEvent(moby, event);

	return 0;
}

//--------------------------------------------------------------------------
void vehicleReinitPhysicsPost(Vehicle* vehicle)
{
	// pointer to gameplay data is stored in $s1
	asm volatile (
		"move %0, $s1"
		: : "r" (vehicle)
	);

  vehicle->maxHP = 10000;
  vehicle->hitPoints = 10000;
  vehicle->fDriverAttackDamage = VehicleDriverDamage[vehicle->vehicleType];
  vehicle->fPassengerAttackDamage = VehiclePassengerDamage[vehicle->vehicleType];
}

//--------------------------------------------------------------------------
int shouldDrawHud(void)
{
  PlayerHUDFlags* hudFlags = hudGetPlayerFlags(0);
  return hudFlags && hudFlags->Flags.Raw != 0;
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
	int localPlayerIndex, heldWeapon;
	Player** players = playerGetAll();
	Player* player = players[pIndex];
	struct RaidsPlayer * playerData = &State.PlayerStates[pIndex];

	if (!playerIsValid(player))
		return;

  // respawn if mission completed
  if (playerIsDead(player) && missionIsComplete())
    playerRespawn(player);

	int actionCooldownTicks = decTimerU8(&playerData->ActionCooldownTicks);
	int messageCooldownTicks = decTimerU8(&playerData->MessageCooldownTicks);
  
  // last hit
  if (player->Health != playerData->LastHealth) playerData->TicksSinceHealthChanged = 0;
  else playerData->TicksSinceHealthChanged += 1;
  playerData->LastHealth = player->Health;

  // player speed
	player->Speed = 1 + (PLAYER_SKILLPOINT_SPEED_FACTOR * State.PlayerStates[pIndex].State.Skills[RAIDS_SKILLS_SPEED]);

	// set max health
  float cmodHealthBuff = BADGE_HEALTH_BUFF_AMOUNT * mapConfig->BankVTable->GetEquippedBadgeEffectStrength(pIndex, RAIDS_BADGE_TYPE_HEATH_BUFF);
	player->MaxHealth = 50 + cmodHealthBuff + (PLAYER_SKILLPOINT_HEALTH_FACTOR * State.PlayerStates[pIndex].State.Skills[RAIDS_SKILLS_HEALTH]);

  // set vehicle max health if driver
  Vehicle* vehicle = player->Vehicle;
  if (vehicle && player->InVehicle && vehicle->pDriver == player) {
    vector_write(player->Velocity, 0); // make sure predictive movements aren't used
    vehicle->maxHP = player->MaxHealth * VehicleHealthMultipliers[vehicle->vehicleType];
    if (vehicle->hitPoints > vehicle->maxHP)
      vehicle->hitPoints = vehicle->maxHP;
  }

  // update death state
  if (!playerData->IsDead && playerIsDead(player)) {
    playerData->IsDead = 1;
  } else if (playerData->IsDead && !playerIsDead(player)) {
    playerData->IsDead = 0;
  }

	if (player->IsLocal) {
		
		GadgetBox* gBox = player->GadgetBox;
		localPlayerIndex = player->LocalPlayerIndex;
		heldWeapon = player->WeaponHeldId;

	  // set max xp
		u64 xp = 0; // mapConfig->BankVTable->GetXP();
    int level = getLevelFromXp(xp);
		u64 lastXp = getXpForLevel(level);
		u64 nextXp = getXpForLevel(level + 1);
    if (xp < lastXp) xp = lastXp;
    float xpPerc = (float)((xp - lastXp) / (double)(nextXp - lastXp));
    //DPRINTF("lvl:%d perc:%f %ld=>%ld xp:%ld\n", level, xpPerc, lastXp, nextXp, xp);
		setPlayerEXP(localPlayerIndex, xpPerc);

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

		if (messageCooldownTicks == 1) {
      hudHidePopup();
		}
	} else {

    // bug where players on GREEN or higher will remain cranking bolt after finishing
    // so we'll check to see if their remote player state is no longer cranking
    // and we'll stop them
    int remoteState = *(int*)((u32)player + 0x3a80);
    int playerStateTimer = player->timers.state; // playerStateTimers[pIndex];
    if (player->PlayerState == PLAYER_STATE_BOLT_CRANK
     && remoteState != PLAYER_STATE_BOLT_CRANK
     && playerStateTimer > TPS*3) {

			PlayerVTable* vtable = playerGetVTable(player);
      vtable->UpdateState(player, remoteState, 1, 1, 1);
    }
  }
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
void initialize(PatchStateContainer_t* gameState)
{
	static int startDelay = TPS * 0.2;
	static int waitingForClientsReady = 0;
  static int firstTime = 1;
	char hasTeam[10] = {0,0,0,0,0,0,0,0,0,0};
	Player** players = playerGetAll();
	int i;

	// Disable normal game ending
	*(u32*)0x006219B8 = 0;	// survivor (8)
	*(u32*)0x00620F54 = 0;	// time end (1)
	*(u32*)0x00621568 = 0;	// kills reached (2)
	*(u32*)0x006211A0 = 0;	// all enemies leave (9)
  *(u32*)0x006210D8 = 0;	// all enemies leave (9)

  // disable MP dialog
  POKE_U32(0x004e3960, 0);

  if (firstTime) {
    firstTime = 0;
    
    // clear state
    memset(&State, 0, sizeof(State));
    
    // load map stats
    if (PATCH_INTEROP) hopLoadMapStats(PATCH_INTEROP->MapLoaderFilename);
  }

  // disable timebase query percentile filter
  // always accept remote time
  POKE_U32(0x01eabd60, 0);

  // disable timebandits hack
  POKE_U32(0x0015B118, 0);

  HOOK_J(0x004546EC, &vehicleReinitPhysicsPost); // puma
  HOOK_J(0x00465244, &vehicleReinitPhysicsPost); // hoverbike
  HOOK_J(0x0047da18, &vehicleReinitPhysicsPost); // landstalker
  HOOK_J(0x0046ED14, &vehicleReinitPhysicsPost); // hovership
  POKE_U32(0x005F6488, 0); // enable vehicle targeting non-players

  // disable guber event delay until createTime+relDispatchTime reached
  // when players desync, their net time falls behind everyone else's
  // causing events that they receive to be delayed for long periods of time
  // leading to even more desyncing issues
  // since raids can cause a lot of frame lag, especially for players on emu/dzo
  // this fix is required to ensure that important mob guber events trigger on everyone's screen
  //POKE_U32(0x00611518, 0x24040000);

  // hook net messages
	netInstallCustomMsgHandler(CUSTOM_MSG_MOB_UNRELIABLE_MSG, &mobOnUnreliableMsgRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_SET_MISSION_FAILED, &onMissionFailedRemote);
  netInstallCustomMsgHandler(CUSTOM_MSG_USE_LIFE, &onUseLifeRemote);

  // component init
  mobInitialize();
  bubbleInit();
  lootInit();
  hopInit();
  statsInit();

	// change hud
	forcePlayerHUD();
  setPlayerEXP(0, 0);
  setPlayerEXP(1, 0);

  // 
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* p = playerGetFromSlot(i);
    if (playerIsValid(p)) {
      int pIdx = p->PlayerId;
      playerSetLocalEquipslot(i, 0, State.PlayerStates[pIdx].LastEquipslots[0]);
      playerSetLocalEquipslot(i, 1, State.PlayerStates[pIdx].LastEquipslots[1]);
      playerSetLocalEquipslot(i, 2, State.PlayerStates[pIdx].LastEquipslots[2]);
    }
  }

  // wait for map code seg to load
  if (!hasMapConfig()) {
    return;
  }
  
  // write map config
  mapConfig->State = &State;
  mapConfig->PushSnackFunc = &pushSnack;
  mapConfig->PushDamageBubbleFunc = &bubblePush;
  mapConfig->GetAmmoRefillCostFunc = &getAmmoRefillCost;
  mapConfig->BeginWorldHopFunc = &hopBegin;
  mapConfig->PopulateSpawnArgsFunc = &mobPopulateSpawnArgsFromConfig;
  mapConfig->RegisterNpcFunc = &mobRegisterNpc;
  mapConfig->OnGetGuberFunc = &getGuber;
  mapConfig->OnGuberEventFunc = &handleEvent;
  mapConfig->TryCreateMobFunc = &mobCreate;
  mapConfig->RequestPrestigeLootFunc = &lootRequestFromPrestige;
  mapConfig->OnMissionCompleteFunc = &onMissionComplete;
  mapConfig->OnMissionFailFunc = &onMissionFail;

	// set game over string
	//strncpy(uiMsgString(0x3477), RAIDS_GAME_OVER, strlen(RAIDS_GAME_OVER)+1);

  // prevent player from doing anything
  padDisableInput();

  if (startDelay) {
    --startDelay;
    return;
  }

  // wait for all clients to be ready
  // or for 5 seconds
  if (!gameState->AllClientsReady && waitingForClientsReady < (5 * TPS)) {
    uiShowPopup(0, "Waiting For Players...");
    ++waitingForClientsReady;
    return;
  }

  // hide waiting for players popup
  hudHidePopup();

  //
  State.ClientsReady = 1;
  mapConfig->ClientsReady = 1;
  if (State.MissionStartTime <= 0 && isInGame()) {
    State.MissionStartTime = gameGetTime();
  }

  // re-enable input
  padEnableInput();

  memset(snackItems, 0, sizeof(snackItems));

	// initialize player states
  State.Difficulty = Difficulties[State.DifficultyStars];
	State.LocalPlayerState = NULL;
	State.NumTeams = 0;
  State.TicksWithNoLivingPlayers = 0;
	State.AlivePlayerCount = -1;
	State.ActivePlayerCount = 0;
  State.LivesLeft = Lives[State.DifficultyStars];
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];

#if PAYDAY
		//State.PlayerStates[i].State.CurrentTokens = 1000;
#endif

		if (p) {

      // set max health
      p->Health = p->MaxHealth = 50 + (PLAYER_SKILLPOINT_HEALTH_FACTOR * State.PlayerStates[i].State.Skills[RAIDS_SKILLS_HEALTH]);

      State.PlayerStates[i].IsDead = 0;
      State.PlayerStates[i].State.Bolts = 0;
      State.PlayerStates[i].State.Experience = 0;
      State.PlayerStates[i].State.Kills = 0;
      State.PlayerStates[i].State.Deaths = 0;
      memset(State.PlayerStates[i].State.AllKills, 0, sizeof(State.PlayerStates[i].State.AllKills));

			// is local
			State.PlayerStates[i].IsLocal = p->IsLocal;
			if (p->IsLocal && !State.LocalPlayerState)
				State.LocalPlayerState = &State.PlayerStates[i];
				
			if (!hasTeam[p->Team]) {
				State.NumTeams++;
				hasTeam[p->Team] = 1;
			}

			++State.ActivePlayerCount;
		}
	}

	// initialize state
  State.DesiredMusicTrack = -1;
  State.DesiredMusicTrackForce = 0;
  State.DesiredMusicTrackSkipTransition = 0;
  State.AmmoDropChance = GAME_DEFAULT_AMMO_DROP_CHANCE;
  State.AmmoRefillCostMultiplier = 1;
	State.MobStats.MobsDrawnCurrent = 0;
	State.MobStats.MobsDrawnLast = 0;
	State.MobStats.MobsDrawGameTime = 0;
  memset(State.MobStats.NumAlive, 0, sizeof(State.MobStats.NumAlive));
  memset(State.MobStats.NumSpawnedThisRound, 0, sizeof(State.MobStats.NumSpawnedThisRound));
  State.MobStats.TotalAlive = 0;
  State.MobStats.TotalSpawning = 0;
  State.MobStats.TotalSpawned = 0;
  
	if (!FirstTimeInitialized) {
    State.InitializedTime = gameGetTime();
  }

	Initialized = 1;
  FirstTimeInitialized = 1;
}

//--------------------------------------------------------------------------
void updateGameState(PatchStateContainer_t * gameState)
{
	int i;

	// game state update
	if (gameState->UpdateGameState)
	{
		//gameState->GameStateUpdate.RoundNumber = State.RoundNumber + 1;
	}

  if (isInGame()) {
    // update custom game stats after each round
    // static int roundCompleted = 0;
    // if (roundCompleted != State.RoundNumber && gameAmIHost()) {
    //   gameState->UpdateCustomGameStats = 1;
    //   roundCompleted = State.RoundNumber;
    // }
  }

	// stats
	if (gameState->UpdateCustomGameStats)
	{
    gameState->CustomGameStatsSize = sizeof(struct RaidsGameData);
		struct RaidsGameData* sGameData = (struct RaidsGameData*)gameState->CustomGameStats.Payload;
		sGameData->Version = 1;

    // set per player stats
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			sGameData->Kills[i] = State.PlayerStates[i].State.Kills;
			sGameData->Deaths[i] = State.PlayerStates[i].State.Deaths;
		}
	}
}

//--------------------------------------------------------------------------
void gameStart(struct GameModule * module, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player ** players = playerGetAll();
	int i;
	int gameTime = gameGetTime();
  static int sendBankAtEnd = 1;

	// first
	dlPreUpdate();

	// 
	updateGameState(gameState);

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

	// Determine if host
	State.IsHost = gameAmIHost();
  playerConfig = gameState->Config;

	if (!Initialized) {
    State.MissionStartTime = 0;
		initialize(gameState);
		return;
	}

  if (!hasMapConfig()) {
    return;
  }

  // prevent input in menus
  if (State.MenuOpen) {
    for (i = 0; i < GAME_MAX_LOCALS; ++i) {
      Player* local = playerGetFromSlot(i);
      if (local) {
        local->timers.noInput = 10;
        local->timers.noCamInputTimer = 10;
      }
    }
  }

  // get local player data
  //struct RaidsPlayer* localPlayerData = &State.PlayerStates[localPlayer->PlayerId];

#if DEBUG_SOUNDS
  {
    static int aaa = 0;
    int play = 0;
		if (padGetButtonDown(0, PAD_RIGHT) > 0) {
			aaa += 1;
      play = 1;
		} else if (padGetButtonDown(0, PAD_LEFT) > 0) {
			aaa -= 1;
      play = 1;
		} else if (padGetButtonDown(0, PAD_UP) > 0) {
      play = 1;
		}

    if (play) {
      //def.Index = aaa;
      //soundPlay(&def, 0, playerGetFromSlot(0)->PlayerMoby, NULL, 0x400);
      //mobyPlaySoundByClass(aaa, 0, playerGetFromSlot(0)->PlayerMoby, MOBY_ID_WEAPON_PICKUP);
      musicPlayTrack(aaa*2, 1);
			printf("%d 0x%x\n", aaa*2, aaa*2);
    }
  }
#endif

#if DEBUG_ANIMS
  {
    static int aaa = 0;
    Moby* animMoby = mobyFindNextByOClass(mobyListGetStart(), 8353);
    if (animMoby && animMoby->PClass) {
      int play = 0;
      if (padGetButtonDown(0, PAD_RIGHT) > 0) {
        aaa += 1;
        play = 1;
      } else if (padGetButtonDown(0, PAD_LEFT) > 0) {
        aaa -= 1;
        play = 1;
      }

      int animCount = *(char*)(animMoby->PClass + 0x0C);
      if (play && animCount > 0) {
        if (aaa >= animCount) aaa = animCount - 1;
        if (aaa < 0) aaa = 0;
        mobyAnimTransition(animMoby, aaa, 0, 0);
			  printf("anim %d 0x%x (of %d)\n", aaa, aaa, animCount);
      }
    }
  }
#endif

#if DEBUG_JOINTS
  {
    int i = 0;
    char buf[32];
    int animJointCount = 0;
    Moby* jointMoby = mobyFindNextByOClass(mobyListGetStart(), 8353);
    MATRIX jointMtx;

    if (jointMoby) {
      // get anim joint count
      void* pclass = jointMoby->PClass;
      if (pclass) {
        animJointCount = **(u32**)((u32)pclass + 0x1C);
      }

      for (i = 0; i < animJointCount; ++i) {
        snprintf(buf, sizeof(buf), "%d", i);
        mobyGetJointMatrix(jointMoby, i, jointMtx);
        int x,y;
        if (gfxWorldSpaceToScreenSpace(&jointMtx[12], &x, &y)) {
          gfxScreenSpaceText(x, y, 0.5, 0.5, 0x80FFFFFF, buf, -1, 4);
        }
      }
    }
  }
#endif

  // play music
  if (isInGame() && musicIsLoaded()) {

    /*
    static int aaa = 150;
    int play = 0;
    if (padGetButtonDown(0, PAD_LEFT) > 0 && aaa > 0) {
      aaa -= 2;
      play = 1;
    } else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
      aaa += 2;
      play = 1;
    } else if (padGetButtonDown(0, PAD_DOWN) > 0) {
      play = 1;
    }

    if (play) {
      State.DesiredMusicTrack = aaa;
      State.DesiredMusicTrackSkipTransition = 1;
      State.DesiredMusicTrackForce = 1;
    }
    */

    static int lastDesiredTrack = -1;
    int currentTrack = musicGetCurrentTrack();

    // if failed or completed, don't update tracks anymore
    if (State.MissionStatus != RAIDS_MISSION_ACTIVE) {
      State.DesiredMusicTrack = -1;
      currentTrack = -1;
    }

    // prevent a blacklisted track from playing
    // unless it was explicitly requested
    if (mapConfig && mapConfig->TrackWhitelistEnabled && currentTrack >= 0) {
      if (lastDesiredTrack != currentTrack) {
        int trackIsWhitelisted = 0;
        for (i = 0; i < mapConfig->TrackWhitelistCount; ++i) {
          if (mapConfig->TrackWhitelist[i] == currentTrack) {
            trackIsWhitelisted = 1;
            break;
          }
        }

        if (!trackIsWhitelisted) {
          if (mapConfig->TrackWhitelistCount <= 0) {
            musicStopTrack();
            DPRINTF("bad track %d, pausing\n", currentTrack);
          } else {
            int randomTrackId = mapConfig->TrackWhitelist[rand(mapConfig->TrackWhitelistCount)];
            DPRINTF("bad track %d, playing %d\n", currentTrack, randomTrackId);
            musicPlayTrack(randomTrackId, 1);
          }
        }
      }
    }
    
    if (State.DesiredMusicTrack >= 0 && (currentTrack != State.DesiredMusicTrack || State.DesiredMusicTrackForce)) {
      
      DPRINTF("music play track %d=>%d (immediate:%d)\n", currentTrack, State.DesiredMusicTrack, State.DesiredMusicTrackSkipTransition);
      if (State.DesiredMusicTrackSkipTransition) {
        musicPlayTrack(State.DesiredMusicTrack, 1);
      } else {
        musicTransitionTrack(0, State.DesiredMusicTrack, 0x400, 0x400);
        POKE_U16(0x00206990, State.DesiredMusicTrack);
      }

      lastDesiredTrack = State.DesiredMusicTrack;
      if (!State.DesiredMusicLoop) {
        State.DesiredMusicTrack = -1;
        State.DesiredMusicTrackSkipTransition = 0;
      }
      State.DesiredMusicTrackForce = 0;
    }
  }

  // get bank on first load
  // send bank when game ends
  struct BankVTable* bankVTable = mapConfig->BankVTable;
  if (bankVTable) {
    if (!gameHasEnded()) {
      if (!bankVTable->GetHasAccount() && !bankVTable->HasPendingAccountRequest()) {
        bankVTable->RequestAccountFromServer();
      }
      if (!bankVTable->GetHasInventory() && !bankVTable->HasPendingInventoryRequest()) {
        bankVTable->RequestInventoryFromServer();
      }
    } else if (sendBankAtEnd) {
      if (bankVTable->GetHasAccount()) bankVTable->SendAccountToServer();
      //bankSendInventoryToServer();
      sendBankAtEnd = 0;
    }
    
    // bolts
    POKE_U32(0x00171b40, bankVTable->GetBolts());
  }

  if (!State.GameOver)
  {
    forcePlayerHUD();
    drawSnack();
    missionCheckForMissionFailed();

    // draw hud
    if (!gameIsAnyStartMenuOpen()) {
      int timer = -1;
      if (missionIsComplete()) {
        timer = State.MissionCompleteTime - State.MissionStartTime;
        drawMissionCompleteMessage();
      } else if (missionIsActive()) {
        drawStars(SCREEN_WIDTH - 15, 65, 0, 0, 16, 4, 0x80008080, TEXT_ALIGN_TOPRIGHT, State.DifficultyStars + 1);
        drawLives(SCREEN_WIDTH - 15, 85, 0, 0, 16, 4, 0x80808080, TEXT_ALIGN_TOPRIGHT, State.LivesLeft + 1);
        timer = gameGetTime() - State.MissionStartTime;
      }

      if (timer > 0) {
        drawTimer(timer);
      }
    }

    // 
    State.ActivePlayerCount = 0;
    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
      if (players[i])
        State.ActivePlayerCount++;

      processPlayer(i);
    }
    
    // count num alive
    if (State.IsHost && gameOptions->GameFlags.MultiplayerGameFlags.Survivor && gameTime > (State.InitializedTime + 5*TIME_SECOND))
    {
      // determine number of players alive
      State.AlivePlayerCount = 0;
      for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        if (playerIsValid(players[i]) && !playerIsDead(players[i])) {
          State.AlivePlayerCount++;
        }
      }

      if (!State.AlivePlayerCount) {
        State.TicksWithNoLivingPlayers++;
        if (State.TicksWithNoLivingPlayers > (3*TPS) && State.LivesLeft > 0) {
          missionUseLife();
          State.TicksWithNoLivingPlayers = 0;
        }
      } else {
        State.TicksWithNoLivingPlayers = 0;
      }
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

  // before game ticks
  bubbleTick();
  hopTick();

  // map frame tick
  if (mapConfig && mapConfig->OnFrameTickFunc)
    mapConfig->OnFrameTickFunc();
  
  // ticks
  mobTick();
  lootTick();
  statsTick();

#if DEBUG
  if (padGetButton(0, PAD_L1 | PAD_CROSS)) {
    *(float*)0x00347BD8 = 0.125;
  } else if (padGetButtonDown(0, PAD_UP | PAD_L1) > 0) {
    // static int aaa = 0;
    // Moby* lootSpawn(VECTOR position, int gadgetId, int rarity);
    // VECTOR p = {2,2,1,0};
    // vector_add(p, p, playerGetFromSlot(0)->PlayerPosition);
    // //lootSpawn(p, WEAPON_ID_MAGMA_CANNON, aaa);
    // lootRequestFromMissionComplete(p);
    // aaa = (aaa + 1) % RAIDS_WEAPON_RARITY_COUNT;
  }
#endif

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

	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickups = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickupsRandom = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 1;

  // enable all vehicles
	gameOptions->GameFlags.MultiplayerGameFlags.Vehicles = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Puma = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Hovership = 1;

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
    gameConfig->grNoPacks = 1;
    gameConfig->grV2s = 2;
    gameConfig->grVampire = 0;
    gameConfig->grHealthBars = 1;
    gameConfig->prChargebootForever = 0;
    gameConfig->prHeadbutt = 0;
    gameConfig->prPlayerSize = 0;
    gameConfig->grCqPersistentCapture = 0;
    gameConfig->grCqDisableTurrets = 0;
    gameConfig->grCqDisableUpgrades = 0;
    gameConfig->grRespawnOverride = 0;
    gameConfig->grNewPlayerSync = 1;
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
	strncpy((char*)(uiElements[19] + 0x60), "BOLTS", 6);
	strncpy((char*)(uiElements[20] + 0x60), "DEATHS", 7);

	// rows
	int* pids = (int*)(uiElements[0] - 0x9C);
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		// match scoreboard player row to their respective dme id
		int pid = pids[i];
		char* name = (char*)(uiElements[7 + i] + 0x18);
		if (pid < 0 || name[0] == 0)
			continue;

		struct RaidsPlayerState* pState = &State.PlayerStates[pid].State;

		// set kills
		sprintf((char*)(uiElements[22 + (i*4) + 0] + 0x60), "%d", pState->Kills);

		// copy over deaths
		strncpy((char*)(uiElements[22 + (i*4) + 2] + 0x60), (char*)(uiElements[22 + (i*4) + 1] + 0x60), 10);

		// set bolts
		//sprintf((char*)(uiElements[22 + (i*4) + 1] + 0x60), "%ld", pState->TotalBolts);
	}
}

//--------------------------------------------------------------------------
void waitForMapConfig(PatchStateContainer_t * gameState)
{
  //int* state = 0x0021e684;
  if (*(u16*)0x004a7dec != 0xA9B0) return;

  if (!mapConfig || mapConfig->Magic != MAP_CONFIG_MAGIC) {
    
    // prevent game from finishing loading
    POKE_U32(0x004A7FD4, 0);
    POKE_U32(0x004A7FDC, 0);
    POKE_U32(0x004A7FE4, 0);
    POKE_U32(0x004A82E8, 0);
  } else if (!hasMapConfig()) {

    // call map code to let it initialize
    ((void (*)(void))EXTRA_CODE_SEG_PTR)();

  } else if (gameState->AllClientsReady && *(u16*)0x0021ddb4 == 6) {

    // let game load
    POKE_U32(0x0021e680, 15);
    POKE_U32(0x0021e684, 15);
    POKE_U32(0x0021ddb4, 15);
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
				//sendPlayerStats(i);
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
  //if (hasMapConfig()) mapConfig->ClientsReady = 0;
  State.OnHubWorld = 0;
  State.ClientsReady = 0;

	setLobbyGameOptions(gameState->GameConfig);

  // reset bolts
  POKE_U32(0x00171b40, 0);
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
    case GAMEMODULE_SCENE_LOADING: break;
    case GAMEMODULE_UNKNOWN: break;
  }
}
