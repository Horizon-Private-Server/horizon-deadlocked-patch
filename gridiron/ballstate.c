/***************************************************
 * FILENAME :		ballstate.c
 *
 * DESCRIPTION :
 * 		Manages ball ownership, prediction, and synchronization.
 */

#include <tamtypes.h>
#include <string.h>

#include <libdl/game.h>
#include <libdl/time.h>
#include <libdl/gamesettings.h>
#include <libdl/player.h>
#include <libdl/guber.h>
#include <libdl/sound.h>
#include <libdl/ui.h>
#include "include/game.h"
#include "include/ball.h"

// Clients predict pickups, but only the host commits ownership changes.
// State snapshots repair held references even when the revision is unchanged.

#define BALL_RETRY_TIME (TIME_SECOND / 4)
#define BALL_REQUEST_TIMEOUT (TIME_SECOND * 3 / 2)
#define BALL_RETURN_TIME (TIME_SECOND * 25)

// NetDataSize is six bits, so no event may contain more than 63 bytes.
typedef char BallRequestFits[(sizeof(BallRequest_t) <= 63) ? 1 : -1];
typedef char BallReplyFits[(sizeof(BallReply_t) <= 63) ? 1 : -1];
typedef char BallScoreFits[(sizeof(BallScoreMessage_t) <= 63) ? 1 : -1];

void ballReply(Moby* moby, const BallRequest_t* request, int accepted);

//--------------------------------------------------------------------------
int ballIsNewerSequence(unsigned int sequence, unsigned int previousSequence)
{
  // Subtract before comparing so counters can wrap around UINT_MAX.
  return (int)(sequence - previousSequence) > 0;
}

//--------------------------------------------------------------------------
int ballSendEvent(Moby* moby, int eventType, const void* payload, int payloadSize)
{
  Guber* guber = guberGetObjectByMoby(moby);
  GuberEvent* event;

  if (!guber)
    return 0;

  event = guberEventCreateEventSafe(guber, eventType, 0, 0);
  if (!event)
    return 0;

  guberEventWrite(event, (void*)payload, payloadSize);
  return 1;
}

//--------------------------------------------------------------------------
void ballGetState(Moby* moby, BallStateMessage_t* state)
{
  BallPVars_t* pvars = moby->PVar;

  state->Revision = pvars->Revision;
  state->ResetCounter = pvars->ResetCounter;
  state->CarrierIdx = pvars->CarrierIdx;
  state->DieTime = pvars->DieTime;
  state->ResetType = pvars->ResetType;
  memcpy(state->Position, moby->Position, sizeof(state->Position));
  memcpy(state->Velocity, pvars->Velocity, sizeof(state->Velocity));
}

//--------------------------------------------------------------------------
int ballHasPendingPickup(Moby* moby, int playerIdx)
{
  BallPVars_t* pvars;

  if (!moby || playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS)
    return 0;

  pvars = moby->PVar;
  return pvars->Pending[playerIdx].Active &&
    pvars->Pending[playerIdx].Request.Action == BALL_ACTION_PICKUP;
}

//--------------------------------------------------------------------------
int ballGetVisualCarrierIdx(Moby* moby)
{
  BallPVars_t* pvars = moby->PVar;
  int i;

  if (pvars->CarrierIdx >= 0)
    return pvars->CarrierIdx;

  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    if (ballHasPendingPickup(moby, i))
      return i;
  }

  return -1;
}

//--------------------------------------------------------------------------
void ballReconcile(Moby* moby)
{
  BallPVars_t* pvars = moby->PVar;
  Player** players = playerGetAll();
  int i;

  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    Player* player = players[i];
    int isConfirmedCarrier;
    int isPredictedCarrier;

    if (!player)
      continue;

    isConfirmedCarrier = i == pvars->CarrierIdx;
    isPredictedCarrier = pvars->CarrierIdx < 0 && player->IsLocal && ballHasPendingPickup(moby, i);
    if (isConfirmedCarrier || isPredictedCarrier)
      player->HeldMoby = moby;
    else if (player->HeldMoby == moby)
      playerDropFlag(player, 1);
  }
}

//--------------------------------------------------------------------------
void ballNotify(Moby* moby, int previousCarrierIdx, int previousResetCounter)
{
  BallPVars_t* pvars = moby->PVar;
  Player* local = playerGetFromSlot(0);
  int sound = -1;
  const char* text = NULL;
  int carrierChanged = previousCarrierIdx != pvars->CarrierIdx;
  int ballWasReset = previousResetCounter != pvars->ResetCounter;

  if (previousCarrierIdx >= 0 && previousCarrierIdx < GAME_MAX_PLAYERS &&
      (carrierChanged || ballWasReset))
    State.PlayerStates[previousCarrierIdx].TimeLastCarrier = gameGetTime();

  if (ballWasReset) {
    if (pvars->ResetType == BALL_RESET_CENTER)
      text = "The ball has returned to the center!";
  } else if (carrierChanged) {
    sound = pvars->CarrierIdx >= 0 ? 0 : 1;
    text = pvars->CarrierIdx >= 0 ? "The ball has been picked up!" : "The ball has been dropped!";
  }

  if (sound >= 0 && local && local->PlayerMoby)
    mobyPlaySoundByClass(sound, 0, local->PlayerMoby, MOBY_ID_RED_FLAG);
  if (text)
    uiShowPopup(0, (char*)text);
}

//--------------------------------------------------------------------------
void ballResolvePending(Moby* moby)
{
  BallPVars_t* pvars = moby->PVar;
  int i;

  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    BallPendingRequest_t* pending = &pvars->Pending[i];
    BallPendingRequest_t completedRequest;
    int matchesCurrentState;
    int pickupWasConfirmed;

    if (!pending->Active)
      continue;

    matchesCurrentState = pending->Request.ResetCounter == pvars->ResetCounter &&
      pending->Request.Revision == pvars->Revision &&
      (pvars->CarrierIdx < 0 || pvars->CarrierIdx == i);
    if (matchesCurrentState)
      continue;

    completedRequest = *pending;
    pending->Active = 0;
    pending->QueuedDrop = 0;
    ballReconcile(moby);

    // The next revision must grant this exact pickup before its queued throw
    // can run. A reset or a later possession cancels the queued action.
    pickupWasConfirmed = completedRequest.Request.Action == BALL_ACTION_PICKUP &&
      completedRequest.Request.ResetCounter == pvars->ResetCounter &&
      pvars->CarrierIdx == i && pvars->Revision == completedRequest.Request.Revision + 1;
    if (completedRequest.QueuedDrop && pickupWasConfirmed)
      ballRequestAction(moby, i, BALL_ACTION_DROP, completedRequest.DropPosition, completedRequest.DropVelocity);
  }
}

//--------------------------------------------------------------------------
void ballCheckAuthority(Moby* moby)
{
  BallPVars_t* pvars = moby->PVar;
  int hostId = gameGetHostId();

  if (pvars->AuthorityClientId == hostId)
    return;

  pvars->AuthorityClientId = hostId;
  pvars->HasState = 0;
  pvars->LastSyncTime = gameGetTime() - TIME_SECOND;
  memset(pvars->Pending, 0, sizeof(pvars->Pending));
  memset(pvars->Results, 0, sizeof(pvars->Results));
  ballReconcile(moby);
}

//--------------------------------------------------------------------------
int ballApplyState(Moby* moby, const BallStateMessage_t* state)
{
  BallPVars_t* pvars = moby->PVar;
  int previousCarrierIdx = pvars->CarrierIdx;
  int previousResetCounter = pvars->ResetCounter;
  int isNewRevision;

  if (state->CarrierIdx < -1 || state->CarrierIdx >= GAME_MAX_PLAYERS ||
      state->ResetType < BALL_RESET_CENTER || state->ResetType > BALL_RESET_TEAM1)
    return 0;

  // Older snapshots cannot undo a newer possession.
  if (pvars->HasState && ballIsNewerSequence(pvars->Revision, state->Revision))
    return 0;

  isNewRevision = !pvars->HasState || pvars->Revision != state->Revision;
  pvars->HasState = 1;
  pvars->Revision = state->Revision;
  pvars->ResetCounter = state->ResetCounter;
  pvars->CarrierIdx = state->CarrierIdx;
  pvars->DieTime = state->DieTime;
  pvars->ResetType = state->ResetType;
  memcpy(moby->Position, state->Position, sizeof(state->Position));
  memcpy(pvars->Velocity, state->Velocity, sizeof(state->Velocity));

  // Equal revisions still repair ownership, without repeating notifications.
  ballResolvePending(moby);
  ballReconcile(moby);
  if (isNewRevision)
    ballNotify(moby, previousCarrierIdx, previousResetCounter);
  return 1;
}

//--------------------------------------------------------------------------
int ballBroadcastState(Moby* moby)
{
  BallStateMessage_t state;

  ballGetState(moby, &state);
  return ballSendEvent(moby, BALL_EVENT_STATE, &state, sizeof(state));
}

//--------------------------------------------------------------------------
void ballCommit(Moby* moby, int previousCarrierIdx, int previousResetCounter)
{
  BallPVars_t* pvars = moby->PVar;

  ++pvars->Revision;
  pvars->HasState = 1;
  ballResolvePending(moby);
  ballReconcile(moby);
  ballNotify(moby, previousCarrierIdx, previousResetCounter);

  // A failed allocation cannot undo the transaction; the heartbeat retries it.
  ballBroadcastState(moby);
}

//--------------------------------------------------------------------------
void ballResetState(Moby* moby, int resetType)
{
  BallPVars_t* pvars = moby->PVar;

  pvars->CarrierIdx = -1;
  ++pvars->ResetCounter;
  pvars->ResetType = resetType;
  pvars->DieTime = gameGetTime() + BALL_RETURN_TIME;
  memset(pvars->Velocity, 0, sizeof(pvars->Velocity));
  ballMoveToResetPosition(moby, resetType);
}

//--------------------------------------------------------------------------
void ballReset(Moby* moby, int resetType)
{
  BallPVars_t* pvars;
  int previousCarrierIdx, previousResetCounter;

  if (!moby || !gameAmIHost() || resetType < BALL_RESET_CENTER || resetType > BALL_RESET_TEAM1)
    return;

  ballCheckAuthority(moby);
  pvars = moby->PVar;
  previousCarrierIdx = pvars->CarrierIdx;
  previousResetCounter = pvars->ResetCounter;
  ballResetState(moby, resetType);
  ballCommit(moby, previousCarrierIdx, previousResetCounter);
}

//--------------------------------------------------------------------------
void ballGetScore(Moby* moby, BallScoreMessage_t* score)
{
  BallPVars_t* pvars = moby->PVar;
  int i;

  score->Sequence = pvars->ScoreSequence;
  score->ScoredByPlayerId = pvars->LastScorePlayer;
  score->TeamScores[0] = State.Teams[0].Score;
  score->TeamScores[1] = State.Teams[1].Score;
  for (i = 0; i < GAME_MAX_PLAYERS; ++i)
    score->PlayerPoints[i] = State.PlayerStates[i].Stats.Points;
}

//--------------------------------------------------------------------------
void ballApplyScore(Moby* moby, const BallScoreMessage_t* score)
{
  BallPVars_t* pvars = moby->PVar;
  GameData* gameData = gameGetData();
  int i;

  if (score->ScoredByPlayerId < 0 || score->ScoredByPlayerId >= GAME_MAX_PLAYERS ||
      !ballIsNewerSequence(score->Sequence, pvars->ScoreSequence))
    return;

  pvars->ScoreSequence = score->Sequence;
  pvars->LastScorePlayer = score->ScoredByPlayerId;
  State.Teams[0].Score = score->TeamScores[0];
  State.Teams[1].Score = score->TeamScores[1];
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    State.PlayerStates[i].Stats.Points = score->PlayerPoints[i];
    if (gameData)
      gameData->PlayerStats.CtfFlagsCaptures[i] = score->PlayerPoints[i];
  }
  onBallScored(score->ScoredByPlayerId);
}

//--------------------------------------------------------------------------
void ballHandleRequest(Moby* moby, const BallRequest_t* request, int originClientId)
{
  BallPVars_t* pvars = moby->PVar;
  GameSettings* settings = gameGetSettings();
  Player* player;
  BallRequestResult_t* result;
  int playerIdx = request->PlayerId;
  int previousCarrierIdx, previousResetCounter, teamIdx;

  // Validate the sender before using the player index or its request history.
  if (!gameAmIHost() || !settings)
    return;
  if (playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS)
    return;
  if (settings->PlayerClients[playerIdx] != originClientId || request->HostId != gameGetHostId())
    return;

  result = &pvars->Results[playerIdx];

  // A vacated player slot may be reused by a client with its own request IDs.
  if (result->Known && result->OriginClientId != originClientId)
    result->Known = 0;

  // Retry the previous reply; never apply the same action twice.
  if (result->Known && !ballIsNewerSequence(request->RequestId, result->RequestId)) {
    int isDuplicate = request->RequestId == result->RequestId;

    ballReply(moby, request, isDuplicate && result->Accepted);
    return;
  }

  // Start with a rejection. Only a successfully committed action sets Accepted.
  result->Known = 1;
  result->RequestId = request->RequestId;
  result->OriginClientId = originClientId;
  result->Accepted = 0;

  player = playerGetFromIndex(playerIdx);
  if (!playerIsValid(player))
    goto reply;
  if (request->Revision != pvars->Revision || request->ResetCounter != pvars->ResetCounter)
    goto reply;

  previousCarrierIdx = pvars->CarrierIdx;
  previousResetCounter = pvars->ResetCounter;

  switch (request->Action)
  {
    case BALL_ACTION_PICKUP:
      if (pvars->CarrierIdx >= 0 || !playerStateCanHoldBall(player))
        goto reply;
      if (player->HeldMoby && player->HeldMoby != moby)
        goto reply;
      if (gameGetTime() - State.PlayerStates[playerIdx].TimeLastCarrier < PICKUP_COOLDOWN)
        goto reply;

      pvars->CarrierIdx = playerIdx;
      pvars->DieTime = 0;
      break;

    case BALL_ACTION_DROP:
      if (pvars->CarrierIdx != playerIdx)
        goto reply;

      pvars->CarrierIdx = -1;
      pvars->DieTime = gameGetTime() + BALL_RETURN_TIME;
      memcpy(moby->Position, request->Position, sizeof(request->Position));
      memcpy(pvars->Velocity, request->Velocity, sizeof(request->Velocity));
      break;

    case BALL_ACTION_SCORE:
    {
      VECTOR delta;
      BallScoreMessage_t score;

      if (pvars->CarrierIdx != playerIdx || !playerStateCanHoldBall(player))
        goto reply;

      teamIdx = State.Teams[0].TeamId == player->Team ? 0 : 1;
      vector_subtract(delta, State.Teams[!teamIdx].BasePosition, player->PlayerPosition);
      if (vector_sqrmag(delta) >= 5)
        goto reply;
      if (!player->Ground.onGood || !player->Ground.pMoby ||
          player->Ground.pMoby->OClass != MOBY_ID_FLAG_SPAWN_POINT)
        goto reply;

      ballGetScore(moby, &score);
      ++score.Sequence;
      score.ScoredByPlayerId = playerIdx;
      ++score.TeamScores[teamIdx];
      ++score.PlayerPoints[playerIdx];
      ballResetState(moby, BALL_RESET_TEAM0 + !teamIdx);

      // Commit possession before respawn/UI callbacks or any further requests.
      result->Accepted = 1;
      ballCommit(moby, previousCarrierIdx, previousResetCounter);
      ballApplyScore(moby, &score);
      ballSendEvent(moby, BALL_EVENT_SCORE, &score, sizeof(score));
      goto reply;
    }

    default:
      goto reply;
  }

  result->Accepted = 1;
  ballCommit(moby, previousCarrierIdx, previousResetCounter);

reply:
  ballReply(moby, request, result->Accepted);
}

//--------------------------------------------------------------------------
void ballHandleReply(Moby* moby, const BallReply_t* reply)
{
  BallPVars_t* pvars = moby->PVar;
  BallPendingRequest_t* pending;

  if (reply->PlayerId < 0 || reply->PlayerId >= GAME_MAX_PLAYERS)
    return;

  if (!gameAmIHost())
    ballApplyState(moby, &reply->State);

  pending = &pvars->Pending[reply->PlayerId];
  if (pending->Active && pending->Request.RequestId == reply->RequestId) {
    pending->Active = 0;
    pending->QueuedDrop = 0;
    ballReconcile(moby);
  }
}

//--------------------------------------------------------------------------
void ballReply(Moby* moby, const BallRequest_t* request, int accepted)
{
  BallReply_t reply;

  ballGetState(moby, &reply.State);
  reply.PlayerId = request->PlayerId;
  reply.RequestId = request->RequestId;
  reply.Accepted = accepted;

  // Local requests use the identical result path, without a network round trip.
  ballHandleReply(moby, &reply);
  ballSendEvent(moby, BALL_EVENT_REPLY, &reply, sizeof(reply));
}

//--------------------------------------------------------------------------
void ballSendRequest(Moby* moby, BallPendingRequest_t* pending)
{
  if (gameAmIHost()) {
    BallRequest_t request = pending->Request;

    pending->LastSent = gameGetTime();
    ballHandleRequest(moby, &request, gameGetMyClientId());
  } else if (ballSendEvent(moby, BALL_EVENT_REQUEST, &pending->Request, sizeof(pending->Request))) {
    pending->LastSent = gameGetTime();
  }
}

//--------------------------------------------------------------------------
void ballRequestAction(Moby* moby, int playerIdx, int action, const float* position, const float* velocity)
{
  BallPVars_t* pvars;
  BallPendingRequest_t* pending;
  Player* player;

  if (!moby || playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS)
    return;

  ballCheckAuthority(moby);
  pvars = moby->PVar;
  player = playerGetFromIndex(playerIdx);
  if (!player || !player->IsLocal)
    return;

  pending = &pvars->Pending[playerIdx];
  if (pending->Active) {
    // Preserve throw/drop input while the pickup is waiting for confirmation.
    if (pending->Request.Action == BALL_ACTION_PICKUP && action == BALL_ACTION_DROP) {
      pending->QueuedDrop = 1;
      memcpy(pending->DropPosition, position, sizeof(pending->DropPosition));
      memcpy(pending->DropVelocity, velocity, sizeof(pending->DropVelocity));
    }
    return;
  }

  if (action == BALL_ACTION_PICKUP) {
    if (pvars->CarrierIdx >= 0 || !playerStateCanHoldBall(player) || player->HeldMoby)
      return;
    if (gameGetTime() - State.PlayerStates[playerIdx].TimeLastCarrier < PICKUP_COOLDOWN)
      return;
  } else if (pvars->CarrierIdx != playerIdx)
    return;

  memset(pending, 0, sizeof(*pending));
  pending->Active = 1;
  pending->Started = gameGetTime();
  pending->LastSent = gameGetTime() - BALL_RETRY_TIME;
  pending->Request.PlayerId = playerIdx;
  pending->Request.Revision = pvars->Revision;
  pending->Request.ResetCounter = pvars->ResetCounter;
  pending->Request.RequestId = ++pvars->NextRequestId[playerIdx];
  pending->Request.HostId = gameGetHostId();
  pending->Request.Action = action;
  if (position)
    memcpy(pending->Request.Position, position, sizeof(pending->Request.Position));
  if (velocity)
    memcpy(pending->Request.Velocity, velocity, sizeof(pending->Request.Velocity));

  ballReconcile(moby);
  ballSendRequest(moby, pending);
}

//--------------------------------------------------------------------------
void ballPickup(Moby* moby, int playerIdx, int resetCounter)
{
  if (moby && resetCounter == ((BallPVars_t*)moby->PVar)->ResetCounter)
    ballRequestAction(moby, playerIdx, BALL_ACTION_PICKUP, NULL, NULL);
}

//--------------------------------------------------------------------------
void ballRequestDrop(Moby* moby, int playerIdx)
{
  VECTOR position;
  Player* player;

  if (!moby || playerIdx < 0 || playerIdx >= GAME_MAX_PLAYERS)
    return;
  player = playerGetFromIndex(playerIdx);
  if (!player)
    return;
  memcpy(position, moby->Position, sizeof(position));
  if (ballHasPendingPickup(moby, playerIdx)) {
    memcpy(position, player->PlayerPosition, sizeof(position));
    position[2] += 1;
  }
  ballRequestAction(moby, playerIdx, BALL_ACTION_DROP, position, player->Velocity);
}

//--------------------------------------------------------------------------
void ballRequestScore(Moby* moby, int playerIdx)
{
  ballRequestAction(moby, playerIdx, BALL_ACTION_SCORE, NULL, NULL);
}

//--------------------------------------------------------------------------
void ballNetworkInitialize(Moby* moby)
{
  BallPVars_t* pvars = moby->PVar;
  Moby* visualMoby = pvars->VisualMoby;

  memset(pvars, 0, sizeof(*pvars));
  pvars->VisualMoby = visualMoby;
  pvars->CarrierIdx = -1;
  pvars->LastScorePlayer = -1;
  pvars->AuthorityClientId = gameGetHostId();
  pvars->LastSyncTime = gameGetTime() - TIME_SECOND;
  pvars->HasState = gameAmIHost();
}

//--------------------------------------------------------------------------
int ballCarrierMustDrop(int carrierIdx, Player* carrier, GameSettings* settings)
{
  if (carrierIdx < 0)
    return 0;

  if (settings && settings->PlayerClients[carrierIdx] < 0)
    return 1;

  // Loading can temporarily remove the player or its network object.
  if (!carrier || !carrier->pNetPlayer)
    return 0;

  if (!playerIsConnected(carrier))
    return 1;

  return carrier->PlayerMoby && !playerStateCanHoldBall(carrier);
}

//--------------------------------------------------------------------------
void ballNetworkUpdate(Moby* moby)
{
  BallPVars_t* pvars = moby->PVar;
  GameSettings* settings = gameGetSettings();
  Player** players = playerGetAll();
  int i;

  ballCheckAuthority(moby);

  if (gameAmIHost()) {
    int carrierIdx = pvars->CarrierIdx;
    Player* carrier = playerGetFromIndex(carrierIdx);

    if (ballCarrierMustDrop(carrierIdx, carrier, settings)) {
      int previousResetCounter = pvars->ResetCounter;

      pvars->CarrierIdx = -1;
      pvars->DieTime = gameGetTime() + BALL_RETURN_TIME;
      if (carrier)
        memcpy(pvars->Velocity, carrier->Velocity, sizeof(float) * 3);
      else
        memset(pvars->Velocity, 0, sizeof(pvars->Velocity));
      ballCommit(moby, carrierIdx, previousResetCounter);
    }

    // Broadcast even when the carrier is remote or clients disagree about it.
    if (gameGetTime() - pvars->LastSyncTime >= TIME_SECOND && ballBroadcastState(moby)) {
      BallScoreMessage_t score;

      pvars->LastSyncTime = gameGetTime();
      if (pvars->ScoreSequence) {
        ballGetScore(moby, &score);
        ballSendEvent(moby, BALL_EVENT_SCORE, &score, sizeof(score));
      }
    }
  }

  // Requests have independent retry/expiry timers for split-screen players.
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    BallPendingRequest_t* pending = &pvars->Pending[i];

    if (!pending->Active)
      continue;

    if (!players[i] || !players[i]->IsLocal ||
        gameGetTime() - pending->Started >= BALL_REQUEST_TIMEOUT) {
      pending->Active = 0;
      pending->QueuedDrop = 0;
    } else if (gameGetTime() - pending->LastSent >= BALL_RETRY_TIME) {
      ballSendRequest(moby, pending);
    }
  }

  ballReconcile(moby);
}

//--------------------------------------------------------------------------
int ballHandleNetworkEvent(Moby* moby, GuberEvent* event)
{
  int originClientId = event->NetEvent.OriginClientIdx;

  ballCheckAuthority(moby);
  switch (event->NetEvent.EventID)
  {
    case BALL_EVENT_REQUEST:
    {
      BallRequest_t request;

      if (!gameAmIHost() || event->NetEvent.NetDataSize != sizeof(request))
        return 0;

      guberEventRead(event, &request, sizeof(request));
      ballHandleRequest(moby, &request, originClientId);
      break;
    }
    case BALL_EVENT_STATE:
    {
      BallStateMessage_t state;

      if (!gameIsHost(originClientId) || gameAmIHost())
        return 0;
      if (event->NetEvent.NetDataSize != sizeof(state))
        return 0;

      guberEventRead(event, &state, sizeof(state));
      ballApplyState(moby, &state);
      break;
    }
    case BALL_EVENT_REPLY:
    {
      BallReply_t reply;

      if (!gameIsHost(originClientId) || gameAmIHost())
        return 0;
      if (event->NetEvent.NetDataSize != sizeof(reply))
        return 0;

      guberEventRead(event, &reply, sizeof(reply));
      ballHandleReply(moby, &reply);
      break;
    }
    case BALL_EVENT_SCORE:
    {
      BallScoreMessage_t score;

      if (!gameIsHost(originClientId) || gameAmIHost())
        return 0;
      if (event->NetEvent.NetDataSize != sizeof(score))
        return 0;

      guberEventRead(event, &score, sizeof(score));
      ballApplyScore(moby, &score);
      break;
    }
  }
  return 0;
}
