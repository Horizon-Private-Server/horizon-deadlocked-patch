/***************************************************
 * FILENAME :		freecam.c
 * 
 * DESCRIPTION :
 * 		Freecam mode entry point and logic.
 * 
 * NOTES :
 * 
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/string.h>
#include <libdl/player.h>
#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/stdio.h>
#include <libdl/camera.h>
#include <libdl/graphics.h>
#include <libdl/pad.h>
#include <libdl/hud.h>
#include <libdl/utils.h>
#include <libdl/ui.h>
#include "config.h"
#include "include/config.h"

#define FREECAM_MOVE_SPEED                      (0.1)
#define FREECAM_FAST_MOVE_SPEED                 (1)

int FreecamInitialized = 0;

// config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;

// config menu
extern int isConfigMenuActive;

// config menu settings
FreecamSettings_t freecamSettings = {
  .lockPosition = 0,
  .airwalk = 0,
  .lockStateToggle = 0,
};

// This contains the spectate related info per local
struct PlayerFreecamData
{
    int Enabled;
    int ControlCharacter;
    int LockStateId;
    VECTOR LastCameraPos;
    float CharcterCameraPitch;
    float CharacterCameraYaw;
    float SpectateCameraPitch;
    float SpectateCameraYaw;
    VECTOR LockedCharacterPosition;
} FreecamData[2];

void freecamGetRotationMatrix(MATRIX output, struct PlayerFreecamData * freecamData)
{
  matrix_unit(output);
  matrix_rotate_y(output, output, freecamData->SpectateCameraPitch);
  matrix_rotate_z(output, output, freecamData->SpectateCameraYaw);
}

void freecamUpdateCamera_PostHook(void)
{
  int i;
  MATRIX m, mBackup;
  VECTOR pBackup;
  for (i = 0; i < playerGetNumLocals(); ++i)
  {
    struct PlayerFreecamData * freecamData = FreecamData + i;
    if (!freecamData->Enabled)
      continue;

    // get game camera
    GameCamera * camera = cameraGetGameCamera(i);

    // backup position + rotation
    vector_copy(pBackup, camera->pos);
    memcpy(mBackup, camera->uMtx, sizeof(float) * 12);

    // write spectate position
    vector_copy(camera->pos, freecamData->LastCameraPos);

    // create and write spectate rotation matrix
    freecamGetRotationMatrix(m, freecamData);
    memcpy(camera->uMtx, m, sizeof(float) * 12);

    // update render camera
    ((void (*)(int))0x004b2010)(i);

    // restore backup
    vector_copy(camera->pos, pBackup);
    memcpy(camera->uMtx, mBackup, sizeof(float) * 12);
  }

}

/*
 * NAME :		enableFreecam
 * 
 * DESCRIPTION :
 * 			
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void enableFreecam(Player * player, struct PlayerFreecamData * data)
{
  data->Enabled = 1;
  data->ControlCharacter = 0;
  //data->LockStateId = -1;
  vector_copy(data->LastCameraPos, player->CameraPos);
  vector_copy(data->LockedCharacterPosition, player->PlayerPosition);
  data->CharcterCameraPitch = player->CameraPitch.Value;
  data->CharacterCameraYaw = player->CameraYaw.Value;
  data->SpectateCameraPitch = player->CameraPitch.Value;
  data->SpectateCameraYaw = player->CameraYaw.Value;
  POKE_U32(0x004C327C, 0);
  POKE_U32(0x004B3970, 0); // disable reticule
  HOOK_J(0x004b2428, &freecamUpdateCamera_PostHook);
  hudGetPlayerFlags(player->LocalPlayerIndex)->Flags.Raw = 0;
}

/*
 * NAME :		disableFreecam
 * 
 * DESCRIPTION :
 * 			
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void disableFreecam(Player * player, struct PlayerFreecamData * data)
{
  data->Enabled = 0;
  player->timers.noInput = 0;
  player->CameraOffset[0] = -6;
  player->CameraPitch.Value = data->CharcterCameraPitch;
  player->CameraYaw.Value = data->CharacterCameraYaw;
  POKE_U32(0x004C327C, 0x0C1302AA);
  POKE_U32(0x004B3970, 0x0C12CB28); // enable reticule
  POKE_U32(0x004b2428, 0x03E00008);
  hudGetPlayerFlags(player->LocalPlayerIndex)->Flags.Raw = 0x333;
}

/*
 * NAME :		padJoystickToFloat
 * 
 * DESCRIPTION :
 * 			
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
float padJoystickToFloat(char value)
{
  if (value < 0)
    return ((u8)value - 128.0) / 128.0;
  
  return (127.0 - value) / -127.0;
}

/*
 * NAME :		padJoystickSmoothCurve
 * 
 * DESCRIPTION :
 * 			
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
float padJoystickSmoothCurve(float value)
{
  float ramped = powf(value, 2);
  if (value < 0)
    return -ramped;

  return ramped;
}

/*
 * NAME :		freecam
 * 
 * DESCRIPTION :
 * 			
 * 
 * NOTES :
 * 
 * ARGS : 
 * 		
 * 
 * RETURN :
 * 
 * AUTHOR :			Connor "Badger41" Williams
 */
void freecam(Player * currentPlayer)
{
  VECTOR t1;
  MATRIX rotationMatrix;
  if (!currentPlayer)
    return;

  memset((void*)0x00240a40, -1, 0x80);
  struct PlayerFreecamData * freecamData = FreecamData + currentPlayer->LocalPlayerIndex;
  PadButtonStatus * pad = playerGetPad(currentPlayer);

  // move camera to exact position by removing distance
  currentPlayer->CameraOffset[0] = 0;

  // get rotation matrix
  freecamGetRotationMatrix(rotationMatrix, freecamData);

  // lock character
  if (freecamSettings.lockPosition)
  {
    // set position
    vector_copy(currentPlayer->PlayerPosition, freecamData->LockedCharacterPosition);

    // prevent dying
    currentPlayer->timers.stuck = 0;
  } else { vector_copy(freecamData->LockedCharacterPosition, currentPlayer->PlayerPosition); }

  // lock character state
  if (freecamSettings.lockStateToggle)
  {
    if (freecamData->LockStateId < 0)
    {
      gfxScreenSpaceText(5, SCREEN_HEIGHT - 5, 1, 1, 0x80FFFFFF, "Press \x1c to lock animation", -1, 6);
    }
    else if (currentPlayer->PlayerState != freecamData->LockStateId)
    {
			PlayerVTable* vtable = playerGetVTable(currentPlayer);
      vtable->UpdateState(currentPlayer, freecamData->LockStateId, 1, 1, 1);
    }

    if (padGetButtonDown(currentPlayer->LocalPlayerIndex, PAD_UP) > 0) {
      freecamData->LockStateId = currentPlayer->PlayerState;
    }
    else if (padGetButtonDown(currentPlayer->LocalPlayerIndex, PAD_DOWN) > 0) {
      freecamData->LockStateId = -1;
    }
  }
  else { freecamData->LockStateId = -1; }

  // handle control
  if (freecamData->ControlCharacter)
  {
    // enable character input
    currentPlayer->timers.noInput = 0;

    // toggle off
    if (padGetButtonDown(currentPlayer->LocalPlayerIndex, PAD_CIRCLE) > 0) {
      freecamData->ControlCharacter = 0;

      // save character rotation to start
      freecamData->CharcterCameraPitch = currentPlayer->CameraPitch.Value;
      freecamData->CharacterCameraYaw = currentPlayer->CameraYaw.Value;

      // restore rotation to spectate camera rotation
      currentPlayer->CameraPitch.Value = freecamData->SpectateCameraPitch;
      currentPlayer->CameraYaw.Value = freecamData->SpectateCameraYaw;
    }
  }
  else
  {
    // toggle character mode
    if (padGetButtonDown(currentPlayer->LocalPlayerIndex, PAD_CIRCLE) > 0) {
      freecamData->ControlCharacter = 1;

      // restore rotation to character
      currentPlayer->CameraPitch.Value = freecamData->CharcterCameraPitch;
      currentPlayer->CameraYaw.Value = freecamData->CharacterCameraYaw;
      return;
    }

    // disable character input
    currentPlayer->timers.noInput = 0x7FFF;

    // move
    float mv = padJoystickToFloat(pad->ljoy_v);
    float mh = padJoystickToFloat(pad->ljoy_h);
    float lv = padJoystickSmoothCurve(padJoystickToFloat(pad->rjoy_v));
    float lh = padJoystickSmoothCurve(padJoystickToFloat(pad->rjoy_h));
    float speed = FREECAM_MOVE_SPEED;

    if (padGetButton(currentPlayer->LocalPlayerIndex, PAD_L1) > 0)
      speed = FREECAM_FAST_MOVE_SPEED;

    // move joystick
    if (fabsf(mv) > 0.001) {
      vector_scale(t1, rotationMatrix, -mv * speed);
      vector_add(freecamData->LastCameraPos, freecamData->LastCameraPos, t1);
    }
    if (fabsf(mh) > 0.001) {
      vector_scale(t1, &rotationMatrix[4], -mh * speed);
      vector_add(freecamData->LastCameraPos, freecamData->LastCameraPos, t1);
    }

    // move up/down
    if (padGetButton(currentPlayer->LocalPlayerIndex, PAD_L2) > 0)
      freecamData->LastCameraPos[2] -= FREECAM_MOVE_SPEED;
    if (padGetButton(currentPlayer->LocalPlayerIndex, PAD_R2) > 0)
      freecamData->LastCameraPos[2] += FREECAM_MOVE_SPEED;

    // look joystick
    float lookSpeed = 0.1;
    if (fabsf(lv) > 0.001) {
      float angle = clampAngle(freecamData->SpectateCameraPitch + lv*lookSpeed);
      const float limit = 80 * MATH_DEG2RAD;
      if (angle > limit)
        angle = limit;
      else if (angle < -limit)
        angle = -limit;

      freecamData->SpectateCameraPitch = angle;
    }
    if (fabsf(lh) > 0.001) {
      freecamData->SpectateCameraYaw = clampAngle(freecamData->SpectateCameraYaw + -lh*lookSpeed);
    }
  }
}

void freecamInitialize(void)
{
  memset(FreecamData, 0, sizeof(FreecamData));
  FreecamInitialized = 1;
}

void processFreecam(void) 
{
  GameSettings * gameSettings = gameGetSettings();
	Player ** players = playerGetAll();
  struct PlayerFreecamData * freecamData = NULL;
  int i = 0;

  // First, we have to ensure we are in-game
	if (!gameSettings || !isInGame()) 
  {
    FreecamData[0].Enabled = 0;
    FreecamData[1].Enabled = 0;
    FreecamInitialized = 0;
    return;
  }

  if (FreecamInitialized != 1)
    freecamInitialize();

  // Loop through every player
  for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
    if (!players[i])
      continue;

		Player * player = players[i];

    // Next, we have to ensure the player is the local player and they are not dead
    if (playerIsLocal(player)) 
    {
      // Grab player-specific spectate data
      freecamData = FreecamData + player->LocalPlayerIndex;

      // airwalk
      if (freecamSettings.airwalk)
        POKE_U32((u32)player + 0x2FC, 0);

      // only process input when in game
      if (gameIsStartMenuOpen() || isConfigMenuActive)
        continue;

      if (!playerIsDead(player))
      {
        if (!freecamData->Enabled)
        {
          // Enable freecam
          if (playerPadGetButtonDown(player, PAD_L1 | PAD_UP) > 0) 
          {
            enableFreecam(player, freecamData);
          }
        }
        // Let the player exit spectate by pressing circle
        else if (playerPadGetButtonDown(player, PAD_TRIANGLE) > 0)
        {
          disableFreecam(player, freecamData);
        }
        else
        {
          freecam(player);
        }
      }
      else if (freecamData->Enabled)
      {
        disableFreecam(player, freecamData);
      }
    }
  }
}
