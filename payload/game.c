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
struct PayloadConfig Config __attribute__((section(".config"))) = {
	.PayloadDeliveredCameraPos = { 448.10, 521.66, 181, 0 },
	.PayloadDeliveredCameraRot = { 0.58, 0, -2.796, 0 },
	.PathVertexCount = 23,
	.Path = {
		{
			{ 128.7475, 144.7906, 102.405, 0 },
			{ 185.1114, 169.6436, 102.405, 0 },
			{ 208.9837, 180.1699, 102.405, 0 }
		},
		{
			{ 209.4837, 191.2119, 101.9565, 0 },
			{ 206.6114, 195.3046, 101.9565, 0 },
			{ 202.8488, 200.6661, 103.6465, 0 }
		},
		{
			{ 199.1589, 208.9861, 104.6167, 0 },
			{ 192.8514, 217.9736, 104.7167, 0 },
			{ 189.9792, 222.0663, 105.7167, 0 }
		},
		{
			{ 187.036, 231.5014, 107.8202, 0 },
			{ 186.1034, 236.4136, 107.8202, 0 },
			{ 185.2994, 240.648, 107.8202, 0 }
		},
		{
			{ 168.5361, 264.7478, 107.7833, 0 },
			{ 171.1214, 272.4136, 107.7833, 0 },
			{ 174.0134, 280.9891, 108.0033, 0 }
		},
		{
			{ 181.2915, 298.4117, 110.603, 0 },
			{ 181.4284, 298.6786, 110.603, 0 },
			{ 183.7095, 303.128, 110.603, 0 }
		},
		{
			{ 186.4492, 319.9485, 110.603, 0 },
			{ 185.9514, 324.9236, 110.603, 0 },
			{ 185.9215, 325.2221, 110.603, 0 }
		},
		{
			{ 187.2868, 338.2384, 110.0949, 0 },
			{ 184.0224, 342.0256, 109.7549, 0 },
			{ 180.7579, 345.8129, 108.2049, 0 }
		},
		{
			{ 161.2265, 346.4893, 109.4374, 0 },
			{ 165.6414, 364.7636, 103.4374, 0 },
			{ 169.502, 380.7439, 103.4374, 0 }
		},
		{
			{ 141.5809, 390.5676, 104.268, 0 },
			{ 141.3304, 390.7327, 104.268, 0 },
			{ 137.1549, 393.4832, 104.268, 0 }
		},
		{
			{ 120.5338, 404.4331, 104.0687, 0 },
			{ 116.3584, 407.1836, 104.0687, 0 },
			{ 114.6882, 408.2838, 105.0687, 0 }
		},
		{
			{ 115.4375, 410.8326, 104.5439, 0 },
			{ 114.6024, 411.3828, 104.5439, 0 },
			{ 113.4917, 412.1144, 104.9639, 0 }
		},
		{
			{ 111.2075, 413.5021, 107.1509, 0 },
			{ 109.9514, 414.0736, 107.1509, 0 },
			{ 83.18167, 426.2528, 106.5909, 0 }
		},
		{
			{ 83.87619, 442.8548, 106.6458, 0 },
			{ 93.35135, 461.3937, 112.5658, 0 },
			{ 95.06253, 464.7418, 115.1358, 0 }
		},
		{
			{ 93.27353, 461.6668, 113.8113, 0 },
			{ 114.3614, 474.9637, 113.8113, 0 },
			{ 135.7199, 488.4313, 109.3313, 0 }
		},
		{
			{ 152.9835, 480.2113, 110.1029, 0 },
			{ 171.4114, 475.5838, 110.1029, 0 },
			{ 176.2608, 474.366, 110.1029, 0 }
		},
		{
			{ 215.4411, 461.4175, 110.0725, 0 },
			{ 229.5815, 444.1739, 110.0725, 0 },
			{ 265.3824, 400.5159, 110.0725, 0 }
		},
		{
			{ 281.5453, 405.3314, 109.1095, 0 },
			{ 295.4115, 391.0437, 109.1095, 0 },
			{ 315.8869, 369.9459, 108.9595, 0 }
		},
		{
			{ 298.0386, 347.8844, 110.5432, 0 },
			{ 320.0114, 325.2437, 108.0132, 0 },
			{ 340.0689, 304.5765, 107.1632, 0 }
		},
		{
			{ 377.869, 310.443, 102.7058, 0 },
			{ 409.9814, 349.4437, 109.6058, 0 },
			{ 421.5247, 363.463, 110.2758, 0 }
		},
		{
			{ 435.7732, 375.7799, 106.9642, 0 },
			{ 449.3715, 408.0737, 106.0161, 0 },
			{ 469.1707, 452.6848, 119.1888, 0 }
		},
		{
			{ 408.9877, 459.9599, 111.5151, 0 },
			{ 385.7215, 485.2438, 106.0557, 0 },
			{ 382.3676, 488.9518, 106.0168, 0 }
		},
		{
			{ 370.2689, 498.0103, 108.8493, 0 },
			{ 365.3214, 498.7338, 109.4492, 0 },
			{ 360.3721, 499.444, 109.4492, 0 }
		}
	},
	.SpawnPointCount = 88,
	.SpawnPoints = {
		{ 92.26102, 461.4337, 112.2898, 367.7448 },
		{ 169.321, 167.0336, 102.2404, 0 },
		{ 182.4211, 157.9336, 102.2007, 0 },
		{ 200.521, 167.7336, 102.264, 12.16391 },
		{ 183.0211, 177.1336, 102.1696, 1.540182 },
		{ 214.0211, 183.8336, 102.3866, 28.2536 },
		{ 179.621, 225.7336, 106.511, 75.50021 },
		{ 184.8211, 258.8336, 107.7833, 104.3304 },
		{ 147.981, 286.5636, 107.7839, 128.4111 },
		{ 209.6711, 202.8936, 104.0984, 42.41813 },
		{ 194.181, 207.1336, 104.1626, 53.98599 },
		{ 189.8211, 245.0336, 107.799, 89.76199 },
		{ 164.121, 258.3336, 107.7833, 112.7055 },
		{ 191.741, 300.8636, 110.4255, 156.53 },
		{ 178.381, 312.1536, 110.4192, 164.5119 },
		{ 193.221, 318.2736, 110.4192, 172.6995 },
		{ 181.341, 329.2736, 109.9268, 182.8209 },
		{ 185.411, 341.2436, 109.8214, 194.8071 },
		{ 176.081, 343.8336, 108.6385, 204.1263 },
		{ 164.841, 349.2336, 107.5171, 215.4853 },
		{ 172.741, 366.0237, 103.4374, 231.6922 },
		{ 176.181, 378.6437, 103.4374, 238.3978 },
		{ 164.971, 385.1937, 103.4374, 246.6962 },
		{ 149.9111, 379.9637, 103.3736, 255.2726 },
		{ 146.5011, 391.9836, 103.2238, 264.2049 },
		{ 128.951, 387.2436, 104.0687, 276.7711 },
		{ 134.831, 406.0236, 104.0687, 282.2016 },
		{ 116.681, 394.6937, 104.0687, 291.2226 },
		{ 122.121, 412.0337, 104.0687, 296.1224 },
		{ 96.55103, 414.1437, 106.6928, 320.1851 },
		{ 104.431, 425.1436, 106.6928, 320.1851 },
		{ 94.20102, 431.8136, 106.7156, 334.6391 },
		{ 85.73103, 439.6736, 107.6262, 345.1808 },
		{ 91.96103, 449.0837, 109.5818, 355.7023 },
		{ 98.81102, 472.2036, 113.6795, 379.3121 },
		{ 108.221, 464.1637, 113.6796, 383.1117 },
		{ 109.141, 473.0536, 113.6795, 388.5851 },
		{ 119.141, 482.3537, 111.3818, 401.9916 },
		{ 132.231, 485.8337, 109.6962, 413.933 },
		{ 144.711, 479.1337, 109.2225, 426.4743 },
		{ 159.021, 484.7737, 109.6113, 439.3804 },
		{ 157.5011, 471.0337, 109.6113, 441.6401 },
		{ 167.561, 474.8236, 109.6113, 450.1527 },
		{ 202.291, 454.7036, 109.9188, 490.8282 },
		{ 210.1011, 468.9537, 109.9188, 490.8282 },
		{ 224.861, 460.3137, 109.9381, 507.2289 },
		{ 216.1211, 443.6837, 109.9189, 510.338 },
		{ 234.271, 433.2636, 109.1095, 531.559 },
		{ 244.5311, 432.3236, 109.1095, 538.7042 },
		{ 250.3211, 414.1336, 109.1095, 555.5786 },
		{ 263.8011, 416.7737, 109.3852, 564.3904 },
		{ 266.7211, 404.0436, 109.1095, 574.2404 },
		{ 287.9211, 401.7336, 109.1095, 593.5356 },
		{ 293.1211, 388.8337, 109.1514, 605.7381 },
		{ 305.9911, 382.5136, 109.1095, 618.2817 },
		{ 310.0311, 361.9236, 109.1095, 638.3704 },
		{ 304.2111, 348.0936, 109.1095, 651.0072 },
		{ 314.3211, 338.5336, 109.2248, 662.9044 },
		{ 315.3611, 326.8437, 108.2788, 673.2794 },
		{ 323.3411, 310.3036, 107.6877, 687.9966 },
		{ 344.721, 334.1337, 108.927, 702.3597 },
		{ 345.2811, 303.5936, 105.1953, 706.7323 },
		{ 366.3211, 331.2336, 107.9513, 734.0363 },
		{ 365.7211, 305.4336, 103.2608, 724.6209 },
		{ 390.8211, 317.2336, 103.7608, 748.9897 },
		{ 383.8211, 340.4337, 107.9724, 756.4991 },
		{ 410.321, 324.7336, 107.1931, 765.7936 },
		{ 425.8212, 336.0638, 109.2345, 783.178 },
		{ 437.8211, 368.1337, 108.0994, 814.6254 },
		{ 415.6211, 371.3336, 109.3158, 803.6523 },
		{ 414.9211, 393.9336, 106.695, 823.2271 },
		{ 433.1211, 409.1336, 107.8619, 848.2197 },
		{ 464.0211, 418.1336, 106.293, 866.4734 },
		{ 457.5511, 436.3836, 111.3674, 881.3357 },
		{ 436.3211, 439.7336, 112.7715, 897.1538 },
		{ 427.4711, 446.5036, 112.7715, 910.1647 },
		{ 441.2011, 458.0336, 112.7715, 905.6942 },
		{ 450.7811, 451.8337, 112.7715, 895.1027 },
		{ 428.6811, 462.7137, 112.8347, 918.261 },
		{ 418.2559, 477.626, 109.3448, 933.8368 },
		{ 394.9211, 455.8336, 106.9606, 943.4291 },
		{ 398.7211, 488.9337, 105.684, 959.4835 },
		{ 378.7211, 476.9337, 106.6208, 965.9283 },
		{ 398.4211, 508.7337, 103.1683, 971.7723 },
		{ 372.8211, 458.7336, 107.1237, 959.4835 },
		{ 374.0511, 501.6937, 108.5469, 985.5977 },
		{ 367.4511, 493.4836, 108.8633, 987.6727 },
		{ 376.8611, 487.5836, 107.0124, 975.6359 },
	}
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
void onSetRoundComplete(int gameTime, enum RoundOutcome outcome, int roundDuration, struct PayloadTeam teams[2])
{
  int i;
	int isGameOver = State.RoundNumber+1 >= State.RoundLimit;

	State.RoundResult = outcome;
	State.RoundEndTime = gameTime + (isGameOver ? 0 : ROUND_TRANSITION_DELAY_MS);
	State.RoundDuration = roundDuration;
	memcpy(State.Teams, teams, sizeof(teams));

	// update attacking team score
  updateTeamScore(State.Teams[1].TeamId);

	// draw win/lose popup
	if (!isGameOver) {
		for (i = 0; i < 2; ++i)
		{
			Player * p = playerGetFromSlot(i);
			if (p) {
				uiShowPopup(i, PAYLOAD_ROUND_COMPLETE);
			}
		}
	}
}

//--------------------------------------------------------------------------
void onSetPlayerStats(int playerId, struct PayloadPlayerStats* stats)
{
	memcpy(&State.PlayerStates[playerId].Stats, stats, sizeof(struct PayloadPlayerStats));
}

//--------------------------------------------------------------------------
void onReceiveTeamScore(int teamScores[GAME_MAX_PLAYERS], int teamRoundTimes[GAME_MAX_PLAYERS])
{
	int i;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		if (State.Teams[0].TeamId == i) {
			State.Teams[0].Score = teamScores[i];
			State.Teams[0].Time = teamRoundTimes[i];
		} else if (State.Teams[1].TeamId == i) {
			State.Teams[1].Score = teamScores[i];
			State.Teams[1].Time = teamRoundTimes[i];
		}
	}
}

//--------------------------------------------------------------------------
void onReceivePayloadState(enum PayloadMobyState state, int pathIndex, float time)
{
	if (State.PayloadMoby && State.PayloadMoby->PVar)
	{
		struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)State.PayloadMoby->PVar;
		pvar->State = state;
		pvar->PathIndex = pathIndex;
		pvar->Time = time;
		pvar->RecomputeDistanceTicks = 0;
	}
}

//--------------------------------------------------------------------------
void flipTeams(void)
{
	struct PayloadTeam team = State.Teams[0];
	State.Teams[0] = State.Teams[1];
	State.Teams[1] = team;
}

//--------------------------------------------------------------------------
void onPlayerKill(char * fragMsg)
{
	VECTOR delta;

	// call base function
	((void (*)(char*))0x00621CF8)(fragMsg);

	// ignore if no payload
	if (!State.PayloadMoby || !State.RoundInitialized)
		return;
	
	char weaponId = fragMsg[3];
	char killedPlayerId = fragMsg[2];
	char sourcePlayerId = fragMsg[0];

	if (sourcePlayerId >= 0) {
		struct PayloadPlayer* sourcePlayerData = &State.PlayerStates[sourcePlayerId];
		struct PayloadPlayer* killedPlayerData = &State.PlayerStates[killedPlayerId];
		if (sourcePlayerData->Player) {
			if (sourcePlayerData->IsAttacking) {
				// if attacking, count kills when player is near the payload
				vector_subtract(delta, sourcePlayerData->Player->PlayerPosition, State.PayloadMoby->Position);
				if (vector_sqrmag(delta) < (PAYLOAD_PLAYER_RADIUS*PAYLOAD_PLAYER_RADIUS)) {
					sourcePlayerData->Stats.KillsWhileHot++;
					DPRINTF("%d %d kills while hot\n", sourcePlayerId, sourcePlayerData->Stats.KillsWhileHot);
				}
			} else if (killedPlayerData->Player) {
				// if defending, count kills on players near the player
				vector_subtract(delta, killedPlayerData->Player->PlayerPosition, State.PayloadMoby->Position);
				if (vector_sqrmag(delta) < (PAYLOAD_PLAYER_RADIUS*PAYLOAD_PLAYER_RADIUS)) {
					sourcePlayerData->Stats.KillsOnHotPlayers++;
					DPRINTF("%d %d kills on hot players\n", sourcePlayerId, sourcePlayerData->Stats.KillsOnHotPlayers);
				}
			}
		}
	}
}

//--------------------------------------------------------------------------
void getResurrectPoint(Player* player, VECTOR outPos, VECTOR outRot, int firstRes)
{
	char possiblePoints[10];
	int pointCount = 0;
	int i = 0;
	int isAttacking = State.PlayerStates[player->PlayerId].IsAttacking;
	int selectedPoint = isAttacking;
	VECTOR delta;

	// pass down to game
	if (!State.PayloadMoby)
		goto set;

	struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)State.PayloadMoby->PVar;
	if (!pvar)
		goto set;

	// if first res, use defaults
	if (firstRes || !State.RoundInitialized)
		goto set;
	
	// find nearest living teammate to payload
	float nearestTeammateDist = 10000;
	int foundLivingTeammate = -1;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		if (State.PlayerStates[i].Player && State.PlayerStates[i].Player != player && State.PlayerStates[i].IsAttacking == isAttacking && !playerIsDead(State.PlayerStates[i].Player))
		{
			vector_subtract(delta, State.PayloadMoby->Position, State.PlayerStates[i].Player->PlayerPosition);
			float dist = vector_length(delta);
			if (dist < nearestTeammateDist) {
				nearestTeammateDist = dist;
				foundLivingTeammate = i;
			}
		}
	}

	// determine max distance from payload to spawn
	float maxDist = PAYLOAD_SPAWN_DISTANCE_IDEAL + PAYLOAD_SPAWN_DISTANCE_FUDGE;
	float minDist = PAYLOAD_SPAWN_DISTANCE_IDEAL - PAYLOAD_SPAWN_DISTANCE_FUDGE;
	if (foundLivingTeammate >= 0) {
		nearestTeammateDist = bezierGetClosestPointOnPath(delta, State.PlayerStates[foundLivingTeammate].Player->PlayerPosition, Config.Path, State.PathSegmentLengths, Config.PathVertexCount);
	}
	
	for (i = 0; i < Config.SpawnPointCount && pointCount < 10; ++i)
	{
		float spawnDist = Config.SpawnPoints[i][3];
		int isOnTeamSide = spawnDist > pvar->Distance;
		int isValid = 0;
		
		// get distance to payload
		float distToPayload = fabsf(pvar->Distance - spawnDist);
		float distToTeammate = fabsf(nearestTeammateDist - spawnDist);

		// evaluate spawn point based on distance to payload or nearest living teammate
		if (foundLivingTeammate >= 0 && distToTeammate < PAYLOAD_SPAWN_NEAR_TEAM_FUDGE && isAttacking != isOnTeamSide && isAttacking != spawnDist > nearestTeammateDist)
		{
			isValid = 1;
		}
		else if (distToPayload < maxDist)
		{
			if (pvar->Distance < PAYLOAD_SPAWN_DISTANCE_DEADZONE && isAttacking && distToPayload < PAYLOAD_SPAWN_DISTANCE_DEADZONE)
			{
				isValid = 1;
			}
			else if (pvar->Distance > (State.PathLength - PAYLOAD_SPAWN_DISTANCE_DEADZONE) && !isAttacking && distToPayload < PAYLOAD_SPAWN_DISTANCE_DEADZONE)
			{
				isValid = 1;
			}
			else if (isAttacking != isOnTeamSide && distToPayload > minDist)
			{
				isValid = 1;
			}
		}

		if (isValid) {
			possiblePoints[pointCount++] = i;
		}
	}

	// if none found, spawn at default
	// otherwise select at random
	if (pointCount == 0) {
		selectedPoint = isAttacking;
	} else {
		selectedPoint = possiblePoints[rand(pointCount)];
	}

set: ;
	// set position
	vector_copy(outPos, Config.SpawnPoints[selectedPoint]);

	// determine and set yaw
	vector_write(outRot, 0);
	if (State.PayloadMoby)
	{
		vector_subtract(delta, State.PayloadMoby->Position, outPos);
		float len = vector_length(delta);
		outRot[2] = atan2f(delta[1] / len, delta[0] / len);
	}
}

//--------------------------------------------------------------------------
int gameGetTeamScore(int team, int score)
{
  if (team == State.Teams[0].TeamId)
    return State.Teams[0].Score;
  if (team == State.Teams[1].TeamId)
    return State.Teams[1].Score;

  return score;
}

//--------------------------------------------------------------------------
void payloadExplode(Moby* payload, struct PayloadMobyPVar* pvar)
{
	const float radius = 25;
	VECTOR delta;

	// spawn explosion
	spawnExplosion(payload->Position, radius);

	// destroy all mobies in vicinity
	Moby* m = NULL;
	if (CollMobysSphere_Fix(payload->Position, 2, payload, 0, radius)) {
		Moby** hits = CollMobysSphere_Fix_GetHitMobies();
		while ((m = *hits++))
		{
			if ((int)guberGetUID(m) <= 0) {
				m->Rotation[3] = m->DrawDist;
				m->DrawDist = 0;
				spawnExplosion(m->Position, 10);
			}
		}
	}
}

//--------------------------------------------------------------------------
void payloadPostDraw(Moby* payload)
{
	int i;
	VECTOR delta, playerOffset = {0,0,1,0};
	if (!payload)
		return;

	struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)payload->PVar;
	if (!pvar)
		return;

	if (State.RoundEndTime || State.GameOver || pvar->State >= PAYLOAD_STATE_DELIVERED)
		return;
	
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * player = State.PlayerStates[i].Player;
		if (player && State.PlayerStates[i].IsAttacking && !playerIsDead(player)) {
			vector_subtract(delta, player->PlayerPosition, payload->Position);
			float dist = vector_length(delta);
			if (dist < PAYLOAD_PLAYER_RADIUS) {
				float t = clamp(((dist / PAYLOAD_PLAYER_RADIUS) - 0.8) * (1 / 0.2), 0, 1);
				u32 color = colorLerp(TEAM_COLORS[State.Teams[1].TeamId], 0x40ffffff, t);
				endpoints[1].iGlowRGBA = endpoints[0].iGlowRGBA = endpoints[1].iCoreRGBA = endpoints[0].iCoreRGBA = color;
				vector_add(endpoints[1].vPos, player->PlayerPosition, playerOffset);
				vector_copy(endpoints[0].vPos, payload->Position);
				gfxDrawCubicLine((void*)0x2225a8, endpoints, 2, (void*)0x383a68, 1);
			}
		}
	}
}

//--------------------------------------------------------------------------
void payloadUpdate(Moby *payload)
{
	int i = 0;
	VECTOR from = {0,0,2.5,0};
	VECTOR to = {0,0,-2.5,0};
	VECTOR delta;
	int totalAttackerCount = 0;
	static int lastGt = 0;
	int gameTime = gameGetTime();
	float dt = (gameTime - lastGt) / (float)TIME_SECOND;
	lastGt = gameTime;
	if (!payload)
		return;

	struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)payload->PVar;
	if (!pvar)
		return;

	if (State.RoundEndTime || State.GameOver)
		return;

	// register post draw function
	gfxRegisterDrawFunction((void**)0x0022251C, &payloadPostDraw, payload);

	// timers
	u8 recomputeDistance = decTimerU8(&pvar->RecomputeDistanceTicks);
	int explode = decTimerU16(&pvar->ExplodeTicks);

	// determine number of attacking players in vicinity
	int lastPCount = pvar->PlayerCount;
	pvar->PlayerCount = 0;
	pvar->IsMaxSpeed = 1;
	for (i = 0; i < GAME_MAX_PLAYERS; i++)
	{
		if (State.PlayerStates[i].Player && State.PlayerStates[i].IsAttacking && !playerIsDead(State.PlayerStates[i].Player))
		{
			vector_subtract(delta, payload->Position, State.PlayerStates[i].Player->PlayerPosition);
			if (vector_sqrmag(delta) < (PAYLOAD_PLAYER_RADIUS*PAYLOAD_PLAYER_RADIUS)) {
				pvar->PlayerCount++;
				State.PlayerStates[i].Stats.PointsT += dt;
				if (State.PlayerStates[i].Stats.PointsT > 1) {
					int pts = (int)State.PlayerStates[i].Stats.PointsT;
					State.PlayerStates[i].Stats.PointsT -= pts;
					State.PlayerStates[i].Stats.Points += pts;
				}
			} else {
				pvar->IsMaxSpeed = 0;
			}

			++totalAttackerCount;
		}
	}

	//
	if (pvar->PlayerCount == 0)
	{
		if (pvar->WithoutPlayerTicks < PAYLOAD_REVERSE_DELAY_TICKS)
			pvar->WithoutPlayerTicks++;
	}
	else
	{
		pvar->WithoutPlayerTicks = 0;
	}

	// 
	if (totalAttackerCount == 0)
		totalAttackerCount = 1;

	// determine speed
	float targetSpeed = (PAYLOAD_MAX_FORWARD_SPEED * pvar->PlayerCount) / (float)totalAttackerCount;
	pvar->Speed = lerpf(pvar->Speed, targetSpeed, 1 - powf(MATH_E, -PAYLOAD_SPEED_SHARPNESS * MATH_DT));
	if (pvar->WithoutPlayerTicks >= PAYLOAD_REVERSE_DELAY_TICKS)
		pvar->Speed = PAYLOAD_MAX_BACKWARD_SPEED;
	if (pvar->State >= PAYLOAD_STATE_DELIVERED)
		pvar->Speed = 0;

	// move
	float distanceMoved = bezierMovePath(&pvar->Time, &pvar->PathIndex, Config.Path, Config.PathVertexCount, pvar->Speed * MATH_DT);
	pvar->Distance += distanceMoved;
	
	// set position
	bezierGetPosition(payload->Position, &Config.Path[pvar->PathIndex], &Config.Path[pvar->PathIndex+1], pvar->Time);
	vector_add(from, payload->Position, from);
	vector_add(to, payload->Position, to);

	// snap to ground
	if (CollLine_Fix(from, to, 2, NULL, 0)) {
		float ydif = CollLine_Fix_GetHitPosition()[2] - payload->Position[2];
		pvar->Levitate = lerpf(pvar->Levitate, ydif, 1 - powf(MATH_E, -PAYLOAD_COLLISION_SNAP_SHARPNESS * MATH_DT));
	}

	// oscilate
	float offset = 3.0 + (1 + sinf(PAYLOAD_LEVITATE_SPEED * (gameGetTime() / 1000.0))) * 0.5 * PAYLOAD_LEVITATE_HEIGHT;

	// periodically recompute distance from start of path
	// this is to ensure consistent readings
	// otherwise the innaccuracy of the move will skew the distance
	if (recomputeDistance == 0)
	{
		// get distance up to current segment
		pvar->Distance = 0;
		i = 0;
		while (i < pvar->PathIndex)
			pvar->Distance += State.PathSegmentLengths[i++];

		// compute distance of current segment
		pvar->Distance += bezierGetLength(&Config.Path[i], &Config.Path[i+1], 0, pvar->Time);

		pvar->RecomputeDistanceTicks = 60;

		// also send state to other clients
		sendPayloadState(pvar->State, pvar->PathIndex, pvar->Time);
	}

	payload->Position[2] += offset + pvar->Levitate;

	// 
	if (pvar->PathIndex == (Config.PathVertexCount-1)) {
		if (pvar->State < PAYLOAD_STATE_DELIVERED) {
			pvar->ExplodeTicks = PAYLOAD_EXPLOSION_COUNTDOWN_TICKS;
			pvar->State = PAYLOAD_STATE_DELIVERED;
			playTimerTickSound();
		} else if (explode == 0 && pvar->State != PAYLOAD_STATE_EXPLODED) {
			payloadExplode(payload, pvar);
			pvar->State = PAYLOAD_STATE_EXPLODED;
			payload->Rotation[3] = payload->DrawDist;
			payload->DrawDist = 0;
			setRoundComplete(CUSTOM_MSG_ROUND_PAYLOAD_DELIVERED, (gameTime - State.RoundStartTime));
		}
	} else if (pvar->PlayerCount)
		pvar->State = PAYLOAD_STATE_MOVING_FORWARD;
	else if (distanceMoved < -MIN_FLOAT_MAGNITUDE)
		pvar->State = PAYLOAD_STATE_MOVING_BACKWARD;
	else
		pvar->State = PAYLOAD_STATE_IDLE;

	switch (pvar->State)
	{
		case PAYLOAD_STATE_DELIVERED:
		{
			padDisableInput();

			// 
			payload->PrimaryColor = colorLerp(0x00404040, 0x000000FF, 0.5 * (1 + sinf(gameGetTime() / (float)(explode + 120))));
			
			// tick timer
			if (explode > 0 && explode % 30 == 0)
			{
				playTimerTickSound();
			}
			break;
		}
		default:
		{
			payload->PrimaryColor = 0x00404040;   
			payload->Rotation[2] = clampAngle(payload->Rotation[2] + 0.01);
			break;
		}
	}

	// update blip
	int blipId = radarGetBlipIndex(payload);
	if (blipId >= 0)
	{
		RadarBlip * blip = radarGetBlips() + blipId;
		blip->X = payload->Position[0];
		blip->Y = payload->Position[1];
		blip->Life = 0x1F;
		blip->Type = 0x12;
		blip->Team = pvar->State == PAYLOAD_STATE_MOVING_FORWARD ? (State.Teams[1].TeamId) : (pvar->State == PAYLOAD_STATE_MOVING_BACKWARD ? State.Teams[0].TeamId : 10);
	}
}

//--------------------------------------------------------------------------
void payloadReset(void)
{
	Moby* payload = State.PayloadMoby;
	if (!payload)
		return;

	struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)payload->PVar;
	if (!pvar)
		return;

	pvar->Time = 0;
	pvar->Distance = 0;
	pvar->Levitate = 0;
	pvar->PathIndex = 0;
	pvar->Speed = 0;
	payloadUpdate(payload);
}

//--------------------------------------------------------------------------
void processPlayer(int pIndex)
{
	VECTOR t;
	int i = 0, localPlayerIndex, heldWeapon, hasMessage = 0;
	int x,y;
	char strBuf[32];

	struct PayloadPlayer * playerData = &State.PlayerStates[pIndex];
	Player * player = playerData->Player;
	Moby* payload = State.PayloadMoby;
	float pulseTime = 0.5 * (1 + sinf(gameGetTime() / 100.0));
	int isNearPayload = 0;
	if (!player || !payload)
		return;
	struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)payload->PVar;
	if (!pvar)
		return;

	if (playerData->IsAttacking) {
		vector_subtract(t, player->PlayerPosition, payload->Position);
		if (vector_sqrmag(t) < (PAYLOAD_PLAYER_RADIUS*PAYLOAD_PLAYER_RADIUS)) {
			isNearPayload = 1;
		}
	}

	if (player->IsLocal) {
		gfxSetupGifPaging(0);

		// draw attacker icons
		if (playerData->IsAttacking) {
			// draw player is near payload icon
			if (isNearPayload) {
				gfxDrawSprite(380, 16, 16, 16, 0, 0, 32, 32, colorLerp(0x80808080, 0x80E0E0E0, pulseTime), gfxGetFrameTex(52));
			}

			// number of players near payload
			if (pvar->PlayerCount) {
				sprintf(strBuf, "x%d", pvar->PlayerCount);
				gfxDrawSprite(410, 16, 16, 16, 0, 0, 32, 32, colorLerp(0x80808080, 0x80E0E0E0, pulseTime), gfxGetFrameTex(16));
				gfxScreenSpaceText(410 + 10, 16 + 12, 0.7, 0.7, colorLerp(0x80808080, 0x8000E0E0, pulseTime), strBuf, -1, 0);
			}
		}


		if (pvar->State >= PAYLOAD_STATE_DELIVERED) {

			// fix camera bug
			patchVoidFallCameraBug(player);

			// disable most of the hud
			PlayerHUDFlags* hud = hudGetPlayerFlags(player->LocalPlayerIndex);
			hud->Flags.Healthbar = 0;
			hud->Flags.Minimap = 0;
			hud->Flags.Weapons = 0;

			// 
			playerSetWeapon(player, WEAPON_ID_WRENCH);

			//vector_copy(player->CameraPos, Config.PayloadDeliveredCameraPos);
			*(u32*)0x005EBB6C = 0xC60D00B0;
			*(u32*)0x005ebbf8 = 0;
			playerSetPosRot(player, Config.PayloadDeliveredCameraPos, Config.PayloadDeliveredCameraRot);
			player->CameraYaw.Value = Config.PayloadDeliveredCameraRot[2];
			player->CameraPitch.Value = Config.PayloadDeliveredCameraRot[0];
			player->Invisible = 100;

		} else {

			// draw world space icon if obstructed
			if (CollLine_Fix(player->CameraPos, payload->Position, 2, payload, 0)) {
				vector_copy(t, payload->Position);
				t[2] += 4;
				if (gfxWorldSpaceToScreenSpace(t, &x, &y))
				{
					u32 color = 0x80E0E0E0;
					switch (pvar->State)
					{
						case PAYLOAD_STATE_MOVING_FORWARD:
						{
							color = TEAM_COLORS[State.Teams[1].TeamId] + 0x40000000;
							break;
						}
						case PAYLOAD_STATE_MOVING_BACKWARD:
						{
							color = TEAM_COLORS[State.Teams[0].TeamId] + 0x40000000;
							break;
						}
					}
					gfxDrawSprite(x-12, y-12, 24, 24, 0, 0, 32, 32, colorLerp(0x80808080, color, 0.5 + (pulseTime*0.5)), gfxGetFrameTex(81));
				}
			}
		}

		// draw round timer
		if (player->LocalPlayerIndex == 0 && State.RoundDuration > 0) {
			int secondsLeft = (State.RoundDuration - (gameGetTime() - State.RoundStartTime)) / TIME_SECOND;
			if (secondsLeft < 0)
				secondsLeft = 0;
			sprintf(strBuf, "%02d:%02d", secondsLeft/60, secondsLeft%60);

			// 
			gfxScreenSpaceText(452+1, 16+1, 1, 1, 0x80000000, strBuf, -1, 0);
			gfxScreenSpaceText(452, 16, 1, 1, 0x80FFFFFF, strBuf, -1, 0);
		}

		// draw progress bar
		if (player->LocalPlayerIndex == 0) {
			const float height = 250.0;
			const float x = 470.0;
			const float y = 50.0;

			// background
			gfxPixelSpaceBox(x, y, 10.0, height, 0x80808080);
			gfxPixelSpaceBox(x+2, y+2, 6.0, height-4, 0x80404040);

			// progress
			for (i = 0; i < 2; ++i) {

				// bar
				float t = (State.Teams[i].Score/10.0) / State.PathLength;
				float h = floorf((height-4.0) * t);
				float yPos = ((height-4.0) + y + 2.0 - h);
				u32 color = TEAM_COLORS[State.Teams[i].TeamId] + 0x40000000;
				gfxPixelSpaceBox(x + 2, yPos, 6.0, h, color);

				// marker
				if (i == 0 && State.RoundNumber > 0) {
					gfxPixelSpaceBox(x, yPos, 10.0, 2.0, color);
				}

				// draw marker icons
				gfxDrawSprite(x + 20, yPos - 6, 12, 12, 0, 0, 32, 32, color, gfxGetFrameTex(42));
			}
		}

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

#if DEBUG
	if (State.IsHost) {
		if (padGetButtonDown(0, PAD_L3 | PAD_R3) > 0) {
			setRoundComplete(CUSTOM_MSG_ROUND_OVER, State.RoundDuration);
		}
		//else if (padGetButtonDown(0, PAD_UP) > 0) {
		//	VECTOR t = {0,0,5,0};
		//	vector_add(t, State.PayloadMoby->Position, t);
		//	playerSetPosRot(players[0], t, State.PayloadMoby->Rotation);
		//}
		else if (padGetButtonDown(0, PAD_LEFT) > 0) {
			struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)State.PayloadMoby->PVar;
			if (pvar) {
				pvar->Time = 0;
				pvar->PathIndex--;
				if (pvar->PathIndex < 0)
					pvar->PathIndex = 0;
			}
		}
		else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
			struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)State.PayloadMoby->PVar;
			if (pvar) {
				pvar->Time = 1;
				pvar->PathIndex++;
				if (pvar->PathIndex > (Config.PathVertexCount-1))
					pvar->PathIndex = Config.PathVertexCount-2;
			}
		}
		else if (padGetButtonDown(0, PAD_L1 | PAD_L2) > 0) {
			--aaa;
			DPRINTF("aaa: %d\n", aaa);
		}
		else if (padGetButtonDown(0, PAD_L1 | PAD_R2) > 0) {
			++aaa;
			DPRINTF("aaa: %d\n", aaa);
		}
	}
	if (padGetButtonDown(0, PAD_L1 | PAD_R1) > 0) {
		playerRespawn(players[0]);
	}
#endif

	// 
	if (State.RoundDuration > 0)
  	gameData->TimeEnd = (State.RoundStartTime - gameData->TimeStart) + State.RoundDuration;
	else
		gameData->TimeEnd = -1;

	// update score
	if (State.PayloadMoby) {
		struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)State.PayloadMoby->PVar;
		State.Teams[1].Score = pvar->Distance * 10;
		State.Teams[1].Time = gameTime - State.RoundStartTime;
	}

  // handle round end logic
  if (State.IsHost)
  {
    int playersAlive = 0;
    int lastAlivePlayer = 0;
    for (i = 0; i < GAME_MAX_PLAYERS; ++i)
    {
      if (State.PlayerStates[i].Player && !playerIsDead(State.PlayerStates[i].Player)) {
        ++playersAlive;
        lastAlivePlayer = i;
      }
    }

    if (State.RoundDuration > 0 && (gameTime - State.RoundStartTime) > State.RoundDuration)
    {
      setRoundComplete(CUSTOM_MSG_ROUND_OVER, State.RoundDuration);
    }
  }

	// set winner
	if (State.Teams[0].Score == State.Teams[1].Score) {
		if (State.Teams[1].Time > State.Teams[0].Time)
			State.WinningTeam = State.Teams[0].TeamId;
		else if (State.Teams[0].Time > State.Teams[1].Time)
			State.WinningTeam = State.Teams[1].TeamId;
		else
			State.WinningTeam = -1;
	}
	else if (State.Teams[0].Score > State.Teams[1].Score)
		State.WinningTeam = State.Teams[0].TeamId;
	else if (State.Teams[1].Score > State.Teams[0].Score)
		State.WinningTeam = State.Teams[1].TeamId;

	// send score to others at end
	if (State.GameOver && State.IsHost)
	{
		sendTeamScore();
	}
}

//--------------------------------------------------------------------------
void resetRoundState(void)
{
  int i;
	GameSettings* gameSettings = gameGetSettings();
	int gameTime = gameGetTime();

	// 
	State.RoundInitialized = 0;
	State.RoundStartTime = gameTime;
	State.RoundEndTime = 0;
	State.RoundResult = 0;

	// 
	flipTeams();
	payloadReset();
	padEnableInput();

	// 
	*(u32*)0x005ebbf8 = 0x0C135D22;

	// reset "destroyed" mobies
	Moby* moby = mobyListGetStart();
	Moby* mEnd = mobyListGetEnd();
	while (moby < mEnd)
	{
		if (!mobyIsDestroyed(moby) && moby->Rotation[3] != 0 && moby->DrawDist == 0) {
			moby->DrawDist = moby->Rotation[3];
			moby->Rotation[3] = 0;
		}

		++moby;
	}

  // 
  for (i = 0; i < GAME_MAX_PLAYERS; ++i)
  {
    Player * p = State.PlayerStates[i].Player;
    if (p) {
			State.PlayerStates[i].IsAttacking = p->Team == State.Teams[1].TeamId;
			if (p->IsLocal && p->PlayerMoby) {
				unpatchVoidFallCameraBug(p);
				// disable most of the hud
				PlayerHUDFlags* hud = hudGetPlayerFlags(p->LocalPlayerIndex);
				hud->Flags.Healthbar = 1;
				hud->Flags.Minimap = 1;
				hud->Flags.Weapons = 1;
				hud->Flags.NormalScoreboard = 0;
			}
      playerRespawn(p);
    }
  }

	// 
	State.RoundInitialized = 1;
}

//--------------------------------------------------------------------------
void initialize(PatchGameConfig_t* gameConfig)
{
	GameSettings* gameSettings = gameGetSettings();
	Player** players = playerGetAll();
	GameData* gameData = gameGetData();
	int i, j;

	// Disable normal game ending
	*(u32*)0x006219B8 = 0;	// survivor (8)
	*(u32*)0x00620F54 = 0;	// time end (1)
	*(u32*)0x00621568 = 0;	// kills reached (2)
	*(u32*)0x006211A0 = 0;	// all enemies leave (9)

	// point get resurrect point to ours
	*(u32*)0x00610724 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);
	*(u32*)0x005e2d44 = 0x0C000000 | ((u32)&getResurrectPoint >> 2);

	// hook into player kill event
	*(u32*)0x00621c7c = 0x0C000000 | ((u32)&onPlayerKill >> 2);

	// hook messages
	netHookMessages();

  // 
  initializeScoreboard();

	// give a 1 second delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 60 * 1;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	// spawn boxes to bridge gap
	Moby* m = mobySpawn(MOBY_ID_BETA_BOX, 0);
	m->Position[0] = 177.6;
	m->Position[2] = 94.36;
	m->Position[1] = 474.28;
	m->Rotation[2] = (-17.483 * MATH_PI) / 180.0;
	m->DrawDist = 0xFF;
	m->UpdateDist = 0xFF;
	m->Scale *= 15.21;
	m = mobySpawn(MOBY_ID_BETA_BOX, 0);
	m->Position[0] = 190.68;
	m->Position[2] = 94.36;
	m->Position[1] = 470.16;
	m->Rotation[2] = (-17.483 * MATH_PI) / 180.0;
	m->DrawDist = 0xFF;
	m->UpdateDist = 0xFF;
	m->Scale *= 15.21;
	m = mobySpawn(MOBY_ID_BETA_BOX, 0);
	m->Position[0] = 204.09;
	m->Position[2] = 94.36;
	m->Position[1] = 465.94;
	m->Rotation[2] = (-17.483 * MATH_PI) / 180.0;
	m->DrawDist = 0xFF;
	m->UpdateDist = 0xFF;
	m->Scale *= 15.21;

	// calculate length of path and segments
	State.PathLength = 0;
	for (i = 0; i < (Config.PathVertexCount-1); ++i)
	{
		State.PathLength += State.PathSegmentLengths[i] = bezierGetLength(&Config.Path[i], &Config.Path[i+1], 0, 1);
	}

	// determine teams
	for (i = 0; i < 2; ++i) {
		State.Teams[i].TeamId = -1;
		State.Teams[i].Score = State.Teams[i].Time = 0;
	}

	int teamCount = 0;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		char team = gameSettings->PlayerTeams[i];
		if (team >= 0) {
			if (team != State.Teams[0].TeamId && team != State.Teams[1].TeamId) {
				State.Teams[teamCount++].TeamId = team;

				if (teamCount >= 2)
					break;
			}
		}
	}

	// 
	if (teamCount == 1)
		State.Teams[1].TeamId = (State.Teams[0].TeamId + 1) % 10;

	// randomize initial defend/attack order
	if (gameSettings->SpawnSeed % 2) {
		flipTeams();
	}

	// initialize player states
	State.LocalPlayerState = NULL;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * p = players[i];
		State.PlayerStates[i].Player = p;
		State.PlayerStates[i].Stats.Points = 0;
		State.PlayerStates[i].Stats.PointsT = 0;
		State.PlayerStates[i].Stats.KillsOnHotPlayers = 0;
		State.PlayerStates[i].Stats.KillsWhileHot = 0;

		// is local
		if (p) {
			State.PlayerStates[i].IsAttacking = State.Teams[1].TeamId == p->Team;
			if (p->IsLocal && !State.LocalPlayerState)
				State.LocalPlayerState = &State.PlayerStates[i];
		}
	}

	// spawn cq light
	//Moby* light = mobySpawn(MOBY_ID_CQ_LIGHT, 0);
	//vector_copy(light->Position, Config.Path[Config.PathVertexCount-1].ControlPoint);
	//light->Position[2] -= 2;
	//light->UpdateDist = 0xFF;
	//light->DrawDist = 0xFF;

	// spawn payload
	State.PayloadMoby = mobySpawn(PAYLOAD_MOBY_ID, sizeof(struct PayloadMobyPVar));
	State.PayloadMoby->DrawDist = 0xFF;
	State.PayloadMoby->UpdateDist = 0xFF;
	State.PayloadMoby->PUpdate = &payloadUpdate;
	struct PayloadMobyPVar* pvar = (struct PayloadMobyPVar*)State.PayloadMoby->PVar;
	pvar->Distance = 0;
	pvar->Time = 0;
	pvar->Speed = 0;
	DPRINTF("payload moby: %08X\n", (u32)State.PayloadMoby);

	// initialize state
	State.GameOver = 0;
	State.RoundLimit = 2;
	State.RoundNumber = 0;
	if (gameData->TimeEnd > 0)
		State.RoundDuration = gameData->TimeEnd / 2;
	else
		State.RoundDuration = 0;
	State.InitializedTime = gameGetTime();
	resetRoundState();

	Initialized = 1;
}

//--------------------------------------------------------------------------
void updateGameState(PatchStateContainer_t * gameState)
{
	int i,j;

	// game state update
	if (gameState->UpdateGameState)
	{
		gameState->GameStateUpdate.RoundNumber = State.RoundNumber + 1;
	}

	// stats
	if (gameState->UpdateCustomGameStats)
	{
		struct PayloadGameData* sGameData = (struct PayloadGameData*)gameState->CustomGameStats.Payload;
		sGameData->Rounds = State.RoundNumber;
		sGameData->Version = 0x00000002;
		

		for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		{
			sGameData->TeamScore[i] = 0;
			sGameData->Points[i] = State.PlayerStates[i].Stats.Points;
			sGameData->KillsOnHotPlayers[i] = State.PlayerStates[i].Stats.KillsOnHotPlayers;
			sGameData->KillsWhileHot[i] = State.PlayerStates[i].Stats.KillsWhileHot;
		}

		for (i = 0; i < 2; ++i)
		{
			sGameData->TeamScore[State.Teams[i].TeamId] = State.Teams[i].Score;
			sGameData->TeamRoundTime[State.Teams[i].TeamId] = State.Teams[i].Time;
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
