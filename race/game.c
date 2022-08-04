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
int aaa = 0;
#endif

// 
struct RaceConfig Config __attribute__((section(".config"))) = {
	.PathVertexCount = 13,
	.Path = {
		{
			{ 434.6114, 129.1436, 106.79, 0 },
			{ 434.6114, 189.1436, 106.79, 0 },
			{ 434.6114, 204.6036, 106.79, 0 },
			 .Disconnected = 0 
		},
		{
			{ 425.9593, 203.5597, 117.5641, 0 },
			{ 435.4033, 218.8396, 122.49, 0 },
			{ 444.8473, 234.1195, 127.4159, 0 },
			 .Disconnected = 0 
		},
		{
			{ 464.4416, 247.7817, 140.4637, 0 },
			{ 483.2513, 240.1076, 142.84, 0 },
			{ 502.0611, 232.4336, 145.2163, 0 },
			 .Disconnected = 0 
		},
		{
			{ 519.8095, 240.8596, 140.2249, 0 },
			{ 522.7714, 243.9236, 142.84, 0 },
			{ 525.7332, 246.9877, 145.4551, 0 },
			 .Disconnected = 1 
		},
		{
			{ 527.1915, 223.9904, 110.6086, 0 },
			{ 531.3, 282.3, 122, 0 },
			{ 531.9529, 291.5673, 123.8105, 0 },
			 .Disconnected = 0 
		},
		{
			{ 553.3903, 383.1577, 116.8666, 0 },
			{ 506.57, 433.68, 130.29, 0 },
			{ 491.2693, 450.1906, 134.6767, 0 },
			 .Disconnected = 0 
		},
		{
			{ 470.1397, 411.2581, 111.4728, 0 },
			{ 427.9113, 456.5436, 114.91, 0 },
			{ 383.4917, 504.1791, 118.5255, 0 },
			 .Disconnected = 0 
		},
		{
			{ 328.7635, 452.2501, 133.19, 0 },
			{ 325.1114, 402.3836, 133.19, 0 },
			{ 323.1684, 375.8546, 133.19, 0 },
			 .Disconnected = 0 
		},
		{
			{ 344.6641, 359.2715, 149.9785, 0 },
			{ 356.2114, 370.8436, 153.39, 0 },
			{ 368.2288, 382.8869, 156.9405, 0 },
			 .Disconnected = 0 
		},
		{
			{ 333.8216, 405.2706, 170.65, 0 },
			{ 322.5714, 391.3736, 170.65, 0 },
			{ 302.594, 366.6964, 170.65, 0 },
			 .Disconnected = 0 
		},
		{
			{ 301.1886, 319.9573, 208.39, 0 },
			{ 303.8113, 280.1436, 208.39, 0 },
			{ 307.0849, 230.4513, 208.39, 0 },
			 .Disconnected = 0 
		},
		{
			{ 269.7726, 129.7062, 123.483, 0 },
			{ 340.5914, 127.3836, 113.62, 0 },
			{ 357.5388, 126.8278, 111.2597, 0 },
			 .Disconnected = 0 
		},
		{
			{ 434.6114, 129.1436, 106.79, 0 },
			{ 434.6114, 189.1436, 106.79, 0 },
			{ 434.6114, 204.6036, 106.79, 0 },
			 .Disconnected = 0 
		}
	},
	.CheckPointCount = 4,
	.CheckPoints = {
		{ 434.6114, 189.1436, 108.94, 90 },
		{ 530.4, 274, 143.84, 92.81549 },
		{ 427.9, 456.36, 116.73, 40.12 },
		{ 303.8113, 280.1436, 210.77, -93.76904 },
	},
	.TargetLaps = 4,
};

struct CubicLineEndPoint endpoints[2] = {
	{
		.iCoreRGBA = 0,
		.iGlowRGBA = 0,
		.bDisabled = 0,
		.bFadeEnd = 0,
		.iNumSkipPoints = 0,
		.numEndPoints = 2,
		.style = 0,
		.vPos = { 0,0,0,0 },
		.vTangent = { 0,0,1,0 },
		.vTangentOccQuat = { 0,0,0,1 }
	},
	{
		.iCoreRGBA = 0,
		.iGlowRGBA = 0,
		.bDisabled = 0,
		.bFadeEnd = 0,
		.iNumSkipPoints = 0,
		.numEndPoints = 2,
		.style = 0,
		.vPos = { 0,0,0,0 },
		.vTangent = { 0,0,1,0 },
		.vTangentOccQuat = { 0,0,0,1 }
	}
};

//--------------------------------------------------------------------------
void onPickupItemSpawned(int type, VECTOR position)
{
	DPRINTF("pickup %d spawned at (%f, %f, %f)\n", type, position[0], position[1], position[2]);
}

//--------------------------------------------------------------------------
void onPlayerKill(char * fragMsg)
{
	VECTOR delta;

	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);

	// ignore if not initialized
	if (!State.InitializedTime)
		return;
	
	char weaponId = fragMsg[3];
	char killedPlayerId = fragMsg[2];
	char sourcePlayerId = fragMsg[0];
}

//--------------------------------------------------------------------------
float getPlayerResurrectOffset(int playerId)
{
	GameSettings* gs = gameGetSettings();
	if (!gs || gs->PlayerCount <= 0)
		return 0;

	float count = (float)gs->PlayerCount;
	float d2 = count / 2;
	return 5 * ((playerId / (count + 1)) - d2);
}

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
	char possiblePoints[10];
	int pointCount = 0;
	int i = 0;
	struct RacePlayer* playerData = &State.PlayerStates[player->PlayerId];
	int selectedPoint = playerData->Stats.LastCheckpoint;
	VECTOR delta;
	VECTOR spawnHitOffset = {0,0,0.1,0};

	// if first res, use defaults
	if (firstRes || !State.InitializedTime)
		goto set;
	
set: ;

	// set position
	float theta = Config.CheckPoints[selectedPoint][3] * MATH_DEG2RAD;
	float radius = getPlayerResurrectOffset(player->PlayerId);
	vector_copy(outPos, Config.CheckPoints[selectedPoint]);
	outPos[0] += cosf(theta) * radius;
	outPos[1] += sinf(theta) * radius;
	if (!firstRes)
		outPos[2] += 4;

	// set yaw
	vector_write(outRot, 0);
	outRot[2] = theta;
}

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
	return (int)State.PlayerStates[team].Stats.LapNumber;
}

//--------------------------------------------------------------------------
void spawnTrack(void)
{
	VECTOR euler = { 0, 0, 0, 0 };
	VECTOR pos;
	VECTOR tangent;
	VECTOR normal;
	VECTOR bitangent;
	VECTOR r0 = { 0, 0, 0, 1 };
	MATRIX bezierRotationMatrix, startRotationMatrix;
	float scale = 3;
	float t = 0;
	float stepDistance = 1.1 * scale;
	int b = 0;
	int bezierCount = Config.PathVertexCount;

	// initialize object rotation matrix
	matrix_unit(startRotationMatrix);
	matrix_rotate_y(startRotationMatrix, startRotationMatrix, 90 * MATH_DEG2RAD);

	DPRINTF("BEGIN TRACK SPAWN\n");

	for (b = 0; b < (bezierCount-1); ++b)
	{
		if (Config.Path[b].Disconnected)
			continue;

		DPRINTF("TRACK %d to %d\n", b, b+1);

		t = 0;
		while (1)
		{
			// Calculate bezier
			bezierGetPosition(pos, &Config.Path[b], &Config.Path[b+1], t);
			bezierGetTangent(tangent, &Config.Path[b], &Config.Path[b+1], t);
			bezierGetNormal(normal, &Config.Path[b], &Config.Path[b+1], t);
			vector_copy(normal, (VECTOR){ 0,0,1,0 });
			vector_outerproduct(bitangent, normal, tangent);

			// Determine euler rotation
			matrix_fromrows(bezierRotationMatrix, normal, bitangent, tangent, r0);
			matrix_multiply(bezierRotationMatrix, startRotationMatrix, bezierRotationMatrix);
			matrix_toeuler(euler, bezierRotationMatrix);
			
			// spawn
			Moby* trackPieceMoby = mobySpawn(MOBY_ID_OTHER_PART_FOR_SARATHOS_BRIDGE, 0);
			if (trackPieceMoby)
			{
				// copy pos/rot
				pos[3] = trackPieceMoby->Position[3];
				vector_copy(trackPieceMoby->Position, pos);
				vector_copy(trackPieceMoby->Rotation, euler);

				// update moby params
				trackPieceMoby->UpdateDist = 0xFF;
				trackPieceMoby->Drawn = 0x01;
				trackPieceMoby->DrawDist = 0x0080;
				trackPieceMoby->Opacity = 0x7E;
				trackPieceMoby->State = 1;

				trackPieceMoby->Scale = (float)0.11 * scale;
				trackPieceMoby->Lights = 0x202;
				trackPieceMoby->GuberMoby = 0;
			}

			// Get next time
			// If same as last time then break
			float distanceMoved = bezierMove(&t, &Config.Path[b], &Config.Path[b+1], stepDistance);
			if (distanceMoved < MIN_FLOAT_MAGNITUDE)
				break;
		}
	}

	DPRINTF("END TRACK SPAWN\n");
}

//--------------------------------------------------------------------------
void processPlayer(int pIndex)
{
	VECTOR t1, t2;
	int i = 0, localPlayerIndex, hasMessage = 0;
	int x,y;
	char strBuf[32];
	int gameTime = gameGetTime();

	struct RacePlayer * playerData = &State.PlayerStates[pIndex];

	// decrement ticks since reached last checkpoint
	decTimerU32(&playerData->TicksSinceReachedLastCheckpoint);

	// ensure we have a player struct
	Player * player = playerData->Player;
	if (!player)
		return;

	// immediately respawn vehicle
	Vehicle* hoverbike = playerData->Hoverbike;
	if (hoverbike) {
		hoverbike->resurrectTimer = 0;
	}

	// if race hasn't started then force position to start
	if (gameTime < State.RaceStartTime) {
		VECTOR p, r;
		getResurrectPoint(player, p, r, 1);
		playerSetPosRot(player, p, r);
		if (hoverbike) {
			vector_copy(hoverbike->pMoby->Position, p);
			vector_copy(hoverbike->pMoby->Rotation, r);
			if (player->Vehicle != hoverbike && (gameTime - State.InitializedTime) > TIME_SECOND)
				vehicleAddPlayer(hoverbike, player);
		}

		sprintf(strBuf, "%d", 1 + ((State.RaceStartTime - gameTime) / 1000));
		drawRoundMessage(strBuf, 2);
	}

	// force player into their hoverbike
	else if (!playerIsDead(player) && (gameTime - State.InitializedTime) > (TIME_SECOND) && hoverbike && player->Vehicle != hoverbike) {
		vector_copy(hoverbike->pMoby->Position, player->PlayerPosition);
		vector_copy(hoverbike->pMoby->Rotation, player->PlayerRotation);
		if (player->Vehicle)
			vehicleRemovePlayer(player->Vehicle, player);
		vehicleAddPlayer(hoverbike, player);
	}

	// check if we've reached the next checkpoint
	int currentCheckpoint = playerData->Stats.LastCheckpoint;
	int targetCheckpoint = (currentCheckpoint + 1) % Config.CheckPointCount;
	float targetCheckpointYaw = Config.CheckPoints[targetCheckpoint][3] * MATH_DEG2RAD;

	// 
	vector_subtract(t1, Config.CheckPoints[targetCheckpoint], player->PlayerPosition);
	vector_fromyaw(t2, targetCheckpointYaw);
	if (vector_sqrmag(t1) < (CHECKPOINT_HIT_RADIUS * CHECKPOINT_HIT_RADIUS) && vector_innerproduct_unscaled(t1, t2) < (CHECKPOINT_HIT_DEPTH * CHECKPOINT_HIT_DEPTH)) {

		// move to next checkpoint
		playerData->Stats.LastCheckpoint = targetCheckpoint;
		playerData->TicksSinceReachedLastCheckpoint = CHECKPOINT_REACHED_FADE_TICKS;

		// play sound
		playCheckpointReachedSound();

		// increment lap if reached first checkpoint again
		if (targetCheckpoint == 0) {
			playerData->Stats.LapNumber++;
		}
	}
}

//--------------------------------------------------------------------------
void drawCheckpoints(void)
{
	const u32 checkpointActiveColor = 0x800000FF;
	const u32 checkpointReachedColor = 0x00FFFFFF;

	int i;
	int gameTime = gameGetTime();
	float pulse = sinf(gameTime / 1000.0) * 0.5;
	VECTOR p;

	if (!State.LocalPlayerState)
		return;

	int targetCheckpointIdx = (State.LocalPlayerState->Stats.LastCheckpoint + 1) % Config.CheckPointCount;

	for (i = 0; i < Config.CheckPointCount; ++i) {
		u32 color = checkpointActiveColor;
		float ringRadius = CHECKPOINT_RADIUS;

		// move checkpoint up and down
		vector_copy(p, Config.CheckPoints[i]);
		p[2] += pulse;

		// only draw if target checkpoint
		// or if last checkpoint was recently reached
		if (State.LocalPlayerState->TicksSinceReachedLastCheckpoint && i == State.LocalPlayerState->Stats.LastCheckpoint) {
			float t = 1 - (State.LocalPlayerState->TicksSinceReachedLastCheckpoint / (float)CHECKPOINT_REACHED_FADE_TICKS);
			ringRadius += CHECKPOINT_REACHED_FADE_MAX_SCALE * t;
			color = colorLerp(color, checkpointReachedColor, sqrtf(t));
		} else if (i != targetCheckpointIdx)
			continue;

		drawRing(Config.CheckPoints[i][3] * MATH_DEG2RAD, p, ringRadius, 0.5, color);
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

#if DEBUG
	if (padGetButtonDown(0, PAD_L3 | PAD_R3))
		State.GameOver = 1;
#endif

  // hook draw
  if (*(u32*)0x004C4748 == 0x03E00008)
    *(u32*)0x004C4748 = 0x08000000 | ((u32)&drawCheckpoints >> 2);

	/*
	static int soundId = 0;
	if (padGetButtonDown(0, PAD_LEFT) > 0) {
		soundId--;
		DPRINTF("%d\n", soundId);
		((void (*)(Player*, int, int))0x005eb280)((Player*)0x347AA0, soundId, 0);
	} else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
		soundId++;
		DPRINTF("%d\n", soundId);
		((void (*)(Player*, int, int))0x005eb280)((Player*)0x347AA0, soundId, 0);
	}*/

	// update score
	
  // handle round end logic
  if (State.IsHost)
  {
		for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
			if (State.PlayerStates[i].Stats.LapNumber >= Config.TargetLaps) {
				State.GameOver = 1;
			}
		}
  }

	// set winner
	int bestScore = 0;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		if (State.PlayerStates[i].Player && State.PlayerStates[i].Stats.LapNumber > bestScore) {
			bestScore = State.PlayerStates[i].Stats.LapNumber;
			State.WinningTeam = State.PlayerStates[i].Player->Team;
		}
	}

	// send score to others at end
	if (State.GameOver && State.IsHost)
	{
		//sendTeamScore();
	}
}

//--------------------------------------------------------------------------
void initialize(PatchGameConfig_t* gameConfig)
{
	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	GameData* gameData = gameGetData();
	int i, j;

	// Disable normal game ending
	//*(u32*)0x006219B8 = 0;	// survivor (8)
	//*(u32*)0x00620F54 = 0;	// time end (1)
	//*(u32*)0x00621568 = 0;	// kills reached (2)
	//*(u32*)0x006211A0 = 0;	// all enemies leave (9)

	// point get resurrect point to ours
	*(u32*)0x00610724 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
	*(u32*)0x005e2d44 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);

	// hook into player kill event
	*(u32*)0x00621c7c = 0x0C000000 | ((u32)&onPlayerKill >> 2);



	// hook messages
	netHookMessages();

  // 
  initializeScoreboard(Config.TargetLaps);

	// give a 1 second delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 60 * 1;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	// spawn track
	spawnTrack();

	// calculate length of path and segments
	State.PathLength = 0;
	for (i = 0; i < (Config.PathVertexCount-1); ++i)
	{
		float length = 0;
		if (!Config.Path[i].Disconnected)
			length = (&Config.Path[i], &Config.Path[i+1], 0, 1);
		State.PathLength += State.PathSegmentLengths[i] = length;
	}

	// calculate length of checkpoints
	VECTOR tOut;
	float lastStartT = 0;
	for (i = 0; i < (Config.CheckPointCount - 1); ++i)
	{
		float newT = bezierGetClosestPointOnPath(tOut, Config.CheckPoints[i+1], Config.Path, State.PathSegmentLengths, Config.PathVertexCount);
		State.CheckPointLengths[i] = newT - lastStartT;
		lastStartT = newT;
		DPRINTF("length of c%d = %f\n", i, State.CheckPointLengths[i]);
	}

	// length of last checkpoint is length to end of path
	State.CheckPointLengths[Config.CheckPointCount - 1] = State.PathLength - lastStartT;
	DPRINTF("length of c%d = %f\n", Config.CheckPointCount - 1, State.CheckPointLengths[Config.CheckPointCount - 1]);

	// initialize player states
	State.LocalPlayerState = NULL;
	memset(State.PlayerStates, 0, sizeof(State.PlayerStates));
	Moby* lastVehicleMoby = mobyFindNextByOClass(mobyListGetStart(), MOBY_ID_HOVERBIKE);
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].Player = p;

		if (p) {

			// spawn at start
			getResurrectPoint(p, p->PlayerPosition, p->PlayerRotation, 1);
			
			// assign vehicle
			if (lastVehicleMoby) {
				State.PlayerStates[i].Hoverbike = (Vehicle*)lastVehicleMoby->NetObject;

				// move vehicle to player
				vector_copy(State.PlayerStates[i].Hoverbike->initPos, p->PlayerPosition);
				vector_copy(State.PlayerStates[i].Hoverbike->initRot, p->PlayerRotation);
				vector_copy(State.PlayerStates[i].Hoverbike->pMoby->Position, p->PlayerRotation);
				//vehicleAddPlayer(State.PlayerStates[i].Hoverbike, p);
				//vehicleRemovePlayer(State.PlayerStates[i].Hoverbike, p);

				// find next
				lastVehicleMoby = mobyFindNextByOClass(lastVehicleMoby + 1, MOBY_ID_HOVERBIKE);
			}

			// is local
			if (p->IsLocal && !State.LocalPlayerState)
				State.LocalPlayerState = &State.PlayerStates[i];
		}
	}

	// initialize state
	State.GameOver = 0;
	State.InitializedTime = gameGetTime();
	State.RaceStartTime = State.InitializedTime + (TIME_SECOND * 5);

	Initialized = 1;
}

//--------------------------------------------------------------------------
void updateGameState(PatchStateContainer_t * gameState)
{
	int i,j;

	// game state update
	if (gameState->UpdateGameState)
	{
		gameState->GameStateUpdate.RoundNumber = 0;
	}

	// stats
	if (gameState->UpdateCustomGameStats)
	{
		struct RaceGameData* sGameData = (struct RaceGameData*)gameState->CustomGameStats.Payload;
		sGameData->Version = 0x00000001;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			sGameData->Points[i] = State.PlayerStates[i].Stats.LapNumber;
		}
	}
}

//--------------------------------------------------------------------------
void setLobbyGameOptions(void)
{
	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions)
		return;
	
	// apply options
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;

	//gameSettings->PlayerTeams[0] = 1;
}
