/***************************************************
 * FILENAME :		gamerules.c
 * 
 * DESCRIPTION :
 * 		Gamerules entrypoint and logic.
 * 
 * NOTES :
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/time.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/map.h>
#include <libdl/player.h>
#include <libdl/cheats.h>
#include <libdl/stdio.h>
#include <libdl/pad.h>
#include <libdl/dl.h>
#include <libdl/sha1.h>
#include <libdl/spawnpoint.h>
#include <libdl/collision.h>
#include <libdl/graphics.h>
#include <libdl/random.h>
#include <libdl/camera.h>
#include <libdl/gameplay.h>
#include <libdl/math3d.h>
#include <libdl/utils.h>
#include <libdl/net.h>
#include "module.h"
#include "messageid.h"
#include "halftime.h"
#include "include/config.h"


#define ROTATING_WEAPONS_SWITCH_FREQUENCY_MIN					(TIME_SECOND * 15)
#define ROTATING_WEAPONS_SWITCH_FREQUENCY_MAX					(TIME_SECOND * 45)
#define HEADBUTT_COOLDOWN_TICKS												(4)

typedef struct RotatingWeaponsChangedMessage
{
	int gameTime;
	int weaponId;
} RotatingWeaponsChangedMessage_t;

enum BETTER_HILL_PTS
{
	TORVAL_13 = 0,
	MARAXUS_13 = 1,
	SARATHOS_11 = 2,
	SARATHOS_14 = 3,
	CATACROM_0D = 4,
	CATACROM_11 = 5,
	DC_11 = 6,
	DC_14 = 7,
	DC_16 = 8,
	DC_17 = 9,
	SHAAR_14 = 10,
	SHAAR_17 = 11,
	VALIX_01 = 24,
	VALIX_13 = 12,
	VALIX_16 = 13,
	MF_20 = 14,
	MF_22 = 15,
	MF_24 = 16,
	GS_20 = 17,
	GS_21 = 18,
	GS_22 = 19,
	GS_23 = 20,
	TEMPUS_16 = 21,
	TEMPUS_17 = 22,
	TEMPUS_18 = 23
};

// config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;

// lobby clients patch config
extern PatchConfig_t lobbyPlayerConfigs[GAME_MAX_PLAYERS];

/*
 *
 */
int GameRulesInitialized = 0;

/*
 *
 */
int FirstPass = 1;

/*
 *
 */
int BetterHillsInitialized = 0;

/*
 *
 */
int HasDisabledHealthboxes = 0;

/*
 *
 */
u32 WeatherOverrideId = 0;

/*
 * 
 */
short PlayerKills[GAME_MAX_PLAYERS];

/*
 *
 */
float VampireHealRate[] = {
	PLAYER_MAX_HEALTH * 0.25,
	PLAYER_MAX_HEALTH * 0.50,
	PLAYER_MAX_HEALTH * 1.00
};

/*
 *
 */
int RotatingWeaponsActiveId = WEAPON_ID_WRENCH;
int RotatingWeaponsNextRotationTime = 0;

/*
 *
 */
char HeadbuttHitTimers[GAME_MAX_PLAYERS];

/*
 * Custom hill spawn points
 */
SpawnPoint BetterHillPoints[] = {
	// TORVAL ID 0x13
	{ { 6, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 233.28482, 278.00937, 106.855156, 0 }, { 0.16666667, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 233.28482, 278.00937, 106.855156, 0 } },

	// MARAXUS ID 0x13
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 525.83344, 742.3953, 103.92746, 1 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 525.83344, 742.3953, 103.92746, 1 } },

	// SARATHOS ID 0x11
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 12, 0, 493.81552, 218.55286, 105.77197, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 493.81552, 218.55286, 105.77197, 1 } },

	// SARATHOS ID 14 (0x0E)
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 337.56897, 193.39743, 106.15551, 1 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 337.56897, 193.39743, 106.15551, 1 } },

	// CATACROM ID 0x0D
	{ { 7, 0, 0, 0, 0, 7, 0, 0, 0, 0, 7, 0, 333.3385, 255.27766, 63.045467, 0 }, { 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 333.3385, 255.27766, 63.045467, 0 } },
	
	// CATACROM ID 0x11
	{ { 7, 0, 0, 0, 0, 7, 0, 0, 0, 0, 7, 0, 324.36548, 366.6925, 65.79945, 0 }, { 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 324.36548, 366.6925, 65.79945, 0 } },

	// DC ID 12 (0x0C)
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 10, 0, 503.79434, 415.31488, 641.28125, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.1, 0, 503.79434, 415.31488, 641.28125, 1 } },

	// DC ID 14
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 12, 0, 503.789, 494.31857, 641.2969, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 503.789, 494.31857, 641.2969, 1 } },

	// DC ID 16
	{ { 16, 0, 0, 0, 0, 16, 0, 0, 0, 0, 16, 0, 349.3588, 529.6109, 641.25, 1 }, { 0.0625, 0, 0, 0, 0, 0.0625, 0, 0, 0, 0, 0.0625, 0, 349.3588, 529.6109, 641.25, 1 } },

	// DC ID 17 (0x11)
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 10, 0, 455.52982, 493.50287, 641.03125, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.1, 0, 455.52982, 493.50287, 641.03125, 1 } },

	// SHAAR ID 0x20
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 12, 0, 621.2297, 562.5671, 509.29688, 0 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 621.2297, 562.5671, 509.29688, 0 } },

	// SHAAR ID 0x23
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 487.00687, 682.48615, 509.3125, 0 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 487.00687, 682.48615, 509.3125, 0 } },

	// VALIX ID 13 (0x0D)
	{ { 8.660254, 5, 0, 0, -5, 8.660254, 0, 0, 0, 0, 10, 0, 292.09567, 472.517, 331.46976, 0 }, { 0.08660254, 0.05, 0, 0, -0.05, 0.08660254, 0, 0, 0, 0, 0.1, 0, 292.09567, 472.517, 331.46976, 0 } },

	// VALIX ID 16 (0x10)
	{ { 8.660254, 5, 0, 0, -5, 8.660254, 0, 0, 0, 0, 10, 0, 351.16696, 400.823, 324.90625, 0 }, { 0.08660254, 0.05, 0, 0, -0.05, 0.08660254, 0, 0, 0, 0, 0.1, 0, 351.16696, 400.823, 324.90625, 0 } },

	// MF ID 20 (0x14)
	{ { 8, 0, 0, 0, 0, 14, 0, 0, 0, 0, 5, 0, 444.0099, 600.22675, 428.04242, 0 }, { 0.125, 0, 0, 0, 0, 0.071428575, 0, 0, 0, 0, 0.2, 0, 444.0099, 600.22675, 428.04242, 0 } },
	
	// MF ID 22 (0x16)
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 410.27823, 598.204, 434.209, 0 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 410.27823, 598.204, 434.209, 0 } },

	// MF ID 22 (0x18)
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 356.82983, 653.3447, 428.36734, 0 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 356.82983, 653.3447, 428.36734, 0 } },

	// GC ID 0x20
	{ { 7, 0, 0, 0, 0, 7, 0, 0, 0, 0, 7, 0, 636.95593, 401.78497, 102.765625, 0 }, { 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 0, 0, 0, 0.14285715, 0, 636.95593, 401.78497, 102.765625, 0 } },

	// GC ID 0x21
	{ { 12, 0, 0, 0, 0, 12, 0, 0, 0, 0, 12, 0, 728.6185, 563.659, 101.578125, 1 }, { 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 0, 0, 0, 0.083333336, 0, 728.6185, 563.659, 101.578125, 1 } },

	// GC ID 22
	{ { 6.7093644, 4.3571124, 0, 0, -4.9017515, 7.548035, 0, 0, 0, 0, 8, 0, 586.91223, 541.42474, 102.828125, 0 }, { 0.10483382, 0.06807988, 0, 0, -0.060515452, 0.09318562, 0, 0, 0, 0, 0.125, 0, 586.91223, 541.42474, 102.828125, 0 } },

	// GC ID 23 (0x17)
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 637.3847, 522.3367, 103, 1 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 637.3847, 522.3367, 103, 1 } },

	// TEMPUS ID 16
	{ { 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 530.847, 538.41187, 100.375, 0 }, { 0.1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.1, 0, 530.847, 538.41187, 100.375, 0 } },

	// TEMPUS ID 17
	{ { 4.9999995, 8.6602545, 0, 0, -6.9282036, 3.9999998, 0, 0, 0, 0, 10, 0, 491.03555, 425.8757, 100.828125, 0 }, { 0.049999997, 0.086602546, 0, 0, -0.10825318, 0.062499996, 0, 0, 0, 0, 0.1, 0, 491.03555, 425.8757, 100.828125, 0 } },

	// TEMPUS ID 18
	{ { 27.499998, 47.6314, 0, 0, -47.6314, 27.499998, 0, 0, 0, 0, 10, 0, 452, 443, 100.828125, 1 }, { 0.009090908, 0.015745917, 0, 0, -0.015745917, 0.009090908, 0, 0, 0, 0, 0.1, 0, 452, 443, 100.828125, 1 } },

	// VALIX CTF SPAWN POINT 01
	{ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 333.81165, 413.57132, 330, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 333.81165, 413.57132, 330, 0 } },
};

/*
 * NAME :		onRotatingWeaponsChangedRemote
 * 
 * DESCRIPTION :
 * 			Raised when client receives RotatingWeaponsChanged message from host.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onRotatingWeaponsChangedRemote(void * connection, void * data)
{
	RotatingWeaponsChangedMessage_t message;
	memcpy(&message, data, sizeof(RotatingWeaponsChangedMessage_t));

	RotatingWeaponsNextRotationTime = message.gameTime;
	RotatingWeaponsActiveId = message.weaponId;
}

/*
 * NAME :		rotatingWeaponsLogic
 * 
 * DESCRIPTION :
 * 			Forces player's weapon to the randomly chosen weapon.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void rotatingWeaponsLogic(void)
{
	int i;
	int gameTime = gameGetTime();
	Player** players = playerGetAll();

	// don't run when playing gun game.
	if (gameConfig.customModeId == CUSTOM_MODE_GUN_GAME)
		return;

	// hook
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_PATCH_IN_GAME_START, &onRotatingWeaponsChangedRemote);

	// determine new weapon from enabled weapons randomly
	if (gameAmIHost() && (RotatingWeaponsNextRotationTime == 0 || gameTime > RotatingWeaponsNextRotationTime))
	{
		GameOptions* gameOptions = gameGetOptions();
		int r = randRangeInt(0, 8);
		int slotId = weaponIdToSlot(RotatingWeaponsActiveId);
		int iterations = 0;
		int matches = 0;
		
		// randomly select weapon from enabled weapons
		while (r) {
			slotId = (slotId + 1) % 7;
			if (gameOptions->WeaponFlags.Raw & (1 << weaponSlotToId(1 + slotId))) {
				--r;
				++matches;
			}
			++iterations;

			if (iterations > 7 && matches == 0)
				return;
		}

		// set weapon
		RotatingWeaponsActiveId = weaponSlotToId(1 + slotId);

		// determine next rotation time
		RotatingWeaponsNextRotationTime = gameTime + randRangeInt(ROTATING_WEAPONS_SWITCH_FREQUENCY_MIN, ROTATING_WEAPONS_SWITCH_FREQUENCY_MAX);

		// send to others
		RotatingWeaponsChangedMessage_t message = {
			.gameTime = RotatingWeaponsNextRotationTime,
			.weaponId = RotatingWeaponsActiveId
		};
		netBroadcastCustomAppMessage(0x40, netGetDmeServerConnection(), CUSTOM_MSG_ID_PATCH_IN_GAME_START, sizeof(message), &message);
	}

	// if active weapon is wrench then just exit
	if (RotatingWeaponsActiveId == WEAPON_ID_WRENCH)
		return;

	// force weapons
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * player = players[i];
		if (player)
		{
			// don't change weapons of the infected
			if (gameConfig.customModeId == CUSTOM_MODE_INFECTED)
				if (player->Team == TEAM_GREEN)
					continue;

			// give weapon
			GadgetBox* gBox = player->GadgetBox;
			if (gBox->Gadgets[RotatingWeaponsActiveId].Level < 0) {
				playerGiveWeapon(player, RotatingWeaponsActiveId, 0);
				gBox->Gadgets[RotatingWeaponsActiveId].Level = 0;
			}

			// set slot and weapon
			if (player->IsLocal) {

				// hide slots 2 and 3
				playerSetLocalEquipslot(player->LocalPlayerIndex, 1, WEAPON_ID_EMPTY);
				playerSetLocalEquipslot(player->LocalPlayerIndex, 2, WEAPON_ID_EMPTY);

				// force equip if not holding correct weapon
				if (player->WeaponHeldId != WEAPON_ID_WRENCH &&
						player->WeaponHeldId != WEAPON_ID_SWINGSHOT &&
						player->WeaponHeldId != MOBY_ID_HACKER_RAY &&
						player->WeaponHeldId != RotatingWeaponsActiveId)
				{
					playerEquipWeapon(player, RotatingWeaponsActiveId);
				}
			}
		}
	}
}

/*
 * NAME :		playerSizeScaleMoby
 * 
 * DESCRIPTION :
 * 			Scales the given moby by the given scale.
 * 			Uses the moby scale table to pull the default moby scale
 * 			and then multiplies that by the given multiplier so that
 * 			it can be called multiple times without compounding the scale multiplier.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void playerSizeScaleMoby(Moby* moby, float scale)
{
	float* mobyDef = *(float**)(0x002495c0 + (4 * moby->MClass));
	moby->Scale = scale * mobyDef[9];
}

/*
 * NAME :		playerSizeLogic
 * 
 * DESCRIPTION :
 * 			Sets player size depending on setting.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void playerSizeLogic(void)
{
	int i, j;
	Player** players = playerGetAll();
	float size, cameraHeight, tpHeight, moveSpeed = 1;
	
	switch (gameConfig.prPlayerSize)
	{
		case 1: size = 1.5; cameraHeight = 0.75; tpHeight = 3; moveSpeed = 1.5; break; // large
		case 2: size = 3; cameraHeight = 2.5; tpHeight = 5; moveSpeed = 3; break; // giant
		case 3: size = 0.2; cameraHeight = -0.80; tpHeight = 0.3; moveSpeed = 0.5; break; // tiny
		case 4: size = 0.666; cameraHeight = -0.25; tpHeight = 1.25; moveSpeed = 0.75; break; // small
	}

	// disable fixed player scale
	//*(u32*)0x005D1580 = 0;

	//
	float m = 1024 * size;
	asm (".set noreorder;");
	*(u16*)0x004f79fc = *(u16*)((u32)&m + 2);

	// chargeboot distance
	m = 6 * size;
	asm (".set noreorder;");
	*(u16*)0x0049A688 = *(u16*)((u32)&m + 2);

	// chargeboot height
	m = tpHeight;
	asm (".set noreorder;");
	*(u16*)0x0049A6B4 = *(u16*)((u32)&m + 2);

	// chargeboot look at height
	m = tpHeight;
	asm (".set noreorder;");
	*(u16*)0x0049a658 = *(u16*)((u32)&m + 2);

	// third person distance and height
	*(float*)0x002391FC = 4 * size;
	*(float*)0x00239200 = tpHeight;
	*(float*)0x00239180 = tpHeight + 0.5;

	// change velocity dampening function's radius
	// to original so new radius doesn't affect movement physics
	//*(u16*)0x005E72C4 = 0x238;
	//*(u16*)0x005E72C8 = 0x23C;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * player = players[i];
		if (player) {

			// write original radius and radius squared
			//*(float*)((u32)player + 0x238) = 0.45;
			//*(float*)((u32)player + 0x23C) = 0.45 * 0.45;

			// speed
			//player->Speed = size;

			// update collision size
			//player->PlayerConstants->colRadius = 0.45 * size;
			//player->PlayerConstants->colTop = 1.05 * size;
			//player->PlayerConstants->colBot = 0.70 * size;
			//player->PlayerConstants->colBotFall = 0.50 * size;

			if (player->PlayerMoby)
				player->PlayerMoby->Scale = 0.25 * size;

			// update camera
			player->CameraOffset[0] = -6 * size;
			player->CameraOffset[2] = cameraHeight;
			player->CameraElevation = 2 + cameraHeight;
		}
	}

	Moby* moby = mobyListGetStart();
	Moby* mEnd = mobyListGetEnd();
	while (moby < mEnd)
	{
		if (!mobyIsDestroyed(moby)) {
			switch (moby->OClass)
			{
				case MOBY_ID_WRENCH:
				case MOBY_ID_DUAL_VIPERS:
				case MOBY_ID_ARBITER:
				case MOBY_ID_THE_ARBITER:
				case MOBY_ID_MAGMA_CANNON:
				case MOBY_ID_FLAIL:
				case MOBY_ID_FLAIL_HEAD:
				case MOBY_ID_B6_OBLITERATOR:
				case MOBY_ID_HOLOSHIELD_LAUNCHER:
				case MOBY_ID_HOLOSHIELD_SHOT:
				case MOBY_ID_MINE_LAUNCHER:
				case MOBY_ID_MINE_LAUNCHER_MINE:
				case MOBY_ID_FUSION_RIFLE:
				case MOBY_ID_CHARGE_BOOTS_PLAYER_EQUIP_VERSION:
				case MOBY_ID_HOVERBIKE:
				case MOBY_ID_HOVERSHIP:
				case MOBY_ID_LANDSTALKER:
				case MOBY_ID_LANDSTALKER_LEG:
				case MOBY_ID_LANDSTALKER_MID:
				case MOBY_ID_PUMA:
				case MOBY_ID_PUMA_TIRE:
				case MOBY_ID_PLAYER_TURRET:
				case MOBY_ID_NODE_BASE:
				case MOBY_ID_CONQUEST_NODE_TURRET:
				case MOBY_ID_CONQUEST_POWER_TURRET:
				case MOBY_ID_CONQUEST_ROCKET_TURRET:
				case MOBY_ID_CONQUEST_TURRET_HOLDER_TRIANGLE_THING:
				case MOBY_ID_HACKER_ORB:
				case MOBY_ID_HACKER_ORB_HOLDER:
				case MOBY_ID_HEALTH_ORB_MULT:
				case MOBY_ID_HEALTH_BOX_MULT:
				case MOBY_ID_HEALTH_PAD0:
				case MOBY_ID_HEALTH_PAD1:
				case MOBY_ID_BLUE_TEAM_HEALTH_PAD:
				case MOBY_ID_PICKUP_PAD:
				case MOBY_ID_WEAPON_PICKUP:
				case MOBY_ID_CHARGEBOOTS_PICKUP_MODEL:
				case MOBY_ID_WEAPON_PACK:
				case MOBY_ID_RED_FLAG:
				case MOBY_ID_BLUE_FLAG:
				case MOBY_ID_GREEN_FLAG:
				case MOBY_ID_ORANGE_FLAG:
					playerSizeScaleMoby(moby, size);
					break;
			}
		}

		++moby;
	}
}

/*
 * NAME :		headbuttDamage
 * 
 * DESCRIPTION :
 * 			Intercepts headbutt damage to handle friendly fire and running over players.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void headbuttDamage(float hitpoints, Moby* hitMoby, Moby* sourceMoby, int damageFlags, VECTOR fromPos, VECTOR t0)
{
	Player * sourcePlayer = guberMobyGetPlayerDamager(sourceMoby);
	Player * hitPlayer = guberMobyGetPlayerDamager(hitMoby);
	if (sourcePlayer && hitPlayer) {

		// prevent friendly fire
		if (!gameConfig.prHeadbuttFriendlyFire && playerGetJuggSafeTeam(sourcePlayer) == playerGetJuggSafeTeam(hitPlayer))
			return;

		// give a cooldown to the headbutt
		if (HeadbuttHitTimers[hitPlayer->PlayerId] > 0)
			return;

		// if target is low health, run them over
		if (hitPlayer->Health <= hitpoints)
			damageFlags = 0x808;
		else
			damageFlags = 0x801;

		HeadbuttHitTimers[hitPlayer->PlayerId] = HEADBUTT_COOLDOWN_TICKS;
		DPRINTF("damaging %d (health:%f) with %f and %X\n", hitPlayer->PlayerId, hitPlayer->Health, hitpoints, damageFlags);
	}

	((void (*)(float, Moby*, Moby*, int, VECTOR, VECTOR))0x00503500)(hitpoints, hitMoby, sourceMoby, damageFlags, fromPos, t0);
}

/*
 * NAME :		headbuttLogic
 * 
 * DESCRIPTION :
 * 			Enables damaging players by chargebooting into them.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void headbuttLogic(void)
{
	int i;

	// enable for all mobies
	*(u32*)0x005F98D0 = 0x24020001;

	// hook damage so we can consider teams/health
	*(u32*)0x005f9920 = 0x0C000000 | ((u32)&headbuttDamage >> 2);

	// set damage
	u16 * damageInstr = (u16*)0x005F9918;
	switch (gameConfig.prHeadbutt)
	{
		case 1: *damageInstr = 0x4148; break; // 25%
		case 2: *damageInstr = 0x41c8; break; // 50%
		case 3: *damageInstr = 0x4248; break; // 100%
	}

	// decrement counters
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		if (HeadbuttHitTimers[i] > 0)
			HeadbuttHitTimers[i]--;
}

/*
 * NAME :		alwaysV2sLogic
 * 
 * DESCRIPTION :
 * 			Enables spawning with a v2.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void alwaysV2sLogic(void)
{
	int i, j = 0;
	Player** players = NULL;
	Player* player = NULL;

	// force v2 on give weapon
	*(u32*)0x005F0504 = 0x24100001;

	// set all player weapons to v2
	if (FirstPass)
	{
		players = playerGetAll();
		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			player = players[i];
			if (player && player->GadgetBox)
			{
				for (j = 0; j < 8; ++j)
				{
					struct GadgetEntry *gadget = &player->GadgetBox->Gadgets[weaponSlotToId(j + 1)];
					if (gadget->Level == 0)
						gadget->Level = 1;
				}
			}
		}
	}
}

/*
 * NAME :		unlimitedChargebootLogic
 * 
 * DESCRIPTION :
 * 			Keeps the player chargebooting while holding L2.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void unlimitedChargebootLogic(void)
{
	int i;
	Player** players = playerGetAll();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * p = players[i];
		if (!p)
			continue;
		
		// if player is cbooting, holding l2, and about to end cboot state
		// then force state to not reach end (keep chargebooting)
		if (p->PlayerState == PLAYER_STATE_CHARGE && playerPadGetButton(p, PAD_L2) > 0 && p->timers.state > 55)
			p->timers.state = 55;
	}
}


/*
 * NAME :		fusionShotsAlwaysHitLogic_Hook
 * 
 * DESCRIPTION :
 * 			Checks if the given shot has already been determined to have hit a moby.
 * 			If true, it sets the hitFlag to ignore non-player colliders.
 * 
 * 			Unsure if shots can still be blocked in another player is in the way on the remote client's screen.
 * 			Probably a rare edge case.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int fusionShotsAlwaysHitLogic_Hook(VECTOR from, VECTOR to, Moby* shotMoby, Moby* moby, u64 t0)
{
	u64 hitFlag = 0;

	if (shotMoby && shotMoby->PVar)
	{
		Moby * hitMoby = *(Moby**)((u32)shotMoby->PVar + 0x68);
		if (hitMoby) {
			Player * hitPlayer = guberGetObjectByMoby(hitMoby);
			if (hitPlayer)
				hitFlag = 1;
		}
	}

	return CollLine_Fix(from, to, hitFlag, moby, t0);
}

/*
 * NAME :		fusionShotsAlwaysHitLogic
 * 
 * DESCRIPTION :
 * 			Patches fusion shot collision raycast to other colliders when it has already been determined that the shot has collided with a guber (networked moby).
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void fusionShotsAlwaysHitLogic(void)
{
	HOOK_JAL(0x003FC66C, &fusionShotsAlwaysHitLogic_Hook);
	POKE_U32(0x003FC670, 0x0240302D);
}

/*
 * NAME :		onGameplayLoadRemoveWeaponPickups
 * 
 * DESCRIPTION :
 * 			Trigger right before processing the gameplay file.
 * 			Removes weapon pickups from gameplay file and map.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onGameplayLoadRemoveWeaponPickups(GameplayHeaderDef_t * gameplay)
{
	int i;
	GameplayMobyHeaderDef_t * mobyInstancesHeader = (GameplayMobyHeaderDef_t*)((u32)gameplay + gameplay->MobyInstancesOffset);

	if (!gameConfig.grNoPickups)
		return;
		
	// iterate each moby, moving all pickups to below the map
	for (i = 0; i < mobyInstancesHeader->StaticCount; ++i) {
		GameplayMobyDef_t* mobyDef = &mobyInstancesHeader->MobyInstances[i];
		if (mobyDef->OClass == MOBY_ID_WEAPON_PICKUP) {
			mobyDef->PosZ = -100;
		}
	}
}

/*
 * NAME :		onGameplayLoadRemoveWeaponPickups
 * 
 * DESCRIPTION :
 * 			Trigger right before processing the gameplay file.
 * 			Removes weapon pickups from gameplay file and map.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void onGameplayLoad(void * gameplayMobies, void * a1, void * a2)
{
	GameplayHeaderDef_t * gameplay;

	// pointer to gameplay data is stored in $s1
	asm volatile (
		"move %0, $s1"
		: : "r" (gameplay)
	);

	// call base
	((void (*)(void*, void*, void*))0x004ecea0)(gameplayMobies, a1, a2);

	// 
	onGameplayLoadRemoveWeaponPickups(gameplay);
}

/*
 * NAME :		vampireLogic
 * 
 * DESCRIPTION :
 * 			Checks if a player has gotten a kill and heals them if so.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void vampireLogic(float healRate)
{
	int i;
	Player ** playerObjects = playerGetAll();
	Player * player;
	GameData * gameData = gameGetData();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		// Check if player has killed someone
		if (gameData->PlayerStats.Kills[i] > PlayerKills[i])
		{
			// Try to heal if player exists
			player = playerObjects[i];
			if (player)
				playerSetHealth(player, clamp(player->Health + healRate, 0, PLAYER_MAX_HEALTH));
			
			// Update our cached kills count
			PlayerKills[i] = gameData->PlayerStats.Kills[i];
		}
	}
}

/*
 * NAME :		betterHillsLogic
 * 
 * DESCRIPTION :
 * 			
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void betterHillsLogic(void)
{
	if (BetterHillsInitialized)
		return;

	//
	BetterHillsInitialized = 1;

	// 
	switch (gameGetSettings()->GameLevel)
	{
		case MAP_ID_CATACROM:
		{
			spawnPointSet(&BetterHillPoints[CATACROM_0D], 0x0D);
			spawnPointSet(&BetterHillPoints[CATACROM_11], 0x11);
			break;
		}
		case MAP_ID_SARATHOS:
		{
			spawnPointSet(&BetterHillPoints[SARATHOS_11], 0x11);
			spawnPointSet(&BetterHillPoints[SARATHOS_14], 0x0E);
			break;
		}
		case MAP_ID_DC:
		{
			spawnPointSet(&BetterHillPoints[DC_11], 0x0C);
			spawnPointSet(&BetterHillPoints[DC_16], 0x10);
			spawnPointSet(&BetterHillPoints[DC_14], 0x0E);
			spawnPointSet(&BetterHillPoints[DC_17], 0x11);
			break;
		}
		case MAP_ID_SHAAR: 
		{
			spawnPointSet(&BetterHillPoints[SHAAR_14], 0x14);
			spawnPointSet(&BetterHillPoints[SHAAR_17], 0x17);
			break;
		}
		case MAP_ID_VALIX: 
		{
			spawnPointSet(&BetterHillPoints[VALIX_01], 0x01);
			spawnPointSet(&BetterHillPoints[VALIX_13], 0x0D);
			spawnPointSet(&BetterHillPoints[VALIX_16], 0x10);
			break;
		}
		case MAP_ID_MF:
		{
			spawnPointSet(&BetterHillPoints[MF_20], 0x14);
			spawnPointSet(&BetterHillPoints[MF_22], 0x16);
			spawnPointSet(&BetterHillPoints[MF_24], 0x18);
			break;
		}
		case MAP_ID_TEMPUS:
		{
			spawnPointSet(&BetterHillPoints[TEMPUS_16], 0x10);
			spawnPointSet(&BetterHillPoints[TEMPUS_17], 0x11);
			spawnPointSet(&BetterHillPoints[TEMPUS_18], 0x12);
			break;
		}
		case MAP_ID_MARAXUS: 
		{
			spawnPointSet(&BetterHillPoints[MARAXUS_13], 0x13);
			break;
		}
		case MAP_ID_GS:
		{
			spawnPointSet(&BetterHillPoints[GS_20], 0x14);
			spawnPointSet(&BetterHillPoints[GS_21], 0x15);
			spawnPointSet(&BetterHillPoints[GS_22], 0x16);
			spawnPointSet(&BetterHillPoints[GS_23], 0x17);
			break;
		}
		case MAP_ID_TORVAL:
		{
			spawnPointSet(&BetterHillPoints[TORVAL_13], 0x13);
			break;
		}
	}
}

/*
 * NAME :		healthbarsLogic
 * 
 * DESCRIPTION :
 * 			
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void healthbarsHook(int nameX, int nameY, u32 nameColor, char * nameStr, int nameStrLen)
{
	// get player index
	int i;
	asm volatile (
		"move %0, $s7"
		: : "r" (i)
	);

	// call pass through function
	gfxScreenSpaceText(nameX, nameY, 1, 1, nameColor, nameStr, nameStrLen, 1);
	
	// ensure player index is valid
	if (i < 0 || i >= GAME_MAX_PLAYERS)
		return;

	// get player and validate state
	Player ** players = playerGetAll();
	Player * player = players[i];
	if (!player || player->Health <= 0)
		return;

	// Draw boxes
	float x = (float)nameX / SCREEN_WIDTH;
	float y = (float)nameY / SCREEN_HEIGHT;
	float health = player->Health / 50;
	float w = (0.05 * 1) + 0.02, h = 0.01, p = 0.002;
	float right = w * health;
	x -= w / 2;
	gfxScreenSpaceBox(x,y,w-p,h-p, 0x80000000);
	gfxScreenSpaceBox(x,y,right,h, nameColor);
}

/*
 * NAME :		invTimerLogic
 * 
 * DESCRIPTION :
 * 			Sets each player's invicibility timer to 0.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void invTimerLogic(void)
{
	int i;
	Player ** players = playerGetAll();

	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * p = players[i];
		if (p)
			p->timers.postHitInvinc = 0;
	}
}

#if TWEAKERS
float tweakerGetPos(char value)
{
	// value between 0.75 and 1.25
	return 0 + ((float)value / 10.0) * 1024 * 2;
}

float tweakerGetScale(char value)
{
	// value between 0.75 and 1.25
	return 1 + ((float)value / 10.0) * 0.9;
}

void tweakers(void)
{
	int i;
	Player** players = playerGetAll();
	GameSettings* gs = gameGetSettings();

	// locally disabled
	if (config.characterTweakers[CHARACTER_TWEAKER_TOGGLE])
		return;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * player = players[i];

		if (player) {
			int clientId = gs->PlayerClients[i];
			if (clientId >= 0) {

				// use local config if local player
				PatchConfig_t * patchConfig = &lobbyPlayerConfigs[clientId];
				if (player->IsLocal)
					patchConfig = &config;

				// if remote disabled always return normal scale
				if (patchConfig->characterTweakers[CHARACTER_TWEAKER_TOGGLE])
					continue;

				// set scale
				float lowerTorsoScale = tweakerGetScale(patchConfig->characterTweakers[CHARACTER_TWEAKER_LOWER_TORSO_SCALE]);
				float upperTorsoScale = tweakerGetScale(patchConfig->characterTweakers[CHARACTER_TWEAKER_UPPER_TORSO_SCALE]);
				player->Tweakers[3].scale *= tweakerGetScale(patchConfig->characterTweakers[CHARACTER_TWEAKER_HEAD_SCALE]) / upperTorsoScale; // head
				player->Tweakers[8].scale *= tweakerGetScale(patchConfig->characterTweakers[CHARACTER_TWEAKER_LEFT_ARM_SCALE]) / upperTorsoScale; // left arm
				player->Tweakers[9].scale *= tweakerGetScale(patchConfig->characterTweakers[CHARACTER_TWEAKER_RIGHT_ARM_SCALE]) / upperTorsoScale; // right arm
				player->Tweakers[6].scale *= tweakerGetScale(patchConfig->characterTweakers[CHARACTER_TWEAKER_LEFT_LEG_SCALE]); // left leg
				player->Tweakers[7].scale *= tweakerGetScale(patchConfig->characterTweakers[CHARACTER_TWEAKER_RIGHT_LEG_SCALE]); // right leg
				player->Tweakers[1].scale *= lowerTorsoScale; // hips
				player->Tweakers[15].scale *= upperTorsoScale / lowerTorsoScale; // torso

				// set pos
				player->Tweakers[3].trans[2] = tweakerGetPos(patchConfig->characterTweakers[CHARACTER_TWEAKER_HEAD_POS]); // head
				player->Tweakers[8].trans[2] = tweakerGetPos(patchConfig->characterTweakers[CHARACTER_TWEAKER_LEFT_ARM_POS]); // left arm
				player->Tweakers[9].trans[2] = tweakerGetPos(patchConfig->characterTweakers[CHARACTER_TWEAKER_RIGHT_ARM_POS]) ; // right arm
				player->Tweakers[6].trans[2] = tweakerGetPos(patchConfig->characterTweakers[CHARACTER_TWEAKER_LEFT_LEG_POS]); // left leg
				player->Tweakers[7].trans[2] = tweakerGetPos(patchConfig->characterTweakers[CHARACTER_TWEAKER_RIGHT_POS]); // right leg
				player->Tweakers[1].trans[2] = tweakerGetPos(patchConfig->characterTweakers[CHARACTER_TWEAKER_LOWER_TORSO_POS]); // lower torso
				player->Tweakers[15].trans[2] = tweakerGetPos(patchConfig->characterTweakers[CHARACTER_TWEAKER_UPPER_TORSO_POS]); // upper torso
			}
		}
	}
}
#endif

/*
 * NAME :		grInitialize
 * 
 * DESCRIPTION :
 * 			Initializes this module.
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
void grInitialize(void)
{
	int i;

	// Initialize player kills to 0
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		PlayerKills[i] = 0;

	// reset
	htReset();
	BetterHillsInitialized = 0;
	HasDisabledHealthboxes = 0;
	RotatingWeaponsNextRotationTime = 0;
	RotatingWeaponsActiveId = WEAPON_ID_WRENCH;
	memset(HeadbuttHitTimers, 0, sizeof(HeadbuttHitTimers));

	GameRulesInitialized = 1;
}

/*
 * NAME :		grGameStart
 * 
 * DESCRIPTION :
 * 			Gamerules game logic entrypoint.
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
void grGameStart(void)
{
	int i = 0;
	GameSettings * gameSettings = gameGetSettings();

	// Initialize
	if (GameRulesInitialized != 1)
	{
		// 
		grInitialize();

		// convert weather id to game value
		WeatherOverrideId = gameConfig.prWeatherId;

		// random weather
		if (WeatherOverrideId == 1)
		{
			sha1(&gameSettings->GameLoadStartTime, 4, &WeatherOverrideId, 4);
			WeatherOverrideId = 1 + (WeatherOverrideId % 16);
			if (WeatherOverrideId == 8)
				WeatherOverrideId = 9;
			else if (WeatherOverrideId == 15)
				WeatherOverrideId = 16;
		}
		// shift down weather (so that the list value is equivalent to game value)
		else if (WeatherOverrideId > 1 && WeatherOverrideId < 9)
			--WeatherOverrideId;
	}

	// Apply weather
	cheatsApplyWeather(WeatherOverrideId);

	if (gameConfig.grNoPacks)
		cheatsApplyNoPacks();

	if (gameConfig.grV2s == 2)
		cheatsApplyNoV2s();
	else if (gameConfig.grV2s == 1)
		alwaysV2sLogic();

	if (gameConfig.prWeatherId)
		cheatsApplyMirrorWorld(1);

	if (gameConfig.grNoHealthBoxes && !HasDisabledHealthboxes)
		HasDisabledHealthboxes = cheatsDisableHealthboxes();

	if (gameConfig.grVampire)
		vampireLogic(VampireHealRate[gameConfig.grVampire-1]);

	if (gameConfig.grHalfTime)
		halftimeLogic();

	if (gameConfig.grBetterHills && gameConfig.customMapId == 0)
		betterHillsLogic();

	if (gameConfig.grHealthBars && isInGame())
	{
		u32 hookValue = 0x0C000000 | ((u32)&healthbarsHook >> 2);
		*(u32*)0x005d608c = hookValue;
		*(u32*)0x005d60c4 = hookValue;
	}

	if (gameConfig.grNoNames)
		gameSettings->PlayerNamesOn = 0;

	if (gameConfig.grNoInvTimer)
		invTimerLogic();

	if (gameConfig.prPlayerSize)
		playerSizeLogic();

	if (gameConfig.prRotatingWeapons)
		rotatingWeaponsLogic();

	if (gameConfig.prHeadbutt)
		headbuttLogic();

	if (gameConfig.prChargebootForever)
		unlimitedChargebootLogic();

	if (gameConfig.grFusionShotsAlwaysHit)
		fusionShotsAlwaysHitLogic();

#if TWEAKERS
	tweakers();
#endif

	FirstPass = 0;
}

/*
 * NAME :		grLobbyStart
 * 
 * DESCRIPTION :
 * 			Gamerules lobby logic entrypoint.
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
void grLobbyStart(void)
{
	GameSettings * gameSettings = gameGetSettings();

	// If we're not in staging then reset
	if (gameSettings)
		return;

	// Reset
	GameRulesInitialized = 0;
	FirstPass = 1;

	// Reset mirror world in lobby
	cheatsApplyMirrorWorld(0);
}


/*
 * NAME :		grLoadStart
 * 
 * DESCRIPTION :
 * 			Gamerules load logic entrypoint.
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
void grLoadStart(void)
{
	// Hook load gameplay file
	if (*(u32*)0x004EE648 == 0x0C13B3A8)
		*(u32*)0x004EE648 = 0x0C000000 | (u32)&onGameplayLoad / 4;
}
