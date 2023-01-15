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

extern int Initialized;

#if DEBUG
int aaa = 300;
#endif

#define MAX_SPAWNED_TARGETS							(3)
#define TARGET_INIT_SPAWN_DELAY_TICKS		(TPS * 1)
#define TARGET_RESPAWN_DELAY						(TIME_SECOND * 1)
#define TARGET_POINTS_LIFETIME					(TIME_SECOND * 20)
#define TARGET_LIFETIME_TICKS						(TPS * 60)
#define TICKS_TO_RESPAWN								(TPS * 0.5)
#define TIMELIMIT_MINUTES								(5)
#define SIZEOF_PLAYER_OBJECT						(0x31F0)

void initializeScoreboard(void);
void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx);

int * LocalBoltCount = (int*)0x00171B40;

int last_names_idx = 0;
int waiting_for_sniper_shot = 0;

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

SimulatedPlayer_t SimPlayers[MAX_SPAWNED_TARGETS];

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
	return State.Points;
}

//--------------------------------------------------------------------------
float getComboMultiplier(void) {
	return State.ComboCounter * 0.25;
}

//--------------------------------------------------------------------------
int getJumpTicks(void) {
	if (rand(2))
		return randRangeInt(TPS * 2, TPS * 5);

	return randRangeInt(5, TPS * 1);
}

//--------------------------------------------------------------------------
int getGadgetIdFromMoby(Moby* moby) {
	if (!moby)
		return -1;

	switch (moby->OClass)
	{
		case MOBY_ID_DUAL_VIPER_SHOT: return WEAPON_ID_VIPERS; break;
		case MOBY_ID_MAGMA_CANNON: return WEAPON_ID_MAGMA_CANNON; break;
		case MOBY_ID_ARBITER_ROCKET0: return WEAPON_ID_ARBITER; break;
		case MOBY_ID_FUSION_RIFLE:
		case MOBY_ID_FUSION_SHOT: return WEAPON_ID_FUSION_RIFLE; break;
		case MOBY_ID_MINE_LAUNCHER_MINE: return WEAPON_ID_MINE_LAUNCHER; break;
		case MOBY_ID_B6_BOMB_EXPLOSION: return WEAPON_ID_B6; break;
		case MOBY_ID_FLAIL: return WEAPON_ID_FLAIL; break;
		case MOBY_ID_HOLOSHIELD_LAUNCHER: return WEAPON_ID_OMNI_SHIELD; break;
		case MOBY_ID_WRENCH: return WEAPON_ID_WRENCH; break;
		default: return -1;
	}
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
		switch (State.TrainingType)
		{
			case TRAINING_TYPE_FUSION:
			{
				if (gadgetId == WEAPON_ID_FUSION_RIFLE) {
					State.ShotsFired += 1;
					waiting_for_sniper_shot = 10;
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}

	// call base
	((void (*)(u64, u64, u64, u64, u64, u64, u64, u64, u64, u64))0x00627808)(a0, a1, a2, a3, t0, t1, t2, t3, t4, t5);
}

//--------------------------------------------------------------------------
void simPlayerKill(SimulatedPlayer_t* target, MobyColDamage* colDamage)
{
	VECTOR delta = {0,0,0,0};
	struct TrainingTargetMobyPVar* pvar = &target->Vars;
	Player* lPlayer = playerGetFromSlot(0);

	// spawn explosion
	//spawnExplosion(target->Player->PlayerPosition, radius);

	// 
	//target->Player->PlayerMoby->ModeBits &= ~0x30;

	// hide moby
	//playerSetPosRot(target->Player, delta, delta);
	target->Player->Health = 0;

	if (colDamage) { DPRINTF("target hit by %08X : %04X\n", (u32)colDamage->Damager, colDamage->Damager ? colDamage->Damager->OClass : -1); }

	// give points
	if (pvar && colDamage && !gameHasEnded()) {
		int givePoints = 0;
		int gadgetId = getGadgetIdFromMoby(colDamage->Damager);
		float pointMultiplier = 1;

		switch (State.TrainingType)
		{
			case TRAINING_TYPE_FUSION:
			{
				givePoints = gadgetId == WEAPON_ID_FUSION_RIFLE;

				break;
			}
			default:
			{
				givePoints = 0;
				break;
			}
		}

		if (givePoints) {

			// add combo multiplier
			pointMultiplier += getComboMultiplier();

			// handle special sniper point bonuses
			if (gadgetId == WEAPON_ID_FUSION_RIFLE) {
				
				if (waiting_for_sniper_shot) {
					incCombo();
					waiting_for_sniper_shot = 0;
				}

				// give bonus by distance
				vector_subtract(delta, target->Player->PlayerPosition, lPlayer->PlayerPosition);
				float distance = vector_length(delta);
				pointMultiplier = pointMultiplier + clamp(powf((distance - 35) / 120, 1.5), 0, 3);

				// give bonus if jump snipe
				if (lPlayer->PlayerState == PLAYER_STATE_JUMP || lPlayer->PlayerState == PLAYER_STATE_RUN_JUMP)
					pointMultiplier += 0.5;
				if (lPlayer->PlayerState == PLAYER_STATE_FLIP_JUMP)
					pointMultiplier += 1.0;
			}

			int timeToKillMs = (timerGetSystemTime() - State.TimeLastKill) / SYSTEM_TIME_TICKS_PER_MS;
			int points = (TPS*(TARGET_POINTS_LIFETIME - timeToKillMs)) / TIME_SECOND;
			if (points < 250)
				points = 250;

			DPRINTF("hit %d -> %d (%.2f x)\n", points, (int)(points * pointMultiplier), pointMultiplier);

			// give points
			State.Points += (int)(points * pointMultiplier);
			State.Kills += 1;
			State.Hits += 1;
			State.TimeLastKill = timerGetSystemTime();
		}
	}
	
	// 
	target->TicksToRespawn = TICKS_TO_RESPAWN;
	State.TargetsDestroyed += 1;
	
	// deactivate
	target->Active = 0;
}

//--------------------------------------------------------------------------
void targetUpdate(SimulatedPlayer_t *sPlayer)
{
	Player* player = playerGetFromSlot(0);
	VECTOR delta, targetPos;
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

	struct padButtonStatus* pad = (struct padButtonStatus*)sPlayer->Pad.rdata;

	u32 lifeTicks = decTimerU32(&pvar->LifeTicks);
	u32 jumpTicks = decTimerU32(&sPlayer->TicksToJump);
	u32 jumpTicksFor = decTimerU32(&sPlayer->TicksToJumpFor);
	u32 strafeSwitchTicks = decTimerU32(&sPlayer->TicksToStrafeSwitch);
	u32 strafeStopTicks = decTimerU32(&sPlayer->TicksToStrafeStop);
	u32 strafeStopTicksFor = decTimerU32(&sPlayer->TicksToStrafeStopFor);
	
	// register post draw function
	//gfxRegisterDrawFunction((void**)0x0022251C, &targetPostDraw, target);

	// timers
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

	// reset pad
	pad->btns = 0xFFFF;

	// 
	if (!jumpTicks) {

		// next jump
		sPlayer->TicksToJump = getJumpTicks();
		sPlayer->TicksToJumpFor = randRangeInt(0, 5);
	}

	// jump
	if (jumpTicksFor) {
		pad->btns &= ~PAD_CROSS;
	}


	// determine current strafe target
	vector_fromyaw(targetPos, yaw + MATH_PI/2);
	vector_scale(targetPos, targetPos, (sPlayer->StrafeDir ? -1 : 1) * 5);
	vector_add(targetPos, pvar->SpawnPos, targetPos);

	// get distance to target
	vector_subtract(delta, targetPos, target->PlayerPosition);
	delta[3] = delta[2] = 0;
	float distanceToTargetPos = vector_length(delta);
	if (distanceToTargetPos < 0.5 || !strafeSwitchTicks) {
		sPlayer->StrafeDir = !sPlayer->StrafeDir;
		sPlayer->TicksToStrafeSwitch = randRangeInt(15, TPS * 1);
	}

	// move to target
	MATRIX m;
	matrix_unit(m);
	//matrix_rotate_z(m, m, -yaw);
	vector_apply(delta, delta, m);
	delta[3] = delta[2] = 0;
	vector_normalize(delta, delta);

	if (!strafeStopTicks) {
		sPlayer->TicksToStrafeStop = randRangeInt(TPS * 2, TPS * 5);
		sPlayer->TicksToStrafeStopFor = randRangeInt(5, 30);
	}

	if (strafeStopTicksFor) {
		delta[0] = delta[1] = 0;
	}

	pad->ljoy_h = (u8)(((clamp(-delta[1] * 1.5, -1, 1) + 1) / 2) * 255);
	pad->ljoy_v = (u8)(((clamp(-delta[0] * 1.5, -1, 1) + 1) / 2) * 255);

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
			simPlayerKill(sPlayer, colDamage);
		} else if (isOwner) {
			simPlayerKill(sPlayer, colDamage);	
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

	//float minDistanceSqr = TARGET_MIN_SPAWN_DISTANCE[(int)State.TrainingType] * TARGET_MIN_SPAWN_DISTANCE[(int)State.TrainingType];
	//float idealDistanceSqr = TARGET_IDEAL_SPAWN_DISTANCE[(int)State.TrainingType] * TARGET_IDEAL_SPAWN_DISTANCE[(int)State.TrainingType];
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
	int pick = rand(2);
	DPRINTF("selected sp %d (score %f)\n", bestIdxs[pick], bestIdxScores[pick]);
	return bestIdxs[pick];
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
	vector_copy(pvar->SpawnPos, sPlayer->Player->PlayerPosition);
	sPlayer->TicksToJump = getJumpTicks();
	sPlayer->Player->Health = 1;
	sPlayer->Active = 1;

	last_names_idx = (last_names_idx + 1 + rand(NAMES_COUNT)) % NAMES_COUNT;
	strncpy(gs->PlayerNames[sPlayer->Player->PlayerId], NAMES[last_names_idx], 16);

#if DEBUG
	sprintf(gs->PlayerNames[sPlayer->Player->PlayerId], "%d", sPlayer->Idx);
#endif
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
			sPlayer->TicksToRespawn = TICKS_TO_RESPAWN;

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
	int nowTime = gameGetTime();
	GameData* gameData = gameGetData();
	GameOptions* gameOptions = gameGetOptions();

	// 
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		processPlayer(i);
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

	// draw combo
	if (State.ComboCounter) {
		float multiplier = 1 + getComboMultiplier();
		snprintf(buf, 32, "x%.2f", multiplier);
		gfxScreenSpaceText(32+1, 130+1, 1, 1, 0x40000000, buf, -1, 1);
		gfxScreenSpaceText(32, 130, 1, 1, 0x8029E5E6, buf, -1, 1);
	}
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
		for (i = 0; i < MAX_SPAWNED_TARGETS; ++i)
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

	if (waiting_for_sniper_shot) {
		--waiting_for_sniper_shot;
		
		if (!waiting_for_sniper_shot) {
			State.ComboCounter = 0;
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
		if (SimPlayers[i].Player) {

			// process input
			//((void (*)(struct PAD*))0x00527e08)(&SimPlayers[i].Pad);

			targetUpdate(&SimPlayers[i]);

			// update pad
			((void (*)(struct PAD*, void*, int))0x00527510)(&SimPlayers[i].Pad, SimPlayers[i].Pad.rdata, 0x14);

			// run game's hero update
			//pVTable->Update(SimPlayers[i].Player);
		}
	}

	POKE_U32(0x0060FCD0, 0x7E022CE0); // enable camera controls
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
	POKE_U32((u32)sPlayer + 0x98 + 0x130, (u32)sPlayer);
	
	sPlayer->Pad.rdata[7] = 0x7F;
	sPlayer->Pad.rdata[6] = 0x7F;
	sPlayer->Pad.rdata[5] = 0x7F;
	sPlayer->Pad.rdata[4] = 0x7F;
	
	// create hero
	gameSettings->PlayerClients[id] = 0;
	gameSettings->PlayerSkins[id] = 0;
	gameSettings->PlayerTeams[id] = 10;

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
#if !DEBUG
	POKE_U16(0x00528320, 0x000F);
	POKE_U16(0x00528326, 0);
#endif

	// set bolt count to 0
	*LocalBoltCount = 0;

	// hook messages
	netHookMessages();

  // 
  initializeScoreboard();

	// 
	cheatsApplyNoPacks();
	cheatsDisableHealthboxes();

	// 
	forcePlayerHUD();

	// set default settings

	// give a small delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 60;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	memset(SimPlayers, 0, sizeof(SimPlayers));
	memset(&State, 0, sizeof(State));

	// initialize state
	State.TrainingType = (enum TrainingType)gameConfig->trainingConfig.type;
	State.TimeLastKill = timerGetSystemTime();
	State.InitializedTime = gameGetTime();

	// respawn player
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		if (players[i] && players[i]->Team != 10) {
			playerRespawn(players[i]);
		}
	}

	HOOK_JAL(0x005CE320, onSimulateHeros);
	//POKE_U32(0x005CE350, 0x10000011);

	for (i = 0; i < MAX_SPAWNED_TARGETS; ++i) {
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
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = TIMELIMIT_MINUTES;
	gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
	
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
