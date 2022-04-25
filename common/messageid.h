
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
     * Start of custom message ids reserved for custom game modes.
     */
    CUSTOM_MSG_ID_GAME_MODE_START = 100,

};

typedef struct ServerDownloadDataRequest
{
    int Id;
    int TargetAddress;
    int TotalSize;
    int DataOffset;
    short DataSize;
    char Data[2048];
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
    int ChannelWorldId;
    int GameWorldId;
    int PlayerCount;
    u32 WeaponFlags;
    char AmIHost;
    char Level;
    char Ruleset;
    char GameFlags[59];
} ForceJoinGameRequest_t;

typedef struct ForceTeamsRequest
{
    int AccountIds[10];
    char Teams[10];
} ForceTeamsRequest_t;

typedef struct ForceMapRequest
{
    int Level;
} ForceMapRequest_t;

enum VoteContext
{
    VOTE_CONTEXT_GAME_SKIP_MAP
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


#endif // _MESSAGEID_H_
