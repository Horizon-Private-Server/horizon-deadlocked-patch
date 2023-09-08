/***************************************************
 * FILENAME :		main.c
 * 
 * DESCRIPTION :
 * 		Infected entrypoint and logic.
 * 
 * NOTES :
 * 		Each offset is determined per app id.
 * 		This is to ensure compatibility between versions of Deadlocked/Gladiator.
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/stdio.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/cheats.h>
#include <libdl/sha1.h>
#include <libdl/map.h>
#include <libdl/dialog.h>
#include <libdl/ui.h>
#include <libdl/spawnpoint.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/radar.h>
#include <libdl/utils.h>
#include "module.h"
#include "include/game.h"

#define MAX_MAP_MOBY_DEFS		(20)
#define MAX_WATER_RATE			(0.02)
#define MAX_SPAWN_RATE			(TIME_SECOND * 1.1)


/*
 *
 */
struct ClimberConfig Config __attribute__((section(".config"))) = {
	
};

/*
 *
 */
u8 WaterPVars[0x70] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x0A, 0xD7, 0xA3, 0x3B, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x40, 0x41, 0x00, 0x00, 0x7A, 0x43, 0x9A, 0x99, 0x99, 0x3E, 0xCD, 0xCC, 0x4C, 0x3E, 0x00, 0x00, 0x40, 0x41, 0x00, 0x00, 0xA0, 0x41, 0x00, 0x00, 0x80, 0x3E, 0x80, 0x80, 0x80, 0x80, 0x22, 0x22, 0x22, 0x80, 0x00, 0x00, 0xB4, 0xC3, 0x82, 0x00, 0x37, 0x00, 0x00, 0x00, 0x48, 0x42, 0x00, 0x80, 0x6D, 0x44, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0xCA, 0x42, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/*
 *
 */
int Initialized = 0;

/*
 *
 */
int SpawnRate = TIME_SECOND * 3;
int BranchRate = TIME_SECOND * 30;
int LastSpawn = 0;
float WaterRaiseRate = 0.1 * (1 / 60.0);
float WaterHeight = 0;
int MobyCount = 0;
unsigned int RandSeed = 0;
Moby * RadarBlipMoby = NULL;
Moby * WaterMoby = NULL;
int BranchDialogs[] = { DIALOG_ID_CLANK_YOU_HAVE_A_CHOICE_OF_2_PATHS, DIALOG_ID_DALLAS_WOAH_THIS_IS_GETTING_INTERESTING, DIALOG_ID_DALLAS_KICKING_PROVERBIAL_BUTT_IDK_WHAT_THAT_MEANS, DIALOG_ID_DALLAS_DARKSTAR_TIGHT_SPOTS_BEFORE  };
int StartDialogs[] = { DIALOG_ID_TEAM_DEADSTAR, DIALOG_ID_DALLAS_SHOWTIME, DIALOG_ID_DALLAS_RATCHET_LAST_WILL_AND_TESTAMENT, DIALOG_ID_DALLAS_WHO_PACKED_YOUR_PARACHUTE, };
int IncrementalDialogs[] = { DIALOG_ID_PLEASE_TAKE_YOUR_TIME, DIALOG_ID_DALLAS_SHOWOFF, DIALOG_ID_JUANITA_MORON, DIALOG_ID_JUANITA_I_CANT_BEAR_TO_LOOK_YES_I_CAN, DIALOG_ID_DALLAS_CARNAGE_LAST_RELATIONSHIP, DIALOG_ID_DALLAS_LOOK_AT_THAT_LITTLE_GUY_GO,  };

struct ClimbChain
{
	int Active;
	int NextItem;
	float LastTheta;
	int LastBranch;
	VECTOR CurrentPosition;
} Chains[4];

/*
 *
 */
VECTOR StartUNK_80 = {
	0.00514222,
	-0.0396723,
	62013.9,
	62013.9
};

VECTOR VECTOR_M0 = { 1, 0, 0, 0 };
VECTOR VECTOR_M1 = { 0, 1, 0, 0 };
VECTOR VECTOR_M2 = { 0, 0, 1, 0 };

int MapMobyDefsCount = 0;
MobyDef * MapMobyDefs[MAX_MAP_MOBY_DEFS];

MobyDef MobyDefs2[] = {
	{ 3, 0.8, 0.5, MOBY_ID_TELEPORT_PAD, 0 },
};

MobyDef MobyDefs[] = {
	
	{ 5, 0.85, 1, MOBY_ID_PART_CATACROM_BRIDGE, 0 },

	{ 5, 0.85, 1, MOBY_ID_SARATHOS_BRIDGE, 1 },
	{ 5, 0.85, 1, MOBY_ID_OTHER_PART_FOR_SARATHOS_BRIDGE, 1 },
	{ 7, 0.85, 1, MOBY_ID_GLASS_FROM_LEVIATHANS_LAIR, 1 },
	{ 4, 0.5, 0.5, MOBY_ID_CIRCLE_BARRICADE, 0 },

	{ 5, 0.8, 1, MOBY_ID_DARK_CATHEDRAL_SECRET_PLATFORM, 1 },
	{ 5, 0.8, 1, MOBY_ID_POSSIBLY_THE_BOTTOM_TO_THE_BATTLE_BOT_PRISON_CONTAINER, 0 },

	
	
	{ 3, 0.5, 0.5, MOBY_ID_BETA_BOX, 0 },
	{ 5, 0.85, 1, MOBY_ID_VEHICLE_PAD, 0 },
	{ 3, 0.8, 0.5, MOBY_ID_TELEPORT_PAD, 0 },
	{ 5, 0.8, 0.7, MOBY_ID_CONQUEST_TURRET_HOLDER_TRIANGLE_THING, 0 },
	{ 3, 0.8, 0.5, MOBY_ID_PICKUP_PAD, 0 },

	{ 3, 1, 0.7, MOBY_ID_SMALL_RED_CIRCULAR_PLATFORM, 0 },
	{ 4, 0.5, 0.7, MOBY_ID_BOT_GRIND_CABLE_LAUNCH_PAD, 0 },
	{ 7, 0.8, 3, MOBY_ID_INTERPLANETARY_TRANSPORT_SHIP, 0 },
	{ 4, 1, 0.5, MOBY_ID_BATTLEDOME_INTERACTIVE_PLATFORM_RED_LIGHTS_ON_THE_SIDE, 0 },
	{ 3, 0.8, 2, MOBY_ID_BLUE_FLOORLIGHT_OBJECT_CONTAINMENT_SUITE_THINGS, 0 },
	{ 2, 0.5, 0.5, MOBY_ID_DREADZONE_SATELLITE, 0 },
	{ 4, 1, 1, MOBY_ID_DREADZONE_EMBLEM, 0 },
	{ 5, 0.3, 0.5, MOBY_ID_MARAXUS_BOX, 0 },
	{ 7, 1, 1, MOBY_ID_TORVAL_PLATFORM, 0 },
	{ 3, 0.5, 3, MOBY_ID_TORVAL_STREET_LIGHT, 0 },
	{ 4, 0.8, 3, MOBY_ID_RED_DREADZONE_LIFT, 0 },
	{ 3, 1, 1.2, MOBY_ID_TURRET_SHIELD_UPGRADE, 0 }
};

//--------------------------------------------------------------------------
int myRand(int mod)
{
  RandSeed = (1664525 * RandSeed + 1013904223);
  return RandSeed % mod;
}

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
	int s = 0, i;
	Player ** players = playerGetAll();

	// find best score on team
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		if (players[i] && players[i]->Team == team)
		{
			if (State.PlayerBestHeight[i] > score)
				score = State.PlayerBestHeight[i];
		}
	}

	return score;
}

void * GetMobyClassPtr(int oClass)
{
    int mClass = *(u8*)(0x0024a110 + oClass);
    return *(u32*)(0x002495c0 + mClass*4);
}

Moby * spawnWithPVars(int OClass)
{
	switch (OClass)
	{
		// case MOBY_ID_VEHICLE_PAD: return mobySpawn(OClass, 0x60);
		// case MOBY_ID_PICKUP_PAD: return mobySpawn(OClass, 0x90);
		// case MOBY_ID_TELEPORT_PAD: return mobySpawn(OClass, 0xD0);
		// case MOBY_ID_TURRET_SHIELD_UPGRADE: return mobySpawn(OClass, 0xD0);
		default: return mobySpawn(OClass, 0);
	}
}

Moby * spawn(MobyDef* def, VECTOR position, VECTOR rotation, float scale)
{
	Moby * sourceBox;

	// Some models are buggy when spawned from a beta box template, so spawn them the old way.
	// On the other hand, some models can only be spawned using a beta box template.
	if(def->SpawnNormal) {
		// Spawn box so we know the correct model and collision pointers
		sourceBox = spawnWithPVars(def->OClass);
		if (!sourceBox)
			return 0;

	} else {
		sourceBox = mobySpawn(MOBY_ID_BETA_BOX, 0);
		if (!sourceBox)
			return 0;

		void* mobyClass = GetMobyClassPtr(def->OClass);

		if(!mobyClass) {
			mobyDestroy(sourceBox);
			return 0;
		}

		sourceBox->OClass = def->OClass;
		sourceBox->PClass = mobyClass;
		sourceBox->CollData = *(int*) ((u32)mobyClass + 0x10);
		sourceBox->MClass = *(u8*)(0x0024a110 + def->OClass);
	}

	// 
	position[3] = sourceBox->Position[3];
	vector_copy(sourceBox->Position, position);

	//
	vector_copy(sourceBox->Rotation, rotation);

	sourceBox->UpdateDist = 0xFF;
	sourceBox->Drawn = 0x01;
	sourceBox->DrawDist = 0x0080;
	sourceBox->Opacity = 0x80;
	sourceBox->State = 0;
	sourceBox->ModeBits = 0x0050;
	sourceBox->GlowRGBA = 0x808C8C8C;


 	sourceBox->Scale = (float)0.11 * scale * def->ObjectScale;
	sourceBox->Lights = 0x303;
	sourceBox->GuberMoby = 0;

	// For this model the vector here is copied to 0x80 in the moby
	// This fixes the occlusion bug
	vector_copy(sourceBox->LSphere, StartUNK_80);

	// attempt to fix texture bugging
	sourceBox->BSphere[3] = 0x79900000;
 
	DPRINTF("source: %08x\n", (u32)sourceBox);

	return sourceBox;
}

struct ClimbChain * GetFreeChain(void)
{
	int i = 0;
	for (; i < 2; ++i)
		if (!Chains[i].Active)
			return &Chains[i];

	return NULL;
}

void DestroyOld(void)
{
	Moby * moby = mobyListGetStart();
	Moby * mEnd = mobyListGetEnd();

	while (moby < mEnd)
	{
		if (!mobyIsDestroyed(moby) && moby->Opacity == 0x7E)
		{
			if (moby->Position[2] < WaterHeight)
			{
				mobyDestroy(moby);
				moby->Opacity = 0x7F;
				--MobyCount;
			}
		}

		++moby;
	}
}

float RandomRange(float min, float max)
{
	int v = myRand(0xFFFF);
	float value = ((float)v / 0xFFFF);

	return (value + min) * (max - min);
}

short RandomRangeShort(short min, short max)
{
	int v = myRand(0xFFFF);
	return (v % (max - min)) + min;
}

void GenerateNext(struct ClimbChain * chain, MobyDef * currentItem, float scale)
{
	// Determine next object
	chain->NextItem = RandomRangeShort(0, MapMobyDefsCount);
	MobyDef * nextItem = MapMobyDefs[chain->NextItem];

	DPRINTF("Moby class: %04x %08x\n", nextItem->OClass, GetMobyClassPtr(nextItem->OClass));

	// Adjust scale by current item
	if (currentItem)
		scale *= (currentItem->ScaleHorizontal + nextItem->ScaleHorizontal) / 2;

	// Generate next position
	chain->LastTheta = clampAngle(RandomRange(chain->LastTheta - (MATH_PI/4), chain->LastTheta + (MATH_PI/4)));
	chain->CurrentPosition[0] += scale * cosf(chain->LastTheta);
	chain->CurrentPosition[1] += scale * sinf(chain->LastTheta);
	chain->CurrentPosition[2] += nextItem->ScaleVertical * RandomRange(1.5, 3);
}

void onReceiveSpawn(u32 seed, int gameTime)
{
	DPRINTF("recv %d\n", gameTime);
	RandSeed = seed;
	int chainIndex;
	VECTOR rot;

	// Destroy submerged objects
	DestroyOld();

	// 
	for (chainIndex = 0; chainIndex < 3; ++chainIndex)
	{
		struct ClimbChain * chain = &Chains[chainIndex];
		if (!chain->Active)
			continue;

		// Generate new random parameters
		float scale =  RandomRange(1, 2);
		rot[0] = RandomRange(-0.3, 0.3);
		rot[1] = RandomRange(-0.3, 0.3);
		rot[2] = RandomRange(-1, 1);
		MobyDef * currentItem = MapMobyDefs[chain->NextItem];

		spawn(currentItem, chain->CurrentPosition, rot, scale);

		// Branch
		if ((gameTime - chain->LastBranch) > BranchRate)
		{
			struct ClimbChain * branchChain = GetFreeChain();
			if (branchChain)
			{
				branchChain->Active = 1;
				branchChain->LastBranch = gameTime;
				branchChain->LastTheta = MATH_PI + chain->LastTheta;
				vector_copy(branchChain->CurrentPosition, chain->CurrentPosition);

				// Determine next object
				GenerateNext(branchChain, currentItem, scale);
				dialogPlaySound(BranchDialogs[RandomRangeShort(0, sizeof(BranchDialogs)/sizeof(int)-1)], 0);
			}

			chain->LastBranch = gameTime;
		}

		// Determine next object
		GenerateNext(chain, currentItem, scale);
	}

	if(MobyCount == 0)
		dialogPlaySound(StartDialogs[RandomRangeShort(0, sizeof(BranchDialogs)/sizeof(int)-1)], 0);
	else if(MobyCount % 20 == 0)
		dialogPlaySound(IncrementalDialogs[RandomRangeShort(0, sizeof(BranchDialogs)/sizeof(int)-1)], 0);


	// 
	LastSpawn = gameTime;

	++MobyCount;
	if (SpawnRate > MAX_SPAWN_RATE)
		SpawnRate -= MobyCount * 6.5;
	else
		SpawnRate = MAX_SPAWN_RATE;

	if (WaterRaiseRate < MAX_WATER_RATE)
		WaterRaiseRate *= 1.1;
	else
		WaterRaiseRate = MAX_WATER_RATE;
}

void spawnTick(void)
{
	int gameTime = gameGetTime();
	int chainIndex = 0;
	VECTOR rot;
	float scale;

	if ((gameTime - LastSpawn) > SpawnRate)
	{
		if (State.IsHost)
		{
			sendSpawn(RandSeed, gameTime);
			onReceiveSpawn(RandSeed, gameTime);
		}
	}

	WaterHeight += WaterRaiseRate;

	// Set water
	((float*)WaterMoby->PVar)[19] = WaterHeight;

	// Set death barrier
	gameSetDeathHeight(WaterHeight);
}

int whoKilledMeHook(void)
{
	return 0;
}

void updateGameState(PatchStateContainer_t * gameState)
{
	int i,j;

	// stats
	if (gameState->UpdateCustomGameStats)
	{
    gameState->CustomGameStatsSize = sizeof(struct ClimberGameData);
		struct ClimberGameData* sGameData = (struct ClimberGameData*)gameState->CustomGameStats.Payload;
		sGameData->Version = 0x00000001;

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			sGameData->BestScore[i] = State.PlayerBestHeight[i];
			sGameData->FinalScore[i] = State.PlayerFinalHeight[i];
			//DPRINTF("%d: best:%f final:%f\n", i, sGameData->BestScore[i], sGameData->FinalScore[i]);
		}
	}
}

/*
 * NAME :		initialize
 * 
 * DESCRIPTION :
 * 			Initializes the gamemode.
 * 
 * NOTES :
 * 			This is called only once at the start.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void initialize(PatchGameConfig_t* gameConfig, PatchStateContainer_t* gameState)
{
  static int startDelay = 60 * 0.2;
	static int waitingForClientsReady = 0;

	VECTOR startPos, to;
	int i;

	// 
	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	GameData* gameData = gameGetData();
	Player ** players = playerGetAll();

	// 
	netHookMessages();

	// Disable normal game ending
	*(u32*)0x006219B8 = 0;	// survivor (8)
	*(u32*)0x00621A10 = 0;  // survivor (8)
	*(u32*)0x00620F54 = 0;	// time end (1)
	*(u32*)0x00621568 = 0;	// kills reached (2)
	*(u32*)0x006211A0 = 0;	// all enemies leave (9)

  if (startDelay) {
    --startDelay;
    return;
  }
  
  // wait for all clients to be ready
  // or for 15 seconds
  if (!gameState->AllClientsReady && waitingForClientsReady < (5 * 60)) {
    uiShowPopup(0, "Waiting For Players...");
    ++waitingForClientsReady;
    return;
  }

  // hide waiting for players popup
  hudHidePopup();

  RandSeed = gameSettings->GameLoadStartTime;

	// Set survivor
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 1;

	// Disable respawn
	gameOptions->GameFlags.MultiplayerGameFlags.RespawnTime = 0xFF;

	// get water moby
	WaterMoby = mobyFindNextByOClass(mobyListGetStart(), MOBY_ID_WATER);
	if (!WaterMoby) {
		if (mobyClassIsLoaded(MOBY_ID_WATER)) {
			WaterMoby = mobySpawn(MOBY_ID_WATER, sizeof(WaterPVars));
		} else {
			WaterMoby = mobySpawn(MOBY_ID_BETA_BOX, sizeof(WaterPVars));
		}
		WaterMoby->DrawDist = 1000;
		WaterMoby->UpdateDist = 0xFF;
		memcpy(WaterMoby->PVar, WaterPVars, sizeof(WaterPVars));
	}
	//DPRINTF("water: %08X\n", (u32)WaterMoby);

	// find random start place that isn't underneath something
	int count = 0;
	while(count < 10)
	{
		int spIdx = RandomRangeShort(0, spawnPointGetCount());
		if (!spawnPointIsPlayer(spIdx)) {
			//DPRINTF("skipping nonplayer sp %d\n", spIdx)
			continue;
		}

		SpawnPoint* sp = spawnPointGet(spIdx);

		{
			vector_copy(startPos, &sp->M0[12]);
			vector_copy(to, startPos);
			to[2] += 100;
		}
		if (CollLine_Fix(startPos, to, 2, NULL, 0) == 0) {
			//DPRINTF("selected spidx:%d\n", spIdx);
			break;
		}

		count++;
	}
	
	// 
	WaterHeight = startPos[2] - 25;
	State.StartTime = gameGetTime();
	State.StartHeight = startPos[2];
	State.EndTime = 0;
	State.GameOver = 0;

	// 
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		State.PlayerFinalHeight[i] = 0;
		State.PlayerBestHeight[i] = 0;
		State.TimePlayerDied[i] = 0;
	}

	//
	initializeScoreboard();

	// Populate moby defs
	for (i = 0; i < sizeof(MobyDefs)/sizeof(MobyDef); ++i)
	{
		if (MapMobyDefsCount >= MAX_MAP_MOBY_DEFS)
			break;

		if (mobyClassIsLoaded(MobyDefs[i].OClass)) {
			MapMobyDefs[MapMobyDefsCount++] = &MobyDefs[i];
			DPRINTF("found moby %04X\n", MobyDefs[i].OClass);
		}
	}

	// spawn radar blip moby
	RadarBlipMoby = mobySpawn(MOBY_ID_BETA_BOX, 0);
	RadarBlipMoby->Opacity = 0;
	vector_copy(RadarBlipMoby->Position, startPos);
	RadarBlipMoby->Position[2] = 0;

	// 
	memset(Chains, 0, sizeof(Chains));
	Chains[0].Active = 1;
	vector_copy(Chains[0].CurrentPosition, startPos);
	Chains[0].LastBranch = LastSpawn = gameGetTime();

	// patch who killed me to prevent damaging others
	*(u32*)0x005E07C8 = 0x0C000000 | ((u32)&whoKilledMeHook >> 2);
	*(u32*)0x005E11B0 = *(u32*)0x005E07C8;

	Initialized = 1;
}

/*
 * NAME :		gameStart
 * 
 * DESCRIPTION :
 * 			Infected game logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when in game.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void gameStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();
	Player ** players = playerGetAll();
	GameData * gameData = gameGetData();
	int i;


	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

	// Determine if host
	State.IsHost = gameAmIHost();

	if (!Initialized) {
		initialize(gameConfig, gameState);
		return;
	}

#if DEBUG
	dlPreUpdate();
	if (padGetButtonDown(0, PAD_L3 | PAD_R3) > 0) {
		DPRINTF("GameOver");
		State.GameOver = 1;
	}
	dlPostUpdate();
#endif

	// update radar
	if (RadarBlipMoby && gameGetTime() < (State.StartTime + (TIME_SECOND * 30)))
	{
		int radarIdx = radarGetBlipIndex(RadarBlipMoby);
		if (radarIdx >= 0)
		{
			RadarBlip* blip = radarGetBlips() + radarIdx;
			blip->Life = 30;
			blip->Team = 10;
			blip->Type = 0x11;
			blip->X = RadarBlipMoby->Position[0];
			blip->Y = RadarBlipMoby->Position[1];
		}
	}

	// 
	updateGameState(gameState);

	if (!State.GameOver)
	{
		// Spawn tick
		spawnTick();

		// Fix health
		int livingPlayerCount = 0;
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			Player * p = players[i];
			if (!p || p->Health <= 0)
				continue;

			// set max height
			float height = p->PlayerPosition[2] - State.StartHeight;
			State.PlayerFinalHeight[i] = height;
			if (height > State.PlayerBestHeight[i])
			{
				State.PlayerBestHeight[i] = height;
				gameData->PlayerStats.TicketScore[i] = height;
			}

			// kill or give health
			if (p->PlayerPosition[2] <= (WaterHeight - 0.5))
			{
				playerSetHealth(p, 0);
				State.TimePlayerDied[i] = gameGetTime();
			}
			else
			{
				playerSetHealth(p, PLAYER_MAX_HEALTH);
				livingPlayerCount++;
			}
		}

		if (!livingPlayerCount)
		{
			State.GameOver = 1;
		}
	}
	else
	{
		float bestHeight = 0;
		for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
			if (State.PlayerBestHeight[i] > bestHeight) {
				gameSetWinner(i, 0);
				bestHeight = State.PlayerBestHeight[i];
			}
		}
		gameEnd(4);
	}
}

void setLobbyGameOptions(void)
{
	// deathmatch options
	static char options[] = { 
		0, 0, 				// 0x06 - 0x08
		0, 0, 0, 0, 	// 0x08 - 0x0C
		1, 1, 1, 0,  	// 0x0C - 0x10
		0, 1, 0, 0,		// 0x10 - 0x14
		-1, -1, 0, 1,	// 0x14 - 0x18
	};

	// set game options
	GameOptions * gameOptions = gameGetOptions();
	GameSettings* gameSettings = gameGetSettings();
	if (!gameOptions || !gameSettings || gameSettings->GameLoadStartTime <= 0)
		return;
		
	// apply options
	memcpy((void*)&gameOptions->GameFlags.Raw[6], (void*)options, sizeof(options)/sizeof(char));
	gameOptions->GameFlags.MultiplayerGameFlags.Juggernaut = 0;
	gameOptions->GameFlags.MultiplayerGameFlags.Survivor = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.AutospawnWeapons = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.SpawnWithChargeboots = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.UnlimitedAmmo = 1;
	gameOptions->GameFlags.MultiplayerGameFlags.Teamplay = 1;
  
  gameOptions->WeaponFlags.Chargeboots = 1;
  gameOptions->WeaponFlags.DualVipers = 0;
  gameOptions->WeaponFlags.MagmaCannon = 1;
  gameOptions->WeaponFlags.Arbiter = 0;
  gameOptions->WeaponFlags.FusionRifle = 0;
  gameOptions->WeaponFlags.MineLauncher = 0;
  gameOptions->WeaponFlags.B6 = 0;
  gameOptions->WeaponFlags.Holoshield = 0;
  gameOptions->WeaponFlags.Flail = 1;
}

/*
 * NAME :		lobbyStart
 * 
 * DESCRIPTION :
 * 			Infected lobby logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when in lobby.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void lobbyStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	int activeId = uiGetActive();
	static int initializedScoreboard = 0;

	// set time ended
	if (Initialized && State.StartTime && !State.EndTime)
		State.EndTime = gameGetTime();

	// 
	updateGameState(gameState);

	// scoreboard
	switch (activeId)
	{
		case 0x15C:
		{
			if (initializedScoreboard)
				break;

			setEndGameScoreboard();
			initializedScoreboard = 1;

			// patch rank computation to keep rank unchanged for base mode
			POKE_U32(0x0077ACE4, 0x4600BB06);
			break;
		}
		case UI_ID_GAME_LOBBY:
		{
			setLobbyGameOptions();
			break;
		}
	}
}

/*
 * NAME :		loadStart
 * 
 * DESCRIPTION :
 * 			Load logic entrypoint.
 * 
 * NOTES :
 * 			This is called only when the game has finished reading the level from the disc and before it has started processing the data.
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void loadStart(void)
{
  setLobbyGameOptions();
}
