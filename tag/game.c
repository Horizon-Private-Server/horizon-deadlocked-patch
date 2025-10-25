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
#include "common.h"
#include "module.h"
#include "messageid.h"
#include "include/game.h"
#include "include/utils.h"

extern int Initialized;

void initializeScoreboard(void);
void onLocalPointsChanged(int playerId, int netDelta);

//--------------------------------------------------------------------------
void onSetPlayerStats(int playerId, struct CGMPlayerStats* stats)
{
  Player* player = playerGetAll()[playerId];
  if (player && player->IsLocal) {

    struct CGMPlayerStats* lastStats = &State.PlayerStates[playerId].Stats;
    int netDt = (stats->PointsGainStolen + stats->PointsBonus - stats->PointsLostStolen)
      - (lastStats->PointsGainStolen + lastStats->PointsBonus - lastStats->PointsLostStolen);

    // update last pts delta
    onLocalPointsChanged(playerId, netDt);
  }

	memcpy(&State.PlayerStates[playerId].Stats, stats, sizeof(struct CGMPlayerStats));
}

//--------------------------------------------------------------------------
void onSetSeeker(int player)
{
  // set player as seeker
  // set everyone else as hider
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    State.PlayerStates[i].IsSeeker = i == player;
  }

  // enable juggernaut aura for player
  POKE_U16(0x005d29a8, player);

  // show popup
  GameSettings* gameSettings = gameGetSettings();
  snprintf(State.PopupMessageBuf, sizeof(State.PopupMessageBuf), "%s is now it!", gameSettings->PlayerNames[player]);
  uiShowPopup(0, State.PopupMessageBuf);
  uiShowPopup(1, State.PopupMessageBuf);
}

//--------------------------------------------------------------------------
void onLocalPointsChanged(int playerId, int netDelta)
{
  if (netDelta) {
    State.PlayerStates[playerId].LastPtsDelta = netDelta;
    State.PlayerStates[playerId].DisplayLastPtsDeltaTicks = TAG_PTS_DELTA_DURATION_TICKS;
  }
}

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
  return State.PlayerStates[team].Stats.Points;
}

//--------------------------------------------------------------------------
int gameCalculateScores(void)
{
  int i;
  GameData* gameData = gameGetData();
  Player** players = playerGetAll();
  
  // recalculate player scores
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    if (!player) continue;

    State.PlayerStates[i].Stats.Points = (int)gameData->PlayerStats.KingHillHoldTime[i]
      + State.PlayerStates[i].Stats.PointsBonus
      + State.PlayerStates[i].Stats.PointsGainStolen
      - State.PlayerStates[i].Stats.PointsLostStolen;
  }
}

//--------------------------------------------------------------------------
void gameUpdateWinningTeam(void)
{
  Player** players = playerGetAll();
  GameOptions* gameOptions = gameGetOptions();
  int winningTeam = 0;
  int bestScore = 0;

  // find best score
  // if two+ players have the same score then return -1
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (players[i] && playerIsConnected(players[i])) {
      if (State.PlayerStates[i].Stats.Points > bestScore) {
        winningTeam = i;
        bestScore = State.PlayerStates[i].Stats.Points;
      } else if (State.PlayerStates[i].Stats.Points == bestScore) {
        winningTeam = -1;
      }
    }
  }

  State.WinningTeam = winningTeam;

  // detect if score reached
  int ptsToWin = gameOptions->GameFlags.MultiplayerGameFlags.HillTimeToWin * 30;
  if (ptsToWin > 0 && bestScore >= ptsToWin) {
    State.GameOver = 7; // hill time reached
  }
}

//--------------------------------------------------------------------------
int gameGetSeekerPlayerIdx(void)
{
  Player** players = playerGetAll();
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (players[i] && playerIsConnected(players[i]) && State.PlayerStates[i].IsSeeker)
      return i;
  }

  return -1;
}

//--------------------------------------------------------------------------
int gameGetSeekerCount(void)
{
  Player** players = playerGetAll();
  int i, count = 0;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (players[i] && playerIsConnected(players[i]) && State.PlayerStates[i].IsSeeker)
      ++count;
  }

  return count;
}

//--------------------------------------------------------------------------
int gameGetRandomHider(void)
{
  Player** players = playerGetAll();
  int r = 1 + rand(GAME_MAX_PLAYERS);
  int i = 0;
  int count = 0;

  // iterate until we've looped through r hiders
  while (r) {
    i = (i + 1) % GAME_MAX_PLAYERS;
    if (players[i] && playerIsConnected(players[i]) && !State.PlayerStates[i].IsSeeker) {
      --r;
      ++count;
    }

    // prevent infinite loop when there are no hiders
    if (i == 0 && count == 0) return -1;
  }

  // return the rth hider
  return i;
}

//--------------------------------------------------------------------------
void gameSetSeeker(int player)
{
  if (player < 0) return;

  // send stats for current seeker
  // since their points gained will have changed
  int currentSeeker = gameGetSeekerPlayerIdx();
  if (currentSeeker >= 0) {
    sendPlayerStats(currentSeeker);
  }

  sendSeeker(player);
  onSetSeeker(player);
}

//--------------------------------------------------------------------------
int onHillMobyConsiderPlayer(Player* player)
{
  // don't consider seeker for points
  if (player && State.PlayerStates[player->PlayerId].IsSeeker) return 0;

  // base
  return playerIsConnected(player);
}

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
	int i;
  int count = spawnPointGetCount();
  int bestIdx = 0;
  float bestDistSqr = 1000000;
  VECTOR delta;

  // hiders use default spawn system
  if (player && !State.PlayerStates[player->PlayerId].IsSeeker) {
    playerGetSpawnpoint(player, outPos, outRot, firstRes);
    return;
  }

  // find hill moby
  Moby* hillMoby = mobyFindNextByOClass(mobyListGetStart(), 0x2604);
  if (!hillMoby) {
    playerGetSpawnpoint(player, outPos, outRot, firstRes);
    return;
  }

  // find closest player spawn to hill
  for (i = 0; i < count; ++i) {
    if (!spawnPointIsPlayer(i)) continue;

    SpawnPoint* sp = spawnPointGet(i);
    vector_subtract(delta, &sp->M0[12], hillMoby->Position);
    float sqrDist = vector_sqrmag(delta);
    if (sqrDist < bestDistSqr) {
      bestIdx = i;
      bestDistSqr = sqrDist;
    }
  }

  // choose closest to hill
  SpawnPoint* sp = spawnPointGet(bestIdx);
  vector_copy(outPos, &sp->M0[12]);
  vector_copy(outRot, &sp->M1[12]);
}

//--------------------------------------------------------------------------
int onWhoHitMe(Player* player, Moby* moby, int b)
{
  // prevent non-seekers from damaging others
  if (moby) {
    Player* sourcePlayer = guberMobyGetPlayerDamager(moby);
    if (sourcePlayer && !State.PlayerStates[sourcePlayer->PlayerId].IsSeeker) {
      // if target is also a hider, then stun them (grief mechanic)
      // make sure hiders cannot stun seeker
      if (!State.PlayerStates[player->PlayerId].IsSeeker
          && player->timers.postHitInvinc == 0
          && player->PlayerState != PLAYER_STATE_ELECTRIC_GET_HIT) {
        playerGetVTable(player)->UpdateState(player, PLAYER_STATE_ELECTRIC_GET_HIT, 1, 0, 0);
      }
      return 0;
    }
  }

  // base
  return ((int (*)(Player*, Moby*, int))0x005dff08)(player, moby, b);
}

//--------------------------------------------------------------------------
void onPlayerKill(char * fragMsg)
{
  Player** players = playerGetAll();

	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);

  // only handle seeker assignment if host
  if (!State.IsHost) return;
  
  // parse frag msg
	char weaponId = fragMsg[3];
	int killedPlayerId = fragMsg[2];
	int sourcePlayerId = fragMsg[0];

  // check if source player is seeker
  // if they are, set the killed player as new seeker
  if (sourcePlayerId >= 0 && killedPlayerId >= 0 && State.PlayerStates[sourcePlayerId].IsSeeker) {

    // steal % of their points
    int pointsStolen = (int)(State.PlayerStates[killedPlayerId].Stats.Points * TAG_POINTS_STEAL_PERCENT);
    if (pointsStolen < 0) pointsStolen = 0;

    // add pts
    DPRINTF("%d pts %d -> %d\n", killedPlayerId, State.PlayerStates[killedPlayerId].Stats.Points, State.PlayerStates[killedPlayerId].Stats.Points - pointsStolen);
    State.PlayerStates[killedPlayerId].Stats.PointsLostStolen += pointsStolen;
    State.PlayerStates[sourcePlayerId].Stats.PointsGainStolen += pointsStolen;

    // seeker gets bonus
    State.PlayerStates[sourcePlayerId].Stats.PointsBonus += TAG_POINTS_TAG_BONUS;

    // if local, pass pt delta to handler
    if (players[sourcePlayerId] && players[sourcePlayerId]->IsLocal) {
      onLocalPointsChanged(sourcePlayerId, pointsStolen + TAG_POINTS_TAG_BONUS);
    }
    if (players[killedPlayerId] && players[killedPlayerId]->IsLocal) {
      onLocalPointsChanged(killedPlayerId, -pointsStolen);
    }

    // set as new seeker
    gameSetSeeker(killedPlayerId);
  }
}

//--------------------------------------------------------------------------
void gameOnPlayerUpdate(Player* player)
{
  if (!player || !playerIsConnected(player)) return;

  if (!State.PlayerStates[player->PlayerId].IsSeeker)
  {
    // lock hider health at configured max % health
    player->Health = minf(player->MaxHealth * TAG_HIDER_MAX_HEALTH_PERCENT, player->Health);
  }
  else
  {
    // lock seeker health at max health
    player->Health = player->MaxHealth;
  }
}

//--------------------------------------------------------------------------
void frameTick(void)
{
  int i;
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {

    Player* player = playerGetFromSlot(i);
    if (!player) continue;

    struct CGMPlayer* cgmPlayer = &State.PlayerStates[player->PlayerId];

    // draw pts delta
    int ptsDt = cgmPlayer->LastPtsDelta;
    int ptsDtTicks = cgmPlayer->DisplayLastPtsDeltaTicks;
    if (ptsDtTicks > 0 && ptsDt) {

      float y = (0.45 * SCREEN_HEIGHT) * (playerGetNumLocals() > 1 ? 0.5 : 1) + (player->LocalPlayerIndex * 0.5 * SCREEN_HEIGHT);
      char buf[32];
      u32 color = colorLerp(0x8000F0F0, 0x800000C0, ptsDt < 0 ? 1 : 0);
      snprintf(buf, sizeof(buf), "%c%d", (ptsDt < 0) ? '-' : '+', (ptsDt < 0) ? -ptsDt : ptsDt);
      gfxHelperDrawText(0.55 * SCREEN_WIDTH, y, 0, 0, 1, color, buf, -1, TEXT_ALIGN_TOPLEFT, COMMON_DZO_DRAW_NORMAL);

      cgmPlayer->DisplayLastPtsDeltaTicks = ptsDtTicks - 1;
    }
  }

  // find player in first place (not hider)
  int bestPts = 0;
  int leaderIdx = -1;
  VECTOR leaderWPos = {0,0,3,0};
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (!State.PlayerStates[i].IsSeeker && State.PlayerStates[i].Stats.Points > bestPts) {
      bestPts = State.PlayerStates[i].Stats.Points;
      leaderIdx = i;
    }
  }

  // draw 1st place player icon
  if (leaderIdx >= 0) {
    VECTOR delta;
    Player* player = playerGetAll()[leaderIdx];
    if (!player || !playerIsConnected(player) || player->IsLocal) return;
    
    // pulse color
    u32 color = colorLerp(0x8000C0C0, 0x80408080, 0.5 * (1 + sinf(gameGetTime() / 250.0)));

    // get position and scale size by distance
    vector_add(leaderWPos, leaderWPos, player->PlayerPosition);
    vector_subtract(delta, leaderWPos, cameraGetGameCamera(0)->pos);
    float dist = vector_length(delta);
    float size = clamp(20 - (dist/16), 0, 24);
    if (size < 16) return;

	  gfxSetupGifPaging(0);
    gfxHelperDrawSprite_WS(leaderWPos, size, size, 32, 32, 73, color, TEXT_ALIGN_MIDDLECENTER, COMMON_DZO_DRAW_NORMAL);
    gfxDoGifPaging();
  }
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

  // recalculate scores
  gameCalculateScores();

  // update first place
  gameUpdateWinningTeam();

  // player updates
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    gameOnPlayerUpdate(players[i]);
  }
 
  // ensure we always have a seeker
  // only run as host so multiple clients don't select different seekers
  if (State.IsHost) {
    int seekerCount = gameGetSeekerCount();
    if (seekerCount <= 0) {
      if (State.TicksUntilSeekerSelection) {
        --State.TicksUntilSeekerSelection;
      } else {
        
        // select
        int hiderIdx = gameGetRandomHider();
        if (hiderIdx >= 0) {
          gameSetSeeker(hiderIdx);
        }

        // reset delay for next iteration
        State.TicksUntilSeekerSelection = TAG_INITIAL_SEEKER_SELECTOR_DELAY;
      }
    }
  }
}

//--------------------------------------------------------------------------
void initialize(PatchStateContainer_t* gameState)
{
  static int startDelay = 60 * 0.2;
	static int waitingForClientsReady = 0;

	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	GameData* gameData = gameGetData();
	int i;

	// Disable normal game ending
	*(u32*)0x006217E4 = 0;	// hill time reached (7)
	*(u32*)0x006216D8 = 0;	// hill time reached (7)

	// hook into player kill event
  HOOK_JAL(0x00621c7c, &onPlayerKill);

  // hook hill moby is player healthy
  HOOK_JAL(0x004449d4, &onHillMobyConsiderPlayer);

	// patch who hit me to prevent damaging others
  HOOK_JAL(0x005E07C8, &onWhoHitMe);
  HOOK_JAL(0x005E11B0, &onWhoHitMe);

	// hook get resurrect point
  HOOK_JAL(0x005e2d44, &getResurrectPoint);
  HOOK_JAL(0x00610724, &getResurrectPoint);

  // move ideal spawn distance from 40 to 2
  // spawns players closer to other players
  POKE_U16(0x00624614, 0x4000);

  // enable juggernaut aura FX
  POKE_U32(0x005d29a8, 0x2404000A);

  // allow gaining hill time after limit reached
  POKE_U32(0x00625BD0, 0x0000102D);

	// hook messages
	netHookMessages();

  // 
  initializeScoreboard();

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

	// initialize player states
	State.LocalPlayerState = NULL;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].PlayerIndex = i;
		State.PlayerStates[i].IsSeeker = 0;
		State.PlayerStates[i].LastPtsDelta = 0;
		State.PlayerStates[i].DisplayLastPtsDeltaTicks = 0;
		State.PlayerStates[i].Stats.Points = 0;
		State.PlayerStates[i].Stats.PointsBonus = 0;
		State.PlayerStates[i].Stats.PointsLostStolen = 0;
		State.PlayerStates[i].Stats.PointsGainStolen = 0;

		// is local
		if (p && p->IsLocal && !State.LocalPlayerState) {
      State.LocalPlayerState = &State.PlayerStates[i];
		}
	}

	// initialize state
	State.GameOver = 0;
	State.InitializedTime = gameGetTime();
  State.TicksUntilSeekerSelection = TAG_INITIAL_SEEKER_SELECTOR_DELAY;

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
		sGameData->Rounds = 0;
		sGameData->Version = 0x00000001;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
      sGameData->PointsLostStolen[i] = State.PlayerStates[i].Stats.PointsLostStolen;
      sGameData->PointsGainStolen[i] = State.PlayerStates[i].Stats.PointsGainStolen;
      sGameData->PointsBonus[i] = State.PlayerStates[i].Stats.PointsBonus;
			sGameData->Points[i] = State.PlayerStates[i].Stats.Points;
		}
	}
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(PatchStateContainer_t * gameState)
{
	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions || !gameSettings || gameSettings->GameLoadStartTime <= 0)
		return;

  // force ffa
  int i;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (gameSettings->PlayerClients[i] >= 0) {
      gameSettings->PlayerTeams[i] = i;
    }
  }
	
	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillArmor = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.HillSharing = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Vehicles = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Puma = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hoverbike = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Hovership = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Landstalker = 0;

  // disable all weapons
  gameOptions->WeaponFlags.DualVipers = 0;
  gameOptions->WeaponFlags.MagmaCannon = 0;
  gameOptions->WeaponFlags.Arbiter = 0;
  gameOptions->WeaponFlags.FusionRifle = 0;
  gameOptions->WeaponFlags.MineLauncher = 0;
  gameOptions->WeaponFlags.B6 = 0;
  gameOptions->WeaponFlags.Holoshield = 0;
  gameOptions->WeaponFlags.Flail = 0;

  // disable healthboxes
  gameState->GameConfig->grNoHealthBoxes = 2;
}
