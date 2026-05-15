#include <libdl/random.h>
#include "include/game.h"



//--------------------------------------------------------------------------
void applyThreeTeamEven(int team1, int team2, int team3)
{
	GameSettings* gameSettings = gameGetSettings();

  char playerPool[GAME_MAX_PLAYERS];
  memset(playerPool, 0, sizeof(playerPool));

  // count number of players
  int i;
  int playerCount = 0;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0) {
      playerPool[playerCount] = i;
      ++playerCount;
    }
  }
  
  // randomize player pool
  int loopCount = playerCount * playerCount;
  for (i = 1; i < loopCount; ++i) {
    int ci = i % playerCount;
    int pi = (i - 1) % playerCount;
    if (rand(2) == 1) {
      char t = playerPool[ci];
      playerPool[ci] = playerPool[pi];
      playerPool[pi] = t;
    }
  }
  
  // assign players to teams
  int mid0 = (playerCount + 2) / 3;
  int mid1 = 2 * mid0;
  for (i = 0; i < playerCount; ++i) {
    int team = i < mid0 ? team1 : (i < mid1 ? team2 : team3);
    int pidx = playerPool[i];
    if (gameSettings->PlayerClients[pidx] >= 0) {
      gameSettings->PlayerTeams[pidx] = team;
    }
  }
}

//--------------------------------------------------------------------------
void applyTwoTeamEven(int team1, int team2)
{
	GameSettings* gameSettings = gameGetSettings();

  char playerPool[GAME_MAX_PLAYERS];
  memset(playerPool, 0, sizeof(playerPool));

  // count number of players
  int i;
  int playerCount = 0;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0) {
      playerPool[playerCount] = i;
      ++playerCount;
    }
  }
  
  // randomize player pool
  int loopCount = playerCount * playerCount;
  for (i = 1; i < loopCount; ++i) {
    int ci = i % playerCount;
    int pi = (i - 1) % playerCount;
    if (rand(2) == 1) {
      char t = playerPool[ci];
      playerPool[ci] = playerPool[pi];
      playerPool[pi] = t;
    }
  }
  
  // assign players to teams
  int midpoint = (playerCount + 1) / 2;
  for (i = 0; i < playerCount; ++i) {
    int team = i < midpoint ? team1 : team2;
    int pidx = playerPool[i];
    if (gameSettings->PlayerClients[pidx] >= 0) {
      gameSettings->PlayerTeams[pidx] = team;
    }
  }
}

//--------------------------------------------------------------------------
void applyFindThreeTeamEven(void)
{
	GameSettings* gameSettings = gameGetSettings();
  int team1 = -1;
  int team2 = -1;
  int team3 = -1;

  // find first two teams in lobby and use those
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0) {
      int team = gameSettings->PlayerTeams[i];
      if (team1 < 0) team1 = team;
      if (team2 < 0 && team != team1) team2 = team;
      if (team3 < 0 && team != team1 && team != team2) team3 = team;
    }
  }

  // default to red vs blue vs green
  if (team1 < 0) team1 = TEAM_BLUE;
  if (team2 < 0) team2 = (team1 + 1) % 3;
  if (team3 < 0) team3 = (team2 + 1) % 3;

  // apply
  applyThreeTeamEven(team1, team2, team3);

	GameOptions * gameOptions = gameGetOptions();
  gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
}

//--------------------------------------------------------------------------
void applyFindTwoTeamEven(void)
{
	GameSettings* gameSettings = gameGetSettings();
  int team1 = -1;
  int team2 = -1;

  // find first two teams in lobby and use those
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0) {
      int team = gameSettings->PlayerTeams[i];
      if (team1 < 0) team1 = team;
      if (team2 < 0 && team != team1) team2 = team;
    }
  }

  // default to red vs blue
  if (team1 < 0) team1 = TEAM_BLUE;
  if (team2 < 0) team2 = (team1 + 1) % 2;

  // apply
  applyTwoTeamEven(team1, team2);

	GameOptions * gameOptions = gameGetOptions();
  gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
}

//--------------------------------------------------------------------------
void applyTeamRedBlueEven(void)
{
  applyTwoTeamEven(TEAM_BLUE, TEAM_RED);

	GameOptions * gameOptions = gameGetOptions();
  gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
}

//--------------------------------------------------------------------------
void applyTeamFfa(void)
{
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
  
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0) {
      gameSettings->PlayerTeams[i] = i;
    }
  }

  gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 0;
}

//--------------------------------------------------------------------------
void applyTeamSingle(int team)
{
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
  
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0) {
      gameSettings->PlayerTeams[i] = team;
    }
  }

  gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
}

//--------------------------------------------------------------------------
void applyTeamClampToGameRule(void)
{
	GameSettings* gameSettings = gameGetSettings();
  int clampTo = TEAM_MAROON;

  switch (gameSettings->GameRules)
  {
    case GAMERULE_CQ: clampTo = TEAM_RED; break;
    case GAMERULE_CTF: clampTo = TEAM_ORANGE; break;
    default: break; // do nothing
  }

  // use modulo to collapse teams to supported range
  // can cause two different teams to collapse to the same value
  // like Blue and Green for Conquest would collapse to Blue and Blue
  // probably want to figure out a better way to do this at some point
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0) {
      gameSettings->PlayerTeams[i] %= (clampTo + 1);
    }
  }
}

//--------------------------------------------------------------------------
void applyTeamType(enum CgmTeamType teamType)
{
  // make sure teams in lobby are supported by game mode
  applyTeamClampToGameRule();

  switch (teamType)
  {
    case TEAM_TYPE_ALLOW_ANY: break; // do nothing
    case TEAM_TYPE_ALL_BLUE: applyTeamSingle(TEAM_BLUE); break;
    case TEAM_TYPE_ALL_RED: applyTeamSingle(TEAM_RED); break;
    case TEAM_TYPE_FFA: applyTeamFfa(); break;
    case TEAM_TYPE_RED_BLUE_EVEN: applyTeamRedBlueEven(); break;
    case TEAM_TYPE_2_TEAMS_EVEN: applyFindTwoTeamEven(); break;
    case TEAM_TYPE_3_TEAMS_EVEN: applyFindThreeTeamEven(); break;
  }
}
