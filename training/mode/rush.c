/***************************************************
 * FILENAME :		rush.c
 * 
 * DESCRIPTION :
 * 		Contains all code specific to the rushing training mode.
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
#include "common.h"
#include "include/game.h"
#include "include/utils.h"


#define MAX_SPAWNED_TARGETS							(3)
#define TARGET_RESPAWN_DELAY						(TIME_SECOND * 1)
#define TARGET_POINTS_LIFETIME					(TIME_SECOND * 10)
#define TARGET_LIFETIME_TICKS						(TPS * 5)
#define TICKS_TO_RESPAWN								(TPS * 5)
#define TICKS_TO_QUICK_RESPAWN					(TPS * 0.5)
#define TIMELIMIT_MINUTES								(5)
#define TARGET_MAX_DIST_FROM_SPAWN_POS	(10)
#define MAX_B6_BALL_RING_SIZE						(5)
#define TARGET_B6_JUMP_REACTION_TIME		(TPS * 0.25)
#define MAX_SPAWNPOINTS                 (120)
#define MAX_REC_SPAWNPOINTS             (2)
#define MIN_SPAWN_DISTANCE_FROM_PLAYER  (10)

//
enum REC_SPAWN_TYPES
{
  REC_SPAWN_TYPE_DEF,
  REC_SPAWN_TYPE_MID,
  REC_SPAWN_TYPE_ATT,
  REC_SPAWN_TYPE_COUNT
};

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

long TimeSinceLastCap = 0;

int TargetTeam = 0;

int last_names_idx = 0;

const float ComboMultiplierFactor = 0.2;
const int SimPlayerCount = MAX_SPAWNED_TARGETS;
SimulatedPlayer_t SimPlayers[MAX_SPAWNED_TARGETS];
int FrequentedSpawnPoints[MAX_SPAWNPOINTS];
int FrequentedSpawnPointsTotal = 0;
int RecommendedSpawnPoints[REC_SPAWN_TYPE_COUNT][MAX_REC_SPAWNPOINTS];

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
  memset(RecommendedSpawnPoints, 0, sizeof(RecommendedSpawnPoints));
  memset(FrequentedSpawnPoints, 0, sizeof(FrequentedSpawnPoints));

	cheatsApplyNoPacks();
	cheatsDisableHealthboxes();

	// 
	HOOK_JAL(0x003F5FDC, &modeOnCreateB6Bomb);
  TimeSinceLastCap = timerGetSystemTime();
  State.ComboCounter = 0;

  // 
  Player* p = playerGetFromSlot(0);
  if (p) TargetTeam = ((p->Team + 1) % 2) + (p->Team - (p->Team % 2));
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
//----------------------------------- CTF ----------------------------------
//--------------------------------------------------------------------------
Moby* modeGetOurFlag(void)
{
  static Moby* flag = NULL;
  Moby* m = mobyListGetStart();
  Player* p = playerGetFromSlot(0);

  if (!p || !m) return flag;

  switch (p->Team)
  {
    case TEAM_BLUE: flag = mobyFindNextByOClass(m, MOBY_ID_BLUE_FLAG); break;
    case TEAM_RED: flag = mobyFindNextByOClass(m, MOBY_ID_RED_FLAG); break;
    case TEAM_GREEN: flag = mobyFindNextByOClass(m, MOBY_ID_GREEN_FLAG); break;
    case TEAM_ORANGE: flag = mobyFindNextByOClass(m, MOBY_ID_ORANGE_FLAG); break;
  }

  return flag;
}

//--------------------------------------------------------------------------
Moby* modeGetTargetFlag(void)
{
  static Moby* flag = NULL;
  Moby* m = mobyListGetStart();

  if (!m) return flag;

  switch (TargetTeam)
  {
    case TEAM_BLUE: flag = mobyFindNextByOClass(m, MOBY_ID_BLUE_FLAG); break;
    case TEAM_RED: flag = mobyFindNextByOClass(m, MOBY_ID_RED_FLAG); break;
    case TEAM_GREEN: flag = mobyFindNextByOClass(m, MOBY_ID_GREEN_FLAG); break;
    case TEAM_ORANGE: flag = mobyFindNextByOClass(m, MOBY_ID_ORANGE_FLAG); break;
  }

  return flag;
}


//--------------------------------------------------------------------------
//--------------------------------- SPAWN ----------------------------------
//--------------------------------------------------------------------------
int modeIsPlayerNearSpawnPoint(int spIdx)
{
  SpawnPoint* sp = spawnPointGet(spIdx);
  VECTOR dt;
  int i;

  Player** players = playerGetAll();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* p = players[i];
    if (!p || playerIsDead(p)) continue;

    vector_subtract(dt, p->PlayerPosition, &sp->M0[12]);
    if (vector_sqrmag(dt) < (MIN_SPAWN_DISTANCE_FROM_PLAYER*MIN_SPAWN_DISTANCE_FROM_PLAYER)) return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------
void modeGetResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
  VECTOR up = {0,0,1,0};

  playerGetSpawnpoint(player, outPos, outRot, 1);

  if (player->PlayerId == 0) {
    Moby* flag = modeGetOurFlag();
    if (flag) {
      vector_copy(outPos, flag->Position);
      vector_write(outRot, 0);
    }
  } else {

    // try use recommended spawns
    int recIdx = player->PlayerId - 1;
    int r = 1 + rand(MAX_REC_SPAWNPOINTS);
    int i = 0, loop = 0;
    modeGenerateRecommendedSpawnpoints(recIdx);
    while (r) {
      loop = 0;
      i = (i + 1) % MAX_REC_SPAWNPOINTS;

      //while ((RecommendedSpawnPoints[recIdx][i] == 0 || modeIsPlayerNearSpawnPoint(RecommendedSpawnPoints[recIdx][i])) && loop < MAX_REC_SPAWNPOINTS) {
      while (RecommendedSpawnPoints[recIdx][i] == 0 && loop < MAX_REC_SPAWNPOINTS) {
        i = (i + 1) % MAX_REC_SPAWNPOINTS;
        ++loop;
      }

      if (loop >= MAX_REC_SPAWNPOINTS) return;
      --r;
    }

    if (RecommendedSpawnPoints[recIdx][i]) {
      SpawnPoint* sp = spawnPointGet(RecommendedSpawnPoints[recIdx][i]);
      vector_add(outPos, up, &sp->M0[12]);
      DPRINTF("spawning %d with recommended %d %f %f %f\n", recIdx, RecommendedSpawnPoints[recIdx][i], outPos[0], outPos[1], outPos[2]);
    }
  }

}

//--------------------------------------------------------------------------
void modeTallyNearbySpawnpoints(void)
{
  int i;
  VECTOR dt;
  Player* p = playerGetFromSlot(0);
  if (!p || playerIsDead(p)) return;

  int spCount = spawnPointGetCount();
  for (i = 0; i < spCount && i < MAX_SPAWNPOINTS; ++i) {
    if (!spawnPointIsPlayer(i)) continue;

    // give score to nearby spawn points
    // closer to sp, more points it gets
    vector_subtract(dt, p->PlayerPosition, &spawnPointGet(i)->M0[12]);
    float dist = vector_sqrmag(dt);
    int score = maxf(0, (400 - dist));
    FrequentedSpawnPoints[i] += score;
    FrequentedSpawnPointsTotal += score;
  }
}

//--------------------------------------------------------------------------
void modeResetFrequentedSpawnpoints(void)
{
  int i;
  FrequentedSpawnPointsTotal = 0;
  for (i = 0; i < MAX_SPAWNPOINTS; ++i) {
    FrequentedSpawnPointsTotal += FrequentedSpawnPoints[i] /= 15;
  }
}

//--------------------------------------------------------------------------
int modeComputeSpawnpointScore(enum REC_SPAWN_TYPES type, int spIdx)
{
  if (spIdx >= spawnPointGetCount()) return 0;
  if (!spawnPointIsPlayer(spIdx)) return 0;

  int i,j;
  VECTOR dt;
  VECTOR spPos;
  VECTOR playerPos;
  VECTOR up = {0,0,1,0};
  SpawnPoint* sp = spawnPointGet(spIdx);
  int score = (FrequentedSpawnPoints[spIdx] / (float)(FrequentedSpawnPointsTotal+1)) * 500;

  Moby* targetFlag = modeGetTargetFlag();
  Moby* playerFlag = modeGetOurFlag();
  Player* player = playerGetFromSlot(0);

  vector_add(spPos, up, &sp->M0[12]);
  vector_add(playerPos, player->PlayerPosition, up);
  int canSeePlayer = !CollLine_Fix(spPos, playerPos, 2, 0, 0);

  vector_scale(dt, player->Velocity, 30);
  vector_add(dt, dt, playerPos);
  int willSeePlayer = !CollLine_Fix(spPos, dt, 2, 0, 0);

  switch (type)
  {
    case REC_SPAWN_TYPE_DEF:
    {
      vector_subtract(dt, spPos, (float*)targetFlag->PVar);
      if (vector_sqrmag(dt) > (50*50)) return 0;
      break;
    }
    case REC_SPAWN_TYPE_ATT:
    {
      vector_subtract(dt, spPos, (float*)playerFlag->PVar);
      if (vector_sqrmag(dt) > (50*50)) return 0;
      break;
    }
    case REC_SPAWN_TYPE_MID:
    {
      if (modeIsPlayerNearSpawnPoint(spIdx)) return 0;

      // try and spawn towards where player is going
      Moby* target = targetFlag;
      if (!flagIsAtBase(targetFlag))
        target = playerFlag;

      vector_subtract(dt, spPos, targetFlag->Position);
      float distToFlag = vector_length(dt);
      vector_subtract(dt, spPos, (float*)target->PVar);
      float distToTarget = vector_length(dt);

      if (!canSeePlayer)
        distToFlag = 10000;
      if (target == playerFlag)
        score /= 5;

      score += (200 / distToFlag) + (1000 / distToTarget);
      break;
    }
  }

  score += (canSeePlayer ? 300 : 0) + (willSeePlayer ? 500 : 0);
  return score;
}

//--------------------------------------------------------------------------
void modeGenerateRecommendedSpawnpoints(enum REC_SPAWN_TYPES type)
{
  int i,j;
  VECTOR dt;

  int pts[MAX_REC_SPAWNPOINTS];
  int idxs[MAX_REC_SPAWNPOINTS];
  int count = 0;
  int spCount = spawnPointGetCount();

  memset(pts, 0, sizeof(pts));
  memset(idxs, 0, sizeof(idxs));

  for (i = 0; i < spCount; ++i) {
    int add = 0;

    int score = modeComputeSpawnpointScore(type, i);
    if (score > 0) {
      if (count < MAX_REC_SPAWNPOINTS) {
        add = 1;
      } else {
        for (j = 0; j < MAX_REC_SPAWNPOINTS; ++j) {
          if (score > pts[j]) {
            add = 1;
            break;
          }
        }
      }
    }

    if (add) {
      for (j = 0; j < (MAX_REC_SPAWNPOINTS-1); ++j) {
        if (score > pts[j]) {
          memmove(&pts[j+1], &pts[j], sizeof(int) * (MAX_REC_SPAWNPOINTS - j - 1));
          memmove(&idxs[j+1], &idxs[j], sizeof(int) * (MAX_REC_SPAWNPOINTS - j - 1));
          break;
        }
      }

      pts[j] = score;
      idxs[j] = i;
      count = j+1;
    }
  }
  
  if (count) {

    // merge old into new
    for (i = 0; i < MAX_REC_SPAWNPOINTS && count < MAX_REC_SPAWNPOINTS; ++i) {
      for (j = 0; j < count; ++j) {
        if (RecommendedSpawnPoints[type][i] == idxs[j]) break;
      }

      if (j == count) {
        idxs[j] = RecommendedSpawnPoints[type][i];
        ++count;
      }
    }

    memcpy(RecommendedSpawnPoints[type], idxs, sizeof(idxs));
  }
  
#if DEBUG
  printf("new %d recommended spawn pts: ", count);
  for (i = 0; i < count; ++i)
    printf("%d:%d ", idxs[i], pts[i]);
  printf("\n");
#endif

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
  
}

//--------------------------------------------------------------------------
void modeOnTargetKilled(SimulatedPlayer_t* target, MobyColDamage* colDamage)
{
	VECTOR delta = {0,0,0,0};
	struct TrainingTargetMobyPVar* pvar = &target->Vars;
	Player* lPlayer = playerGetFromSlot(0);

	// kill target
	target->Player->Health = 0;

  // give health to killer
  if (colDamage) {
    lPlayer->Health = lPlayer->MaxHealth;
  }

	// 
	target->TicksToRespawn = colDamage ? TICKS_TO_RESPAWN : TICKS_TO_QUICK_RESPAWN;
	State.TargetsDestroyed += 1;
	
	// deactivate
	target->Active = 0;
}

//--------------------------------------------------------------------------
void modeUpdateTarget(SimulatedPlayer_t *sPlayer)
{
	int i;
	VECTOR delta, targetPos;
  VECTOR projectedPlayerPos;
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

  // no
  if (playerIsDead(target)) return;

	// kill if too far from spawn pos
	vector_subtract(delta, target->PlayerPosition, pvar->SpawnPos);
	if (vector_sqrmag(delta) > (TARGET_MAX_DIST_FROM_SPAWN_POS*TARGET_MAX_DIST_FROM_SPAWN_POS)) {
		modeOnTargetKilled(sPlayer, 0);
		return;
	}

  // compute next position of player
  vector_scale(delta, player->Velocity, 5);
  vector_add(projectedPlayerPos, player->PlayerPosition, delta);

  // get distance to target
	vector_subtract(delta, projectedPlayerPos, target->PlayerPosition);
	float len = vector_length(delta);

  // check if can see target
  VECTOR targetPosUp = {0,0,1,0};
  VECTOR playerPosUp = {0,0,1,0};
  vector_add(targetPosUp, targetPosUp, target->PlayerPosition);
  vector_add(playerPosUp, playerPosUp, player->PlayerPosition);
  int canSeeTarget = !CollLine_Fix(targetPosUp, playerPosUp, 2, 0, 0);
  if (canSeeTarget) sPlayer->TicksSinceSeenPlayer = 0;
  else sPlayer->TicksSinceSeenPlayer += 1;

  // kill if not near target and life ran out
	u32 lifeTicks = decTimerU32(&pvar->LifeTicks);
	if (!lifeTicks && ((len > 20 && sPlayer->TicksSinceSeenPlayer > (TPS * 5)) || playerIsDead(player))) {
    if (!playerIsDead(target)) modeOnTargetKilled(sPlayer, 0);
		return;
	}

  // don't move when idle
  if (State.AggroMode == TRAINING_AGGRESSION_IDLE)
    return;

  // is player facing us or near us
  int shouldStrafe = vector_innerproduct(delta, player->CameraForward) < 0 || len < 40;
	
	// face player
	float targetYaw = atan2f(delta[1] / len, delta[0] / len);
	float targetPitch = asinf((-delta[2] + 1) / len);
	sPlayer->Yaw = lerpfAngle(sPlayer->Yaw, targetYaw, 0.13);
	sPlayer->Pitch = lerpfAngle(sPlayer->Pitch, targetPitch, 0.13);

	MATRIX m;
	matrix_unit(m);
	matrix_rotate_y(m, m, sPlayer->Pitch);
	matrix_rotate_z(m, m, sPlayer->Yaw);
	memcpy(target->Camera->uMtx, m, sizeof(VECTOR) * 3);
	vector_copy(target->CameraDir, &m[4]);
	target->CameraYaw.Value = sPlayer->Yaw;
	target->CameraPitch.Value = sPlayer->Pitch;

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
	if (jumpTicksFor && shouldStrafe) {
		pad->btns &= ~PAD_CROSS;
	}

	// cycle
	if (cycleTicks == 1 && State.AggroMode < TRAINING_AGGRESSION_PASSIVE) {
		sPlayer->CycleIdx = (sPlayer->CycleIdx + 1 /*+rand(3)*/) % 3;
    if (len > 30) sPlayer->CycleIdx = 1;
		sPlayer->TicksFireDelay = randRangeInt(3, 10);
		playerEquipWeapon(target, sPlayer->CycleIdx ? (sPlayer->CycleIdx == 1 ? WEAPON_ID_FUSION_RIFLE : WEAPON_ID_B6) : WEAPON_ID_MAGMA_CANNON );
		//playerEquipWeapon(target, WEAPON_ID_FUSION_RIFLE);
	}

	// fire weapons
	if (!fireDelayTicks && target->timers.gadgetRefire == 0 && State.AggroMode < TRAINING_AGGRESSION_PASSIVE) {
		u32 fireTicks = decTimerU32(&sPlayer->TicksToFire);
		if (!fireTicks) {
			if (target->WeaponHeldId == WEAPON_ID_FUSION_RIFLE && (strafeStopTicksFor <= 0 || strafeStopTicksFor > 5)) {
				sPlayer->TicksToFire = 1;
			} else {
				pad->btns &= ~PAD_R1;
				sPlayer->TicksToFire = randRangeInt(10, 30);
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
    if (target->WeaponHeldId == WEAPON_ID_FUSION_RIFLE) sPlayer->TicksToStrafeStop = randRangeInt(TPS * 0.7, TPS * 1.0);
		else sPlayer->TicksToStrafeStop = randRangeInt(TPS * 1, TPS * 3);
		sPlayer->TicksToStrafeStopFor = randRangeInt(10, 20);
	}

	if (strafeStopTicksFor || !shouldStrafe) {
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
  if (gs->PlayerNames[sPlayer->Player->PlayerId][0] == 0)
	  strncpy(gs->PlayerNames[sPlayer->Player->PlayerId], NAMES[last_names_idx], 16);

#if DEBUG
	sprintf(gs->PlayerNames[sPlayer->Player->PlayerId], "%d", sPlayer->Idx);
#endif
}

//--------------------------------------------------------------------------
//-------------------------------- GAMEPLAY --------------------------------
//--------------------------------------------------------------------------
int modeGetPlayerCurrentTimeMs(void)
{
  int capTimeMs = (int)((timerGetSystemTime() - TimeSinceLastCap) / SYSTEM_TIME_TICKS_PER_MS);

  return maxf(0, capTimeMs - State.ComboCounter * TIME_SECOND * 10);
}

//--------------------------------------------------------------------------
void modeProcessPlayer(int pIndex)
{
  static int playerDeadLastFrame = 0;
  if (pIndex != 0)
    return;
    
  Player* player = playerGetAll()[pIndex];
  GameData* gameData = gameGetData();
  Moby* targetFlag = modeGetTargetFlag();
  int playerDead = playerIsDead(player);

  // infinite health
	if (player && State.AggroMode == TRAINING_AGGRESSION_AGGRO_NO_DAMAGE && !playerDead) {
    player->Health = player->MaxHealth;
  }

  // reset time since last cap on each player spawn
  if (!playerDead && playerDeadLastFrame) {
    TimeSinceLastCap = timerGetSystemTime();
  }

  // heal on cap
  static int caps = 0;
  if (gameData && gameData->PlayerStats.CtfFlagsCaptures[pIndex] != caps) {
    caps = gameData->PlayerStats.CtfFlagsCaptures[pIndex];

    // heal
    player->Health = player->MaxHealth;

    // points
    float capTime = modeGetPlayerCurrentTimeMs() / 1000.0;
    State.Points += 1000 / logf(capTime * 0.05);

    // reset combo
    State.ComboCounter = 0;
    
    TimeSinceLastCap = timerGetSystemTime();
  }

  playerDeadLastFrame = playerDead;
}

//--------------------------------------------------------------------------
void modeTick(void)
{
	char buf[32];
  int i;
  static int tallyTicker = 60;
  static int recSpTicker = 10;
  static int resetTallyTicker = 10;

  Player* player = playerGetFromSlot(0);
  Moby* targetFlag = modeGetTargetFlag();

	// draw computed time since last cap
  int capTimeMs = modeGetPlayerCurrentTimeMs();
	if (shouldDrawHud()) {
		float timeT = clamp(powf(capTimeMs / (float)60000, 2), 0, 1);
		u32 color = colorLerp(0x8029E5E6, 0x800000FF, timeT);

		snprintf(buf, 32, "%02d", capTimeMs / 1000);
    gfxHelperDrawText(0, 130, 32 + 1, 1, 1, 0x40000000, buf, -1, 1, COMMON_DZO_DRAW_NORMAL);
    gfxHelperDrawText(0, 130, 32, 0, 1, color, buf, -1, 1, COMMON_DZO_DRAW_NORMAL);
	}

  --tallyTicker;
  if (tallyTicker <= 0) {
    modeTallyNearbySpawnpoints();
    tallyTicker = 60;

    --recSpTicker;
    if (recSpTicker <= 0) {
      recSpTicker = 10;
      //modeGenerateRecommendedSpawnpoints(REC_SPAWN_TYPE_DEF);
      //modeGenerateRecommendedSpawnpoints(REC_SPAWN_TYPE_MID);
      //modeGenerateRecommendedSpawnpoints(REC_SPAWN_TYPE_ATT);
    }

    --resetTallyTicker;
    if (resetTallyTicker <= 0) {
      resetTallyTicker = 10;
      modeResetFrequentedSpawnpoints();
    }
  }
  
  // if (0)
  // {
  //   static int renderSpawnsTicker = 0;
  //   if (renderSpawnsTicker <= 0) {
  //     modeGenerateRecommendedSpawnpoints(REC_SPAWN_TYPE_MID);
  //     renderSpawnsTicker = 60;
  //   } else {
  //     renderSpawnsTicker--;
  //   }

  //   int x,y;
  //   for (i = 0; i < MAX_REC_SPAWNPOINTS; ++i) {
  //     int spIdx = RecommendedSpawnPoints[REC_SPAWN_TYPE_MID][i];
  //     if (spIdx > 0) {
  //       SpawnPoint* sp = spawnPointGet(spIdx);
  //       if (gfxWorldSpaceToScreenSpace(&sp->M0[12], &x, &y)) {
  //         gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "+", -1, 4);
  //       }
  //     }
  //   }

  //   if (gfxWorldSpaceToScreenSpace((float*)modeGetOurFlag()->PVar, &x, &y)) {
  //     gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "+", -1, 4);
  //   }
  // }

  // reset flag when player dies
  if (player && targetFlag && playerIsDead(player) && !flagIsAtBase(targetFlag)) {
    flagReturnToBase(targetFlag, 0, -1);

    // reset bots
    if (0) {
      modeResetFrequentedSpawnpoints();
      tallyTicker = 60;
      recSpTicker = 10;
      resetTallyTicker = 60;
      memset(RecommendedSpawnPoints, 0, sizeof(RecommendedSpawnPoints));
      for (i = 0; i < SimPlayerCount; ++i) {
        spawnTarget(&SimPlayers[i]);
      }
    }
  }
}

//--------------------------------------------------------------------------
//------------------------------- SCOREBOARD -------------------------------
//--------------------------------------------------------------------------
void modeSetEndGameScoreboard(PatchGameConfig_t * gameConfig)
{
  GameData * gameData = gameGetData();
	u32 * uiElements = (u32*)(*(u32*)(0x011C7064 + 4*18) + 0xB0);
	int i;
	
  // title
  snprintf((char*)0x003D3AE0, 0x3F, "Rush %s (%s)"
    , gameConfig->trainingConfig.variant == 0 ? "Ranked" : "Endless"
    , TRAINING_AGGRO_NAMES[gameConfig->trainingConfig.aggression]);
	
	// column headers start at 17
	strncpy((char*)(uiElements[18] + 0x60), "POINTS", 7);
	strncpy((char*)(uiElements[19] + 0x60), "KILLS", 6);
	strncpy((char*)(uiElements[20] + 0x60), "DEATHS", 7);
	strncpy((char*)(uiElements[21] + 0x60), "CAPS", 6);

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

		// set points
		sprintf((char*)(uiElements[22 + (i*4) + 0] + 0x60), "%d", State.Points);
		sprintf((char*)(uiElements[22 + (i*4) + 1] + 0x60), "%d", State.Kills);
		sprintf((char*)(uiElements[22 + (i*4) + 2] + 0x60), "%d", *(int*)(0x001e0d78 + 0x6b8));
		sprintf((char*)(uiElements[22 + (i*4) + 3] + 0x60), "%d", gameData->PlayerStats.CtfFlagsCaptures[pid]);
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
	gameConfig->grBetterHills = 1;
	gameConfig->grHalfTime = 0;
	gameConfig->grOvertime = 0;
	gameConfig->prPlayerSize = 0;
	gameConfig->prRotatingWeapons = 0;

	// teams
	if (!endless) gameSettings->PlayerTeams[0] = (gameSettings->PlayerTeams[0] % 4);

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
	gameOptions->GameFlags.MultiplayerGameFlags.CapsToWin = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.CrazyMode = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.FlagReturn = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.FlagVehicleCarry = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.UNK_12 = 1; // CTF
	gameOptions->GameFlags.MultiplayerGameFlags.UNK_13 = 0; // KOTH
	gameOptions->GameFlags.MultiplayerGameFlags.UNK_25 = 2; // CTF SPAWNS
	gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = endless ? 0 : TIMELIMIT_MINUTES;
	gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillMovingTime = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillTimeToWin = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillArmor = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillSharing = 1;
	
  if (!endless) gameSettings->GameLevel = 53; // maraxus
  gameSettings->GameRules = GAMERULE_CTF;

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
