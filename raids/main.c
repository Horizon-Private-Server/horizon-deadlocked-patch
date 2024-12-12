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
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/random.h>
#include <libdl/net.h>
#include <libdl/sound.h>
#include <libdl/dl.h>
#include <libdl/utils.h>
#include <libdl/collision.h>
#include <libdl/radar.h>
#include "module.h"
#include "messageid.h"
#include "config.h"
#include "common.h"
#include "include/game.h"
#include "include/hop.h"
#include "include/bank.h"
#include "include/loot.h"
#include "include/inventory.h"
#include "include/levelselect.h"
#include "include/mob.h"
#include "include/bubble.h"
#include "include/utils.h"

char LocalPlayerStrBuffer[2][64];
int Initialized = 0;
int FirstTimeInitialized = 0;

struct RaidsState State;
struct RaidsMapConfig* mapConfig = (struct RaidsMapConfig*)(0x01EF0000 + 0x10);

struct RaidsSnackItem snackItems[SNACK_ITEM_MAX_COUNT] = {};
int snackItemsCount = 0;

PatchConfig_t* playerConfig = NULL;

u8 mobPlaySoundCooldownTicks[MAX_MOB_SPAWN_PARAMS][MOBS_PLAY_SOUND_COOLDOWN_MAX_SOUNDIDS] = {};

int playerStates[GAME_MAX_PLAYERS] = {};
int playerStateTimers[GAME_MAX_PLAYERS] = {};
float Difficulties[RAIDS_DIFFICULTY_COUNT] = {
  [RAIDS_DIFFICULTY_1STAR] 1.0,
  [RAIDS_DIFFICULTY_2STAR] 30.0,
  [RAIDS_DIFFICULTY_3STAR] 150.0,
  [RAIDS_DIFFICULTY_4STAR] 400.0,
  [RAIDS_DIFFICULTY_5STAR] 1000.0,
};

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
void drawMissionCompleteMessage(void)
{
  float x = SCREEN_WIDTH * 0.5, y = SCREEN_HEIGHT * 0.3;
  char strBuf[64];

  snprintf(strBuf, sizeof(strBuf), "%s", State.CurrentMapDef->Name);
  gfxHelperDrawText(x, y, 0, 0, 1.5, 0x80FFFFFF, strBuf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  int time = State.MissionCompleteTime - State.MissionStartTime;
  snprintf(strBuf, sizeof(strBuf), "Completed in %02d:%02d", time / TIME_MINUTE, (time % TIME_MINUTE) / TIME_SECOND);
  gfxHelperDrawText(x, y, 0, 30, 1.0, 0x80FFFFFF, strBuf, -1, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);

  uiShowHelpPopup(0, "Use Up to open the Planet Select menu.", 100);
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
int shouldDrawHud(void)
{
  PlayerHUDFlags* hudFlags = hudGetPlayerFlags(0);
  return hudFlags && hudFlags->Flags.Raw != 0;
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
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
  // pass to base if we don't have a player start
  playerGetSpawnpoint(player, outPos, outRot, firstRes);
}

//--------------------------------------------------------------------------
// TODO: Move this into the code segment, overwriting GuiMain_GetGadgetVersionName at (0x00541850)
char * customGetGadgetVersionName(int localPlayerIndex, int weaponId, int showWeaponLevel, int capitalize, int minLevel)
{
	Player* p = playerGetFromSlot(localPlayerIndex);
	char* buf = (char*)(0x2F9D78 + localPlayerIndex*0x40);
  struct GadgetDef* gadgetDef = weaponGetDef(weaponId, 0);
	int level = 0;
  int msgId = capitalize ? gadgetDef->uppercaseTag : gadgetDef->quickSelectTag;
	if (p && p->GadgetBox) {
		level = p->GadgetBox->Gadgets[weaponId].Level;
	}

	if (level >= 9)
    msgId = capitalize ? gadgetDef->upgUCTag : gadgetDef->upgQSTag;

	char* str = uiMsgString(msgId);
	if (0 && level >= minLevel) {
		snprintf(buf, 0x40, "%s V%d", str, level+1);
		return buf;
	} else {
		snprintf(buf, 0x40, "%s", str);
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
		Player * p = playerGetAll()[*(short*)(pvar + 0xC0)];
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
void respawnDeadPlayers(void) {
	int i;
	Player** players = playerGetAll();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		if (p && playerIsDead(p)) {
      if (p->IsLocal) {
			  playerRespawn(p);
      }
		}
		
		State.PlayerStates[i].IsDead = 0;
    //memset(State.PlayerStates[i].State.WeaponPrestige, 0, sizeof(State.PlayerStates[i].State.WeaponPrestige));
	}
}

//--------------------------------------------------------------------------
void onV10MagDamageMoby(Moby* target, MobyColDamageIn* in)
{
  if (in->Damager) {
    VECTOR dt;
    vector_subtract(dt, target->Position, in->Damager->Position);
    float dist = vector_length(dt);
    float min = 0.2;
    Player* damager = guberMobyGetPlayerDamager(in->Damager);
    if (damager) min += 0.05 * playerGetWeaponAlphaModCount(damager->GadgetBox, WEAPON_ID_MAGMA_CANNON, ALPHA_MOD_AREA);

    float falloff = minf(1, maxf(min, minf(1, 1 - (dist / 32))));
    in->DamageHp *= falloff;
  }

  mobyCollDamageDirect(target, in);
}

//--------------------------------------------------------------------------
void playerOnPushedIntoWall(Player* player)
{
  if (!player || !player->SkinMoby || !player->PlayerMoby) return;
  
  // push mobs away
  //mobReactToExplosionAt(player->PlayerId, player->PlayerPosition, 1, 8);

  // move player out of clipped wall
  // using lastGoodPos doesn't always return us to before the clip
  if (player->IsLocal) {
    playerSetPosRot(player, player->Ground.lastGoodPos, player->PlayerRotation);
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
	int localPlayerIndex, heldWeapon, hasMessage = 0;
	Player** players = playerGetAll();
	Player* player = players[pIndex];
	struct RaidsPlayer * playerData = &State.PlayerStates[pIndex];

	if (!player || !player->PlayerMoby)
		return;

	int actionCooldownTicks = decTimerU8(&playerData->ActionCooldownTicks);
	int messageCooldownTicks = decTimerU8(&playerData->MessageCooldownTicks);
  
  // last hit
  if (player->Health != playerData->LastHealth) playerData->TicksSinceHealthChanged = 0;
  else playerData->TicksSinceHealthChanged += 1;
  playerData->LastHealth = player->Health;

  // player speed
	player->Speed = 1 + (PLAYER_SKILLPOINT_SPEED_FACTOR * State.PlayerStates[pIndex].State.Skills[RAIDS_SKILLS_SPEED]);

	// set max health
	player->MaxHealth = 50 + (PLAYER_SKILLPOINT_HEALTH_FACTOR * State.PlayerStates[pIndex].State.Skills[RAIDS_SKILLS_HEALTH]);

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
		u64 xp = bankGetXP();
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
int whoKilledMeHook(Player* player, Moby* moby, int b)
{
  if (!moby)
    return 0;

  // only allow mobs or special mobys
  if (mobyIsMob(moby) || moby->Bolts == -1) {
    return ((int (*)(Player*, Moby*, int))0x005dff08)(player, moby, b);
  }

	return 0;
}

//--------------------------------------------------------------------------
int onMobyPlayDesiredSound(int sound, int a1, Moby* moby)
{
  // catch mobs
  if (mobyIsMob(moby)) {
    int midx = ((struct MobPVar*)moby->PVar)->MobVars.SpawnParamsIdx;
    if (midx >= 0) {
      int sidx = sound % MOBS_PLAY_SOUND_COOLDOWN_MAX_SOUNDIDS;
      if (mobPlaySoundCooldownTicks[midx][sidx]) return -1;
      mobPlaySoundCooldownTicks[midx][sidx] = MOBS_PLAY_SOUND_COOLDOWN;
    }
  }

  // pass to mobyPlaySound
  return mobyPlaySound(sound, a1, moby);
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

  if (firstTime) {
    firstTime = 0;
    
    // clear state
    memset(&State, 0, sizeof(State));
  }

  // clear if magic not valid
  if (mapConfig->Magic != MAP_CONFIG_MAGIC) {
    memset(mapConfig, 0, sizeof(struct RaidsMapConfig));
    return;
    //mapConfig->Magic = MAP_CONFIG_MAGIC;
  }

	// Disable normal game ending
	*(u32*)0x006219B8 = 0;	// survivor (8)
	*(u32*)0x00620F54 = 0;	// time end (1)
	*(u32*)0x00621568 = 0;	// kills reached (2)
	*(u32*)0x006211A0 = 0;	// all enemies leave (9)
  *(u32*)0x006210D8 = 0;	// all enemies leave (9)

  // force ammo drops to local players only
  POKE_U32(0x003ACC24, 0x8E351A9C);
  HOOK_JAL(0x003aca8c, &ammoPickupTargetGetGadgetMaxAmmo);

  // double ammo pickup amount
  POKE_F32(0x003978C0, 0.3);

  // spawn area mod explosion on each ricochet of the v10 vipers
  //HOOK_JAL(0x003C283C, &onV10VipersHitSurface);

  // disable holoshields from disappearing
  //*(u16*)0x00401478 = 2;

  // disable ammo drop despawn
  //POKE_U32(0x004FE814, 0);

  // disable jump pad effect
  POKE_U32(0x0042608C, 0);

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

	// Disable sniper shot corn
	//*(u32*)0x003FC410 = 0;
	*(u32*)0x003FC5A8 = 0;

	// Fix v10 arb overlapping shots
	*(u32*)0x003F2E70 = 0x24020000;

  // fix bolt crank moby (1A27) resetting itself on capture
  POKE_U32(0x003D74C0, 0);

  // hook when MobyPlayDesiredSound plays a sound for a moby
  // lets us reduce the # of mob sounds
  HOOK_JAL(0x004f790c, &onMobyPlayDesiredSound);
  HOOK_J(0x004fa800, &onMobyPlayDesiredSound);

  // disable timebase query percentile filter
  // always accept remote time
  POKE_U32(0x01eabd60, 0);

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
	//*(u32*)0x005DD890 = 0x0C000000 | ((u32)&customBangelizeWeapons >> 2);

	// Enable weapon version and v10 name variant in places that display weapon name
	*(u32*)0x00541850 = 0x08000000 | ((u32)&customGetGadgetVersionName >> 2);
	*(u32*)0x00541854 = 0;

	// patch who killed me to prevent damaging others
	*(u32*)0x005E07C8 = 0x0C000000 | ((u32)&whoKilledMeHook >> 2);
	*(u32*)0x005E11B0 = *(u32*)0x005E07C8;

  // patch mobs pushing you into walls and killing you
  POKE_U32(0x005e4188, 0);
  POKE_U32(0x005e419c, 0);
  HOOK_JAL(0x005e41bc, &playerOnPushedIntoWall);

  // disable guber event delay until createTime+relDispatchTime reached
  // when players desync, their net time falls behind everyone else's
  // causing events that they receive to be delayed for long periods of time
  // leading to even more desyncing issues
  // since raids can cause a lot of frame lag, especially for players on emu/dzo
  // this fix is required to ensure that important mob guber events trigger on everyone's screen
  POKE_U32(0x00611518, 0x24040000);

	// set default ammo for flail to 8
	//*(u8*)0x0039A3B4 = 8;

	// disable targeting players
	*(u32*)0x005F8A80 = 0x10A20002;
	*(u32*)0x005F8A84 = 0x0000102D;
	*(u32*)0x005F8A88 = 0x24440001;

  // hook net messages
	netInstallCustomMsgHandler(CUSTOM_MSG_MOB_UNRELIABLE_MSG, &mobOnUnreliableMsgRemote);

  // write map config
  mapConfig->State = &State;
  mapConfig->PushSnackFunc = &pushSnack;
  mapConfig->GetBankFunc = &bankGetLocalBank;
  mapConfig->BeginWorldHopFunc = &hopBegin;
  mapConfig->SendBankAccountToServerFunc = &bankSendAccountToServer;
  mapConfig->PopulateSpawnArgsFunc = &mobPopulateSpawnArgsFromConfig;
  mapConfig->RegisterNpcFunc = &mobRegisterNpc;
  mapConfig->OnGetGuberFunc = &getGuber;
  mapConfig->OnGuberEventFunc = &handleEvent;
  mapConfig->TryCreateMobFunc = &mobCreate;

	// set game over string
	//strncpy(uiMsgString(0x3477), RAIDS_GAME_OVER, strlen(RAIDS_GAME_OVER)+1);

	// disable v2s and packs
	cheatsApplyNoV2s();
	cheatsApplyNoPacks();

	// change hud
	forcePlayerHUD();
  setPlayerEXP(0, 0);
  setPlayerEXP(1, 0);

  // prevent player from doing anything
  padDisableInput();

  // component init
  mobInitialize();
  bubbleInit();
  bankInit();
  inventoryInit();
  lootInit();
  hopInit();
  levelselectInit();

  memset(playerStates, 0, sizeof(playerStates));
  memset(playerStateTimers, 0, sizeof(playerStateTimers));

  if (startDelay) {
    --startDelay;
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
  State.ClientsReady = 1;
  mapConfig->ClientsReady = 1;
  if (State.MissionStartTime <= 0) State.MissionStartTime = gameGetTime();

  // re-enable input
  padEnableInput();

  memset(snackItems, 0, sizeof(snackItems));

	// initialize player states
  State.Difficulty = Difficulties[State.DifficultyStars];
	State.LocalPlayerState = NULL;
	State.NumTeams = 0;
	State.AlivePlayerCount = -1;
	State.ActivePlayerCount = 0;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];

#if PAYDAY
		//State.PlayerStates[i].State.CurrentTokens = 1000;
#endif

		if (p) {

      // set max health
      p->Health = p->MaxHealth = 50 + (PLAYER_SKILLPOINT_HEALTH_FACTOR * State.PlayerStates[i].State.Skills[RAIDS_SKILLS_HEALTH]);

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
  State.AmmoDropChance = GAME_DEFAULT_AMMO_DROP_CHANCE;
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
		initialize(gameState);
		return;
	}


  // get local player data
  //struct RaidsPlayer* localPlayerData = &State.PlayerStates[localPlayer->PlayerId];

#if DEBUG_SOUNDS
  {
    static int aaa = 0;
    const int mobyClass = MOBY_ID_HEALTH_BOX_MULT;
    Player* localPlayer = playerGetFromSlot(0);
		if (padGetButtonDown(0, PAD_RIGHT) > 0) {
			aaa += 1;
			printf("%d\n", aaa);
      mobyPlaySoundByClass(aaa, 0, localPlayer->PlayerMoby, mobyClass);
		} else if (padGetButtonDown(0, PAD_LEFT) > 0) {
			aaa -= 1;
			printf("%d\n", aaa);
      mobyPlaySoundByClass(aaa, 0, localPlayer->PlayerMoby, mobyClass);
		} else if (padGetButtonDown(0, PAD_UP) > 0) {
			printf("%d\n", aaa);
      mobyPlaySoundByClass(aaa, 0, localPlayer->PlayerMoby, mobyClass);
		}
  }
#endif

  // get bank on first load
  // send bank when game ends
  if (!gameHasEnded()) {
    if (!bankGetHasAccount() && !bankHasPendingAccountRequest()) {
      bankRequestAccountFromServer();
    }
    if (!bankGetHasInventory() && !bankHasPendingInventoryRequest()) {
      bankRequestInventoryFromServer();
    }
  } else if (sendBankAtEnd) {
    if (bankGetHasAccount()) bankSendAccountToServer();
    //bankSendInventoryToServer();
    sendBankAtEnd = 0;
  }

  RaidsPlayerBank_t* localBank = bankGetLocalBank();
  int refreshInventoryFlag = localBank->Inventory.RefreshLocalInventory;

  // map frame tick
  if (mapConfig && mapConfig->OnFrameTickFunc)
    mapConfig->OnFrameTickFunc();
  
  if (!State.GameOver)
  {
    forcePlayerHUD();
    drawSnack();

    // draw mission complete message
    if (State.MissionComplete && !gameIsAnyStartMenuOpen()) { // && (gameGetTime() - State.MissionCompleteTime) < (10*TIME_SECOND)) {
      drawMissionCompleteMessage();
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
        if (players[i] && players[i]->SkinMoby && !playerIsDead(players[i]) && players[i]->Health > 0) {
          State.AlivePlayerCount++;
        }
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

  // ticks
  mobTick();
  bubbleTick();
  bankTick();
  inventoryTick();
  lootTick();
  hopTick();
  levelselectFrameTick();

  // reset inventory refresh flag
  if (refreshInventoryFlag)
    localBank->Inventory.RefreshLocalInventory = 0;

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

  }

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

  POKE_U32(0x00171b40, bankGetBolts());

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
    gameConfig->prPlayerSize = 0;
    gameConfig->grCqPersistentCapture = 0;
    gameConfig->grCqDisableTurrets = 0;
    gameConfig->grCqDisableUpgrades = 0;
    gameConfig->grRespawnOverride = 0;
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
  if (mapConfig) mapConfig->ClientsReady = 0;
  State.OnHubWorld = 0;
  State.ClientsReady = 0;

	setLobbyGameOptions(gameState->GameConfig);
  
	// point get resurrect point to ours
	*(u32*)0x00610724 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
	*(u32*)0x005e2d44 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);

  // reset bolts
  POKE_U32(0x00171b40, 0);
}

//--------------------------------------------------------------------------
void start(struct GameModule * module, PatchStateContainer_t * gameState, enum GameModuleContext context)
{
  switch (context)
  {
    case GAMEMODULE_LOBBY: lobbyStart(module, gameState); break;
    case GAMEMODULE_LOAD: loadStart(module, gameState); break;
    case GAMEMODULE_GAME_FRAME: gameStart(module, gameState); break;
    case GAMEMODULE_GAME_UPDATE: break;
  }
}
