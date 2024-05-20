/***************************************************
 * FILENAME :		game.c
 * 
 * DESCRIPTION :
 * 		TEAM DEFENDER.
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
#include "include/ball.h"
#include "include/utils.h"

extern int Initialized;

int tryEquipBall(int playerIdx);
int playerStateCanHoldBall(Player * player);
int (*baseFlagHandleGuberEvent)(Moby*, GuberEvent*) = (int (*)(Moby*, GuberEvent*))0x00417BB8;

char ScoreMessageBuffer[64];

//--------------------------------------------------------------------------
void onSetPlayerStats(int playerId, struct CGMPlayerStats* stats)
{
	memcpy(&State.PlayerStates[playerId].Stats, stats, sizeof(struct CGMPlayerStats));
}

//--------------------------------------------------------------------------
void onReceiveTeamScore(int teamScores[2])
{
  State.Teams[0].Score = teamScores[0];
  State.Teams[1].Score = teamScores[1];
}

//--------------------------------------------------------------------------
void onReceiveBallPickupRequest(int fromPlayerIdx)
{
  if (!State.BallMoby || !isInGame()) return;

  Player* player = playerGetAll()[fromPlayerIdx];
  if (!player) return;

  if (playerStateCanHoldBall(player) && ballGetCarrierIdx(State.BallMoby) < 0) {
    if (!tryEquipBall(fromPlayerIdx)) {
      ballResendPickup(State.BallMoby);
    }
  } else {
    ballResendPickup(State.BallMoby);
  }
}

//--------------------------------------------------------------------------
void onReceiveBallScored(int scoredByPlayerIdx)
{
  if (!State.BallMoby || !isInGame()) return;

  Player** players = playerGetAll();

  // increment points
  Player* player = players[scoredByPlayerIdx];
  if (player) {
    int teamIdx = 0;
    if (State.Teams[teamIdx].TeamId != player->Team) teamIdx = 1;
    State.Teams[teamIdx].Score += 1;
    State.PlayerStates[scoredByPlayerIdx].Stats.Points += 1;
    ballReset(State.BallMoby, BALL_RESET_TEAM0 + !teamIdx);
  } else {
    // reset ball to center
    ballReset(State.BallMoby, BALL_RESET_CENTER);
  }

  // return to base
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (players[i] && players[i]->PlayerMoby) {
      playerRespawn(players[i]);
    }
  }

  // popup and play sound
  mobyPlaySoundByClass(3, 0, playerGetFromSlot(0)->PlayerMoby, MOBY_ID_RED_FLAG);
  snprintf(ScoreMessageBuffer, sizeof(ScoreMessageBuffer), "%s scored!", gameGetSettings()->PlayerNames[scoredByPlayerIdx]);
  uiShowPopup(0, ScoreMessageBuffer);
}

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
  VECTOR offset;

  // always spawn at base facing ball spawn
  if (player->Team == State.Teams[0].TeamId) {
    vector_copy(outPos, State.Teams[0].BasePosition);
  } else {
    vector_copy(outPos, State.Teams[1].BasePosition);
  }

  // rotation is look at ball
  vector_subtract(offset, State.BallSpawnPosition, outPos);
  vector_normalize(offset, offset);
  vector_write(outRot, 0);
  outRot[2] = atan2f(offset[1], offset[0]);

  // calculate position from center of base
  float theta = MATH_PI * 2 * (player->PlayerId / (float)GAME_MAX_PLAYERS);
  vector_fromyaw(offset, theta);
  vector_scale(offset, offset, 2.5);
  vector_add(outPos, outPos, offset);
}

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
  if (State.Teams[0].TeamId == team) return State.Teams[0].Score;
  if (State.Teams[1].TeamId == team) return State.Teams[1].Score;

  return 0;
}

//--------------------------------------------------------------------------
int playerStateCanHoldBall(Player * player)
{
	if (player->Health <= 0)
		return 0;

  if (player->InVehicle)
    return 0;

	if (player->PlayerState == PLAYER_STATE_DEATH
	 || player->PlayerState == PLAYER_STATE_DEATH_FALL
	 || player->PlayerState == PLAYER_STATE_LAVA_DEATH
	 || player->PlayerState == PLAYER_STATE_QUICKSAND_SINK
	 || player->PlayerState == PLAYER_STATE_DEATH_NO_FALL)
	 return 0;

	return 1;
}

//--------------------------------------------------------------------------
int tryEquipBall(int playerIdx)
{
  Player* player = playerGetAll()[playerIdx];
	if (playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS || !player || !State.BallMoby)
		return 0;

	if (!playerStateCanHoldBall(player))
		return 0;

  // cooldown
  if ((gameGetTime() - State.PlayerStates[playerIdx].TimeLastCarrier) < PICKUP_COOLDOWN)
    return 0;

	if (player->HeldMoby)
		return 0;

	// hold
  if (!gameAmIHost()) {
    sendBallPickupRequest(playerIdx);
    player->HeldMoby = State.BallMoby;
  } else {
	  ballPickup(State.BallMoby, playerIdx);
  }

  State.PlayerStates[playerIdx].TimeLastEquipPing = gameGetTime();
  return 1;
}

//--------------------------------------------------------------------------
int tryPingEquip(int playerIdx)
{
  Player* player = playerGetAll()[playerIdx];
	if (playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS || !player || !player->IsLocal || !State.BallMoby)
		return 0;

	if (!playerStateCanHoldBall(player))
		return 0;

  // cooldown
  if ((gameGetTime() - State.PlayerStates[playerIdx].TimeLastEquipPing) < EQUIP_PING_COOLDOWN)
    return 0;

	if (player->HeldMoby != State.BallMoby)
		return 0;

	// hold
  if (!gameAmIHost()) {
    sendBallPickupRequest(playerIdx);
  } else {
	  ballPickup(State.BallMoby, playerIdx);
  }

  State.PlayerStates[playerIdx].TimeLastEquipPing = gameGetTime();
  return 1;
}

//--------------------------------------------------------------------------
void tryDropBall(int playerIdx)
{
  Player* player = playerGetAll()[playerIdx];
	if (playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS || !player || !State.BallMoby || !player->IsLocal)
		return;

	// force player to drop flag
	if (player->HeldMoby == State.BallMoby)
	{
		playerDropFlag(player, 0);
		State.PlayerStates[playerIdx].TimeLastCarrier = gameGetTime();
	}
}

//--------------------------------------------------------------------------
void playerTryEnterThrowState(Player* player)
{
  Moby* nextSwingTarget = *(Moby**)((u32)player + 0x1ed0);
  char forceSwingSwitch = *(char*)((u32)player + 0x266c);
  if (forceSwingSwitch || nextSwingTarget) {
    PlayerVTable * table = playerGetVTable(player);
    table->UpdateState(player, PLAYER_STATE_IDLE, 1, 1, 1);
  } else {
    PlayerVTable * table = playerGetVTable(player);
    table->UpdateState(player, PLAYER_STATE_THROW_SHURIKEN, 1, 1, 1);
  }
}

//--------------------------------------------------------------------------
void playerLogic(int playerIdx)
{
	Player * localPlayer = playerGetFromSlot(0);
  Player * player = playerGetAll()[playerIdx];
  struct CGMPlayer * playerState = &State.PlayerStates[playerIdx];
	VECTOR temp = {0,0,0,0};
	int gameTime = gameGetTime();

  if (!player) return;

  // get hero state
  int state = player->PlayerState;
  int canHoldBall = playerStateCanHoldBall(player);
  int ballCarrierIdx = ballGetCarrierIdx(State.BallMoby);
  int friendlyTeamIdx = 0;
  if (State.Teams[friendlyTeamIdx].TeamId != player->Team)
    friendlyTeamIdx = 1;
  int enemyTeamIdx = !friendlyTeamIdx;
  
  // if player already holds ball then periodically update other clients
  if (player->IsLocal && State.BallMoby && ballCarrierIdx == playerIdx) {
    tryPingEquip(playerIdx);
  }

  // otherwise, check for pickup
  else if (player->IsLocal && canHoldBall && State.BallMoby && ballCarrierIdx < 0) {
    vector_subtract(temp, player->PlayerPosition, State.BallMoby->Position);
    if (vector_sqrmag(temp) < 4) {
      tryEquipBall(playerIdx);
    }
  }
  
#if DEBUG
  // quick pickup
  if (player->IsLocal && canHoldBall && State.BallMoby && ballCarrierIdx < 0) {
    if (padGetButtonDown(player->LocalPlayerIndex, PAD_L1 | PAD_UP) > 0) {
      tryEquipBall(playerIdx);
    }
  }
#endif

  if (player->HeldMoby == State.BallMoby) {

    // drop if ball belongs to someone else
    if (ballCarrierIdx >= 0 && ballCarrierIdx != playerIdx) {
		  playerDropFlag(player, 0);
    } else {
      vector_subtract(temp, State.Teams[enemyTeamIdx].BasePosition, player->PlayerPosition);

      if (player->IsLocal && vector_sqrmag(temp) < 5 && player->Ground.onGood && player->Ground.pMoby && player->Ground.pMoby->OClass == MOBY_ID_FLAG_SPAWN_POINT)
      {
        sendBallScored(playerIdx);
      }
      else if (!canHoldBall && player->IsLocal)
      {
        tryDropBall(playerIdx);
      }
      else if (playerPadGetButtonDown(player, PAD_R1))
      {
        if (state == PLAYER_STATE_CHARGE
          || state == PLAYER_STATE_WALK
          || state == PLAYER_STATE_IDLE
          || state == PLAYER_STATE_THROW_SHURIKEN
          || state == PLAYER_STATE_JUMP
          || state == PLAYER_STATE_DOUBLE_JUMP
          || state == PLAYER_STATE_SKID
          || state == PLAYER_STATE_SLIDE
          || state == PLAYER_STATE_SLOPESLIDE
          )
        {
          playerTryEnterThrowState(player);
        }
      }
      else if (playerPadGetButtonDown(player, PAD_CIRCLE) && player->IsLocal)
      {
        tryDropBall(playerIdx);
      }
      else if (playerPadGetButtonDown(player, PAD_TRIANGLE) && state == PLAYER_STATE_THROW_SHURIKEN)
      {
        PlayerVTable * table = playerGetVTable(player);
        table->UpdateState(player, PLAYER_STATE_IDLE, 1, 1, 1);
      }
      else if (playerPadGetButton(player, PAD_R1) && state == PLAYER_STATE_THROW_SHURIKEN)
      {
        playerTryEnterThrowState(player);
      }
      else if (playerPadGetButtonUp(player, PAD_R1) && state == PLAYER_STATE_THROW_SHURIKEN && player->IsLocal)
      {
        tryDropBall(playerIdx);
        ballThrow(State.BallMoby, THROW_BASE_POWER);
      }
    }
  }
}

//--------------------------------------------------------------------------
void frameTick(PatchStateContainer_t * gameState)
{
  
}

//--------------------------------------------------------------------------
void gameTick(PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
  GameData * gameData = gameGetData();
	Player ** players = playerGetAll();
	int i;
	char buffer[32];
	int gameTime = gameGetTime();

  if (gameState->HalfTimeState == 1 || gameState->HalfTimeState == 2) return;
  if (gameState->OverTimeState == 1 || gameState->OverTimeState == 2) return;

  // track time holding flag pts
  int carrierIdx = ballGetCarrierIdx(State.BallMoby);
  if (carrierIdx >= 0 && carrierIdx < GAME_MAX_PLAYERS) {
    State.PlayerStates[carrierIdx].Stats.TimeWithBall += 1 / 60.0;
  }

  // move scores into game data
  // which will be used by the game to determine who wins
  // and how to sort the end game scoreboard
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    playerLogic(i);
  }

  // end game logic
  if (gameAmIHost()) {

    // target reached
    int capsToWin = gameOptions->GameFlags.MultiplayerGameFlags.CapsToWin;
    if (capsToWin > 0) {
      if (State.Teams[0].Score > capsToWin) {
        gameSetWinner(State.Teams[0].TeamId, 1);
        gameEnd(1);
        gameSetWinner(State.Teams[0].TeamId, 1);
      } else if (State.Teams[1].Score > capsToWin) {
        gameSetWinner(State.Teams[1].TeamId, 1);
        gameEnd(1);
        gameSetWinner(State.Teams[1].TeamId, 1);
      }
    }
  }

  // set flag cap score for OT patch
  gameData->TeamStats.FlagCaptureCounts[State.Teams[0].TeamId] = State.Teams[0].Score;
  gameData->TeamStats.FlagCaptureCounts[State.Teams[1].TeamId] = State.Teams[1].Score;
}

//--------------------------------------------------------------------------
int findClosestSpawnPointToPosition(VECTOR position, float deadzone)
{
  // find closest spawn point
  int bestIdx = -1;
  int i;
  float bestDist = 1000000;
  float sqrDeadzone = deadzone * deadzone;
  int spCount = spawnPointGetCount();
  for (i = 0; i < spCount; ++i) {
    if (!spawnPointIsPlayer(i)) continue;

    struct SpawnPoint* sp = spawnPointGet(i);
    VECTOR delta;
    vector_subtract(delta, position, &sp->M0[12]);
    float sqrDist = vector_sqrmag(delta);
    if (sqrDist < bestDist && sqrDist >= sqrDeadzone) {
      bestDist = sqrDist;
      bestIdx = i;
    }
  }

  return bestIdx;
}

//--------------------------------------------------------------------------
void findTeamBallSpawnPoint(int teamIdx)
{
  VECTOR dt;

  // try and guess a spot near the base and see if it is in view of the base
  vector_subtract(dt, State.BallSpawnPosition, State.Teams[teamIdx].BasePosition);
  vector_normalize(dt, dt);
  vector_scale(dt, dt, 10);
  vector_add(dt, dt, State.Teams[teamIdx].BasePosition);

  // check for ground
  VECTOR ground, down = {0,0,-5,0}, up = {0,0,1,0};
  vector_add(ground, dt, down);
  if (CollLine_Fix(dt, ground, COLLISION_FLAG_IGNORE_DYNAMIC, NULL, NULL)) {
    int cType = CollLine_Fix_GetHitCollisionId() & 0xF;
    if (cType != 0x0B && cType != 0x01 && cType != 0x04 && cType != 0x05 && cType != 0x0D && cType != 0x03) {

      vector_add(ground, State.Teams[teamIdx].BasePosition, up);
      if (!CollLine_Fix(ground, dt, COLLISION_FLAG_IGNORE_DYNAMIC, NULL, NULL)) {
        vector_copy(State.Teams[teamIdx].BallSpawnPosition, dt);
        DPRINTF("found team ball spawn near base %d\n", State.Teams[teamIdx].TeamId);
        return;
      }
    }
  }

  int spIdx = findClosestSpawnPointToPosition(dt, 0);
  DPRINTF("closest sp team:%d is %d\n", State.Teams[teamIdx].TeamId, spIdx);
  if (spIdx >= 0) {
    vector_copy(State.Teams[teamIdx].BallSpawnPosition, &spawnPointGet(spIdx)->M0[12]);
    State.Teams[teamIdx].BallSpawnPosition[2] += 2;
  } else {
    vector_copy(State.Teams[teamIdx].BallSpawnPosition, State.Teams[teamIdx].BasePosition);
  }
}

//--------------------------------------------------------------------------
void findBallCenterSpawnPoint(void)
{
  static int found = 0;
  if (found) return;

	// reset team scores
	GameSettings* gameSettings = gameGetSettings();
  int i;
	for (i = 0; i < 2; ++i) {
		State.Teams[i].Score = 0;
    State.Teams[i].TeamId = -1;
	}

  // determine teams
  int teamCount = 0;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    int team = gameSettings->PlayerTeams[i];
    if (team < 0) continue;

    if (teamCount == 0 || team != State.Teams[teamCount-1].TeamId) {
      State.Teams[teamCount].TeamId = team;
      teamCount += 1;
    }
  }

  // auto team if game started with only 1 team
  if (teamCount == 1) {
    switch (State.Teams[0].TeamId) {
      case TEAM_BLUE: State.Teams[1].TeamId = TEAM_RED; break;
      case TEAM_RED: State.Teams[1].TeamId = TEAM_BLUE; break;
      case TEAM_GREEN: State.Teams[1].TeamId = TEAM_ORANGE; break;
      case TEAM_ORANGE: State.Teams[1].TeamId = TEAM_GREEN; break;
    }
  }

  // find and update flags
  Moby* moby = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();
  VECTOR basePositions[4];
  int hasBasePosition[4] = {0,0,0,0};
	while (moby < mEnd)
	{
    int flag = 0, team = 0;
    switch (moby->OClass)
    {
      case MOBY_ID_BLUE_FLAG:
      {
        flag = 1;
        team = TEAM_BLUE;
        break;
      }
      case MOBY_ID_RED_FLAG:
      {
        flag = 1;
        team = TEAM_RED;
        break;
      }
      case MOBY_ID_GREEN_FLAG:
      {
        flag = 1;
        team = TEAM_GREEN;
        break;
      }
      case MOBY_ID_ORANGE_FLAG:
      {
        flag = 1;
        team = TEAM_ORANGE;
        break;
      }
    }

    if (flag) {
      // hide and disable
      moby->ModeBits |= MOBY_MODE_BIT_DISABLED | MOBY_MODE_BIT_HIDDEN | MOBY_MODE_BIT_NO_UPDATE;
      moby->State = 2;
    
      // update team base position
      if (!hasBasePosition[team]) { vector_copy(basePositions[team], (float*)moby->PVar); hasBasePosition[team] = 1; }
      else { vector_copy((float*)moby->PVar, basePositions[team]); }
      *(u16*)(moby->PVar + 0x14) = team;

#if DEBUG
      printf("flag %04X at %08X ", moby->OClass, (u32)moby);
      vector_print((float*)moby->PVar);
      printf("\n");
#endif
    }

		++moby;
	}

  // set team base
  vector_copy(State.Teams[0].BasePosition, basePositions[State.Teams[0].TeamId]);
  vector_copy(State.Teams[1].BasePosition, basePositions[State.Teams[1].TeamId]);

  // determine ball spawn
  // as closest player spawn
  // to median of bases
  VECTOR spMedianPosition={0,0,0,0};
  vector_add(spMedianPosition, spMedianPosition, State.Teams[0].BasePosition);
  vector_add(spMedianPosition, spMedianPosition, State.Teams[1].BasePosition);
  vector_scale(spMedianPosition, spMedianPosition, 0.5);
  int medianSpIdx = findClosestSpawnPointToPosition(spMedianPosition, 0);

  // set spawn point position for center spawn, and base spawns
  vector_copy(State.BallSpawnPosition, &spawnPointGet(medianSpIdx)->M0[12]);
  State.BallSpawnPosition[2] += 2;

  // find ball spawn for each base
  findTeamBallSpawnPoint(0);
  findTeamBallSpawnPoint(1);

  found = 1;
  
  DPRINTF("teams %d %d\n", State.Teams[0].TeamId, State.Teams[1].TeamId);
}

//--------------------------------------------------------------------------
void initialize(PatchGameConfig_t* gameConfig, PatchStateContainer_t* gameState)
{
  static int startDelay = 60 * 0.2;
	static int waitingForClientsReady = 0;

	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	GameData* gameData = gameGetData();
	int i;

  // change deathmatch points to use playerPoints
  // instead of playerKills - playerSuicides
  // POKE_U16(0x006236B4, 0x7E8); // move to read buffer playerRank (reads are -0x28 from buffer)
  // POKE_U32(0x006236D8, 0x8CA2FFD8); // read points as word
  // POKE_U32(0x006236F4, 0x8CA2FFD8); // read points as word
  // POKE_U32(0x00623700, 0); // stop counting suicides
  // POKE_U32(0x00623708, 0); // stop counting deaths (tiebreaker)
  // POKE_U16(0x00623710, 4); // increment by 4 (sizeof(int))

  // allow player to drop flag with CIRCLE
  //POKE_U32(0x005DFD44, 0);

	// point get resurrect point to ours
	*(u32*)0x00610724 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
	*(u32*)0x005e2d44 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);

	// hook messages
	netHookMessages();

  //
  initializeScoreboard();

  // init ball moby
  ballInitialize();
  findBallCenterSpawnPoint();

  if (startDelay) {
    --startDelay;
    return;
  }
  
  // wait for all clients to be ready
  // or for 5 seconds
  if (!gameState->AllClientsReady && waitingForClientsReady < (5 * 60)) {
    uiShowPopup(0, "Waiting For Players...");
    ++waitingForClientsReady;
    return;
  }

  // hide waiting for players popup
  hudHidePopup();

  // spawn ball
  if (State.IsHost) {
    ballCreate(State.BallSpawnPosition);
  }

	// initialize player states
	State.LocalPlayerState = NULL;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].PlayerIndex = i;
		State.PlayerStates[i].Stats.Points = 0;
		State.PlayerStates[i].Stats.TimeWithBall = 0;

		// is local
		if (p && p->IsLocal && !State.LocalPlayerState) {
      State.LocalPlayerState = &State.PlayerStates[i];
		}
	}

	// initialize state
	State.InitializedTime = gameGetTime();

  // respawn locals
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* local = playerGetFromSlot(i);
    if (local && local->PlayerMoby && local->pNetPlayer) {
      playerRespawn(local);
    }
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
    // 
	}

	// stats
	if (gameState->UpdateCustomGameStats)
	{
    gameState->CustomGameStatsSize = sizeof(struct CGMGameData);
		struct CGMGameData* sGameData = (struct CGMGameData*)gameState->CustomGameStats.Payload;
		sGameData->Version = 0x00000001;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
      sGameData->TimeWithBall[i] = State.PlayerStates[i].Stats.TimeWithBall;
			sGameData->Points[i] = State.PlayerStates[i].Stats.Points;
		}

    for (i = 0; i < 2; ++i)
    {
			sGameData->TeamIds[i] = State.Teams[i].TeamId;
			sGameData->TeamScore[i] = State.Teams[i].Score;
    }
	}
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(void)
{
  static int set = 0;

	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions || !gameSettings || gameSettings->GameLoadStartTime <= 0 || set)
		return;
	
	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;

  // force 2 teams of blue,red,green,orange
  int team1 = -1;
  int team2 = -1;
  int mappings[GAME_MAX_PLAYERS] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  int alternator = 0;
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    int team = gameSettings->PlayerTeams[i];
    if (team >= 0) {
      if (team1 < 0) {
        team1 = team;
        if (team1 > TEAM_ORANGE) team1 = team1 % 4;
        mappings[team] = 0;
      }
      else if (team != team1 && team2 < 0) {
        team2 = team;
        if (team2 > TEAM_ORANGE) team2 = !team1;
        mappings[team] = 1;
      }
      else if (mappings[team] < 0) {
        mappings[team] = alternator++ % 2;
      }
    }
  }
  
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    int team = gameSettings->PlayerTeams[i];
    if (team >= 0 && team != team1 && team != team2) {
      gameSettings->PlayerTeams[i] = mappings[team] ? team2 : team1;
    }
  }

  //gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
  //if (gameOptions->GameFlags.MultiplayerGameFlags.Timelimit == 0)
  //  gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 5;

  set = 1;
}
