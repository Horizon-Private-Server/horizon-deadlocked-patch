/***************************************************
 * FILENAME :		fusion.c
 * 
 * DESCRIPTION :
 * 		Contains all code specific to the fusion rifle training mode.
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


#define MAX_SPAWNED_TARGETS							(3)
#define TARGET_RESPAWN_DELAY						(TIME_SECOND * 1)
#define TARGET_POINTS_LIFETIME					(TIME_SECOND * 20)
#define TARGET_LIFETIME_TICKS						(TPS * 60)
#define TICKS_TO_RESPAWN								(TPS * 0.5)
#define TIMELIMIT_MINUTES								(5)
#define SIZEOF_PLAYER_OBJECT						(0x31F0)
#define TARGET_TEAM											(9)

//--------------------------------------------------------------------------
//-------------------------- FORWARD DECLARATIONS --------------------------
//--------------------------------------------------------------------------
void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx);
float getComboMultiplier(void);
void incCombo(void);

//--------------------------------------------------------------------------
//---------------------------------- DATA ----------------------------------
//--------------------------------------------------------------------------
extern int Initialized;
extern struct TrainingMapConfig Config;

int TargetTeam = TARGET_TEAM;

int last_names_idx = 0;
int waiting_for_sniper_shot = 0;

const float ComboMultiplierFactor = 0.1;
const int SimPlayerCount = MAX_SPAWNED_TARGETS;
SimulatedPlayer_t SimPlayers[MAX_SPAWNED_TARGETS];

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

//--------------------------------------------------------------------------
//------------------------------- INITIALIZE -------------------------------
//--------------------------------------------------------------------------
void modeInitialize(void)
{
	cheatsApplyNoPacks();
	cheatsDisableHealthboxes();

	// disable ADS
#if !DEBUG
	POKE_U16(0x00528320, 0x000F);
	POKE_U16(0x00528326, 0);
#endif
}

//--------------------------------------------------------------------------
//--------------------------------- SPAWN ----------------------------------
//--------------------------------------------------------------------------
int modeGetBestSpawnPointIdx(void)
{
	int pick = rand(Config.SpawnPointCount);
	return pick;

	/*
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
	*/
}

//--------------------------------------------------------------------------
void modeGetResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
	VECTOR targetSpawnOffset;

	vector_write(outRot, 0);

	if (player->Team == TARGET_TEAM)
	{
		SimulatedPlayer_t* sPlayer = &SimPlayers[player->PlayerId - 1];

		while (1) {
			// determine target spawn point
			int targetSpawnIdx = modeGetBestSpawnPointIdx();
			
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
//--------------------------------- WEAPON ---------------------------------
//--------------------------------------------------------------------------
void modeOnGadgetFired(int gadgetId)
{
  if (gadgetId == WEAPON_ID_FUSION_RIFLE) {
    State.ShotsFired += 1;
    waiting_for_sniper_shot = 10;
  }
}

//--------------------------------------------------------------------------
//--------------------------------- TARGET ---------------------------------
//--------------------------------------------------------------------------
int modeGetJumpTicks(void)
{
	if (rand(2))
		return randRangeInt(TPS * 2, TPS * 5);

	return randRangeInt(5, TPS * 1);
}

//--------------------------------------------------------------------------
void modeOnTargetHit(SimulatedPlayer_t* target, MobyColDamage* colDamage)
{

}

//--------------------------------------------------------------------------
void modeOnTargetKilled(SimulatedPlayer_t* target, MobyColDamage* colDamage)
{
	VECTOR delta = {0,0,0,0};
	struct TrainingTargetMobyPVar* pvar = &target->Vars;
	Player* lPlayer = playerGetFromSlot(0);

	// kill target
	target->Player->Health = 0;

	if (colDamage) { DPRINTF("target hit by %08X : %04X\n", (u32)colDamage->Damager, colDamage->Damager ? colDamage->Damager->OClass : -1); }

	// give points
	if (pvar && colDamage && !gameHasEnded()) {
		int gadgetId = getGadgetIdFromMoby(colDamage->Damager);
		float pointMultiplier = 1;
		int pointAdditive = 0;

    // only give points if killed by fusion
		if (gadgetId == WEAPON_ID_FUSION_RIFLE) {

			// add combo multiplier
			pointMultiplier += getComboMultiplier();

			// handle special sniper point bonuses
			if (gadgetId == WEAPON_ID_FUSION_RIFLE) {
				
				if (waiting_for_sniper_shot) {
					incCombo();
					waiting_for_sniper_shot = 0;
				}

				// give bonus if jump snipe
				if (lPlayer->PlayerState == PLAYER_STATE_JUMP || lPlayer->PlayerState == PLAYER_STATE_RUN_JUMP)
					pointMultiplier += 0.5;
				if (lPlayer->PlayerState == PLAYER_STATE_FLIP_JUMP)
					pointMultiplier += 1.0;

				// give bonus by distance
				vector_subtract(delta, target->Player->PlayerPosition, lPlayer->PlayerPosition);
				float distance = vector_length(delta);
				pointMultiplier *= 1 + clamp(powf(maxf(distance - 35, 0) / 40, 2), 0, 3);
				pointAdditive += (int)(distance * 5);
			}

      // determine time since last kill within range [0,1]
      // where 1 is the longest amount of time before we don't add bonus points
      // and 0 is time to kill <= 1 second
			int timeToKillMs = (timerGetSystemTime() - State.TimeLastKill) / SYSTEM_TIME_TICKS_PER_MS;
			float timeToKillR = powf(clamp((timeToKillMs - (TIME_SECOND * 1)) / (float)TARGET_POINTS_LIFETIME, 0, 1), 0.5);

      // max points for quick kill is 2000, min for any kill is 250
			int points = 2000 * (1 - timeToKillR);
			if (points < 250)
				points = 250;

      // add additional points before multiplication
			points += pointAdditive;

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
void modeUpdateTarget(SimulatedPlayer_t *sPlayer)
{
	VECTOR delta, targetPos;
	Player* player = playerGetFromSlot(0);
	if (!sPlayer || !player || !sPlayer->Active)
		return;

	Player* target = sPlayer->Player;
	Moby* targetMoby = target->PlayerMoby;
	struct TrainingTargetMobyPVar* pvar = &sPlayer->Vars;
	if (!pvar)
		return;
    
	struct padButtonStatus* pad = (struct padButtonStatus*)sPlayer->Pad.rdata;

	u32 jumpTicks = decTimerU32(&sPlayer->TicksToJump);
	u32 jumpTicksFor = decTimerU32(&sPlayer->TicksToJumpFor);
	u32 strafeSwitchTicks = decTimerU32(&sPlayer->TicksToStrafeSwitch);
	u32 strafeStopTicks = decTimerU32(&sPlayer->TicksToStrafeStop);
	u32 strafeStopTicksFor = decTimerU32(&sPlayer->TicksToStrafeStopFor);
	
	// timers
#if !DEBUG
	u32 lifeTicks = decTimerU32(&pvar->LifeTicks);
	if (!lifeTicks)
	{
		modeOnTargetKilled(sPlayer, 0);
		return;
	}
#endif

	// set camera type to lock strafe
	POKE_U32(0x001711D4 + (4*sPlayer->Idx), 1);

	// face player
	vector_subtract(delta, player->PlayerPosition, target->PlayerPosition);
	float len = vector_length(delta);
	float yaw = atan2f(delta[1] / len, delta[0] / len);
	
	MATRIX m;
	matrix_unit(m);
	matrix_rotate_z(m, m, yaw);
	memcpy(target->Camera->uMtx, m, sizeof(VECTOR) * 3);
	vector_copy(target->CameraDir, &m[4]);
	target->CameraYaw.Value = sPlayer->Yaw;

	// 
	if (!jumpTicks) {

		// next jump
		sPlayer->TicksToJump = modeGetJumpTicks();
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
	matrix_unit(m);
	matrix_rotate_z(m, m, -yaw);
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

}

//--------------------------------------------------------------------------
void modeInitTarget(SimulatedPlayer_t *sPlayer)
{
  GameSettings* gs = gameGetSettings();

	// init vars
	struct TrainingTargetMobyPVar* pvar = &sPlayer->Vars;

	pvar->State = TARGET_STATE_IDLE;
	pvar->TimeCreated = gameGetTime();
	pvar->LifeTicks = TARGET_LIFETIME_TICKS;
	pvar->Jitter = 0;
	pvar->JumpSpeed = 5;
	pvar->StrafeSpeed = 5;
	vector_write(pvar->Velocity, 0);

	sPlayer->TicksToJump = modeGetJumpTicks();
	sPlayer->Player->Health = 1;
	vector_copy(pvar->SpawnPos, sPlayer->Player->PlayerPosition);
	sPlayer->Active = 1;
  sPlayer->Points = 0;

	last_names_idx = (last_names_idx + 1 + rand(NAMES_COUNT)) % NAMES_COUNT;
	strncpy(gs->PlayerNames[sPlayer->Player->PlayerId], NAMES[last_names_idx], 16);

#if DEBUG
	sprintf(gs->PlayerNames[sPlayer->Player->PlayerId], "%d", sPlayer->Idx);
#endif
}

//--------------------------------------------------------------------------
//-------------------------------- GAMEPLAY --------------------------------
//--------------------------------------------------------------------------
void modeProcessPlayer(int pIndex)
{
	
}

//--------------------------------------------------------------------------
void modeTick(void)
{
	char buf[32];

	// draw combo
	if (State.ComboCounter) {
		float multiplier = 1 + getComboMultiplier();
		snprintf(buf, 32, "x%.1f", multiplier);
		gfxScreenSpaceText(32+1, 130+1, 1, 1, 0x40000000, buf, -1, 1);
		gfxScreenSpaceText(32, 130, 1, 1, 0x8029E5E6, buf, -1, 1);
	}

  // decrement fusion shot hit detect ticker
  // if this is non-zero, then we are waiting for a shot to hit
  // if no shot hits by the time this is zero
  // then we assume that's a miss and reset the combo counter
	if (waiting_for_sniper_shot) {
		--waiting_for_sniper_shot;
		
		if (!waiting_for_sniper_shot) {
			State.ComboCounter = 0;
		}
	}
}

//--------------------------------------------------------------------------
//------------------------------- SCOREBOARD -------------------------------
//--------------------------------------------------------------------------
void modeSetEndGameScoreboard(PatchGameConfig_t * gameConfig)
{
	u32 * uiElements = (u32*)(*(u32*)(0x011C7064 + 4*18) + 0xB0);
	int i;
	
	// column headers start at 17
	strncpy((char*)(uiElements[18] + 0x60), "POINTS", 7);
	strncpy((char*)(uiElements[19] + 0x60), "KILLS", 6);
	strncpy((char*)(uiElements[20] + 0x60), "ACCURACY", 9);
	strncpy((char*)(uiElements[21] + 0x60), "COMBO", 6);

	// first team score
	sprintf((char*)(uiElements[1] + 0x60), "%d", State.Points);

	// rows
	int* pids = (int*)(uiElements[0] - 0x9C);
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		// match scoreboard player row to their respective dme id
		int pid = pids[i];
		char* name = (char*)(uiElements[7 + i] + 0x18);
		if (pid != 0 || name[0] == 0)
			continue;

		float accuracy = 0;
		if (State.ShotsFired > 0)
			accuracy = 100 * (State.Hits / (float)State.ShotsFired);

		// set points
		sprintf((char*)(uiElements[22 + (i*4) + 0] + 0x60), "%d", State.Points);
		sprintf((char*)(uiElements[22 + (i*4) + 1] + 0x60), "%d", State.Kills);
		sprintf((char*)(uiElements[22 + (i*4) + 2] + 0x60), "%.2f%%", accuracy);
		sprintf((char*)(uiElements[22 + (i*4) + 3] + 0x60), "%d", State.BestCombo);
	}
}

//--------------------------------------------------------------------------
//-------------------------------- SETTINGS --------------------------------
//--------------------------------------------------------------------------
void modeSetLobbyGameOptions(PatchGameConfig_t * gameConfig)
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
}
