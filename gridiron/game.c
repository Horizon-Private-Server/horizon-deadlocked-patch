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

void initializeScoreboard(void);
int (*baseFlagHandleGuberEvent)(Moby*, GuberEvent*) = (int (*)(Moby*, GuberEvent*))0x00417BB8;

//--------------------------------------------------------------------------
void onSetPlayerStats(int playerId, struct CGMPlayerStats* stats)
{
	memcpy(&State.PlayerStates[playerId].Stats, stats, sizeof(struct CGMPlayerStats));
}

//--------------------------------------------------------------------------
void onReceiveTeamScore(int teamScores[GAME_MAX_PLAYERS])
{
	int i;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    State.Teams[i].Score = teamScores[i];
	}
}

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
  return State.Teams[team].Score;
}

//--------------------------------------------------------------------------
int getPlayerScore(int playerIdx)
{
  return State.PlayerStates[playerIdx].Stats.Points;
}

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{

}

//--------------------------------------------------------------------------
int playerStateCanHoldBall(Player * player)
{
	if (player->Health <= 0)
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
void tryEquipBall(int playerIdx)
{
  Player* player = playerGetAll()[playerIdx];
	if (playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS || !player || !State.BallMoby)
		return;

	if (!playerStateCanHoldBall(player))
		return;

  // 3 second cooldown
  if ((gameGetTime() - State.PlayerStates[playerIdx].TimeLastCarrier) < (TIME_SECOND * 3))
    return;

	if (player->HeldMoby)
		return;

	// hold
	ballPickup(State.BallMoby, playerIdx);
}

//--------------------------------------------------------------------------
void tryDropBall(int playerIdx)
{
  Player* player = playerGetAll()[playerIdx];
	if (playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS || !player || !State.BallMoby)
		return;

	// force player to drop flag
	if (player->HeldMoby == State.BallMoby)
	{
		playerDropFlag(player, 0);
		State.PlayerStates[playerIdx].TimeLastCarrier = gameGetTime();
	}
}

//--------------------------------------------------------------------------
void playerLogic(int playerIdx)
{
	Player * localPlayer = (Player*)0x00347AA0;
  Player * player = playerGetAll()[playerIdx];
	VECTOR temp = {0,0,0,0};
	int gameTime = gameGetTime();

  if (!player) return;

  // get hero state
  int state = player->PlayerState;

  if (!playerStateCanHoldBall(playerIdx))
    tryDropBall(playerIdx);

  else if (playerPadGetButton(player, PAD_R1) && player->WeaponHeldId != WEAPON_ID_SWINGSHOT)
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
      PlayerVTable * table = playerGetVTable(player);
      table->UpdateState(player, PLAYER_STATE_THROW_SHURIKEN, 1, 1, 1);
    }
  }
  else if (playerPadGetButtonUp(player, PAD_R1) && state == PLAYER_STATE_THROW_SHURIKEN)
  {
    tryDropBall(playerIdx);
    ballThrow(State.BallMoby);
  }
}

//--------------------------------------------------------------------------
void frameTick(void)
{
  
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

  // update who has ball
  State.CurrentBallCarrierPlayerIdx = -1;
  if (State.BallMoby && State.BallMoby->PVar) {
    State.CurrentBallCarrierPlayerIdx = ((BallPVars_t*)State.BallMoby->PVar)->CarrierIdx;
  }

  // track time holding flag pts
  if (State.CurrentBallCarrierPlayerIdx >= 0 && State.CurrentBallCarrierPlayerIdx < GAME_MAX_PLAYERS) {
    State.PlayerStates[State.CurrentBallCarrierPlayerIdx].Stats.TimeWithBall += 1 / 60.0;
  }

  // move scores into game data
  // which will be used by the game to determine who wins
  // and how to sort the end game scoreboard
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    playerLogic(i);
    gameData->TeamStats.TeamTicketScore[i] = State.Teams[i].Score;
    gameData->PlayerStats.TicketScore[i] = State.PlayerStates[i].Stats.Points;
  }
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

  // hook into computePlayerScore (endgame pts)
  HOOK_J(0x00625510, &getPlayerScore);
  POKE_U32(0x00625514, 0);

	// point get resurrect point to ours
  HOOK_JAL(0x00610724, &getResurrectPoint);
  HOOK_JAL(0x005e2d44, &getResurrectPoint);

  // change deathmatch points to use playerPoints
  // instead of playerKills - playerSuicides
  POKE_U16(0x006236B4, 0x7E8); // move to read buffer playerRank (reads are -0x28 from buffer)
  POKE_U32(0x006236D8, 0x8CA2FFD8); // read points as word
  POKE_U32(0x006236F4, 0x8CA2FFD8); // read points as word
  POKE_U32(0x00623700, 0); // stop counting suicides
  POKE_U32(0x00623708, 0); // stop counting deaths (tiebreaker)
  POKE_U16(0x00623710, 4); // increment by 4 (sizeof(int))

  // allow player to drop flag with CIRCLE
  POKE_U32(0x005DFD44, 0);

	// hook messages
	netHookMessages();

  // 
  initializeScoreboard();

  // init ball moby
  ballInitialize();

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

	// reset team scores
	for (i = 0; i < 2; ++i) {
		State.Teams[i].Score = 0;
    State.Teams[i].TeamId = -1;
	}

  // determine teams
  int teamCount = 0;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    int team = gameSettings->PlayerTeams[i];
    if (team < 0) continue;

    if (team != State.Teams[teamCount].TeamId) {
      State.Teams[teamCount].TeamId = team;
      teamCount += 1;
    }
  }

  // auto team if game started with only 1 team
  if (teamCount == 1) {
    State.Teams[1].TeamId = !State.Teams[0].TeamId;
  }

  DPRINTF("teams %d %d\n", State.Teams[0].TeamId, State.Teams[1].TeamId);

  // find flag bases
  Moby* moby = mobyListGetStart();
  Moby* mEnd = mobyListGetEnd();
	while (moby < mEnd)
	{
    if (moby->OClass == 0x266E) {
      void * pvars = moby->PVar;
      int team = -1;
      if (pvars)
        team = *(char*)pvars;

      if (team == State.Teams[0].TeamId)
      {
        DPRINTF("flag at %08X %08X %d\n", (u32)moby, (u32)pvars, team);
        vector_copy(State.Teams[0].BasePosition, moby->Position);
      }
      else if (team == State.Teams[1].TeamId)
      {
        DPRINTF("flag at %08X %08X %d\n", (u32)moby, (u32)pvars, team);
        vector_copy(State.Teams[1].BasePosition, moby->Position);
      }
    }

		++moby;
	}

  // determine ball spawn
  // as closest player spawn
  // to median of bases
  int spCount = spawnPointGetCount();
  VECTOR spMedianPosition;
  vector_add(spMedianPosition, spMedianPosition, State.Teams[0].BasePosition);
  vector_add(spMedianPosition, spMedianPosition, State.Teams[1].BasePosition);
  vector_scale(spMedianPosition, spMedianPosition, 0.5);

  // find closest spawn point
  int bestIdx = 0;
  float bestDist = 100000;
  for (i = 0; i < spCount; ++i) {
    if (!spawnPointIsPlayer(i)) continue;

    struct SpawnPoint* sp = spawnPointGet(i);
    VECTOR delta;
    vector_subtract(delta, spMedianPosition, &sp->M0[12]);
    float sqrDist = vector_sqrmag(delta);
    if (sqrDist < bestDist) {
      bestDist = sqrDist;
      bestIdx = i;
    }
  }

  // set sp position as base
  vector_copy(State.BallSpawnPosition, &spawnPointGet(bestIdx)->M0[12]);
  State.BallSpawnPosition[2] += 2;

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
  State.CurrentBallCarrierPlayerIdx = -1;
	State.InitializedTime = gameGetTime();

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
