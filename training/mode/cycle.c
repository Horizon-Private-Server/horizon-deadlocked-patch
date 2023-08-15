/***************************************************
 * FILENAME :		cycle.c
 * 
 * DESCRIPTION :
 * 		Contains all code specific to the cycle training mode.
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
#define TARGET_POINTS_LIFETIME					(TIME_SECOND * 10)
#define TARGET_LIFETIME_TICKS						(TPS * 60)
#define TICKS_TO_RESPAWN								(TPS * 0.5)
#define TIMELIMIT_MINUTES								(5)
#define TARGET_MAX_DIST_FROM_HILL				(50)
#define COMBO_MAX_TIME_OUT_HILL_MS			(TIME_SECOND * 3)
#define MAX_B6_BALL_RING_SIZE						(5)
#define TARGET_B6_JUMP_REACTION_TIME		(TPS * 0.25)
#define TARGET_TEAM											(TEAM_PURPLE)

//--------------------------------------------------------------------------
//-------------------------- FORWARD DECLARATIONS --------------------------
//--------------------------------------------------------------------------
void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx);
float getComboMultiplier(void);
void incCombo(void);
Moby* modeOnCreateB6Bomb(int oclass, int pvarSize);
int shouldDrawHud(void);

//--------------------------------------------------------------------------
//---------------------------------- DATA ----------------------------------
//--------------------------------------------------------------------------
extern int Initialized;
extern struct TrainingMapConfig Config;

Moby* HillMoby = NULL;
int TargetTeam = TARGET_TEAM;

long TimeLastInHill = 0;

int last_names_idx = 0;

const float ComboMultiplierFactor = 0.2;
const int SimPlayerCount = MAX_SPAWNED_TARGETS;
SimulatedPlayer_t SimPlayers[MAX_SPAWNED_TARGETS];

int B6BallsRingIndex = 0;
Moby* B6Balls[MAX_B6_BALL_RING_SIZE];

// 
struct TrainingMapConfig Config __attribute__((section(".config"))) = {
	.SpawnPointCount = 0,
	.SpawnPoints = {
	},
	.PlayerSpawnPoint = { 659.35, 700.48, 99.98, -1.96 },
};

//--------------------------------------------------------------------------
//------------------------------- INITIALIZE -------------------------------
//--------------------------------------------------------------------------
void modeInitialize(void)
{
	memset(B6Balls, 0, sizeof(B6Balls));

	cheatsApplyNoPacks();
	cheatsDisableHealthboxes();

	// 
	HOOK_JAL(0x003F5FDC, &modeOnCreateB6Bomb);

	// disable ADS
#if !DEBUG
	POKE_U16(0x00528320, 0x000F);
	POKE_U16(0x00528326, 0);
#endif
}

//--------------------------------------------------------------------------
//---------------------------------- HOOKS ---------------------------------
//--------------------------------------------------------------------------
Moby* modeOnCreateB6Bomb(int oclass, int pvarSize)
{
	Moby* m = mobySpawn(oclass, pvarSize);

	if (m && m->OClass == MOBY_ID_B6_BALL0) {
		B6Balls[B6BallsRingIndex] = m;
		B6BallsRingIndex = (B6BallsRingIndex + 1) % MAX_B6_BALL_RING_SIZE;
	}

	return m;
}

//--------------------------------------------------------------------------
//---------------------------------- HILL ----------------------------------
//--------------------------------------------------------------------------
int modeGetHillSpawnPointIdx(void)
{
	if (!HillMoby || !HillMoby->PVar)
		return 1;

	return *(int*)((u32)HillMoby->PVar + 0x80);
}

//--------------------------------------------------------------------------
int modePlayerIsInHill(void)
{
	return *(int*)0x0036E150;
}

//--------------------------------------------------------------------------
//--------------------------------- SPAWN ----------------------------------
//--------------------------------------------------------------------------
void modeGetResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
	VECTOR targetSpawnOffset;

	vector_write(outRot, 0);

	if (player->Team == TARGET_TEAM)
	{
		SimulatedPlayer_t* sPlayer = &SimPlayers[player->PlayerId - 1];

		while (1) {

			int targetSpawnPointIdx = modeGetHillSpawnPointIdx();
			SpawnPoint* sp = spawnPointGet(targetSpawnPointIdx);


			targetSpawnOffset[0] = randRange(-1, 1);
			targetSpawnOffset[1] = randRange(-1, 1);
			targetSpawnOffset[2] = 0;
			vector_normalize(targetSpawnOffset, targetSpawnOffset);
			vector_scale(targetSpawnOffset, targetSpawnOffset, randRange(0, 1));
			State.TargetLastSpawnIdx = targetSpawnPointIdx;
			sPlayer->TicksToRespawn = TICKS_TO_RESPAWN;

			vector_scale(targetSpawnOffset, targetSpawnOffset, vector_length(&sp->M0[0]));
			vector_add(targetSpawnOffset, &sp->M0[12], targetSpawnOffset);
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
		// reset combo
		State.ComboCounter = 0;

		// respawn
		float theta = (player->PlayerId / (float)GAME_MAX_PLAYERS) * MATH_TAU;
		vector_copy(outPos, Config.PlayerSpawnPoint);
		outPos[0] += cosf(theta) * 1.5;
		outPos[1] += sinf(theta) * 1.5;
		outPos[2] += 1;

		outRot[2] = Config.PlayerSpawnPoint[3];
	}
}

//--------------------------------------------------------------------------
//--------------------------------- WEAPON ---------------------------------
//--------------------------------------------------------------------------
void modeOnGadgetFired(int gadgetId)
{
  if (gadgetId == WEAPON_ID_FUSION_RIFLE) {
    State.ShotsFired += 1;
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
	VECTOR delta = {0,0,0,0};
	struct TrainingTargetMobyPVar* pvar = &target->Vars;
	Player* lPlayer = playerGetFromSlot(0);

	if (colDamage) { DPRINTF("target hit by %08X : %04X\n", (u32)colDamage->Damager, colDamage->Damager ? colDamage->Damager->OClass : -1); }

	// give points
	if (pvar && colDamage && !gameHasEnded()) {
		int gadgetId = getGadgetIdFromMoby(colDamage->Damager);
		float pointMultiplier = 1;
		int pointAdditive = 0;
		int timeToKillMs = (timerGetSystemTime() - State.TimeLastKill) / SYSTEM_TIME_TICKS_PER_MS;

		// assign base points by gadget
		switch (gadgetId)
		{
			case WEAPON_ID_FUSION_RIFLE: pointAdditive += 500; break;
			case WEAPON_ID_B6: pointAdditive += 200; break;
			case WEAPON_ID_MAGMA_CANNON: pointAdditive += 100; break;
			case WEAPON_ID_WRENCH: pointAdditive += 50; break;
		}

		// handle special sniper point bonuses
		if (gadgetId == WEAPON_ID_FUSION_RIFLE) {
			
			// give bonus if jump snipe
			if (lPlayer->PlayerState == PLAYER_STATE_JUMP || lPlayer->PlayerState == PLAYER_STATE_RUN_JUMP)
				pointMultiplier += 0.25;
			if (lPlayer->PlayerState == PLAYER_STATE_FLIP_JUMP)
				pointMultiplier += 0.5;
		}
		else if (colDamage->Damager && colDamage->Damager->OClass == MOBY_ID_B6_BALL0) {

			// give bonus by accuracy of b6
			vector_subtract(delta, colDamage->Damager->Position, target->Player->PlayerPosition);
			float distance = vector_length(delta);
			pointMultiplier += 0.25 * (1 - clamp((distance - 2) / 5, 0, 1));
		}

		// determine time since last kill within range [0,1]
		// where 1 is the longest amount of time before we don't add bonus points
		// and 0 is time to kill <= 1 second
		float timeToKillR = powf(clamp(timeToKillMs / (float)TARGET_POINTS_LIFETIME, 0, 1), 0.5);

		// max points for quick kill is 1000
		int points = 200 * (1 - timeToKillR);

		// add additional points before multiplication
		points += pointAdditive;

		DPRINTF("hit %d -> %d (%.2f x)\n", points, (int)(points * pointMultiplier), pointMultiplier);

		// give points
		target->Points += (int)(points * pointMultiplier * (colDamage->DamageHp / 50));
		if (gadgetId == WEAPON_ID_FUSION_RIFLE)
			State.Hits += 1;
	}
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
		float pointMultiplier = 2;
		int pointAdditive = 0;
		int timeToKillMs = (timerGetSystemTime() - State.TimeLastKill) / SYSTEM_TIME_TICKS_PER_MS;

		// assign base points by gadget
		switch (gadgetId)
		{
			case WEAPON_ID_FUSION_RIFLE: pointAdditive += 500; break;
			case WEAPON_ID_B6: pointAdditive += 200; break;
			case WEAPON_ID_MAGMA_CANNON: pointAdditive += 100; break;
			case WEAPON_ID_WRENCH: pointAdditive += 50; break;
		}

		// add combo multiplier
		pointMultiplier += getComboMultiplier();

		// increment combo
		if (modePlayerIsInHill())
			incCombo();

		// handle special sniper point bonuses
		if (gadgetId == WEAPON_ID_FUSION_RIFLE) {
			
			// give bonus if jump snipe
			if (lPlayer->PlayerState == PLAYER_STATE_JUMP || lPlayer->PlayerState == PLAYER_STATE_RUN_JUMP)
				pointMultiplier += 0.25;
			if (lPlayer->PlayerState == PLAYER_STATE_FLIP_JUMP)
				pointMultiplier += 0.5;
		}
		else if (colDamage->Damager && colDamage->Damager->OClass == MOBY_ID_B6_BALL0) {

			// give bonus by accuracy of b6
			vector_subtract(delta, colDamage->Damager->Position, target->Player->PlayerPosition);
			float distance = vector_length(delta);
			pointMultiplier += 0.25 * (1 - clamp((distance - 2) / 5, 0, 1));
		}

		// determine time since last kill within range [0,1]
		// where 1 is the longest amount of time before we don't add bonus points
		// and 0 is time to kill <= 1 second
		float timeToKillR = powf(clamp((timeToKillMs - (TIME_SECOND * 1)) / (float)TARGET_POINTS_LIFETIME, 0, 1), 0.5);

		// max points for quick kill is 5000
		int points = 5000 * (1 - timeToKillR);

		// add additional points before multiplication
		points += pointAdditive;

		points += target->Points;
    target->Points = 0;

		DPRINTF("killed %d -> %d (%.2f x)\n", points, (int)(points * pointMultiplier), pointMultiplier);

		// give points
		State.Points += (int)(points * pointMultiplier);
		State.Kills += 1;
		if (gadgetId == WEAPON_ID_FUSION_RIFLE)
			State.Hits += 1;
		State.TimeLastKill = timerGetSystemTime();
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
	int i;
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
	u32 cycleTicks = decTimerU32(&sPlayer->TicksToCycle);
	u32 fireDelayTicks = decTimerU32(&sPlayer->TicksFireDelay);
	
	// set camera type to lock strafe
	POKE_U32(0x001711D4 + (4*sPlayer->Idx), 1);

	// kill if too far from hill
	int hillSpIdx = modeGetHillSpawnPointIdx();
	SpawnPoint* sp = spawnPointGet(hillSpIdx);
	vector_subtract(delta, target->PlayerPosition, &sp->M0[12]);
	if (vector_sqrmag(delta) > (TARGET_MAX_DIST_FROM_HILL*TARGET_MAX_DIST_FROM_HILL)) {
		modeOnTargetKilled(sPlayer, 0);
		return;
	}

  // don't move when idle
  if (State.AggroMode == TRAINING_AGGRESSION_IDLE)
    return;

	// face player
	vector_copy(delta, player->PlayerPosition);
	delta[2] += 1;
	vector_subtract(delta, delta, target->PlayerPosition);
	float len = vector_length(delta);
	float targetYaw = atan2f(delta[1] / len, delta[0] / len);
	float targetPitch = asinf(-delta[2] / len);
	sPlayer->Yaw = lerpfAngle(sPlayer->Yaw, targetYaw, 0.05);
	
	MATRIX m;
	matrix_unit(m);
	matrix_rotate_y(m, m, targetPitch);
	matrix_rotate_z(m, m, sPlayer->Yaw);
	memcpy(target->Camera->uMtx, m, sizeof(VECTOR) * 3);
	vector_copy(target->CameraDir, &m[4]);
	target->CameraYaw.Value = sPlayer->Yaw;

	// jump if b6 is near
	if (jumpTicks > TARGET_B6_JUMP_REACTION_TIME) {
		for (i = 0; i < MAX_B6_BALL_RING_SIZE; ++i)
		{
			Moby* b6ball = B6Balls[i];
			if (!b6ball || mobyIsDestroyed(b6ball) || b6ball->OClass != MOBY_ID_B6_BALL0)
				continue;
			
			vector_subtract(delta, b6ball->Position, target->PlayerPosition);
			float sqrDist = vector_sqrmag(delta);
			if (sqrDist < (8*8) && target->PlayerState != PLAYER_STATE_DOUBLE_JUMP && target->PlayerState != PLAYER_STATE_JUMP && target->PlayerState != PLAYER_STATE_RUN_JUMP) {
				sPlayer->TicksToJump = randRangeInt(0, TARGET_B6_JUMP_REACTION_TIME);
			}
		}
	}

	// 
	if (!jumpTicks) {

		// next jump
		sPlayer->TicksToJump = modeGetJumpTicks();
		sPlayer->TicksToJumpFor = randRangeInt(0, 10);
	}

	// jump
	if (jumpTicksFor) {
		pad->btns &= ~PAD_CROSS;
	}

	// cycle
	if (cycleTicks == 1 && State.AggroMode < TRAINING_AGGRESSION_PASSIVE) {
		sPlayer->CycleIdx = (sPlayer->CycleIdx + 1 /*+rand(3)*/) % 3;
		sPlayer->TicksFireDelay = randRangeInt(5, 20);
		playerEquipWeapon(target, sPlayer->CycleIdx ? (sPlayer->CycleIdx == 1 ? WEAPON_ID_FUSION_RIFLE : WEAPON_ID_B6) : WEAPON_ID_MAGMA_CANNON );
		//playerEquipWeapon(target, WEAPON_ID_FUSION_RIFLE);
	}

	// fire weapons
	if (!fireDelayTicks && target->timers.gadgetRefire == 0 && State.AggroMode < TRAINING_AGGRESSION_PASSIVE) {
		u32 fireTicks = decTimerU32(&sPlayer->TicksToFire);
		if (!fireTicks) {
			if (target->WeaponHeldId == WEAPON_ID_FUSION_RIFLE && strafeStopTicksFor > 10) {
				sPlayer->TicksToFire = 1;
			} else {
				pad->btns &= ~PAD_R1;
				sPlayer->TicksToFire = randRangeInt(5, 60);
				sPlayer->TicksToCycle = randRangeInt(2, 10);
			}
		}
	}

	// determine current strafe target
	vector_fromyaw(targetPos, sPlayer->Yaw + MATH_PI/2);
	vector_scale(targetPos, targetPos, (sPlayer->StrafeDir ? -1 : 1) * 10);
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
	matrix_rotate_z(m, m, -sPlayer->Yaw);
	vector_apply(delta, delta, m);
	delta[3] = delta[2] = 0;
	vector_normalize(delta, delta);

	if (!strafeStopTicks) {
		sPlayer->TicksToStrafeStop = randRangeInt(TPS * 1, TPS * 3);
		sPlayer->TicksToStrafeStopFor = randRangeInt(10, 20);
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
	sPlayer->Player->Health = 50;
	vector_copy(pvar->SpawnPos, sPlayer->Player->PlayerPosition);
	sPlayer->Active = 1;

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
  Player* player = playerGetAll()[pIndex];

  // infinite health
	if (player && State.AggroMode == TRAINING_AGGRESSION_AGGRO_NO_DAMAGE && !playerIsDead(player)) {
    player->Health = player->MaxHealth;
  }
}

//--------------------------------------------------------------------------
void modeTick(void)
{
	char buf[32];

	// update time last in hill
	if (modePlayerIsInHill())
		TimeLastInHill = timerGetSystemTime();

	// reset combo after max seconds outside hill
	int timeSinceLastInHill = (timerGetSystemTime() - TimeLastInHill) / SYSTEM_TIME_TICKS_PER_MS;
	if (timeSinceLastInHill > COMBO_MAX_TIME_OUT_HILL_MS)
		State.ComboCounter = 0;

	// draw combo
	float multiplier = 1 + getComboMultiplier();
	if (multiplier > 1 && shouldDrawHud()) {
		u32 color = 0x8029E5E6;
		float timeT = clamp(powf(timeSinceLastInHill / (float)COMBO_MAX_TIME_OUT_HILL_MS, 0.5), 0, 1);
		if (timeT < 1 && timeT > 0.2)
			color = colorLerp(color, 0x800000FF, timeT);

		snprintf(buf, 32, "x%.1f", multiplier);
		gfxScreenSpaceText(32+1, 130+1, 1, 1, 0x40000000, buf, -1, 1);
		gfxScreenSpaceText(32, 130, 1, 1, color, buf, -1, 1);
	}

	// find hill moby
	if (!HillMoby) {
		HillMoby = mobyFindNextByOClass(mobyListGetStart(), 0x2604);
		DPRINTF("hill moby %08X\n", (u32)HillMoby);
	}
}

//--------------------------------------------------------------------------
//------------------------------- SCOREBOARD -------------------------------
//--------------------------------------------------------------------------
void modeSetEndGameScoreboard(PatchGameConfig_t * gameConfig)
{
	u32 * uiElements = (u32*)(*(u32*)(0x011C7064 + 4*18) + 0xB0);
	int i;
	
  // title
  snprintf((char*)0x003D3AE0, 0x3F, "Cycle %s (%s)"
    , gameConfig->trainingConfig.variant == 0 ? "Ranked" : "Endless"
    , TRAINING_AGGRO_NAMES[gameConfig->trainingConfig.aggression]);
	
	// column headers start at 17
	strncpy((char*)(uiElements[18] + 0x60), "POINTS", 7);
	strncpy((char*)(uiElements[19] + 0x60), "KILLS", 6);
	strncpy((char*)(uiElements[20] + 0x60), "DEATHS", 7);
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

		// fusion accuracy
		float accuracy = 0;
		if (State.ShotsFired > 0)
			accuracy = 100 * (State.Hits / (float)State.ShotsFired);

		// set points
		sprintf((char*)(uiElements[22 + (i*4) + 0] + 0x60), "%d", State.Points);
		sprintf((char*)(uiElements[22 + (i*4) + 1] + 0x60), "%d", State.Kills);
		sprintf((char*)(uiElements[22 + (i*4) + 2] + 0x60), "%d", *(int*)(0x001e0d78 + 0x6b8));
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

  int endless = gameConfig->trainingConfig.variant == 1;

	//
	gameConfig->grNoInvTimer = 1;
	gameConfig->grV2s = 0;
	gameConfig->grVampire = 0;
	gameConfig->grBetterHills = 0;
	gameConfig->prPlayerSize = 0;
	gameConfig->prRotatingWeapons = 0;

	// teams
	gameSettings->PlayerTeams[0] = 0;

	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Vehicles = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Puma = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hovership = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.SpecialPickups = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Homenodes = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.NodeType = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.UNK_13 = 1; // KOTH
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = endless ? 0 : TIMELIMIT_MINUTES;
	gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillMovingTime = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillTimeToWin = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillArmor = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillSharing = 1;
	
  gameSettings->GameLevel = 54; // ghost station
  gameSettings->GameRules = GAMERULE_KOTH;

  gameOptions->WeaponFlags.Chargeboots = 1;
  gameOptions->WeaponFlags.DualVipers = 0;
  gameOptions->WeaponFlags.MagmaCannon = 1;
  gameOptions->WeaponFlags.Arbiter = 0;
  gameOptions->WeaponFlags.FusionRifle = 1;
  gameOptions->WeaponFlags.MineLauncher = 0;
  gameOptions->WeaponFlags.B6 = 1;
  gameOptions->WeaponFlags.Holoshield = 0;
  gameOptions->WeaponFlags.Flail = 0;
}
