#ifndef INFECTED_GAME_H
#define INFECTED_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

/*
 * Infected team.
 */
#define INFECTED_TEAM			(TEAM_GREEN)


struct InfectedGameData
{
	u32 Version;
	int Infections[GAME_MAX_PLAYERS];
	char IsInfected[GAME_MAX_PLAYERS];
	char IsFirstInfected[GAME_MAX_PLAYERS];
};


void updateTeamScore(int team);

extern int WinningTeam;
extern int InfectedMask;
extern char FirstInfected[GAME_MAX_PLAYERS];
extern int Infections[GAME_MAX_PLAYERS];
extern int SurvivorCount;
extern int InfectedCount;

#endif // INFECTED_GAME_H
