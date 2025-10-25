#ifndef TRAINING_GAME_H
#define TRAINING_GAME_H

#include <tamtypes.h>
#include "messageid.h"
#include "bezier.h"
#include "config.h"
#include <libdl/player.h>
#include <libdl/math3d.h>

#define TPS																		(60)

#define TARGET_MOBY_ID												(0x251C)

#define TRAINING_MAX_SPAWNPOINTS							(25)

#define MIN_FLOAT_MAGNITUDE										(0.0001)

enum TargetState
{
	TARGET_STATE_IDLE = 0,
	TARGET_STATE_JUMPING = 1 << 0,
	TARGET_STATE_STRAFING = 1 << 1,
};

enum TargetEvent
{
	TARGET_EVENT_SPAWN,
	TARGET_EVENT_DESTROY,
};

struct TrainingMapConfig
{
	int SpawnPointCount;
	VECTOR SpawnPoints[TRAINING_MAX_SPAWNPOINTS];
	VECTOR PlayerSpawnPoint;
};

// warning: multiple differing types with the same name, only one recovered
struct ReactVars {
	/*   0 */ int flags;
	/*   4 */ int lastReactFrame;
	/*   8 */ Moby* pInfectionMoby;
	/*   c */ float acidDamage;
	/*  10 */ char eternalDeathCount;
	/*  11 */ char doHotSpotChecks;
	/*  12 */ char unchainable;
	/*  13 */ char isShielded;
	/*  14 */ char state;
	/*  15 */ char deathState;
	/*  16 */ char deathEffectState;
	/*  17 */ char deathStateType;
	/*  18 */ signed char knockbackRes;
	/*  19 */ char padC;
	/*  1a */ char padD;
	/*  1b */ char padE;
	/*  1c */ char padF;
	/*  1d */ char padG;
	/*  1e */ char padH;
	/*  1f */ char padI;
	/*  20 */ float minorReactPercentage;
	/*  24 */ float majorReactPercentage;
	/*  28 */ float deathHeight;
	/*  2c */ float bounceDamp;
	/*  30 */ int deadlyHotSpots;
	/*  34 */ float curUpGravity;
	/*  38 */ float curDownGravity;
	/*  3c */ float shieldDamageReduction;
	/*  40 */ float damageReductionSameOClass;
	/*  44 */ int deathCorn;
	/*  48 */ int deathType;
	/*  4c */ short int deathSound;
	/*  4e */ short int deathSound2;
	/*  50 */ float peakFrame;
	/*  54 */ float landFrame;
	/*  58 */ float drag;
	/*  5c */ short unsigned int effectStates;
	/*  5e */ short unsigned int effectPrimMask;
	/*  60 */ short unsigned int effectTimers[16];
};

struct TrainingTargetMobyPVar
{
  struct TargetVars * TargetVarsPtr;
	char _pad0[0x0C];
	struct ReactVars * ReactVarsPtr;
	char _pad1[0x08];
  struct MoveVars_V2 * MoveVarsPtr;
	char _pad2[0x14];
  struct FlashVars * FlashVarsPtr;
	char _pad3[0x18];

	struct TargetVars TargetVars;
	struct ReactVars ReactVars;
	struct MoveVars_V2 MoveVars;
	struct FlashVars FlashVars;
	VECTOR SpawnPos;
	VECTOR Velocity;
	int State;
	u32 LifeTicks;
	int TimeCreated;
	float Jitter;
	float StrafeSpeed;
	float JumpSpeed;
};

struct TargetDamageEventArgs
{
	int SourceUID;
	u16 SourceOClass;
};

struct TrainingState
{
	int InitializedTime;
	int Points;
	int Kills;
	int Hits;
	int ShotsFired;
	int BestCombo;
	int TargetsDestroyed;
	int GameOver;
	int WinningTeam;
	int IsHost;
  enum TRAINING_AGGRESSION AggroMode;
	enum TRAINING_TYPE TrainingType;
	int TargetLastSpawnIdx;
	int ComboCounter;
	long TimeLastSpawn;
	long TimeLastKill;
};

typedef struct SimulatedPlayer {
	struct PAD Pad;
	struct TrainingTargetMobyPVar Vars;
	Player* Player;
	u32 TicksToRespawn;
	u32 TicksToJump;
	u32 TicksToJumpFor;
	u32 TicksToStrafeSwitch;
	u32 TicksToStrafeQuickSwitchFor;
	u32 TicksToStrafeStop;
	u32 TicksToStrafeStopFor;
	u32 TicksToStrafeStoppedFor;
	u32 TicksToFire;
	u32 TicksFireDelay;
	u32 TicksToCycle;
	u32 TicksToAimYaw;
	u32 TicksToAimPitch;
	u32 TicksToTbag;
	u32 TicksAimNearPlayer;
  u32 TicksSinceSeenPlayer;
	int SniperFireStopForTicks;
	int StrafeDir;
	int Idx;
	int CycleIdx;
	int Points;
	float YawOff;
	float Yaw;
	float YawVel;
	float YawAcc;
	float PitchOff;
	float Pitch;
	float PitchVel;
	float PitchAcc;
	char Created;
	char Active;
} SimulatedPlayer_t;

struct TrainingGameData
{
	u32 Version;
	int Points;
	int Time;
	int Kills;
	int Hits;
	int Misses;
	int BestCombo;
};

extern struct TrainingState State;


extern const float TARGET_MIN_SPAWN_DISTANCE[TRAINING_TYPE_MAX];
extern const float TARGET_IDEAL_SPAWN_DISTANCE[TRAINING_TYPE_MAX];
extern const float TARGET_BUFFER_SPAWN_DISTANCE[TRAINING_TYPE_MAX];

extern const char* NAMES[];
extern const int NAMES_COUNT;

extern const char* TRAINING_AGGRO_NAMES[];

void updateTeamScore(int team);



#endif // TRAINING_GAME_H
