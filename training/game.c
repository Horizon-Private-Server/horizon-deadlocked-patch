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
#include <libdl/dl.h>
#include <libdl/utils.h>
#include "module.h"
#include "messageid.h"
#include "include/game.h"
#include "include/utils.h"
#include "include/bezier.h"

extern int Initialized;

#if DEBUG
int aaa = 300;
#endif

#define TARGET_GOAL								(20)
#define MAX_SPAWNED_TARGETS				(2)
#define TARGET_INIT_SPAWN_DELAY		(TIME_SECOND * 2)
#define TARGET_RESPAWN_DELAY			(TIME_SECOND * 1)
#define TARGET_LIFETIME_TICKS			(TPS * 20)
#define SIZEOF_PLAYER_OBJECT			(0x31F0)

void initializeScoreboard(void);

int * LocalBoltCount = (int*)0x00171B40;

const char* NAMES[] = {"Chivas Joker", "BooKooKiLLa", "Sunny D", "Poten-TiaL", "//Who Cares", "FlagCapper-Lag", "VeRNoN", "D34DLY", "aAsdf", "Star Killer", "-Chidori-", "Flailer*", "Red Pirate", "Djabsolut121", "Hothead2005", "thepimp", "Thorax", "Undeadghost", "CANADIAN--KID", "_GuNNeR_", "InfiniteDragon", "-RePubLiC-", "HopesandFears", "Blind Falcon", "GdOgS4122", ":--:iCe:--:", "-mexico-", "f!r3*", "Silent Kid:']", "Sapphire Medal", "=):::::::::::::>", "}{ole of death", "}{alf Glitch", "sTaLe FisH", "-Xz Death zX-", "u s a #1", "FatBlack", "Quabbi", "Wemsrox", "PrO StAtUs", "GoT MiLK? O_o", "Canada Ftw", "-EnsLaveD-", "cArMeL-GirL", "-FreaKaZoiD-", "Major Nuke", "-:ZeRo:-", "dRuiD", "K.O", "rawr.<3", "FrostPrincess", "DeviN982", "`dUaLiTy`", "Lucci", "-French-", "ArmbaR", "ThePac", "Texasboy", "iNtent To harM", "XsV", "-Advanced-", "reDFlag", "Certain Death", "Army-Gunz", "Tek No", "bLaH!!!!", ".nEw YoRk.", "[_-BoT-LeG-_]", "omega3", "Mr.Snowman7", "My1lastfate", "riT", "Man In A Suit", "DGK", "Mr.$un$hIn3", "EviL DoN", "-xTc-", "KreeD", "Kiku", "-eXo-", "KyLe1234", "-shortii-", "BAD GAS", "Nonkeyboii", "-ShotGunStyle-", "FLuFFy PiLLoW", "G.i. Reaper", "}{itMaN", "inmate", "First Place", "Sasori", "Q.w.e.r.t.y", "VoLcoM.", "Boj", "Shadowchamp", "Flying Tomato", "KillaseasoN", "*wast3d*", "Ryno Mikey", "-J1D M4N-"};

const int NAMES_COUNT = sizeof(NAMES) / sizeof(char*);

// 
struct TrainingMapConfig Config __attribute__((section(".config"))) = {
	.SpawnPointCount = 4,
	.SpawnPoints = {
		{ 367.9749, 175.8122, 107.9281, 20.9445 },
		{ 397.2048, 135.9122, 106.4281, 22.75173 },
		{ 489.9348, 217.6122, 108.0681, 20.52197 },
		{ 446.5349, 261.2122, 108.7681, 23.78278 },
	},
	.PlayerSpawnPoint = { 415.0349, 182.1122, 108.3381, 26 },
};

// target destroy sound
SoundDef targetDestroySoundDef = {
	.BankIndex = 3,
	.Flags = 0x02,
	.Index = 0,
	.Loop = 1,
	.MaxPitch = 0,
	.MinPitch = 0,
	.MaxVolume = 500,
	.MinVolume = 1000,
	.MinRange = 0.0,
	.MaxRange = 500.0
};


typedef struct SimulatedPlayer {
	struct PAD Pad;
	struct TrainingTargetMobyPVar Vars;
	Player* Player;
	int TargetLastSpawnedTime;
	int TargetLastDestroyedTime;
	int Idx;
	char Created;
	char Active;
} SimulatedPlayer_t;

SimulatedPlayer_t SimPlayers[MAX_SPAWNED_TARGETS];

u128 SIM_PLAYER_BUFFER[MAX_SPAWNED_TARGETS * (SIZEOF_PLAYER_OBJECT / sizeof(u128))];

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
	return State.Points;
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
void * GetMobyClassPtr(int oClass)
{
    int mClass = *(u8*)(0x0024a110 + oClass);
    return *(u32*)(0x002495c0 + mClass*4);
}

void queueGadgetEventHooked(u64 a0, u64 a1, u64 a2, u64 a3, u64 t0, u64 t1, u64 t2, u64 t3, u64 t4, u64 t5)
{
	int type = a1;
	int gadgetId = t2;

	// intercept weapon fire
	if (type == 8)
	{
		switch (State.TrainingType)
		{
			case TRAINING_TYPE_FUSION:
			{
				if (gadgetId == WEAPON_ID_FUSION_RIFLE)
					State.ShotsFired += 1;
				break;
			}
		}
	}

	// call base
	((void (*)(u64, u64, u64, u64, u64, u64, u64, u64, u64, u64))0x00627808)(a0, a1, a2, a3, t0, t1, t2, t3, t4, t5);
}

//--------------------------------------------------------------------------
void simPlayerKill(SimulatedPlayer_t* target, int givePoints)
{
	const float radius = 3;
	VECTOR delta = {0,0,0,0};
	struct TrainingTargetMobyPVar* pvar = &target->Vars;

	// spawn explosion
	//spawnExplosion(target->Player->PlayerPosition, radius);

	// 
	//target->Player->PlayerMoby->ModeBits &= ~0x30;

	// hide moby
	//playerSetPosRot(target->Player, delta, delta);
	target->Player->Health = 0;

	// give points
	if (pvar && givePoints) {
		State.Points += pvar->LifeTicks;
		State.Kills += 1;
		State.Hits += 1;
	}

	// 
	target->TargetLastDestroyedTime = gameGetTime();
	State.TargetsDestroyed += 1;
	
	// deactivate
	target->Active = 0;
}

//--------------------------------------------------------------------------
void targetUpdate(SimulatedPlayer_t *sPlayer)
{
	int i = 0;
	Player* player = playerGetFromSlot(0);
	VECTOR delta;
	static int lastGt = 0;
	int gameTime = gameGetTime();
	int isOwner = gameAmIHost();
	float dt = (gameTime - lastGt) / (float)TIME_SECOND;
	lastGt = gameTime;
	if (!sPlayer || !player)
		return;

	Player* target = sPlayer->Player;
	Moby* targetMoby = target->PlayerMoby;
	struct TrainingTargetMobyPVar* pvar = &sPlayer->Vars;
	if (!pvar)
		return;

	if (State.GameOver)
		return;

	// register post draw function
	//gfxRegisterDrawFunction((void**)0x0022251C, &targetPostDraw, target);

	// timers
	u32 lifeTicks = decTimerU32(&pvar->LifeTicks);
#if !DEBUG
	if (!lifeTicks)
	{
		simPlayerKill(sPlayer, 0);
		return;
	}
#endif

	// set camera type to third person
	POKE_U32(0x001711D4 + (4*sPlayer->Idx), 0);

	// face player
	vector_subtract(delta, player->PlayerPosition, target->PlayerPosition);
	float len = vector_length(delta);
	float yaw = atan2f(delta[1] / len, delta[0] / len);
	//target->PlayerRotation[2] = yaw;
	//target->CameraYaw.Value = yaw;
	target->PlayerYaw = yaw;
	//vector_copy(target->CameraDir, delta);

	// handle strafing
	if (pvar->State & TARGET_STATE_STRAFING)
	{

	}

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
		if (damager && (isOwner || playerIsLocal(damager))) {
			simPlayerKill(sPlayer, 1);
		} else if (isOwner) {
			simPlayerKill(sPlayer, 1);	
		}

		// 
		targetMoby->CollDamage = -1;
	}
}

//--------------------------------------------------------------------------
int getBestSpawnPointIdx(void)
{
	int i,j;
	VECTOR delta;
	Player* player = playerGetFromSlot(0);
	if (!player)
		return 0;

	float minDistanceSqr = TARGET_MIN_SPAWN_DISTANCE[(int)State.TrainingType] * TARGET_MIN_SPAWN_DISTANCE[(int)State.TrainingType];
	float idealDistanceSqr = TARGET_IDEAL_SPAWN_DISTANCE[(int)State.TrainingType] * TARGET_IDEAL_SPAWN_DISTANCE[(int)State.TrainingType];
	float bufferDistanceSqr = TARGET_BUFFER_SPAWN_DISTANCE[(int)State.TrainingType] * TARGET_BUFFER_SPAWN_DISTANCE[(int)State.TrainingType];

	float bestIdxScores[3] = {-1,-1,-1};
	int bestIdxs[3] = {0,1,2};

	for (i = 0; i < Config.SpawnPointCount; ++i) {
		vector_subtract(delta, player->PlayerPosition, Config.SpawnPoints[i]);
		delta[3] = 0;
		float sqrDist = vector_sqrmag(delta) - (Config.SpawnPoints[i][3] * Config.SpawnPoints[i][3]);

		if (sqrDist > 0)
		{
			float score = maxf(0, sqrDist);
			if (i == State.TargetLastSpawnIdx)
				score *= 20;

			// favor points in line of sight
			else if (CollLine_Fix(player->CameraPos, Config.SpawnPoints[i], 2, NULL, 0) == 0) {
				score *= 0.35;
			}

			score = maxf(0, score - bufferDistanceSqr);
			
			int largestIdx = 0;
			float largestIdxScore = -1;
			for (j = 0; j < 3; ++j) {
				// set if empty
				if (bestIdxScores[j] < 0) {
					largestIdx = bestIdxs[j];
					break;
				}
				// otherwise if bestscore is larger than last
				else if (largestIdxScore < 0 || bestIdxScores[j] > largestIdxScore) {
					largestIdxScore = bestIdxScores[j];
					largestIdx = bestIdxs[j];
				}
			}

			if (score < bestIdxScores[largestIdx] || bestIdxScores[largestIdx] < 0) {
				bestIdxScores[largestIdx] = score;
				bestIdxs[largestIdx] = i;
			}
		}
	}

	// pick random from best
	int pick = rand(3);
	DPRINTF("selected sp %d (score %f)\n", bestIdxs[pick], bestIdxScores[pick]);
	return bestIdxs[pick];
}

//--------------------------------------------------------------------------
void spawnTarget(SimulatedPlayer_t* sPlayer)
{
	VECTOR targetSpawnOffset;
	VECTOR finalSpawnPosition;
	GameSettings* gs = gameGetSettings();

	if (!sPlayer)
		return;

	// create
	if (!sPlayer->Created)
		createSimPlayer(sPlayer, sPlayer->Idx);

	// destroy existing
	if (sPlayer->Active)
		simPlayerKill(sPlayer, 0);

	// init vars
	struct TrainingTargetMobyPVar* pvar = &sPlayer->Vars;

	pvar->State = TARGET_STATE_IDLE;
	pvar->TimeCreated = gameGetTime();
	pvar->LifeTicks = TARGET_LIFETIME_TICKS;
	pvar->Jitter = 0;
	pvar->JumpSpeed = 5;
	pvar->StrafeSpeed = 5;
	vector_write(pvar->Velocity, 0);

	sPlayer->Player->PlayerMoby->ModeBits |= 0x1030;
	sPlayer->Player->PlayerMoby->ModeBits &= ~0x100;
	playerRespawn(sPlayer->Player);
	sPlayer->Player->Health = 1;
	sPlayer->Active = 1;

	strncpy(gs->PlayerNames[sPlayer->Player->PlayerId], NAMES[rand(NAMES_COUNT)], 16);
}

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
	VECTOR targetSpawnOffset;

	vector_write(outRot, 0);

	if (player->Team == 10)
	{
		SimulatedPlayer_t* sPlayer = &SimPlayers[player->PlayerId - 1];

		while (1) {
			// determine target spawn point
			int targetSpawnIdx = getBestSpawnPointIdx();
			
			targetSpawnOffset[0] = randRange(-1, 1);
			targetSpawnOffset[1] = randRange(-1, 1);
			targetSpawnOffset[2] = 0;
			vector_normalize(targetSpawnOffset, targetSpawnOffset);
			vector_scale(targetSpawnOffset, targetSpawnOffset, randRange(0, 1));
			State.TargetLastSpawnIdx = targetSpawnIdx;
			sPlayer->TargetLastSpawnedTime = gameGetTime();

			vector_scale(targetSpawnOffset, targetSpawnOffset, Config.SpawnPoints[targetSpawnIdx][3]);
			vector_add(targetSpawnOffset, Config.SpawnPoints[targetSpawnIdx], targetSpawnOffset);
			vector_copy(outPos, targetSpawnOffset);
				
			// snap to ground
			targetSpawnOffset[2] -= 4;
			if (CollLine_Fix(outPos, targetSpawnOffset, 2, NULL, 0)) {
				vector_copy(outPos, CollLine_Fix_GetHitPosition());
				break;
			}
		}
	}
	else
	{
		float theta = (player->PlayerId / (float)GAME_MAX_PLAYERS) * MATH_TAU;
		vector_copy(outPos, Config.PlayerSpawnPoint);
		outPos[0] += cosf(theta) * 1.5;
		outPos[1] += sinf(theta) * 1.5;
		outPos[2] += 1;
	}
}

//--------------------------------------------------------------------------
void processPlayer(int pIndex)
{
	
}

//--------------------------------------------------------------------------
void frameTick(void)
{
	int i = 0;
	char buf[32];

	// 
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		processPlayer(i);
	}

	// draw counter
	snprintf(buf, 32, "%d/%d", State.TargetsDestroyed, TARGET_GOAL);
	gfxScreenSpaceText(15, SCREEN_HEIGHT - 15, 1, 1, 0x80FFFFFF, buf, -1, 6);
}

//--------------------------------------------------------------------------
void gameTick(void)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
  GameData * gameData = gameGetData();
	Player ** players = playerGetAll();
	int i;
	char buffer[32];
	int gameTime = gameGetTime();

	*LocalBoltCount = State.Points;

  // handle game end logic
  if (State.IsHost)
  {
		if (State.TargetsDestroyed >= TARGET_GOAL)
		{
			State.GameOver = 1;
		}
		else
		{
			// keep spawning
			for (i = 0; i < MAX_SPAWNED_TARGETS; ++i)
			{
				if ((!SimPlayers[i].Active || playerIsDead(SimPlayers[i].Player)) && (gameTime - SimPlayers[i].TargetLastDestroyedTime) > TARGET_RESPAWN_DELAY && (gameTime - SimPlayers[i].TargetLastSpawnedTime) > TARGET_RESPAWN_DELAY)
				{
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
	
	POKE_U32(0x0060FCD0, 0); // prevent game from overwriting our camera overwriting (causing remote players to constantly try and turn)

	// update remote clients
	for (i = 0; i < MAX_SPAWNED_TARGETS; ++i) {
		if (SimPlayers[i].Active && SimPlayers[i].Player) {
			PlayerVTable* pVTable = playerGetVTable(SimPlayers[i].Player);
			int forceTeleportToPos = 0;

			// process input
			//((void (*)(struct PAD*))0x00527e08)(&SimPlayers[i].Pad);

			targetUpdate(&SimPlayers[i]);

			// update pad
			((void (*)(struct PAD*, void*, int))0x00527510)(&SimPlayers[i].Pad, SimPlayers[i].Pad.rdata, 0x14);

			// run game's hero update
			pVTable->Update(SimPlayers[i].Player);
		}
	}

	POKE_U32(0x0060FCD0, 0x7E022CE0); // enable camera controls
}

//--------------------------------------------------------------------------
void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx)
{
	int id = idx + 1;
	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	
	// reset
	memset(sPlayer, 0, sizeof(SimulatedPlayer_t));
	
	// spawn player
	memcpy(&sPlayer->Pad, players[0]->Paddata, sizeof(struct PAD));
	POKE_U32((u32)sPlayer + 0x98 + 0x130, (u32)sPlayer);
	sPlayer->Player = (Player*)((u32)SIM_PLAYER_BUFFER + (SIZEOF_PLAYER_OBJECT * idx)); //(Player*)0x0034AC90; // player 2 (local)

	sPlayer->Pad.rdata[7] = 0x7F;
	sPlayer->Pad.rdata[6] = 0x7F;
	sPlayer->Pad.rdata[5] = 0x7F;
	sPlayer->Pad.rdata[4] = 0x7F;
	
	// create hero
	gameSettings->PlayerClients[id] = 0;
	gameSettings->PlayerSkins[id] = 0;
	gameSettings->PlayerTeams[id] = 10;

	POKE_U32(0x0021DDDC, &sPlayer->Pad);
	//POKE_U16(0x006103DC, 0x000B);
	//POKE_U16(0x00610410, 0x000B);

	Player* lastP2 = players[1];
	((void (*)(int))0x006103d0)(1); // local hero
	players[1] = lastP2;

	players[id] = sPlayer->Player;
	memcpy(sPlayer->Player, (void*)0x0034AC90, SIZEOF_PLAYER_OBJECT);

	//POKE_U32(0x0034C730, sPlayer->Player);

	memset((void*)0x0034AC90, 0, SIZEOF_PLAYER_OBJECT);

	//POKE_U32(0x00344C3C, 0); // ensure player list only has first local player

	sPlayer->Player->Paddata = &sPlayer->Pad;
	sPlayer->Player->PlayerId = id;
	sPlayer->Player->MpIndex = id;
	sPlayer->Player->Team = 10;
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
	GameSettings* gameSettings = gameGetSettings();
	GameOptions* gameOptions = gameGetOptions();
	Player** players = playerGetAll();
	GameData* gameData = gameGetData();
	int i;

	// set target moby to NULL
	//State.CurrentTarget = NULL;

	// Disable normal game ending
	*(u32*)0x006219B8 = 0;	// survivor (8)
	*(u32*)0x00620F54 = 0;	// time end (1)
	*(u32*)0x00621568 = 0;	// kills reached (2)
	*(u32*)0x006211A0 = 0;	// all enemies leave (9)

	// point get resurrect point to ours
	*(u32*)0x00610724 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
	*(u32*)0x005e2d44 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);

	// set vtable callbacks
	//*(u32*)0x003A0BC4 = (u32)&targetGetGuber;
	//*(u32*)0x003A0BD4 = (u32)&guberHandleEvent;
	//*(u32*)0x003A0B24 = (u32)&targetGetGuber;
	//*(u32*)0x003A0B34 = (u32)&guberHandleEvent;

	// hook queue gadget event
	HOOK_JAL(0x005f037c, &queueGadgetEventHooked);

	// disable ADS
	POKE_U16(0x00528320, 0x000F);
	POKE_U16(0x00528326, 0);

	// hook messages
	netHookMessages();

  // 
  initializeScoreboard();

	// 
	cheatsApplyNoPacks();
	cheatsDisableHealthboxes();

	// 
	forcePlayerHUD();

	// force weapons
	switch (State.TrainingType)
	{
		case TRAINING_TYPE_FUSION:
		{
			gameOptions->WeaponFlags.DualVipers = 0;
			gameOptions->WeaponFlags.MagmaCannon = 0;
			gameOptions->WeaponFlags.Arbiter = 0;
			gameOptions->WeaponFlags.FusionRifle = 1;
			gameOptions->WeaponFlags.MineLauncher = 0;
			gameOptions->WeaponFlags.B6 = 0;
			gameOptions->WeaponFlags.Holoshield = 0;
			gameOptions->WeaponFlags.Flail = 0;
			break;
		}
	}

	// set default settings

	// give a 1 second delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 60 * 1;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	memset(SimPlayers, 0, sizeof(SimPlayers));

	// initialize state
	State.GameOver = 0;
	State.WinningTeam = 0;
	State.TrainingType = (enum TrainingType)gameConfig->trainingConfig.type;
	State.Points = 0;
	State.Kills = 0;
	State.Hits = 0;
	State.ShotsFired = 0;
	State.TargetsDestroyed = 0;
	State.TargetLastSpawnIdx = 0;
	State.InitializedTime = gameGetTime();

	// respawn player
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		if (players[i] && players[i]->Team != 10) {
			playerRespawn(players[i]);
		}
	}

	HOOK_JAL(0x005CE320, onSimulateHeros);

	for (i = 0; i < MAX_SPAWNED_TARGETS; ++i) {
		SimPlayers[i].Idx = i;
		SimPlayers[i].TargetLastSpawnedTime = gameGetTime() + (TARGET_INIT_SPAWN_DELAY * i);
	}

	Initialized = 1;
}

//--------------------------------------------------------------------------
void updateGameState(PatchStateContainer_t * gameState)
{
	int i;

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
		struct TrainingGameData* sGameData = (struct TrainingGameData*)gameState->CustomGameStats.Payload;
		sGameData->Version = 0x00000001;
		sGameData->Points = State.Points;
		sGameData->Kills = State.Kills;
		sGameData->Hits = State.Hits;
		sGameData->Misses = State.ShotsFired - State.Hits;
		sGameData->Time = gameGetTime() - State.InitializedTime;
	}
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
	gameOptions->GameFlags.MultiplayerGameFlags.UNK_0D = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Puma = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hovership = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickups = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 20;
	gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
	
	switch (gameConfig->trainingConfig.type)
	{
		case TRAINING_TYPE_FUSION:
		{
			gameSettings->GameLevel = 44; // sarathos
			gameSettings->GameRules = GAMERULE_DM;

			gameOptions->WeaponFlags.Chargeboots = 1;
			gameOptions->WeaponFlags.DualVipers = 0;
			gameOptions->WeaponFlags.MagmaCannon = 0;
			gameOptions->WeaponFlags.Arbiter = 0;
			gameOptions->WeaponFlags.FusionRifle = 1;
			gameOptions->WeaponFlags.MineLauncher = 0;
			gameOptions->WeaponFlags.B6 = 0;
			gameOptions->WeaponFlags.Holoshield = 0;
			gameOptions->WeaponFlags.Flail = 0;
			break;
		}
	}
}
