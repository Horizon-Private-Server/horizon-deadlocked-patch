#ifndef __PATCH_CONFIG_H__
#define __PATCH_CONFIG_H__

#include "messageid.h"
#include <libdl/gamesettings.h>

#define MAX_CUSTOM_MAP_DEFINITIONS              (64)
#define CONFIG_ITEM_HELP_SIZE                   (64)

enum PatchGameNetMessage
{
  // RotatingWeaponsChanged
  CUSTOM_MSG_ROTATING_WEAPONS_CHANGED = CUSTOM_MSG_ID_PATCH_IN_GAME_START,
  CUSTOM_MSG_CLIENT_READY,
  CUSTOM_MSG_PLAYER_VOTED_TO_END,
  CUSTOM_MSG_VOTE_TO_END_STATE_UPDATED,
  CUSTOM_MSG_DZO_COSMETICS_UPDATE,
  CUSTOM_MSG_PLAYER_SYNC_STATE_UPDATE,
  CUSTOM_MSG_PLAYER_LATENCY_TEST_PING,
  CUSTOM_MSG_PLAYER_QUICK_CHAT,
  CUSTOM_MSG_B6_BALL_FIRED
};

enum ActionType
{
  ACTIONTYPE_DRAW,
  ACTIONTYPE_GETHEIGHT,
  ACTIONTYPE_SELECT,
  ACTIONTYPE_SELECT_SECONDARY,
  ACTIONTYPE_INCREMENT,
  ACTIONTYPE_DECREMENT,
  ACTIONTYPE_VALIDATE
};

enum ElementState
{
  ELEMENT_HIDDEN = 0,
  ELEMENT_VISIBLE = (1 << 0),
  ELEMENT_EDITABLE = (1 << 1),
  ELEMENT_SELECTABLE = (1 << 2),
};

enum LabelType
{
  LABELTYPE_HEADER,
  LABELTYPE_LABEL
};

struct MenuElem;
struct TabElem;
struct MenuElem_ListData;
struct MenuElem_OrderedListData;
struct MenuElem_RangeData;

typedef void (*ActionHandler)(struct TabElem* tab, struct MenuElem* element, int actionType, void * actionArg);
typedef void (*ButtonSelectHandler)(struct TabElem* tab, struct MenuElem* element);
typedef void (*MenuElementStateHandler)(struct TabElem* tab, struct MenuElem* element, int * state);
typedef int (*MenuElementListStateHandler)(struct MenuElem_ListData* listData, char* value);
typedef int (*MenuElementOrderedListStateHandler)(struct MenuElem_OrderedListData* listData, char* value);
typedef int (*MenuElementRangeStateHandler)(struct MenuElem_RangeData* listData, char* value);
typedef void (*TabStateHandler)(struct TabElem* tab, int * state);

typedef struct MenuElem
{
  char name[48];
  ActionHandler handler;
  MenuElementStateHandler stateHandler;
  void * userdata;
  char * help;
} MenuElem_t;

typedef struct MenuElem_ListData
{
  char * value;
  MenuElementListStateHandler stateHandler;
  int count;
  char * items[];
} MenuElem_ListData_t;

typedef struct MenuElem_OrderedListDataItem
{
  char value;
  char* name;
} MenuElem_OrderedListDataItem_t;

typedef struct MenuElem_OrderedListData
{
  char * value;
  MenuElementOrderedListStateHandler stateHandler;
  int count;
  MenuElem_OrderedListDataItem_t items[];
} MenuElem_OrderedListData_t;

typedef struct MenuElem_RangeData
{
  char * value;
  MenuElementRangeStateHandler stateHandler;
  char minValue;
  char maxValue;
  char stepValue;
} MenuElem_RangeData_t;

typedef struct TabElem
{
  char name[32];
  TabStateHandler stateHandler;
  MenuElem_t * elements;
  int elementsCount;
  int selectedMenuItem;
  int menuOffset;
} TabElem_t;

typedef struct CustomMapVersionFileDef
{
  int Version;
  int BaseMapId;
  int ForcedCustomModeId;
  short ExtraDataCount;
  short ShrubMinRenderDistance;
  char Name[32];
} CustomMapVersionFileDef_t;

typedef void (*SndCompleteProc)(int loc, int user_data);

typedef struct MapLoaderState
{
    u8 Enabled;
    u8 MapId;
		u8 CheckState;
    char MapName[32];
    char MapFileName[128];
    int LoadingFileSize;
    int LoadingFd;
    void * LevelBuffer;
    void * SoundBuffer;
    SndCompleteProc SoundLoadCb;
    u64 SoundLoadUserData;
} MapLoaderState_t;

typedef struct FreecamSettings
{
  char lockPosition;
  char airwalk;
  char lockStateToggle;
} FreecamSettings_t;

typedef struct VoteToEndState
{
  int TimeoutTime;
  int Count;
  char Votes[GAME_MAX_PLAYERS];
} VoteToEndState_t;

typedef struct FireB6Ball
{
  float Position[3];
  float Velocity[3];
  short Level;
  char FromPlayerId;
} FireB6Ball_t;

typedef struct CustomDzoCommandDrawVoteToEnd
{
  int SecondsLeft;
  int Count;
  int VotesRequired;
} CustomDzoCommandDrawVoteToEnd_t;

typedef struct CustomDzoCommandDrawQuickChat
{
  char FromPlayerId;
  char ChatId;
} CustomDzoCommandDrawQuickChat_t;

typedef struct PlayerSyncStateUpdatePacked
{
  float Position[3];
  float Rotation[3];
  int GameTime;
  short CameraDistance;
  short CameraYaw;
  short CameraPitch;
  short NoInput;
  short Health;
  u8 PadBits0;
  u8 PadBits1;
  u8 MoveX;
  u8 MoveY;
  u8 GadgetId;
  char State;
  char StateId;
  char PlayerIdx;
  u8 CmdId;
} PlayerSyncStateUpdatePacked_t;

extern int isConfigMenuActive;
extern CustomMapDef_t customMapDefs[MAX_CUSTOM_MAP_DEFINITIONS];
extern int customMapDefCount;
extern MapLoaderState_t MapLoaderState;

extern u32 SONY_MAC_ADDRESSES[];
extern int SONY_MAC_ADDRESSES_COUNT;

#endif // __PATCH_CONFIG_H__
