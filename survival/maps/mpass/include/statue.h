#ifndef _SURVIVAL_MPASS_STATUE_H_
#define _SURVIVAL_MPASS_STATUE_H_

#define STATUE_MOBY_OCLASS                (0x2402)

enum StatueEventType {
	STATUE_EVENT_SPAWN,
  STATUE_EVENT_STATE_UPDATE
};

enum StatueMobyState {
	STATUE_STATE_DEACTIVATED,
	STATUE_STATE_ACTIVATED,
};

typedef struct StatuePVar
{
  Moby* ActivatedOrbMoby;
} StatuePVar_t;

void statueSetState(Moby* moby, enum StatueMobyState state);
int statueCreate(VECTOR position, VECTOR rotation);
void statueSpawn(void);
void statueInit(void);


extern VECTOR statueSpawnPositionRotations[];
extern const int statueSpawnPositionRotationsCount;


#endif // _SURVIVAL_MPASS_STATUE_H_
