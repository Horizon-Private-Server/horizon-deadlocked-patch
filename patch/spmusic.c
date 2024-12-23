#include <libdl/game.h>
#include <libdl/gamesettings.h>
#include <libdl/stdio.h>
#include <libdl/string.h>
#include <libdl/team.h>
#include <libdl/ui.h>
#include <libdl/pad.h>
#include <libdl/music.h>
#include <libdl/utils.h>
#include "config.h"

#define SP_MUSIC_TRACK_COUNT                (64)

extern PatchConfig_t config;
extern PatchGameConfig_t gameConfig;

int spMusicTrackTable[SP_MUSIC_TRACK_COUNT][2] = {
  { 0x0000A59E, 0x0000AB5A },
  { 0x0000B1BE, 0x0000B77A },
  { 0x0000BE86, 0x0000C3C4 },
  { 0x0000C99A, 0x0000CEC1 },
  { 0x0000D470, 0x0000D937 },
  { 0x0000DE88, 0x0000E3A5 },
  { 0x0000E95C, 0x0000EE25 },
  { 0x0000F3FA, 0x0000F92D },
  { 0x0000FEE8, 0x000103AF },
  { 0x00010910, 0x00010E26 },
  { 0x000113D4, 0x000118DF },
  { 0x00011EFA, 0x000123C1 },
  { 0x0001299C, 0x00012E74 },
  { 0x00013411, 0x0001393B },
  { 0x00013EE9, 0x0001439F },
  { 0x00014985, 0x00014DF8 },
  { 0x00015377, 0x000158AA },
  { 0x00015EAE, 0x00016381 },
  { 0x000168EC, 0x00016DA2 },
  { 0x000172E8, 0x000177EE },
  { 0x00017E04, 0x000182CB },
  { 0x00018881, 0x00018D37 },
  { 0x00019287, 0x00019750 },
  { 0x00019CA5, 0x0001A18F },
  { 0x0001A703, 0x0001AC20 },
  { 0x0001B22C, 0x0001B6E2 },
  { 0x0001BCC6, 0x0001C213 },
  { 0x0001C81E, 0x0001CCEC },
  { 0x0001D244, 0x0001D7E1 },
  { 0x0001DE16, 0x0001E2CC },
  { 0x0001E907, 0x0001EE45 },
  { 0x0001F4A3, 0x0001F9B3 },
  { 0x0001FFE9, 0x000204D6 },
  { 0x00020A4B, 0x00020F12 },
  { 0x0002145F, 0x00021946 },
  { 0x00021F24, 0x0002246B },
  { 0x00022A4A, 0x00022F55 },
  { 0x000234FC, 0x000239A8 },
  { 0x00023F02, 0x000243B8 },
  { 0x00024961, 0x00024E2F },
  { 0x000253B1, 0x00025894 },
  { 0x00025E0B, 0x000262C1 },
  { 0x0002680F, 0x00026CC5 },
  { 0x00027215, 0x0002772B },
  { 0x00027D0E, 0x00028235 },
  { 0x000287EC, 0x00028D3A },
  { 0x00029314, 0x000297FE },
  { 0x00029D90, 0x0002A27A },
  { 0x0002A7E6, 0x0002AC6C },
  { 0x0002B1B1, 0x0002B74E },
  { 0x0002BDE1, 0x0002C282 },
  { 0x0002C7B9, 0x0002CC5C },
  { 0x0002D175, 0x0002D604 },
  { 0x0002DB1B, 0x0002DFE2 },
  { 0x0002E598, 0x0002EA4E },
  { 0x0002EFB8, 0x0002F46E },
  { 0x0002F9C8, 0x0002FE69 },
  { 0x000303BA, 0x00030886 },
  { 0x00030E0F, 0x000312D6 },
  { 0x0003182B, 0x00031D04 },
  { 0x0003227F, 0x00032735 },
  { 0x00032CFB, 0x000332D2 },

  // these are extras added
  { 0x0009BBAC - 0x000F8D29, 0x0009C017 - 0x000F8D29 }, // challenge complete
  { 0x0009C482 - 0x000F8D29, 0x0009C7C7 - 0x000F8D29 }, // challenge failed
};

//--------------------------------------------------------------------------
void spMusicLoad(void)
{
  int i;
  u32 newTracksAddr = 0x001CF940;
  for (i = 0; i < SP_MUSIC_TRACK_COUNT; ++i) {
    POKE_U32(newTracksAddr + 0x00, spMusicTrackTable[i][0]);
    POKE_U32(newTracksAddr + 0x08, spMusicTrackTable[i][1]);
    newTracksAddr += 0x10;
  }

  ((void (*)(int,int,int))0x0051f928)(4,13 + SP_MUSIC_TRACK_COUNT,0x400);
  POKE_U16(0x004A8328, 13 + SP_MUSIC_TRACK_COUNT - 2);
  //musicPlayTrack(MUSIC_TRACK_DREADZONE_STATION, 1);

  DPRINTF("SPMUSIC loaded %d tracks\n", SP_MUSIC_TRACK_COUNT);
}

//--------------------------------------------------------------------------
void spMusicRun(void)
{
  static int hasLoaded = 0;
  int shouldLoadSpMusic = config.enableSingleplayerMusic || gameConfig.customModeId == CUSTOM_MODE_RAIDS;
  if (!shouldLoadSpMusic) return;
  
  if (!isInGame()) {
    hasLoaded = 0;
  } else if (!hasLoaded) {
    spMusicLoad();
    hasLoaded = 1;
  }
}
