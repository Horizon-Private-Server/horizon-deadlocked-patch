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
#include "common.h"
#include "include/game.h"
#include "include/utils.h"

extern int Initialized;

void initializeScoreboard(void);
void addAssistFeedItem(int localPlayerIdx);
void createFlag(VECTOR position);
int (*baseFlagHandleGuberEvent)(Moby*, GuberEvent*) = (int (*)(Moby*, GuberEvent*))0x00417BB8;

struct AssistFeedItem AssistFeedItems[ASSIST_FEED_MAX_ITEMS];

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
void onReceivePlayerAssist(int playerIdx)
{
  // give assist points to player
  State.PlayerStates[playerIdx].Stats.Points += POINTS_PER_ASSIST;

  // give assist points to team
  Player* player = playerGetAll()[playerIdx];
  if (player) {

    State.Teams[player->Team].Score += POINTS_PER_ASSIST;

    if (player->IsLocal) {
      addAssistFeedItem(player->LocalPlayerIndex);
    }
  }
}

//--------------------------------------------------------------------------
void onReceiveFlagBasePosition(VECTOR basePosition)
{
  if (!State.FlagMoby) return;

  struct FlagPVars* pvars = (struct FlagPVars*)State.FlagMoby->PVar;
  vector_copy(pvars->BasePosition, basePosition);
  DPRINTF("base %.2f %.2f %.2f\n", pvars->BasePosition[0], pvars->BasePosition[1], pvars->BasePosition[2]);
}

//--------------------------------------------------------------------------
void onPlayerKill(char * fragMsg)
{
	VECTOR delta;
	Player** players = playerGetAll();
  static int firstKill = 1;

	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);

	char weaponId = fragMsg[3];
	char killedPlayerId = fragMsg[2];
	char sourcePlayerId = fragMsg[0];

  // award points to source
  if (sourcePlayerId >= 0 && sourcePlayerId < GAME_MAX_PLAYERS) {

    Player* sourcePlayer = players[sourcePlayerId];
    if (sourcePlayer) {

      // add to player and team
      if (weaponId == WEAPON_ID_WRENCH && sourcePlayer->HeldMoby == State.FlagMoby)
      {
        // flag kill
        State.PlayerStates[sourcePlayerId].Stats.Points += POINTS_PER_FLAG_KILL;
        State.Teams[sourcePlayer->Team].Score += POINTS_PER_FLAG_KILL;
      }
      else
      {
        State.PlayerStates[sourcePlayerId].Stats.Points += POINTS_PER_KILL;
        State.Teams[sourcePlayer->Team].Score += POINTS_PER_KILL;
      }

      // if host, we also want to award assist points to the carrier's team
      // so check if carrier matches source player's team
      if (State.IsHost && State.CurrentFlagCarrierPlayerIdx >= 0 && State.CurrentFlagCarrierPlayerIdx < GAME_MAX_PLAYERS) {

        Player* carrierPlayer = players[State.CurrentFlagCarrierPlayerIdx];
        if (carrierPlayer && carrierPlayer->Team == sourcePlayer->Team) {
          // send player assist
          // and handle locally
          sendPlayerAssist(State.CurrentFlagCarrierPlayerIdx);
          onReceivePlayerAssist(State.CurrentFlagCarrierPlayerIdx);
        }
      }
    }
  }

  // spawn on player
  Player* killedPlayer = players[killedPlayerId];
  if (weaponId >= 0 && killedPlayer && State.IsHost && !State.FlagMoby && firstKill) {
    createFlag(killedPlayer->PlayerPosition);
    firstKill = 0;
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
void drawAssistFeed(void)
{
  int i;
  int firstUsedItem = ASSIST_FEED_MAX_ITEMS;
  int lastUsedItem = 0;
  int drawn = 0;
  char buf[12];

  for (i = 0; i < ASSIST_FEED_MAX_ITEMS; ++i) {
    struct AssistFeedItem* item = &AssistFeedItems[i];
    if (item->TicksLeft > 0) {

      // fade out when item is about to be deleted
      float alpha = clamp(item->TicksLeft / 15.0, 0, 1);
      u32 color = 0x0000FFFF;
      u32 alphaCol = (int)(alpha * 128) << 24;

      // draw
      snprintf(buf, sizeof(buf), "+%d", POINTS_PER_ASSIST);
      gfxHelperDrawText(0.77 * SCREEN_WIDTH, 0.09 * SCREEN_HEIGHT, 2, (drawn * 16) + 2, 1, alphaCol, buf, -1, 0, COMMON_DZO_DRAW_NORMAL);
      gfxHelperDrawText(0.77 * SCREEN_WIDTH, 0.09 * SCREEN_HEIGHT, 0, (drawn * 16) + 0, 1, alphaCol | color, buf, -1, 0, COMMON_DZO_DRAW_NORMAL);
    
      // increment
      item->TicksLeft -= 1;
      drawn += 1;

      // save index of first and last items in array
      // only if item is going to be drawn next tick
      if (item->TicksLeft > 0) {
        if (i < firstUsedItem) firstUsedItem = i;
        if (i > lastUsedItem) lastUsedItem = i;
      }
    }
  }

  // move items down array
  // replacing empty items
  if (firstUsedItem > 0 && firstUsedItem < ASSIST_FEED_MAX_ITEMS) {
    memmove(AssistFeedItems, &AssistFeedItems[firstUsedItem], (lastUsedItem - firstUsedItem + 1) * sizeof(struct AssistFeedItem));
    memset(&AssistFeedItems[lastUsedItem], 0, (ASSIST_FEED_MAX_ITEMS - lastUsedItem) * sizeof(struct AssistFeedItem));
  }
}

//--------------------------------------------------------------------------
void addAssistFeedItem(int localPlayerIdx)
{
  int freeIdx = 0;

  // find first free slot
  for (freeIdx = 0; freeIdx < ASSIST_FEED_MAX_ITEMS; ++freeIdx) {
    if (AssistFeedItems[freeIdx].TicksLeft <= 0) break;
  }

  // if no free slots
  // move all down
  // and push new item to end of array
  if (freeIdx >= ASSIST_FEED_MAX_ITEMS) {
    memmove(AssistFeedItems, &AssistFeedItems[1], (ASSIST_FEED_MAX_ITEMS - 1) * sizeof(struct AssistFeedItem));
    freeIdx = ASSIST_FEED_MAX_ITEMS - 1;
  }

  AssistFeedItems[freeIdx].TicksLeft = ASSIST_FEED_ITEM_LIFETIME_TICKS;
  AssistFeedItems[freeIdx].LocalPlayerIdx = localPlayerIdx;
}

//--------------------------------------------------------------------------
void flagUpdate(Moby* flagMoby)
{
  if (!flagMoby) return;
  Player** players = playerGetAll();
  Player* carrierPlayer = NULL;
  GuberMoby* guber = guberGetObjectByMoby(flagMoby);
  struct FlagPVars* pvars = (struct FlagPVars*)flagMoby->PVar;
  float one = 1;
  static int lastCarrier = -1;

  // force team white
  pvars->Team = TEAM_WHITE;
  flagMoby->PrimaryColor = 0xFFFFFFFF;

  // draw aura
  gfxRegisterDrawFunction(0x22251c, 0x41a048, flagMoby);

  // check for drop
  int carrierIdx = pvars->CarrierIdx;
  if (carrierIdx >= 0 && carrierIdx < GAME_MAX_PLAYERS) {
    carrierPlayer = players[carrierIdx];
    if (!carrierPlayer || carrierPlayer->HeldMoby != flagMoby || playerIsDead(carrierPlayer)) {

      // send drop event
      if ((!carrierPlayer && State.IsHost) || (carrierPlayer && carrierPlayer->IsLocal)) {
        GuberEvent* event = guberEventCreateEvent(guber, 2, 0, 0xFA);
        if (event) {
          guberEventWrite(event, flagMoby->Position, 0x0C);
          guberEventWrite(event, &one, 0x04);
        }
      }

      carrierIdx = -1;
    }
  }
  else {
    
    // handle falling
    // or force to base
    if (flagIsAtBase(flagMoby)) {
      vector_copy(flagMoby->Position, pvars->BasePosition);
    } else {
      VECTOR gravity = {0,0,-0.11,0};
      VECTOR offset = {0,0,0.01,0};
      VECTOR to;
      vector_add(to, flagMoby->Position, gravity);
      if (CollLine_Fix(flagMoby->Position, to, 0, flagMoby, 0) == 0) vector_add(flagMoby->Position, to, offset);
    }

  }

  // draw blip
  int blipIdx = radarGetBlipIndex(flagMoby);
  if (blipIdx >= 0) {
    RadarBlip* blip = radarGetBlips() + blipIdx;
    blip->Life = 0x1F;
    blip->Rotation = 0;
    blip->Team = carrierPlayer ? carrierPlayer->Team : pvars->Team;
    blip->Type = 2;
		blip->X = flagMoby->Position[0];
		blip->Y = flagMoby->Position[1];
  }

  // change base position when carrier changes
  if (State.IsHost && carrierIdx >= 0 && lastCarrier != carrierIdx) {
    while (!spawnGetRandomPlayerPoint(pvars->BasePosition)) ;
    sendFlagBasePosition(pvars->BasePosition);
    DPRINTF("base %.2f %.2f %.2f\n", pvars->BasePosition[0], pvars->BasePosition[1], pvars->BasePosition[2]);
  }

  lastCarrier = carrierIdx;

  // force up
  flagMoby->Rotation[1] = -MATH_PI / 2;
  flagMoby->ModeBits = 0x0010;
  if (carrierIdx >= 0 && carrierIdx < GAME_MAX_PLAYERS)
    flagMoby->ModeBits |= 0x0100;
}

//--------------------------------------------------------------------------
int flagHandleGuberEvent(Moby* moby, GuberEvent* event)
{
  // intercept flag event
  // and force moby update to our custom flag update
  // also store a reference to the flag in State
  moby->PUpdate = &flagUpdate;
  State.FlagMoby = moby;
  DPRINTF("flag event %08X %d\n", (u32)moby, event->NetEvent.EventID);

  int r = baseFlagHandleGuberEvent(moby, event);
  
  // on first spawn, reset base position to position
  // so that it doesn't respawn
  struct FlagPVars* pvars = (struct FlagPVars*)moby->PVar;
  if (event->NetEvent.EventID == 0 && pvars) {
    vector_copy(pvars->BasePosition, moby->Position);
  }

  return r;
}

//--------------------------------------------------------------------------
void createFlag(VECTOR position)
{
	GuberEvent * guberEvent = 0;
  VECTOR p = {0,0,2,0};
  int a = 0;
  int b = 0;

  vector_add(p, p, position);

	// create guber object
	GuberMoby * guberMoby = guberMobyCreateSpawned(MOBY_ID_BLUE_FLAG, 0x100, &guberEvent, NULL);
	if (guberEvent)
	{
		guberEventWrite(guberEvent, p, 0x0C);
		guberEventWrite(guberEvent, p, 0x0C);
		guberEventWrite(guberEvent, &a, 0x04);
		guberEventWrite(guberEvent, &b, 0x04);
	}
	else
	{
		DPRINTF("failed to guberevent flag\n");
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

  // update who has flag
  State.CurrentFlagCarrierPlayerIdx = -1;
  if (State.FlagMoby && State.FlagMoby->PVar) {
    State.CurrentFlagCarrierPlayerIdx = ((struct FlagPVars*)State.FlagMoby->PVar)->CarrierIdx;
  }

  // track time holding flag pts
  if (State.CurrentFlagCarrierPlayerIdx >= 0 && State.CurrentFlagCarrierPlayerIdx < GAME_MAX_PLAYERS) {
    State.PlayerStates[State.CurrentFlagCarrierPlayerIdx].Stats.TimeWithFlag += 1 / 60.0;
  }

  // move scores into game data
  // which will be used by the game to determine who wins
  // and how to sort the end game scoreboard
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    gameData->TeamStats.TeamTicketScore[i] = State.Teams[i].Score;
    gameData->PlayerStats.TicketScore[i] = State.PlayerStates[i].Stats.Points;
  }

  // 
  drawAssistFeed();
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

	// hook into player kill event
	*(u32*)0x00621c7c = 0x0C000000 | ((u32)&onPlayerKill >> 2);

  // hook into computePlayerScore (endgame pts)
  HOOK_J(0x00625510, &getPlayerScore);
  POKE_U32(0x00625514, 0);

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

  // change flag guber handler
  POKE_U32(0x003A0DD4, &flagHandleGuberEvent);

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

	// reset team scores
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		State.Teams[i].Score = 0;
	}

	// initialize player states
	State.LocalPlayerState = NULL;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].PlayerIndex = i;
		State.PlayerStates[i].Stats.Points = 0;
		State.PlayerStates[i].Stats.TimeWithFlag = 0;

		// is local
		if (p && p->IsLocal && !State.LocalPlayerState) {
      State.LocalPlayerState = &State.PlayerStates[i];
		}
	}

  // reset assist feed
  memset(AssistFeedItems, 0, sizeof(AssistFeedItems));
  
	// initialize state
  State.CurrentFlagCarrierPlayerIdx = -1;
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
			sGameData->TeamScore[i] = State.Teams[i].Score;
      sGameData->TimeWithFlag[i] = State.PlayerStates[i].Stats.TimeWithFlag;
			sGameData->Points[i] = State.PlayerStates[i].Stats.Points;
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
  gameOptions->GameFlags.MultiplayerGameFlags.KillsToWin = 0;
  if (gameOptions->GameFlags.MultiplayerGameFlags.Timelimit == 0)
    gameOptions->GameFlags.MultiplayerGameFlags.Timelimit = 5;

  set = 1;
}
