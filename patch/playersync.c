#include <libdl/game.h>
#include <libdl/utils.h>
#include <libdl/player.h>
#include <libdl/net.h>
#include <libdl/stdlib.h>
#include <libdl/stdio.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/radar.h>
#include <libdl/graphics.h>

#include "config.h"
#include "module.h"
#include "include/config.h"

#define PLAYER_SYNC_DATAS_PTR     (*(PlayerSyncPlayerData_t**)0x000CFFB0)
#define CMD_BUFFER_SIZE           (32)

typedef struct PlayerSyncStateUpdateUnpacked
{
  VECTOR Position;
  VECTOR Rotation;
  int GameTime;
  float CameraDistance;
  float CameraHeight;
  float CameraYaw;
  float CameraPitch;
  float Health;
  short NoInput;
  u16 PadBits;
  u8 MoveX;
  u8 MoveY;
  u8 GadgetId;
  u8 State;
  char StateId;
  char PlayerIdx;
  char Valid;
  u8 CmdId;
} PlayerSyncStateUpdateUnpacked_t;

typedef struct PlayerSyncPlayerData
{
  VECTOR LastReceivedPosition;
  VECTOR LastLocalPosition;
  VECTOR LastLocalRotation;
  int LastNetTime;
  int LastState;
  int LastStateTime;
  int TicksSinceLastUpdate;
  char LastStateId;
  char Pad[32];
  u8 StateUpdateCmdId;
  PlayerSyncStateUpdateUnpacked_t StateUpdates[CMD_BUFFER_SIZE];
} PlayerSyncPlayerData_t;


// config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;

extern PatchStateContainer_t patchStateContainer;

//--------------------------------------------------------------------------
int playerSyncCmdDelta(int fromCmdId, int toCmdId)
{
  int delta = toCmdId - fromCmdId;
  if (delta < -(CMD_BUFFER_SIZE/2))
    delta += CMD_BUFFER_SIZE;
  else if (delta > (CMD_BUFFER_SIZE/2))
    delta -= CMD_BUFFER_SIZE;

  return delta;
}

//--------------------------------------------------------------------------
int playerSyncGetSendRate(void)
{
  // in survival the mobs already bloat the network, and player syncing is less important, so we can reduce the send rate a lot
  if (gameConfig.customModeId == CUSTOM_MODE_SURVIVAL) return 10;

  // in larger lobbies we want to reduce network bandwidth by reducing send rate
  GameSettings* gs = gameGetSettings();
  if (gs && gs->PlayerCountAtStart > 8) return 5;
  if (gs && gs->PlayerCountAtStart > 6) return 3;
  if (gs && gs->PlayerCountAtStart > 4) return 1;

  return 0;
}

//--------------------------------------------------------------------------
float playerSyncLerpAngleIfDelta(float from, float to, float lerpAmount, float minAngleBetween)
{
  float dt = clampAngle(from - to);
  if (dt < minAngleBetween) return from;

  return lerpfAngle(from, to, lerpAmount);
}

//--------------------------------------------------------------------------
void playerSyncOnPlayerUpdateSetState(Player* player, int toState, int a2, int a3, int t0)
{
  // prevent synced players from bonking on our screens
  // if the bonk, they'll send that as a state update
  if (!player->IsLocal && toState == PLAYER_STATE_JUMP_BOUNCE) return;

  PlayerVTable* vtable = playerGetVTable(player);
  vtable->UpdateState(player, toState, a2, a3, t0);
}

//--------------------------------------------------------------------------
int playerSyncHandlePlayerPadHook(Player* player)
{
  int padIdx = 0;
  
  // enable pad
  if (!player->IsLocal && PLAYER_SYNC_DATAS_PTR) {

    // get player sync data
    PlayerSyncPlayerData_t* data = &PLAYER_SYNC_DATAS_PTR[player->PlayerId];
  
    // process input
    ((void (*)(struct PAD*))0x00527e08)(player->Paddata);

    // update pad
    ((void (*)(struct PAD*, void*, int))0x00527510)(player->Paddata, data->Pad, 0x14);
  }

  int result = ((int (*)(Player*))0x0060cec0)(player);

  return result;
}

//--------------------------------------------------------------------------
void playerSyncHandlePlayerState(Player* player)
{
  MATRIX m, mInv;
  VECTOR dt;
  int i;
  if (!player || player->IsLocal || !player->PlayerMoby || !PLAYER_SYNC_DATAS_PTR) return;

  PlayerSyncPlayerData_t* data = &PLAYER_SYNC_DATAS_PTR[player->PlayerId];
  PlayerSyncStateUpdateUnpacked_t* stateCurrent = &data->StateUpdates[data->StateUpdateCmdId];
  if (!stateCurrent->Valid) return;
  
  int rate = playerSyncGetSendRate();
  PlayerVTable* vtable = playerGetVTable(player);
  float tPos = 0.15;
  float tRot = 0.15;
  float tCam = 0.5;
  int padIdx = 0;

  // reset pad
  data->Pad[2] = 0xFF;
  data->Pad[3] = 0xFF;

  // set no input
  if (data->TicksSinceLastUpdate == 0) {
    player->timers.noInput = stateCurrent->NoInput;
  }

  // extrapolate
  //if (data->TicksSinceLastUpdate > 0 && data->TicksSinceLastUpdate <= rate && !playerIsDead(player)) {
  if (data->TicksSinceLastUpdate > 0 && data->TicksSinceLastUpdate <= rate && !playerIsDead(player)) {
    //DPRINTF("extrapolate %d\n", data->TicksSinceLastUpdate);
    
    // extrapolate position
    vector_subtract(dt, player->PlayerPosition, data->LastLocalPosition);
    vector_add(stateCurrent->Position, stateCurrent->Position, dt);

    // extrapolate rotation
    vector_subtract(dt, player->PlayerRotation, data->LastLocalRotation);
    vector_add(stateCurrent->Rotation, stateCurrent->Rotation, dt);
    //stateCurrent->CameraYaw += dt[2];
  }

  // snap position
  vector_subtract(dt, data->LastReceivedPosition, player->PlayerPosition);
  if (vector_sqrmag(dt) > (7*7)) {
    vector_copy(player->PlayerPosition, data->LastReceivedPosition);
    vector_copy(stateCurrent->Position, data->LastReceivedPosition);
    //DPRINTF("tp\n");
  }

  // lerp position if distance is greater than threshold
  vector_subtract(dt, stateCurrent->Position, player->PlayerPosition);
  if (vector_sqrmag(dt) > (0.1*0.1)) {
    vector_lerp(player->PlayerPosition, player->PlayerPosition, stateCurrent->Position, tPos);
  }

  vector_copy(player->PlayerMoby->Position, player->PlayerPosition);
  vector_copy(player->RemoteHero.receivedSyncPos, player->PlayerPosition);
  vector_copy(player->RemoteHero.posAtSyncFrame, player->PlayerPosition);

  // lerp rotation
  player->PlayerRotation[0] = lerpfAngle(player->PlayerRotation[0], stateCurrent->Rotation[0], tRot);
  player->PlayerRotation[1] = lerpfAngle(player->PlayerRotation[1], stateCurrent->Rotation[1], tRot);
  player->PlayerRotation[2] = lerpfAngle(player->PlayerRotation[2], stateCurrent->Rotation[2], tRot);
  vector_copy(player->RemoteHero.receivedSyncRot, player->PlayerRotation);

  // lerp camera position
  vector_write(player->CameraOffset, 0);
  vector_write(player->CameraRotOffset, 0);
  player->CameraOffset[0] = -stateCurrent->CameraDistance;
  vector_copy(&player->CameraMatrix[12], player->CameraPos);
  vector_copy((float*)((u32)player + 0x2cd0), player->CameraPos);

  // lerp camera rotations
  player->CameraYaw.Value = lerpfAngle(player->CameraYaw.Value, stateCurrent->CameraYaw, tCam);
  player->CameraPitch.Value = -lerpfAngle(-player->CameraPitch.Value, -stateCurrent->CameraPitch, tCam);

  // compute matrix
  matrix_unit(m);
  matrix_rotate_y(m, m, player->CameraPitch.Value);
  matrix_rotate_z(m, m, player->CameraYaw.Value);
  vector_copy(player->CameraForward, &m[0]);
  vector_copy(player->CameraDir, player->CameraForward);
  vector_copy((float*)((u32)player + 0x590 + 0x40), player->CameraForward);

  // compute inv matrix
  matrix_unit(mInv);
  matrix_rotate_y(mInv, mInv, clampAngle(player->CameraPitch.Value + MATH_PI));
  matrix_rotate_z(mInv, mInv, clampAngle(player->CameraYaw.Value + MATH_PI));
  memcpy((void*)((u32)player + 0x2cf0), mInv, sizeof(VECTOR)*3);

  // set camera rotation
  VECTOR camRot;
  camRot[1] = player->CameraPitch.Value;
  camRot[2] = player->CameraYaw.Value;
  camRot[0] = 0;
  vector_copy((float*)((u32)player + 0x2ce0), camRot);

  // set health
  player->Health = stateCurrent->Health;

  // set net camera rotation
  if (player->pNetPlayer) {
    vector_pack(camRot, player->pNetPlayer->padMessageElems[padIdx].msg.cameraRot);
  }

  // set joystick
  float moveX = (stateCurrent->MoveX - 127) / 128.0;
  float moveY = (stateCurrent->MoveY - 127) / 128.0;
  float mag = minf(1, sqrtf((moveX*moveX) + (moveY*moveY)));
  float ang = atan2f(moveY, moveX);
  *(float*)((u32)player + 0x2e08) = mag;
  *(float*)((u32)player + 0x2e0c) = ang;
  *(float*)((u32)player + 0x2e38) = mag;
  *(float*)((u32)player + 0x0120) = moveX;
  *(float*)((u32)player + 0x0124) = -moveY;

  struct tNW_Player* netPlayer = player->pNetPlayer;
  if (netPlayer) {
    netPlayer->padMessageElems[padIdx].msg.pad_data[4] = 0x7F;
    netPlayer->padMessageElems[padIdx].msg.pad_data[5] = 0x7F;
    netPlayer->padMessageElems[padIdx].msg.pad_data[6] = stateCurrent->MoveX;
    netPlayer->padMessageElems[padIdx].msg.pad_data[7] = stateCurrent->MoveY;
  }

  // flail is not synced very well
  // so we're gonna pass R1 pad through to try and sync it up better
  // still not perfect
  if (!player->timers.noInput && (stateCurrent->PadBits & PAD_R1) == 0 && player->WeaponHeldId == WEAPON_ID_FLAIL) {
    data->Pad[3] &= ~0x08;
  }

  // set remote received state
  player->RemoteHero.stateAtSyncFrame = player->PlayerState;
  player->RemoteHero.receivedState = stateCurrent->State;

  // update state
  if (stateCurrent->StateId != data->LastStateId) {
    int skip = 0;
    int playerState = player->PlayerState;

    // from
    switch (playerState)
    {
      case PLAYER_STATE_VEHICLE:
      {
        // leave vehicle
        if (stateCurrent->State != PLAYER_STATE_VEHICLE && player->Vehicle) {
          player->Vehicle->justExited = 1;
          if (player->Vehicle->pDriver == player) player->Vehicle->pDriver = 0;
          if (player->Vehicle->pPassenger == player) player->Vehicle->pPassenger = 0;
          player->InVehicle = 0;
          player->Vehicle = 0;
        }
        break;
      }
      case PLAYER_STATE_SWING:
      {
        // if we're swinging, don't force out of it
        skip = 1;
        break;
      }
      case PLAYER_STATE_GET_HIT:
      {
        // if we're getting hit, don't force out of it
        skip = 1;
        break;
      }
    }

    // to
    switch (stateCurrent->State)
    {
      case PLAYER_STATE_JUMP_ATTACK:
      case PLAYER_STATE_COMBO_ATTACK:
      {
        //DPRINTF("wrench %d: pstate:%d pstatetime:%d lasttime:%d\n", gameGetTime(), playerState, player->timers.state, data->LastStateTime);
        skip = 1;
        if (stateCurrent->State == playerState && player->timers.state < data->LastStateTime) {
          data->LastStateId = stateCurrent->StateId;
          data->LastState = stateCurrent->State;
        } else if ((player->timers.state % 10) != 0) {
          data->Pad[3] &= ~0x80;
        }
        break;
      }
      case PLAYER_STATE_FLAIL_ATTACK:
      {
        // let pad handle flail
        skip = 1;
        break;
      }
      case PLAYER_STATE_GET_HIT:
      {
        // let game handle flinchings
        //skip = 1;
        break;
      }
      case PLAYER_STATE_SWING:
      {
        // force R1 when on swingshot
        // let game handle the rest
        data->Pad[3] &= ~0x08;
        skip = 1;
        break;
      }
    }

    if (!skip) {
      //DPRINTF("%d new state %d (from %d)\n", player->PlayerId, stateCurrent->State, player->PlayerState);

      if (player->PlayerSubstate > 1) {
        //DPRINTF("substate fix %d=>0\n", player->PlayerSubstate);
        player->PlayerSubstate = 0;
      }

      vtable->UpdateState(player, stateCurrent->State, 1, 1, 1);
      data->LastStateId = stateCurrent->StateId;
      data->LastState = stateCurrent->State;

    } else {
      data->LastStateTime = player->timers.state;
    }
  }

  // handles case where player fires while still chargebooting
  // causing them to stop on remote client's screen
  // if they do fire and stop, the client should tell the remote clients
  // the remote clients should assume they didn't stop
  // this fixes wrench lag
  if (player->timers.state < 0x3D && player->PlayerState == PLAYER_STATE_CHARGE) {
    *(char*)((u32)player + 0x265a) = 0;
  }

  //
  if (player->pNetPlayer && player->pNetPlayer->pNetPlayerData) {
    player->pNetPlayer->pNetPlayerData->handGadget = stateCurrent->GadgetId;
    player->pNetPlayer->pNetPlayerData->lastKeepAlive = data->LastNetTime;
    player->pNetPlayer->pNetPlayerData->timeStamp = data->LastNetTime;
    player->pNetPlayer->pNetPlayerData->hitPoints = stateCurrent->Health;
    vector_copy(player->pNetPlayer->pNetPlayerData->vPosition, player->PlayerPosition);
  }

  vector_copy(data->LastLocalPosition, player->PlayerPosition);
  vector_copy(data->LastLocalRotation, player->PlayerRotation);
  data->TicksSinceLastUpdate += 1;
}

//--------------------------------------------------------------------------
int playerSyncOnReceivePlayerState(void* connection, void* data)
{
  if (!isInGame() || !PLAYER_SYNC_DATAS_PTR) return sizeof(PlayerSyncStateUpdatePacked_t);
  if (!gameConfig.grNewPlayerSync) return sizeof(PlayerSyncStateUpdatePacked_t);

  PlayerSyncStateUpdatePacked_t msg;
  PlayerSyncStateUpdateUnpacked_t unpacked;
  memcpy(&msg, data, sizeof(msg));

  // unpack
  memcpy(unpacked.Position, msg.Position, sizeof(float) * 3);
  memcpy(unpacked.Rotation, msg.Rotation, sizeof(float) * 3);
  unpacked.GameTime = msg.GameTime;
  unpacked.CameraDistance = msg.CameraDistance / 1024.0;
  unpacked.CameraYaw = msg.CameraYaw / 10240.0;
  unpacked.CameraPitch = msg.CameraPitch / 10240.0;
  unpacked.NoInput = msg.NoInput;
  unpacked.Health = msg.Health;
  unpacked.MoveX = msg.MoveX;
  unpacked.MoveY = msg.MoveY;
  unpacked.PadBits = (msg.PadBits1 << 8) | (msg.PadBits0);
  unpacked.GadgetId = msg.GadgetId;
  unpacked.State = msg.State;
  unpacked.StateId = msg.StateId;
  unpacked.PlayerIdx = msg.PlayerIdx;
  unpacked.CmdId = msg.CmdId;
  unpacked.Valid = 1;

  // target sync player data
  PlayerSyncPlayerData_t* data = &PLAYER_SYNC_DATAS_PTR[msg.PlayerIdx];

  // move into buffer
  memcpy(data->StateUpdates[msg.CmdId], &unpacked, sizeof(unpacked));
  int cmdDt = playerSyncCmdDelta(data->StateUpdateCmdId, unpacked.CmdId);
  //DPRINTF("%d => %d (%d)\n", data->StateUpdateCmdId, unpacked.CmdId, cmdDt);
  if (cmdDt > 0) {
    data->LastNetTime = unpacked.GameTime;
    data->StateUpdateCmdId = unpacked.CmdId;
    data->TicksSinceLastUpdate = 0;
    vector_copy(data->LastReceivedPosition, unpacked.Position);
  }

  //DPRINTF("recv player sync state %d time:%d dt:%d\n", msg.PlayerIdx, msg.GameTime, msg.GameTime - gameGetTime());
  return sizeof(PlayerSyncStateUpdatePacked_t);
}

//--------------------------------------------------------------------------
void playerSyncBroadcastPlayerState(Player* player)
{
  VECTOR dt;
  PlayerSyncStateUpdatePacked_t msg;
  void * connection = netGetDmeServerConnection();
  if (!connection || !player || !player->IsLocal || !PLAYER_SYNC_DATAS_PTR) return;

  PlayerSyncPlayerData_t* data = &PLAYER_SYNC_DATAS_PTR[player->PlayerId];

  // set cur frame
  player->LocalHero.UNK_LOCALHERO[0x1EE] = 0;

  // detect state change
  if (data->LastState != player->PlayerState || player->timers.state < data->LastStateTime) {
    data->LastState = player->PlayerState;
    data->LastStateId = (data->LastStateId + 1) % 256;
  }
  data->LastStateTime = player->timers.state;

  // compute camera yaw and pitch
  float yaw = player->Camera->rot[2];
  float pitch = player->Camera->rot[1];

  // compute camera distance
  vector_subtract(dt, player->Camera->pos, player->CameraPos);
  float dist = vector_length(dt);

  memcpy(msg.Position, player->PlayerPosition, sizeof(float) * 3);
  memcpy(msg.Rotation, player->PlayerRotation, sizeof(float) * 3);
  msg.GameTime = data->LastNetTime = gameGetTime();
  msg.PlayerIdx = player->PlayerId;
  msg.CameraDistance = (short)(dist * 1024.0);
  msg.CameraPitch = (short)(pitch * 10240.0);
  msg.CameraYaw = (short)(yaw * 10240.0);
  msg.CameraPitch = (short)(pitch * 10240.0);
  msg.NoInput = player->timers.noInput;
  msg.Health = (short)player->Health;
  msg.MoveX = ((struct PAD*)player->Paddata)->rdata[6];
  msg.MoveY = ((struct PAD*)player->Paddata)->rdata[7];
  msg.PadBits0 = ((struct PAD*)player->Paddata)->rdata[2];
  msg.PadBits1 = ((struct PAD*)player->Paddata)->rdata[3];
  msg.GadgetId = player->Gadgets[0].id;
  msg.State = player->PlayerState;
  msg.StateId = data->LastStateId;
  msg.CmdId = data->StateUpdateCmdId = (data->StateUpdateCmdId + 1) % CMD_BUFFER_SIZE;

  netBroadcastCustomAppMessage(0, connection, CUSTOM_MSG_PLAYER_SYNC_STATE_UPDATE, sizeof(msg), &msg);
}

//--------------------------------------------------------------------------
int playerSyncDisablePlayerStateUpdates(void)
{
  return 0;
}

//--------------------------------------------------------------------------
void playerSyncTick(void)
{
  static int delay = 50;
  static int initialized = 0;
  int i;

  // net
  netInstallCustomMsgHandler(CUSTOM_MSG_PLAYER_SYNC_STATE_UPDATE, &playerSyncOnReceivePlayerState);

  if (!isInGame()) {
    delay = 50;
    initialized = 0;
    return;
  }

#if DEBUG
  // always on
  gameConfig.grNewPlayerSync = 1;
#endif

  if (!gameConfig.grNewPlayerSync) return;
  
  // allocate buffer
  if (PLAYER_SYNC_DATAS_PTR == 0) {
    PLAYER_SYNC_DATAS_PTR = malloc(sizeof(PlayerSyncPlayerData_t) * GAME_MAX_PLAYERS);
    initialized = 0;
  }

  // not enough memory to allocate
  // fail and disable
  if (PLAYER_SYNC_DATAS_PTR == 0) {
    gameConfig.grNewPlayerSync = 0;
    return;
  }

  // reset buffer
  if (!initialized && PLAYER_SYNC_DATAS_PTR) {
    memset(PLAYER_SYNC_DATAS_PTR, 0, sizeof(PlayerSyncPlayerData_t) * GAME_MAX_PLAYERS);
  }

  // init
  initialized = 1;

  // hooks
  HOOK_JAL(0x0060eb80, &playerSyncDisablePlayerStateUpdates);
  HOOK_JAL(0x0060684c, &playerSyncHandlePlayerPadHook);
  //HOOK_JAL(0x0060cd44, &playerSyncOnPlayerUpdateSetState);

  // player link always healthy
  POKE_U32(0x005F7BDC, 0x24020001);

  // disable tnw_PlayerData update gadgetid
  POKE_U32(0x0060F010, 0);

  // disable tnw_PlayerData time update
  POKE_U32(0x0060FFAC, 0);

  // disable send GetHit
  POKE_U32(0x0060ff08, 0);

  // player updates
  Player** players = playerGetAll();
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    playerSyncHandlePlayerState(players[i]);
  }

  // delay
  if (delay) {
    --delay;
    return;
  } else {
    delay = playerSyncGetSendRate();
  }

  // player updates
  for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    playerSyncBroadcastPlayerState(players[i]);
  }
}
