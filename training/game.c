/***************************************************
 * FILENAME :		game.c
 * 
 * DESCRIPTION :
 * 		PAYLOAD.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/camera.h>
#include <libdl/weapon.h>
#include <libdl/radar.h>
#include <libdl/hud.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/collision.h>
#include <libdl/dialog.h>
#include <libdl/ui.h>
#include <libdl/stdio.h>
#include <libdl/graphics.h>
#include <libdl/spawnpoint.h>
#include <libdl/random.h>
#include <libdl/net.h>
#include <libdl/sound.h>
#include <libdl/color.h>
#include <libdl/pad.h>
#include <libdl/dl.h>
#include <libdl/utils.h>
#include "module.h"
#include "messageid.h"
#include "include/game.h"
#include "include/utils.h"
#include "include/bezier.h"

#define TARGET_INIT_SPAWN_DELAY_TICKS		(TPS * 1)

extern int Initialized;
extern int TargetTeam;

void modeInitialize(void);
void initializeScoreboard(void);
void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx);

int modeGetBestSpawnPointIdx(void);
void modeGetResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes);

void modeOnGadgetFired(int gadgetId);
void modeOnTargetKilled(SimulatedPlayer_t* target, MobyColDamage* colDamage);
void modeOnTargetHit(SimulatedPlayer_t* target, MobyColDamage* colDamage);
void modeUpdateTarget(SimulatedPlayer_t *sPlayer);
void modeInitTarget(SimulatedPlayer_t *sPlayer);

void modeProcessPlayer(int pIndex);
void modeTick(void);

int * LocalBoltCount = (int*)0x00171B40;

extern float ComboMultiplierFactor;
extern const int SimPlayerCount;
extern SimulatedPlayer_t SimPlayers[];

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
	return State.Points;
}

//--------------------------------------------------------------------------
float getComboMultiplier(void) {
	return State.ComboCounter * ComboMultiplierFactor;
}

//--------------------------------------------------------------------------
void incCombo(void) {
	State.ComboCounter += 1;
	if (State.ComboCounter > State.BestCombo)
		State.BestCombo = State.ComboCounter;
}

//--------------------------------------------------------------------------
void forcePlayerHUD(void)
{
	int i;

	// replace normal scoreboard with bolt counter
	for (i = 0; i < 2; ++i)
	{
		PlayerHUDFlags* hudFlags = hudGetPlayerFlags(i);
		if (hudFlags) {
			hudFlags->Flags.BoltCounter = 1;
			hudFlags->Flags.NormalScoreboard = 0;
		}
	}
}

//--------------------------------------------------------------------------
void queueGadgetEventHooked(u64 a0, u64 a1, u64 a2, u64 a3, u64 t0, u64 t1, u64 t2, u64 t3, u64 t4, u64 t5)
{
	int type = a1;
	int gadgetId = t2;

	// intercept weapon fire
	if (type == 8)
	{
		modeOnGadgetFired(gadgetId);
	}

	// call base
	((void (*)(u64, u64, u64, u64, u64, u64, u64, u64, u64, u64))0x00627808)(a0, a1, a2, a3, t0, t1, t2, t3, t4, t5);
}

//--------------------------------------------------------------------------
void targetUpdate(SimulatedPlayer_t *sPlayer)
{
	Player* player = playerGetFromSlot(0);
	int isOwner = gameAmIHost();
	if (!sPlayer || !player || !sPlayer->Active)
		return;

	Player* target = sPlayer->Player;
	Moby* targetMoby = target->PlayerMoby;
	struct TrainingTargetMobyPVar* pvar = &sPlayer->Vars;
	if (!pvar)
		return;

	if (State.GameOver)
		return;

	// reset pad
	struct padButtonStatus* pad = (struct padButtonStatus*)sPlayer->Pad.rdata;
	pad->btns = 0xFFFF;
	pad->ljoy_h = 0x7F;
	pad->ljoy_v = 0x7F;
	pad->circle_p = 0;
	pad->cross_p = 0;
	pad->square_p = 0;
	pad->triangle_p = 0;
	pad->down_p = 0;
	pad->left_p = 0;
	pad->right_p = 0;
	pad->up_p = 0;
	pad->l1_p = 0;
	pad->l2_p = 0;
	pad->r1_p = 0;
	pad->r2_p = 0;
	


	// send to mode for custom logic
	modeUpdateTarget(sPlayer);

	// process damage
	int damageIndex = targetMoby->CollDamage;
	MobyColDamage* colDamage = NULL;
	float damage = 0.0;

	if (damageIndex >= 0) {
		colDamage = mobyGetDamage(targetMoby, 0x400C00, 0);
		if (colDamage)
			damage = colDamage->DamageHp;
	}
	((void (*)(Moby*, float*, MobyColDamage*))0x005184d0)(targetMoby, &damage, colDamage);

	if (colDamage && damage > 0) {
		Player * damager = guberMobyGetPlayerDamager(colDamage->Damager);
		if (damager && damager == player) {
			if (damage >= target->Health)
				modeOnTargetKilled(sPlayer, colDamage);
			else
				modeOnTargetHit(sPlayer, colDamage);
		}
		// 
		//targetMoby->CollDamage = -1;
	}
}

//--------------------------------------------------------------------------
void spawnTarget(SimulatedPlayer_t* sPlayer)
{
	GameSettings* gs = gameGetSettings();

	if (!sPlayer)
		return;

	// create
	if (!sPlayer->Created)
		createSimPlayer(sPlayer, sPlayer->Idx);

	// destroy existing
	if (sPlayer->Active)
		modeOnTargetKilled(sPlayer, 0);

	playerRespawn(sPlayer->Player);
	modeInitTarget(sPlayer);
}

//--------------------------------------------------------------------------
void frameTick(void)
{
	int i = 0;
	char buf[32];
	int nowTime = gameGetTime();
	GameData* gameData = gameGetData();
	GameOptions* gameOptions = gameGetOptions();

	// 
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		modeProcessPlayer(i);
	}

	// draw counter
	//snprintf(buf, 32, "%d/%d", State.TargetsDestroyed, TARGET_GOAL);
	//gfxScreenSpaceText(15, SCREEN_HEIGHT - 15, 1, 1, 0x80FFFFFF, buf, -1, 6);

	// compute time left
	float secondsLeft = ((gameOptions->GameFlags.MultiplayerGameFlags.Timelimit * TIME_MINUTE) - (nowTime - gameData->TimeStart)) / (float)TIME_SECOND;
	if (secondsLeft < 0)
		secondsLeft = 0;
	int secondsLeftInt = (int)secondsLeft;
	float timeSecondsRounded = secondsLeftInt;
	if ((secondsLeft - timeSecondsRounded) > 0.5)
		timeSecondsRounded += 1;

	// draw timer
	sprintf(buf, "%02d:%02d", secondsLeftInt/60, secondsLeftInt%60);
	gfxScreenSpaceText(479+1, 57+1, 0.8, 0.8, 0x80000000, buf, -1, 1);
	gfxScreenSpaceText(479, 57, 0.8, 0.8, 0x80FFFFFF, buf, -1, 1);

	modeTick();
}

//--------------------------------------------------------------------------
void gameTick(void)
{
	int i;
	
	*LocalBoltCount = State.Points;

  // handle game end logic
  if (State.IsHost)
  {
		// keep spawning
		for (i = 0; i < SimPlayerCount; ++i)
		{
			if (!SimPlayers[i].Active || playerIsDead(SimPlayers[i].Player))
			{
				u32 timer = decTimerU32(&SimPlayers[i].TicksToRespawn);
				if (!timer) {
					spawnTarget(&SimPlayers[i]);
				}
			}
		}
  }
}

//--------------------------------------------------------------------------
void onSimulateHeros(void)
{
	int i;


	// update local hero first
	Player * lPlayer = playerGetFromSlot(0);
	if (lPlayer) {
		PlayerVTable* pVTable = playerGetVTable(lPlayer);

		pVTable->Update(lPlayer); // update hero
	}
	
	//POKE_U32(0x0060FCD0, 0); // prevent game from overwriting our camera overwriting (causing remote players to constantly try and turn)

	// update remote clients
	for (i = 0; i < SimPlayerCount; ++i) {
		if (SimPlayers[i].Player) {
			PlayerVTable* pVTable = playerGetVTable(SimPlayers[i].Player);

			targetUpdate(&SimPlayers[i]);

			// process input
			((void (*)(struct PAD*))0x00527e08)(&SimPlayers[i].Pad);

			// update pad
			((void (*)(struct PAD*, void*, int))0x00527510)(&SimPlayers[i].Pad, SimPlayers[i].Pad.rdata, 0x14);

			// run game's hero update
			pVTable->Update(SimPlayers[i].Player);
		}
	}

	//POKE_U32(0x0060FCD0, 0x7E022CE0); // enable camera controls
}

//--------------------------------------------------------------------------
void fixSimPlayerNetPlayerPtr(u64 pIdx, u64 a1, u64 a2, u64 a3, u64 t0, u64 t1, u64 t2, u64 t3, u64 t4)
{
	// call base
	((void (*)(u64, u64, u64, u64, u64, u64, u64, u64, u64))0x0015fa68)(pIdx, a1, a2, a3, t0, t1, t2, t3, t4);

	Player* newPlayer = playerGetAll()[pIdx];
	Player* p2 = playerGetFromSlot(1);

	// if the player has been found without a pointer to the NetPlayer object
	// then copy the pointer from player 2
	if (newPlayer && !newPlayer->pNetPlayer && p2) {
		newPlayer->pNetPlayer = p2->pNetPlayer;
	}
}

//--------------------------------------------------------------------------
void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx)
{
	int id = idx + 1;
	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	
	HOOK_JAL(0x00610e38, &fixSimPlayerNetPlayerPtr);

	// reset
	memset(sPlayer, 0, sizeof(SimulatedPlayer_t));
	
	// spawn player
	memcpy(&sPlayer->Pad, players[0]->Paddata, sizeof(struct PAD));
	
	sPlayer->Pad.rdata[7] = 0x7F;
	sPlayer->Pad.rdata[6] = 0x7F;
	sPlayer->Pad.rdata[5] = 0x7F;
	sPlayer->Pad.rdata[4] = 0x7F;
	sPlayer->Pad.rdata[3] = 0xFF;
	sPlayer->Pad.rdata[2] = 0xFF;
	sPlayer->Pad.port = 10;
	sPlayer->Pad.slot = 10;
	sPlayer->Pad.id = id;
	
	// create hero
	gameSettings->PlayerClients[id] = 0;
	gameSettings->PlayerSkins[id] = 0;
	gameSettings->PlayerTeams[id] = TargetTeam;

	POKE_U32(0x0021DDDC + (4 * idx), (u32)&sPlayer->Pad);
	//POKE_U16(0x006103DC, 0x000B);
	//POKE_U16(0x00610410, 0x000B);

	((void (*)(int))0x006103d0)(id); // local hero
	Player* newPlayer = players[id];
	sPlayer->Player = newPlayer;

	players[id] = sPlayer->Player;

	//memcpy(sPlayer->Player, (void*)newPlayer, SIZEOF_PLAYER_OBJECT);

	POKE_U32(0x0021DDDC + (4 * idx), 0);
	//POKE_U32(0x0034C730, sPlayer->Player);

	//memset((void*)newPlayer, 0, SIZEOF_PLAYER_OBJECT);

	//POKE_U32(0x00344C3C, 0); // ensure player list only has first local player

	sPlayer->Player->Paddata = (void*)&sPlayer->Pad;
	sPlayer->Player->PlayerId = id;
	sPlayer->Player->MpIndex = id;
	sPlayer->Player->Team = TargetTeam;
	sPlayer->Idx = idx;
	POKE_U32((u32)sPlayer->Player + 0x1AA0, (u32)sPlayer->Player);

	if (sPlayer->Player->PlayerMoby) {
		sPlayer->Player->PlayerMoby->NetObject = sPlayer->Player;
		*(u32*)((u32)sPlayer->Player->PlayerMoby->PVar + 0xE0) = (u32)sPlayer->Player;
	}

	sPlayer->Active = 0;
	sPlayer->Created = 1;
	DPRINTF("target player: %08X (moby: %08X)\n", (u32)sPlayer->Player, (u32)sPlayer->Player->PlayerMoby);
}

//--------------------------------------------------------------------------
void initialize(PatchGameConfig_t* gameConfig)
{
	Player** players = playerGetAll();
	int i;

	// set target moby to NULL
	//State.CurrentTarget = NULL;

	// Disable normal game ending
	*(u32*)0x006219B8 = 0;	// survivor (8)
	*(u32*)0x00621568 = 0;	// kills reached (2)
	*(u32*)0x006211A0 = 0;	// all enemies leave (9)
#if DEBUG
	*(u32*)0x00620F54 = 0;	// time end (1)
#endif

	// point get resurrect point to ours
	*(u32*)0x00610724 = 0x0C000000 | ((u32)&modeGetResurrectPoint >> 2);
	*(u32*)0x005e2d44 = 0x0C000000 | ((u32)&modeGetResurrectPoint >> 2);

	// disable default update for targets
	// we update manually inside of onSimulateHeros
	POKE_U32(0x005CD458, 0);
	POKE_U32(0x005CD464, 0x90822FEA);

	// set vtable callbacks
	//*(u32*)0x003A0BC4 = (u32)&targetGetGuber;
	//*(u32*)0x003A0BD4 = (u32)&guberHandleEvent;
	//*(u32*)0x003A0B24 = (u32)&targetGetGuber;
	//*(u32*)0x003A0B34 = (u32)&guberHandleEvent;

	// hook queue gadget event
	HOOK_JAL(0x005f037c, &queueGadgetEventHooked);

	// set bolt count to 0
	*LocalBoltCount = 0;

	// hook messages
	netHookMessages();
  initializeScoreboard();
	forcePlayerHUD();
	modeInitialize();

	// give a small delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 15;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	memset(SimPlayers, 0, sizeof(SimulatedPlayer_t) * SimPlayerCount);
	memset(&State, 0, sizeof(State));

	// initialize state
	State.TrainingType = (enum TRAINING_TYPE)gameConfig->trainingConfig.type;
	State.TimeLastKill = timerGetSystemTime();
	State.InitializedTime = gameGetTime();

	// respawn player
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		if (players[i] && players[i]->Team != TargetTeam) {
			playerRespawn(players[i]);
		}
	}

	HOOK_JAL(0x005CE320, onSimulateHeros);
	//POKE_U32(0x005CE350, 0x10000011);

	for (i = 0; i < SimPlayerCount; ++i) {
		SimPlayers[i].Idx = i;
		SimPlayers[i].TicksToRespawn = TARGET_INIT_SPAWN_DELAY_TICKS * i;
	}

	Initialized = 1;
}

//--------------------------------------------------------------------------
void updateGameState(PatchStateContainer_t * gameState)
{
	// game state update
	if (gameState->UpdateGameState)
	{
		if (isInGame()) {
			Player* p = playerGetFromSlot(0);
			if (p)
				gameState->GameStateUpdate.TeamScores[p->Team] = State.Points;
		}
	}

	// stats
	if (gameState->UpdateCustomGameStats)
	{
    gameState->CustomGameStatsSize = sizeof(struct TrainingGameData);
		struct TrainingGameData* sGameData = (struct TrainingGameData*)gameState->CustomGameStats.Payload;
		sGameData->Version = 0x00000002;
		sGameData->Points = State.Points;
		sGameData->Kills = State.Kills;
		sGameData->Hits = State.Hits;
		sGameData->Misses = State.ShotsFired - State.Hits;
		sGameData->BestCombo = State.BestCombo;
		sGameData->Time = gameGetTime() - State.InitializedTime;
	}
}
