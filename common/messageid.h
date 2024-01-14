
/***************************************************
 * FILENAME :		messageid.h
 * 
 * DESCRIPTION :
 * 		
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#ifndef _MESSAGEID_H_
#define _MESSAGEID_H_


#define PATCH_GAME_STATS_VERSION              (1)

/*
 * NAME :		CustomMessageId
 * 
 * DESCRIPTION :
 * 			Contains the different message ids.
 * 
 * NOTES :
 * 
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
enum CustomMessageId
{
    UNUSED = 0,

    /*
     * Info on custom map.
     */
    CUSTOM_MSG_ID_SET_MAP_OVERRIDE = 1,

    /*
     * Sent to the server when the client wants the map loader irx modules.
     */
    CUSTOM_MSG_ID_CLIENT_REQUEST_MAP_IRX_MODULES = 2,

    /*
     * Sent after the server has sent all the map loader irx modules.
     */
    CUSTOM_MSG_ID_SERVER_SENT_MAP_IRX_MODULES = 3,

    /*
     * Sent in response to CUSTOM_MSG_ID_SET_MAP_OVERRIDE by the client.
     */
    CUSTOM_MSG_ID_SET_MAP_OVERRIDE_RESPONSE = 4,

    /*
     * Sent by the host when a game lobby has started.
     */
    CUSTOM_MSG_ID_GAME_LOBBY_STARTED = 5,

    /*
     * Sent from the client to the server when the client has updated their patch config.
     */
    CUSTOM_MSG_ID_CLIENT_USER_CONFIG = 6,

    /*
     * Sent from the client to the server when the client wants to update the patch.
     */
    CUSTOM_MSG_ID_CLIENT_REQUEST_PATCH = 7,

    /*
     * Sent from the server to the host when the server wants the host to update all the teams
     */
    CUSTOM_MSG_ID_SERVER_REQUEST_TEAM_CHANGE = 8,
    
    /*
     * Sent from the client to the server when the client has updated their patch game config.
     */
    CUSTOM_MSG_ID_CLIENT_USER_GAME_CONFIG = 9,

    /*
     * Sent from the server to the client when the host has changed and is propogating the patch game config.
     */
    CUSTOM_MSG_ID_SERVER_SET_GAME_CONFIG = 10,

    /*
     *
     */
    CUSTOM_MSG_ID_CLIENT_SET_GAME_STATE = 11,

    /*
     * Client requests server to sent current map override info.
     */
    CUSTOM_MSG_ID_REQUEST_MAP_OVERRIDE = 12,

    /*
     * Sent to the client when the server initiates a data download.
     */
    CUSTOM_MSG_ID_SERVER_DOWNLOAD_DATA_REQUEST = 13,

    /*
     * Sent from the client to the server when the client receives a data request message.
     */
    CUSTOM_MSG_ID_CLIENT_DOWNLOAD_DATA_RESPONSE = 14,

    /*
     * Sent from the client to the server when the game ends, containing all the stat information of the game.
     */
    CUSTOM_MSG_ID_CLIENT_SEND_GAME_DATA = 15,

    /*
     * Sent to the server when the client initiates a map download.
     */
    CUSTOM_MSG_ID_CLIENT_INITIATE_DOWNLOAD_MAP_REQUEST = 16,

    /*
     * Sent to the client in response to the client's initiate map download request.
     */
    CUSTOM_MSG_ID_SERVER_INITIATE_DOWNLOAD_MAP_RESPONSE = 17,

    /*
     * Sent to the client when the server sends a map data chunk.
     */
    CUSTOM_MSG_ID_SERVER_DOWNLOAD_MAP_CHUNK_REQUEST = 18,

    /*
     * Sent from the client to the server when the client receives a map data request message.
     */
    CUSTOM_MSG_ID_CLIENT_DOWNLOAD_MAP_CHUNK_RESPONSE = 19,

    /*
     * Sent from the server to the host containing each players rank.
     */
    CUSTOM_MSG_ID_SERVER_SET_RANKS = 20,

    /*
     * Sent by the host when a game has entered the end scoreboard screen.
     */
    CUSTOM_MSG_ID_GAME_LOBBY_REACHED_END_SCOREBOARD = 21,

    /*
     * Sent from the client to the server when the client wishes to enter a queue.
     */
    CUSTOM_MSG_ID_QUEUE_BEGIN_REQUEST = 22,

    /*
     * Sent from the server to the client indicating that the client has entered a queue.
     */
    CUSTOM_MSG_ID_QUEUE_BEGIN_RESPONSE = 23,

    /*
     * Sent from the client to the server when the client wishes to know if they are in a queue and its stats.
     */
    CUSTOM_MSG_ID_GET_MY_QUEUE_REQUEST = 24,

    /*
     * Sent from the server to the client with information about the client's queue.
     */
    CUSTOM_MSG_ID_GET_MY_QUEUE_RESPONSE = 25,

    /*
     * Sent from the server to the client when the server wants the client to join a game.
     */
    CUSTOM_MSG_ID_FORCE_JOIN_GAME_REQUEST = 26,

    /*
     * Sent from the server to the client when the server wants the host to force the team colors of the lobby.
     */
    CUSTOM_MSG_ID_FORCE_TEAMS_REQUEST = 27,

    /*
     * Sent from the server to the client when the server wants the host to force lobby's map.
     */
    CUSTOM_MSG_ID_FORCE_MAP_REQUEST = 28,

    /*
     * Sent from the client to the server when the client issues their vote.
     */
    CUSTOM_MSG_ID_VOTE_REQUEST = 29,

    /*
     * Sent from the server to the client when the server wants the host to force start the game.
     */
    CUSTOM_MSG_ID_FORCE_START_GAME_REQUEST = 30,

    /*
     * Sent from the server to the client when the server wants the client to leave the game.
     */
    CUSTOM_MSG_ID_FORCE_LEAVE_GAME_REQUEST = 31,

    /*
     * Sent from the server to the client with information when a queue'd game will start.
     */
    CUSTOM_MSG_ID_SET_GAME_START_TIME_REQUEST = 32,

    /*
     * Sent from the server to the client with the patch config for another client in the current lobby.
     */
    CUSTOM_MSG_ID_SERVER_SET_LOBBY_CLIENT_PATCH_CONFIG_REQUEST = 33,

    /*
     * Sent from the server to the client containing a message the server wishes to show on the client.
     */
    CUSTOM_MSG_ID_SERVER_SHOW_SNACK_MESSAGE_REQUEST = 34,

    /*
     * Sent from a client to the host when they pick up the flag.
     */
    CUSTOM_MSG_ID_FLAG_REQUEST_PICKUP = 35,

    /*
     * Sent from a client to the server when they want the custom gamemode binary sent to them.
     */
    CUSTOM_MSG_REQUEST_CUSTOM_MODE_PATCH = 36,

    /*
     * Sent from the server to the client after the the custom gamemode binary download has initiated.
     */
    CUSTOM_MSG_RESPONSE_CUSTOM_MODE_PATCH = 37,

    /*
     * Sent by the server containing the global custom maps version.
     */
    CUSTOM_MSG_ID_SERVER_SENT_CMAPS_GLOBAL_VERSION = 38,

    /*
     * Sent by the client to the server when the client wants a boot elf.
     */
    CUSTOM_MSG_ID_CLIENT_REQUEST_BOOT_ELF = 39,

    /*
     * Sent by the server to the client when the server has finished sending a boot elf.
     */
    CUSTOM_MSG_ID_SERVER_RESPONSE_BOOT_ELF = 40,

    /*
     * Sent by the client to the server when the client's "client type" changes.
     * Ie, are they a DZO client or a normal client.
     */
    CUSTOM_MSG_ID_CLIENT_SET_CLIENT_TYPE = 41,

    /*
     * Sent by the client to the server when the client has collected a horizon bolt.
     */
    CUSTOM_MSG_ID_CLIENT_PICKED_UP_HORIZON_BOLT = 42,

    /*
     * Sent by the client to the server when the client requests the current scavenger hunt settings.
     */
    CUSTOM_MSG_ID_CLIENT_REQUEST_SCAVENGER_HUNT_SETTINGS = 43,

    /*
     * Sent by the server to the client containing the current scavenger hunt settings.
     */
    CUSTOM_MSG_ID_SERVER_RESPONSE_SCAVENGER_HUNT_SETTINGS = 44,

    /*
     * Sent by the server to the client containing the name overrides for everyone in the lobby.
     */
    CUSTOM_MSG_ID_SERVER_SET_LOBBY_NAME_OVERRIDES = 45,

    /*
     * Sent by the client to the server when the client wishes to know the server datetime.
     */
    CUSTOM_MSG_ID_CLIENT_REQUEST_SERVER_DATE_TIME = 46,

    /*
     * Sent by the server to the client containing the server's current datetime.
     */
    CUSTOM_MSG_ID_SERVER_DATE_TIME_RESPONSE = 47,

    /*
     * Start of custom message ids reserved for custom game modes.
     */
    CUSTOM_MSG_ID_GAME_MODE_START = 100,

    /*
     * Start of custom message ids reserved for in game only patch custom messages.
     */
    CUSTOM_MSG_ID_PATCH_IN_GAME_START = 200
};

/*
 * NAME :		CustomDzoCommandId
 * 
 * DESCRIPTION :
 * 			Contains the different message ids used with the Dzo Client command interop.
 * 
 * NOTES :
 * 
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
enum CustomDzoCommandId
{
  CUSTOM_DZO_CMD_ID_SND_DRAW_TIMER = 1,
  CUSTOM_DZO_CMD_ID_SND_DRAW_ROUND_RESULT = 2,
  CUSTOM_DZO_CMD_ID_SURVIVAL_DRAW_HUD = 3,
  CUSTOM_DZO_CMD_ID_SURVIVAL_DRAW_REVIVE_MSG = 4,
  CUSTOM_DZO_CMD_ID_VOTE_TO_END = 5,
  CUSTOM_DZO_CMD_ID_SURVIVAL_SPAWN_DAMAGE_BUBBLE = 6,
};

typedef struct SetMapOverrideResponse
{
  int MapId;
  int MapVersion;
} SetMapOverrideResponse_t;

typedef struct ServerDownloadDataRequest
{
    int Id;
    int TargetAddress;
    int TotalSize;
    int DataOffset;
    short DataSize;
    char Data[1362];
} ServerDownloadDataRequest_t;

typedef struct ClientDownloadDataResponse
{
    int Id;
    int BytesReceived;
} ClientDownloadDataResponse_t;

typedef struct ClientInitiateMapDownloadRequest
{
    int MapId;
} ClientInitiateMapDownloadRequest_t;

typedef struct ServerInitiateMapDownloadResponse
{
    int MapId;
    int MapVersion;
    int TotalSize;
    char MapName[32];
    char MapFileName[128];
} ServerInitiateMapDownloadResponse_t;

typedef struct ServerDownloadMapChunkRequest
{
    int Id;
    short DataSize;
    char IsEnd;
    char Data[1024*6];
} ServerDownloadMapChunkRequest_t;

typedef struct ClientDownloadMapChunkResponse
{
    int Id;
    int BytesReceived;
    int Cancel;
} ClientDownloadMapChunkResponse_t;

typedef struct ServerSetRanksRequest
{
    int Enabled;
    int AccountIds[10];
    float Ranks[10];
} ServerSetRanksRequest_t;

typedef struct QueueBeginRequest
{
    int QueueId;
} QueueBeginRequest_t;

typedef struct QueueBeginResponse
{
    int Status;
    int CurrentQueueId;
    int SecondsInQueue;
    int PlayersInQueue;
} QueueBeginResponse_t;

typedef struct GetMyQueueResponse
{
    int CurrentQueueId;
    int SecondsInQueue;
    int PlayersInQueue;
} GetMyQueueResponse_t;

typedef struct ForceJoinGameRequest
{
    int DmeWorldId;
    int ChannelWorldId;
    int GameWorldId;
    int PlayerCount;
    u32 WeaponFlags;
    char AmIHost;
    char Level;
    char Ruleset;
    char GameFlags[59];
} ForceJoinGameRequest_t;

typedef struct ForceLeaveGameRequest
{
    int Reason;
} ForceLeaveGameRequest_t;

typedef struct ForceTeamsRequest
{
    int AccountIds[10];
    float BaseRanks[10];
    char Teams[10];
} ForceTeamsRequest_t;

typedef struct ForceMapRequest
{
    int Level;
} ForceMapRequest_t;

typedef struct ServerShowSnackMessage
{
    char Message[64];
} ServerShowSnackMessage_t;

enum VoteContext
{
    VOTE_CONTEXT_GAME_SKIP_MAP,
    VOTE_CONTEXT_GAME_NEW_TEAMS
};

typedef struct VoteRequest
{
    enum VoteContext Context;
    int Vote;
} VoteRequest_t;

typedef struct SetGameStartTimeRequest
{
    int SecondsUntilStart;
} SetGameStartTimeRequest_t;

typedef struct ClientRequestPickUpFlag
{
    int GameTime;
    int PlayerId;
    u32 FlagUID;
} ClientRequestPickUpFlag_t;

typedef struct ClientRequestBootElf
{
    int BootElfId;
} ClientRequestBootElf_t;

typedef struct ServerResponseBootElf
{
    int BootElfId;
    u32 Address;
    u32 Size;
} ServerResponseBootElf_t;

typedef struct ClientSetClientTypeRequest
{
    int ClientType;
    u8 mac[6];
} ClientSetClientTypeRequest_t;

struct PingRequest
{
  long Time;
  int SourceClientId;
  int ReturnedFromClientId;
};

typedef struct ScavengerHuntSettingsResponse
{
  int Enabled;
  float SpawnFactor;
} ScavengerHuntSettingsResponse_t;

typedef struct SetNameOverridesMessage
{
  int AccountIds[10];
  char Names[10][16];
} SetNameOverridesMessage_t;

typedef struct ServerDateTimeMessage
{
  u16 Year;
  u8 Month;
  u8 Day;
} ServerDateTimeMessage_t;

#endif // _MESSAGEID_H_
