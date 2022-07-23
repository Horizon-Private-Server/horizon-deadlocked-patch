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
#include <libdl/pad.h>
#include <libdl/hud.h>
#include <libdl/ui.h>
#include "config.h"

#define FREECAM_MOVE_SPEED                      (0.1)
#define FREECAM_FAST_MOVE_SPEED                 (2)

int FreecamInitialized = 0;

// config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;

// This contains the spectate related info per local
struct PlayerFreecamData
{
    int Enabled;
    VECTOR LastCameraPos;
} FreecamData[2];

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
  player->timers.noInput = 0x7FFF;
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
  hudGetPlayerFlags(player->LocalPlayerIndex)->Flags.Raw = 0x303;
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
  if (!currentPlayer)
    return;

  struct PlayerFreecamData * freecamData = FreecamData + currentPlayer->LocalPlayerIndex;
  PadButtonStatus * pad = playerGetPad(currentPlayer);

  // move camera to exact position by removing distance
  currentPlayer->CameraOffset[0] = 0;

  // move
  float v = padJoystickToFloat(pad->ljoy_v);
  float h = padJoystickToFloat(pad->ljoy_h);
  float speed = FREECAM_MOVE_SPEED;

  if ((pad->btns & PAD_L1) == 0)
    speed = FREECAM_FAST_MOVE_SPEED;

  if (fabsf(v) > 0.001) {
    vector_scale(t1, currentPlayer->CameraMatrix, -v * speed);

    vector_add(freecamData->LastCameraPos, freecamData->LastCameraPos, t1);
  }
  if (fabsf(h) > 0.001) {
    vector_scale(t1, &currentPlayer->CameraMatrix[4], -h * speed);

    vector_add(freecamData->LastCameraPos, freecamData->LastCameraPos, t1);
  }
  if ((pad->btns & PAD_L2) == 0) {
    freecamData->LastCameraPos[2] -= speed;
  }
  if ((pad->btns & PAD_R2) == 0) {
    freecamData->LastCameraPos[2] += speed;
  }

  // Interpolate camera towards target player
  vector_copy(currentPlayer->CameraPos, freecamData->LastCameraPos);
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
    if (playerIsLocal(player) && !playerIsDead(player)) 
    {
      // Grab player-specific spectate data
      freecamData = FreecamData + player->LocalPlayerIndex;

      if (!freecamData->Enabled)
      {
        // Enable freecam
        if (playerPadGetButtonDown(player, PAD_L1 | PAD_UP) > 0) 
        {
          enableFreecam(player, freecamData);
          vector_copy(freecamData->LastCameraPos, player->CameraPos);
        }
      }
      // Let the player exit spectate by pressing square
      else if (playerPadGetButtonDown(player, PAD_SQUARE) > 0)
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
