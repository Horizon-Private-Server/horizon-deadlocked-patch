#ifndef _GRIDIRON_BALL_H_
#define _GRIDIRON_BALL_H_

#include <libdl/moby.h>
#include <libdl/math3d.h>
#include <libdl/gamesettings.h>

struct GuberEvent;

#define BALL_MOBY_OCLASS          (MOBY_ID_BETA_BOX)

void ballReset(Moby* moby, int resetType);
void ballMoveToResetPosition(Moby* moby, int resetType);
int ballGetResetCounter(Moby* moby);
int ballGetCarrierIdx(Moby* moby);
void ballPickup(Moby * moby, int playerIdx, int resetCounter);
void ballThrow(Moby * moby, int playerIdx, float power);
void ballRequestDrop(Moby* moby, int playerIdx);
void ballRequestScore(Moby* moby, int playerIdx);
void ballNetworkUpdate(Moby* moby);
void ballNetworkInitialize(Moby* moby);
int ballHandleNetworkEvent(Moby* moby, struct GuberEvent* event);
int ballHasPendingPickup(Moby* moby, int playerIdx);
int ballGetVisualCarrierIdx(Moby* moby);
void ballRequestAction(Moby* moby, int playerIdx, int action, const float* position, const float* velocity);
void ballUpdate(Moby * moby);
void ballCreate(VECTOR position);
void ballInitialize(void);

enum BallAction { BALL_ACTION_PICKUP, BALL_ACTION_DROP, BALL_ACTION_SCORE };

// Wire vectors contain three floats; VECTOR's fourth, aligned lane is local only.
typedef struct BallRequest
{
  int PlayerId;
  unsigned int Revision;
  int ResetCounter;
  unsigned int RequestId;
  int HostId;
  int Action;
  float Position[3];
  float Velocity[3];
} BallRequest_t;

typedef struct BallStateMessage
{
  unsigned int Revision;
  int ResetCounter;
  int CarrierIdx;
  int DieTime;
  int ResetType;
  float Position[3];
  float Velocity[3];
} BallStateMessage_t;

typedef struct BallReply
{
  BallStateMessage_t State;
  int PlayerId;
  unsigned int RequestId;
  int Accepted;
} BallReply_t;

typedef struct BallScoreMessage
{
  unsigned int Sequence;
  int ScoredByPlayerId;
  int TeamScores[2];
  int PlayerPoints[GAME_MAX_PLAYERS];
} BallScoreMessage_t;

typedef struct BallPendingRequest
{
  BallRequest_t Request;
  int Active;
  int Started;
  int LastSent;
  int QueuedDrop;
  float DropPosition[3];
  float DropVelocity[3];
} BallPendingRequest_t;

typedef struct BallRequestResult
{
  unsigned int RequestId;
  int OriginClientId;
  int Known;
  int Accepted;
} BallRequestResult_t;

typedef struct BallPVars
{
  VECTOR Velocity;
  Moby* VisualMoby;
  int CarrierIdx;
  int DieTime;
  int ResetCounter;
  unsigned int Revision;
  int ResetType;
  int HasState;
  int AuthorityClientId;
  int LastSyncTime;
  unsigned int ScoreSequence;
  int LastScorePlayer;
  BallPendingRequest_t Pending[GAME_MAX_PLAYERS];
  unsigned int NextRequestId[GAME_MAX_PLAYERS];
  BallRequestResult_t Results[GAME_MAX_PLAYERS];
} BallPVars_t;

enum BallEventType {
	BALL_EVENT_SPAWN,

  // Reserve the old ownership event IDs; they no longer change ball state.
	BALL_EVENT_RESET,
  BALL_EVENT_PICKUP,
  BALL_EVENT_DROP,
  BALL_EVENT_REQUEST,
  BALL_EVENT_STATE,
  BALL_EVENT_REPLY,
  BALL_EVENT_SCORE,
};

#endif // _GRIDIRON_BALL_H_
