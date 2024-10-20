#include <libdl/stdio.h>
#include <libdl/stdlib.h>
#include <libdl/net.h>
#include <libdl/mc.h>
#include <libdl/string.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/pad.h>
#include <libdl/gamesettings.h>
#include <libdl/game.h>
#include <libdl/utils.h>
#include "include/config.h"
#include "rpc.h"
#include "messageid.h"
#include "module.h"
#include "config.h"

#include <sifcmd.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <sifrpc.h>
#include <loadfile.h>

#define NEWLIB_PORT_AWARE
#include <io_common.h>

#define MAP_FRAG_PAYLOAD_MAX_SIZE               (1024)
#define LOAD_MODULES_STATE                      (*(u8*)0x000CFFF0)
#define LOAD_MODULES_RESULT                     (*(u8*)0x000CFFF1)
#define HAS_LOADED_MODULES                      (LOAD_MODULES_STATE == 100)

#define USB_FS_ID                               (*(u8*)0x000CFFF2)
#define USB_SRV_ID                              (*(u8*)0x000CFFF3)

#define USB_FS_MODULE_PTR												(*(void**)0x000CFFF8)
#define USB_SRV_MODULE_PTR											(*(void**)0x000CFFFC)

void hook(void);
void loadModules(void);

int readLevelVersion(char * name, int * version);
int readGlobalVersion(int * version);

int usbFsModuleSize = 0;
int usbSrvModuleSize = 0;

int mapsRemoteGlobalVersion = -2;
int mapsLocalGlobalVersion = -1;

// patch config
extern PatchConfig_t config;

// game config
extern PatchGameConfig_t gameConfig;

// patch state container
extern PatchStateContainer_t patchStateContainer;

// game mode overrides
extern struct MenuElem_OrderedListData dataCustomModes;

// map overrides
extern struct MenuElem_ListData dataCustomMaps;

// custom map defs
CustomMapDef_t *customMapDefs = NULL;
int customMapDefCount = 0;

extern u32 colorBlack;
extern u32 colorBg;
extern u32 colorRed;
extern u32 colorContentBg;
extern u32 colorText;
extern int isUnloading;
extern char mapOverrideResponse;

enum MenuActionId
{
	ACTION_ERROR_LOADING_MODULES = -1,

	ACTION_MODULES_NOT_INSTALLED = 0,
	ACTION_DOWNLOADING_MODULES = 1,
	ACTION_MODULES_INSTALLED = 2,
	ACTION_NEW_MAPS_UPDATE = 3,
  ACTION_REFRESHING_MAPLIST = 4,

	ACTION_NONE = 100
};

// memory card fd
int initialized = 0;
int actionState = ACTION_MODULES_NOT_INSTALLED;
int rpcInit = 0;

// 
char membuffer[256];
int useHost = 0;

#define MASS_PATH_PREFIX      "mass:"
#define HOST_PATH_PREFIX      "host:"

// paths for level specific files
char * fWad = "%sdl/%s.wad";
char * fSound = "%sdl/%s.sound";
char * fBg = "%sdl/%s.bg";
char * fMap = "%sdl/%s.map";
char * fVersion = "%sdl/%s.version";
char * fGlobalVersion = "%sdl/version";

typedef struct MapOverrideMessage
{
  CustomMapDef_t CustomMap;
} MapOverrideMessage;

typedef struct MapClientRequestModulesMessage
{
	u32 Module1Start;
	u32 Module2Start;
} MapClientRequestModulesMessage;

typedef struct MapServerSentModulesMessage
{
	int Version;
	int Module1Size;
	int Module2Size;
} MapServerSentModulesMessage;

MapLoaderState_t MapLoaderState;

#if MAPDOWNLOADER
struct MapDownloadState
{
    u8 Enabled;
    u8 MapId;
    char MapName[32];
    char MapFileName[128];
    int Fd;
		int Version;
    int TotalSize;
		int TotalDownloaded;
		int Ticks;
		int Cancel;
} DownloadState;
#endif

//------------------------------------------------------------------------------
char * getMapPathPrefix(void)
{
  if (useHost) return HOST_PATH_PREFIX;
  return MASS_PATH_PREFIX;
}

//------------------------------------------------------------------------------
int onSetMapOverride(void * connection, void * data)
{
  MapOverrideMessage payload;
  memcpy(&payload, data, sizeof(payload));

  // reset
  int lastSelectedCustomMapId = patchStateContainer.SelectedCustomMapId;
  patchStateContainer.SelectedCustomMapId = 0;

	if (payload.CustomMap.BaseMapId == 0)
	{
		DPRINTF("recv empty map\n");
		MapLoaderState.Enabled = 0;
		MapLoaderState.CheckState = 0;
    MapLoaderState.MapFileName[0] = 0;
    MapLoaderState.MapName[0] = 0;
		mapOverrideResponse = 1;
	}
	else
	{
		// check for version
		int version = -1;
		if (LOAD_MODULES_STATE != 100)
			version = -1;
    
    // find map in our list
    int i = 0;
    for (i = 0; i < customMapDefCount; ++i) {
      if (strncmp(customMapDefs[i].Filename, payload.CustomMap.Filename, sizeof(customMapDefs[i].Filename)) == 0) {
        patchStateContainer.SelectedCustomMapId = i + 1;
        version = customMapDefs[i].Version;
        break;
      }
    }

		if (!patchStateContainer.SelectedCustomMapId) {
			version = -2;
    }

    // read global maps version from usb
    readLocalGlobalVersion();

		// print
		DPRINTF("MapId:%d MapName:%s MapFileName:%s Version:%d\n", payload.CustomMap.BaseMapId, payload.CustomMap.Name, payload.CustomMap.Filename, version);

		// send response
    SetMapOverrideResponse_t msg;
    msg.MapVersion = version;
    strncpy(msg.MapFilename, payload.CustomMap.Filename, sizeof(msg.MapFilename));
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, connection, NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_SET_MAP_OVERRIDE_RESPONSE, sizeof(msg), &msg);

    strncpy(MapLoaderState.MapName, payload.CustomMap.Name, sizeof(MapLoaderState.MapName));
    strncpy(MapLoaderState.MapFileName, payload.CustomMap.Filename, sizeof(MapLoaderState.MapFileName));
    
		// enable
		if (version >= 0)
		{
			MapLoaderState.Enabled = 1;
			MapLoaderState.CheckState = 0;
			mapOverrideResponse = version;
			MapLoaderState.MapId = payload.CustomMap.BaseMapId;
			MapLoaderState.LoadingFd = -1;
			MapLoaderState.LoadingFileSize = -1;
		}
		else
		{
			MapLoaderState.Enabled = 0;
			mapOverrideResponse = version;
		}
	}

  patchStateContainer.SelectedCustomMapChanged = lastSelectedCustomMapId != patchStateContainer.SelectedCustomMapId;
	return sizeof(MapOverrideMessage);
}

//------------------------------------------------------------------------------
int onServerSentMapIrxModules(void * connection, void * data)
{
	DPRINTF("server sent map irx modules\n");

	MapServerSentModulesMessage * msg = (MapServerSentModulesMessage*)data;
  mapsRemoteGlobalVersion = msg->Version;

  // we've already initialized the usb interface
  if (rpcInit > 0)
	  return sizeof(MapServerSentModulesMessage);

	// initiate loading
	if (LOAD_MODULES_STATE == 0)
		LOAD_MODULES_STATE = 7;

	// 
	usbFsModuleSize = msg->Module1Size;
	usbSrvModuleSize = msg->Module2Size;

	// 
	loadModules();

	//
	int init = rpcInit = rpcUSBInit();

	DPRINTF("rpcUSBInit: %d, %08X:%d, %08X:%d\n", init, (u32)USB_FS_MODULE_PTR, usbFsModuleSize, (u32)USB_SRV_MODULE_PTR, usbSrvModuleSize);
	
	//
	if (init < 0)
	{
		actionState = ACTION_ERROR_LOADING_MODULES;
	}
	else
	{
    // check if host fs exists
    useHost = 1;
    if (!readGlobalVersion(NULL)) useHost = 0;

    // read local global version
    readLocalGlobalVersion();
		if (mapsLocalGlobalVersion != mapsRemoteGlobalVersion)
		{
			// Indicate new version
			actionState = ACTION_NEW_MAPS_UPDATE;
		}
		else
		{
			// Indicate maps installed
			actionState = ACTION_MODULES_INSTALLED;
		}
		
		DPRINTF("local maps version %d || remote maps version %d\n", mapsLocalGlobalVersion, mapsRemoteGlobalVersion);

    // refresh map list
    refreshCustomMapList();
		
		// if in game, ask server to resend map override to use
		if (gameGetSettings())
			netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_REQUEST_MAP_OVERRIDE, 0, NULL);
	}

	return sizeof(MapServerSentModulesMessage);
}

//------------------------------------------------------------------------------
int onServerSendMapVersion(void * connection, void * data)
{
  // read local global version first
  useHost = 1;
  if (!readGlobalVersion(NULL)) useHost = 0;
  readLocalGlobalVersion();

  memcpy(&mapsRemoteGlobalVersion, data, 4);
	DPRINTF("server sent map version %d, local %d\n", mapsRemoteGlobalVersion, mapsLocalGlobalVersion);
  return 4;
}

#if MAPDOWNLOADER
//------------------------------------------------------------------------------
int onServerSentMapChunk(void * connection, void * data)
{
	ServerDownloadMapChunkRequest_t * msg = (ServerDownloadMapChunkRequest_t*)data;
	ClientDownloadMapChunkResponse_t response = {
		.Cancel = 0,
		.Id = msg->Id
	};
	response.BytesReceived = DownloadState.TotalDownloaded += msg->DataSize;

	DPRINTF("server sent map chunk %d, %d bytes (%d/%d)\n", msg->Id, msg->DataSize, DownloadState.TotalDownloaded, DownloadState.TotalSize);

	// 
	if (LOAD_MODULES_STATE == 100)
	{
		// open
		if (DownloadState.Fd < 0) {
			sprintf(membuffer, msg->Id == 0 ? fWad : fBg, getMapPathPrefix(), DownloadState.MapFileName);
			DownloadState.Fd = usbOpen(membuffer, FIO_O_CREAT | FIO_O_WRONLY | FIO_O_TRUNC);

			// failed to open
			if (DownloadState.Fd < 0) {
				DownloadState.Cancel = 1;
				DownloadState.Enabled = 2;
			}
		}

		if (DownloadState.Fd >= 0) {
			if (!usbWrite(DownloadState.Fd, msg->Data, msg->DataSize)) {
				DownloadState.Cancel = 1;
				DownloadState.Enabled = 2;
				DownloadState.Fd = -1;
			}
			else if (DownloadState.Cancel) {
				usbClose(DownloadState.Fd);
				DownloadState.Fd = -1;
				DownloadState.Enabled = 2;
			}
			else if (msg->IsEnd) {
				usbClose(DownloadState.Fd);
				DownloadState.Fd = -1;

				// finished
				if (DownloadState.TotalDownloaded >= DownloadState.TotalSize) {
					DownloadState.Enabled = 2;

					// save version
					sprintf(membuffer, fVersion, getMapPathPrefix(), DownloadState.MapFileName);
					int fd = usbOpen(membuffer, FIO_O_CREAT | FIO_O_WRONLY | FIO_O_TRUNC);
					if (fd < 0)
					{
						DPRINTF("failed to create version\n");
					}
					else
					{
						usbWrite(fd, &DownloadState.Version, sizeof(DownloadState.Version));
						usbClose(fd);
					}
				}
			}
		}
	}

	// 
	response.Cancel = DownloadState.Cancel;
	

	char b[sizeof(ClientDownloadMapChunkResponse_t) + 4];
	b[0] = CUSTOM_MSG_ID_CLIENT_DOWNLOAD_MAP_CHUNK_RESPONSE;
	memcpy(b+4, &response, sizeof(ClientDownloadMapChunkResponse_t));

	internal_netSendMessage(0x40, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, NET_CUSTOM_MESSAGE_CLASS, NET_CUSTOM_MESSAGE_ID, 4 + sizeof(ClientDownloadMapChunkResponse_t), b);
	
	
	//netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_DOWNLOAD_MAP_CHUNK_RESPONSE, sizeof(ClientDownloadMapChunkResponse_t), &response);

	return sizeof(ServerDownloadMapChunkRequest_t);
}

//------------------------------------------------------------------------------
int onServerSentMapInitiated(void * connection, void * data)
{
	ServerInitiateMapDownloadResponse_t * msg = (ServerInitiateMapDownloadResponse_t*)data;
	ClientDownloadMapChunkResponse_t response = {
		.Id = msg->MapId,
		.BytesReceived = 0,
		.Cancel = 0
	};
	DPRINTF("server sent map init %d,%d,%s\n", msg->MapId, msg->MapVersion, msg->MapFileName);

	DownloadState.Enabled = 1;
	DownloadState.MapId = msg->MapId;
	DownloadState.Version = msg->MapVersion;
	DownloadState.TotalSize = msg->TotalSize;
	DownloadState.Fd = -1;
	DownloadState.TotalDownloaded = 0;
	DownloadState.Ticks = 0;
	DownloadState.Cancel = 0;
	strncpy(DownloadState.MapFileName, msg->MapFileName, sizeof(DownloadState.MapFileName));
	strncpy(DownloadState.MapName, msg->MapName, sizeof(DownloadState.MapName));

	// just create the version file to reset it
	// and also to verify that the usb drive is present
	sprintf(membuffer, fVersion, getMapPathPrefix(), DownloadState.MapFileName);
	int fd = usbOpen(membuffer, FIO_O_CREAT | FIO_O_WRONLY | FIO_O_TRUNC);
	if (fd < 0)
	{
		DownloadState.Cancel = 1;
		DownloadState.Enabled = 0;
	}
	else
	{
		usbClose(fd);
	}

	// 
	response.Cancel = DownloadState.Cancel;
	netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_DOWNLOAD_MAP_CHUNK_RESPONSE, sizeof(ClientDownloadMapChunkResponse_t), &response);

	return sizeof(ServerInitiateMapDownloadResponse_t);
}
#endif

//------------------------------------------------------------------------------
int readLocalGlobalVersion(void)
{
  if (!readGlobalVersion(&mapsLocalGlobalVersion))
    return (mapsLocalGlobalVersion = -1);
    
  return mapsLocalGlobalVersion;
}

//------------------------------------------------------------------------------
void loadModules(void)
{
	int mod_res = 0;
	if (LOAD_MODULES_STATE < 7)
		return;

	if (LOAD_MODULES_STATE != 100)
	{
		//
		SifInitRpc(0);

		// Load modules
		USB_FS_ID = SifExecModuleBuffer(USB_FS_MODULE_PTR, usbFsModuleSize, 0, NULL, &mod_res);
		USB_SRV_ID = SifExecModuleBuffer(USB_SRV_MODULE_PTR, usbSrvModuleSize, 0, NULL, &mod_res);

		DPRINTF("Loading MASS: %d\n", USB_FS_ID);
		DPRINTF("Loading USBSERV: %d\n", USB_SRV_ID);
	}

	LOAD_MODULES_STATE = 100;
}

//------------------------------------------------------------------------------
u64 loadHookFunc(u64 a0, u64 a1)
{
	// Load our usb modules
	loadModules();

	// Loads sound driver
	return ((u64 (*)(u64, u64))0x001518C8)(a0, a1);
}

//------------------------------------------------------------------------------
int readFileLength(char * path)
{
	int fd, fSize;

	// Open
	rpcUSBopen(path, FIO_O_RDONLY);
	rpcUSBSync(0, NULL, &fd);

	// Ensure the file was opened successfully
	if (fd < 0)
	{
		DPRINTF("error opening file (%s): %d\n", path, fd);
		return -1;
	}
	
	// Get length of file
	rpcUSBseek(fd, 0, SEEK_END);
	rpcUSBSync(0, NULL, &fSize);

	// Close file
	rpcUSBclose(fd);
	rpcUSBSync(0, NULL, NULL);

	return fSize;
}

//------------------------------------------------------------------------------
int readFile(char * path, void * buffer, int offset, int length)
{
	int r, fd, fSize;

  //DPRINTF("read file (%s)\n", path);

	// Open
	rpcUSBopen(path, FIO_O_RDONLY);
	rpcUSBSync(0, NULL, &fd);

	// Ensure the file was opened successfully
	if (fd < 0)
	{
		DPRINTF("error opening file (%s): %d\n", path, fd);
		return -1;
	}
	
	// Get length of file
	rpcUSBseek(fd, 0, SEEK_END);
	rpcUSBSync(0, NULL, &fSize);

	// limit read length to size of file
	if (fSize < (length+offset))
		length = fSize-offset;

	// Go start read point
	rpcUSBseek(fd, offset, SEEK_SET);
	rpcUSBSync(0, NULL, NULL);

	// Read map
	if (rpcUSBread(fd, (void*)buffer, length) != 0)
	{
		DPRINTF("error reading from file.\n");
		rpcUSBclose(fd);
		rpcUSBSync(0, NULL, NULL);
		return -1;
	}
	rpcUSBSync(0, NULL, &r);

	// Close file
	rpcUSBclose(fd);
	rpcUSBSync(0, NULL, NULL);

  //DPRINTF("finished reading %s (%d/%d)\n", path, r, length);

	return r;
}

//------------------------------------------------------------------------------
int readGlobalVersion(int * version)
{
	int r;
  char buf[4];
  char filename[128];

  snprintf(filename, sizeof(filename), fGlobalVersion, getMapPathPrefix());
	r = readFile(filename, (void*)buf, 0, 4);
	if (r != 4)
	{
		DPRINTF("error reading file (%s)\n", filename);
		return 0;
	}

  if (version) *version = *(int*)buf;
	return 1;
}

//--------------------------------------------------------------
int readLevelVersion(char * name, int * version)
{
	int r;

	// Generate version filename
	sprintf(membuffer, fVersion, getMapPathPrefix(), name);

	// read
	r = readFile(membuffer, (void*)version, 0, 4);
	if (r != 4)
	{
		DPRINTF("error reading file (%s)\n", membuffer);
		return 0;
	}

	return 1;
}

//--------------------------------------------------------------
int getLevelSizeUsb()
{
	// Generate wad filename
	sprintf(membuffer, fWad, getMapPathPrefix(), MapLoaderState.MapFileName);

	// get file length
	MapLoaderState.LoadingFileSize = readFileLength(membuffer);

	// Check the file has a valid size
	if (MapLoaderState.LoadingFileSize <= 0)
	{
		DPRINTF("error seeking file (%s): %d\n", membuffer, MapLoaderState.LoadingFileSize);
		return 0;
	}

	return MapLoaderState.LoadingFileSize;
}

//--------------------------------------------------------------
int readLevelMapUsb(u8 * buf, int size)
{
	// Generate toc filename
	sprintf(membuffer, fMap, getMapPathPrefix(), MapLoaderState.MapFileName);

	// read
	return readFile(membuffer, (void*)buf, 0, size) > 0;
}

//--------------------------------------------------------------
int readLevelBgUsb(u8 * buf, int size)
{
	// Ensure a wad isn't already open
	if (MapLoaderState.LoadingFd >= 0)
	{
		DPRINTF("readLevelBgUsb() called but a file is already open (%d)!", MapLoaderState.LoadingFd);
		return 0;
	}

	// Generate toc filename
	sprintf(membuffer, fBg, getMapPathPrefix(), MapLoaderState.MapFileName);

	// read
	return readFile(membuffer, (void*)buf, 0, size) > 0;
}

#if MAPDOWNLOADER
//--------------------------------------------------------------
int usbOpen(char* filepath, int flags)
{
	int fd = 0;

	// open wad file
	rpcUSBopen(filepath, flags);
	rpcUSBSync(0, NULL, &fd);
	
	// Ensure wad successfully opened
	if (fd < 0)
	{
		DPRINTF("error opening file (%s): %d\n", filepath, fd);
		return -1;									
	}
	
	return fd;
}

//--------------------------------------------------------------
void usbClose(int fd)
{
	DPRINTF("close %d\n", fd);
	// open wad file
	rpcUSBclose(fd);
	rpcUSBSync(0, NULL, NULL);
}

//--------------------------------------------------------------
int usbWrite(int fd, u8* buf, int len)
{
	int r = 0, rPos = 0;

	// Ensure the wad is open
	if (fd < 0)
	{
		DPRINTF("error opening file: %d\n", fd);
		return 0;									
	}

	while (rPos < len)
	{
		int size = len;
		if (size > 8192)
			size = 8192;

		// Try to write to usb
		if ((r = rpcUSBwrite(fd, buf, size)) != 0)
		{
			rpcUSBSync(0, NULL, NULL);
			DPRINTF("error writing to file %d\n", r);
			rpcUSBclose(fd);
			rpcUSBSync(0, NULL, NULL);
			fd = -1;
			return 0;
		}

		rpcUSBSync(0, NULL, &r);
		rPos += r;
		buf += r;

		if (r != size)
			break;
	}

	return 1;
}
#endif

//--------------------------------------------------------------
int openSoundUsb()
{
	// Ensure a wad isn't already open
	if (MapLoaderState.LoadingFd >= 0)
	{
		DPRINTF("openSoundUsb() called but a file is already open (%d)!", MapLoaderState.LoadingFd);
		return 0;
	}

	// Generate wad filename
	sprintf(membuffer, fSound, getMapPathPrefix(), MapLoaderState.MapFileName);

	// open wad file
	rpcUSBopen(membuffer, FIO_O_RDONLY);
	rpcUSBSync(0, NULL, &MapLoaderState.LoadingFd);
	
	// Ensure wad successfully opened
	if (MapLoaderState.LoadingFd < 0)
	{
		DPRINTF("error opening file (%s): %d\n", membuffer, MapLoaderState.LoadingFd);
		return 0;									
	}

	// Get length of file
	rpcUSBseek(MapLoaderState.LoadingFd, 0, SEEK_END);
	rpcUSBSync(0, NULL, &MapLoaderState.LoadingFileSize);

	// Check the file has a valid size
	if (MapLoaderState.LoadingFileSize <= 0)
	{
		DPRINTF("error seeking file (%s): %d\n", membuffer, MapLoaderState.LoadingFileSize);
		rpcUSBclose(MapLoaderState.LoadingFd);
		rpcUSBSync(0, NULL, NULL);
		MapLoaderState.LoadingFd = -1;
    MapLoaderState.Enabled = 0;
		return 0;
	}

	// Go back to start of file
	// The read will be called later
	rpcUSBseek(MapLoaderState.LoadingFd, 0, SEEK_SET);
	rpcUSBSync(0, NULL, NULL);

	DPRINTF("%s is %d byte long.\n", membuffer, MapLoaderState.LoadingFileSize);
	return MapLoaderState.LoadingFileSize;
}

//--------------------------------------------------------------
int openLevelUsb()
{
	// Ensure a wad isn't already open
	if (MapLoaderState.LoadingFd >= 0)
	{
		DPRINTF("openLevelUsb() called but a file is already open (%d)!", MapLoaderState.LoadingFd);
		return 0;
	}

	// Generate wad filename
	sprintf(membuffer, fWad, getMapPathPrefix(), MapLoaderState.MapFileName);

	// open wad file
	rpcUSBopen(membuffer, FIO_O_RDONLY);
	rpcUSBSync(0, NULL, &MapLoaderState.LoadingFd);
	
	// Ensure wad successfully opened
	if (MapLoaderState.LoadingFd < 0)
	{
		DPRINTF("error opening file (%s): %d\n", membuffer, MapLoaderState.LoadingFd);
		return 0;									
	}

	// Get length of file
	rpcUSBseek(MapLoaderState.LoadingFd, 0, SEEK_END);
	rpcUSBSync(0, NULL, &MapLoaderState.LoadingFileSize);

	// Check the file has a valid size
	if (MapLoaderState.LoadingFileSize <= 0)
	{
		DPRINTF("error seeking file (%s): %d\n", membuffer, MapLoaderState.LoadingFileSize);
		rpcUSBclose(MapLoaderState.LoadingFd);
		rpcUSBSync(0, NULL, NULL);
		MapLoaderState.LoadingFd = -1;
    MapLoaderState.Enabled = 0;
		return 0;
	}

	// Go back to start of file
	// The read will be called later
	rpcUSBseek(MapLoaderState.LoadingFd, 0, SEEK_SET);
	rpcUSBSync(0, NULL, NULL);

	DPRINTF("%s is %d byte long.\n", membuffer, MapLoaderState.LoadingFileSize);
	return MapLoaderState.LoadingFileSize;
}

//--------------------------------------------------------------
int readLevelUsb(u8 * buf)
{
	// Ensure the wad is open
	if (MapLoaderState.LoadingFd < 0 || MapLoaderState.LoadingFileSize <= 0)
	{
		DPRINTF("error opening file: %d\n", MapLoaderState.LoadingFd);
		return 0;									
	}

	// Try to read from usb
	if (rpcUSBread(MapLoaderState.LoadingFd, buf, MapLoaderState.LoadingFileSize) != 0)
	{
		DPRINTF("error reading from file.\n");
		rpcUSBclose(MapLoaderState.LoadingFd);
		rpcUSBSync(0, NULL, NULL);
		MapLoaderState.LoadingFd = -1;
		return 0;
	}
				
	return 1;
}

//------------------------------------------------------------------------------
void customMapInsert(char* versionFileBuffer, char* filenameWithoutExtension)
{
  // parse extra data
  CustomMapVersionFileDef_t versionFileDef;
  int extraDataModeMask = 0, i;
  memcpy(&versionFileDef, versionFileBuffer, sizeof(CustomMapVersionFileDef_t));
  for (i = 0; i < versionFileDef.ExtraDataCount && i < 24; ++i) {
    short modeId = *(short*)((u32)versionFileBuffer + 0x30 + 8*i);
    if (modeId > 0) {
      extraDataModeMask |= (1 << modeId);
    }
  }

  // insert alphabetically
  int insertAtIdx = customMapDefCount;
  for (i = 0; i < customMapDefCount; ++i) {
    int c = strncmp(versionFileDef.Name, customMapDefs[i].Name, sizeof(customMapDefs[i].Name));
    if (c < 0) {
      insertAtIdx = i;
      break;
    }
  }

  // move maps forward
  if (insertAtIdx < customMapDefCount) {
    memmove(&customMapDefs[insertAtIdx+1], &customMapDefs[insertAtIdx], sizeof(CustomMapDef_t)*(customMapDefCount-insertAtIdx));
  }

  // bring to custom map defs
  customMapDefs[insertAtIdx].Version = versionFileDef.Version;
  customMapDefs[insertAtIdx].BaseMapId = versionFileDef.BaseMapId;
  customMapDefs[insertAtIdx].ForcedCustomModeId = versionFileDef.ForcedCustomModeId;
  customMapDefs[insertAtIdx].CustomModeExtraDataMask = extraDataModeMask;
  customMapDefs[insertAtIdx].ShrubMinRenderDistance = versionFileDef.ShrubMinRenderDistance;
  strncpy(customMapDefs[insertAtIdx].Filename, filenameWithoutExtension, sizeof(customMapDefs[insertAtIdx].Filename));
  strncpy(customMapDefs[insertAtIdx].Name, versionFileDef.Name, sizeof(customMapDefs[insertAtIdx].Name));
  customMapDefCount++;
  
  //DPRINTF("(%d/%d) \"%s\" f:\"%s\" v:%d bmap:%d mode:%d mask:%x shrub:%d\n", insertAtIdx, customMapDefCount, versionFileDef.Name, filenameWithoutExtension, versionFileDef.Version, versionFileDef.BaseMapId, versionFileDef.ForcedCustomModeId, extraDataModeMask, versionFileDef.ShrubMinRenderDistance);
}

//------------------------------------------------------------------------------
void refreshCustomMapList(void)
{
  int fd, r, i;
  const char* versionExt = ".version";
  char dirpath[16];
  char filename[64];
  char filenameWithoutExtension[64];
  char fullpath[256];
  char buffer[256];
  int versionExtLen = strlen(versionExt);
  int actionStateAtStart = actionState;
  iox_dirent_t dirent;
  io_dirent_t* iomanDirent = (io_dirent_t*)&dirent;
  
  // reset
  dataCustomMaps.count = 1;
  customMapDefCount = 0;
  memset(customMapDefs, 0, sizeof(customMapDefs));

  // need usb modules
  if (!HAS_LOADED_MODULES) return;

#if DSCRPRINT
  clearScrPrintLine();
#endif

  // check if host fs exists
  //checkForHostFs();

  //
  snprintf(dirpath, sizeof(dirpath), "%sdl", getMapPathPrefix());
  DPRINTF("dir path %s\n", dirpath);

	// Open
	rpcUSBdopen(dirpath);
	rpcUSBSync(0, NULL, &fd);

	// Ensure the dir was opened successfully
	if (fd < 0)
	{
		DPRINTF("error opening dir (%s): %d\n", dirpath, fd);
		return;
	}
	
  DPRINTF("opening dir (%s): returned %d\n", dirpath, fd);

  // read
  actionState = ACTION_REFRESHING_MAPLIST;
  do 
  {
    uiRunCallbacks();

    // handle case where irx modules 
    if (actionState != ACTION_REFRESHING_MAPLIST) {
      actionStateAtStart = actionState;
      actionState = ACTION_REFRESHING_MAPLIST;
    }

    // read next entry
    // stop if we've reached the end
    if (rpcUSBdread(fd, &dirent) != 0) break;
    rpcUSBSync(0, NULL, &r);
    if (r <= 0) break;

    // extract filename
    // for some reason there's a mixup between if we're using ioman or iomanX
    // PS2s use iomanX but the emu HLE hostfs thinks we're using ioman
    if (useHost) strncpy(filename, iomanDirent->name, sizeof(filename));
    else strncpy(filename, dirent.name, sizeof(filename));

    // OSX creates index files starting with a '.'
    // filter those out
    if (filename[0] == '.') continue;

    // we want to parse the .version files
    // check if filename ends with ".version"
    int len = strlen(filename);
    if (strcmp(&filename[len-versionExtLen], versionExt) != 0) continue;

    #if DSCRPRINT
    snprintf(buffer, sizeof(buffer), "y %s", filename);
    pushScrPrintLine(buffer);
    #endif

    DPRINTF("found version %s\n", filename);

    // parse version file
    CustomMapVersionFileDef_t versionFileDef;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, filename);
    int read = readFile(fullpath, buffer, 0, sizeof(buffer));

    // ensure version file is valid
    if (read < sizeof(CustomMapVersionFileDef_t))
    {
      DPRINTF("%s (%d) does not match expected file size %d. Skipping.\n", filename, read, sizeof(CustomMapVersionFileDef_t));
      continue;
    }

    // compute filename without extension
    strncpy(filenameWithoutExtension, filename, sizeof(filenameWithoutExtension));
    len = strlen(filenameWithoutExtension);
    filenameWithoutExtension[len - versionExtLen] = 0;

    // ensure version file has matching .wad
    snprintf(fullpath, sizeof(fullpath), fWad, getMapPathPrefix(), filenameWithoutExtension);
    int fWadLen = readFileLength(fullpath);
    if (fWadLen <= 0) continue;

    // parse extra data
    customMapInsert(buffer, filenameWithoutExtension);

    // reached max maps
    if (customMapDefCount >= MAX_CUSTOM_MAP_DEFINITIONS) break;
  } while (1);

  // close
  rpcUSBdclose(fd);
	rpcUSBSync(0, NULL, NULL);
  
  // populate config
  for (i = 0; i < customMapDefCount; ++i)
  {
    dataCustomMaps.items[i+1] = (char*)customMapDefs[i].Name;
    dataCustomMaps.count += 1;
  }

  // clamp
  if (patchStateContainer.SelectedCustomMapId >= dataCustomMaps.count)
    patchStateContainer.SelectedCustomMapId = dataCustomMaps.count - 1;

  actionState = actionStateAtStart;
}

//------------------------------------------------------------------------------
int beginLoadingLevelWad(void)
{
  int fSize = openLevelUsb();
  if (fSize > 0) {
    if (readLevelUsb(MapLoaderState.LevelBuffer) > 0) {
      MapLoaderState.Loaded |= 1;
      DPRINTF("loading level wad...");
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
int beginLoadingSoundWad(void* dest)
{
  int fSize = openSoundUsb();
  if (fSize > 0) {
    MapLoaderState.SoundBuffer = dest;
    DPRINTF("level: %08X\nsound: %08X\n", MapLoaderState.LevelBuffer, MapLoaderState.SoundBuffer);

    // read sound bank in background
    // let hookedCheck handle when sound is finished loading
    DPRINTF("begin sound bank read\n");
    if (readLevelUsb(MapLoaderState.SoundBuffer) > 0) {
      MapLoaderState.Loaded |= 2;
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
void hookedLoad(u64 arg0, void * dest, u32 sectorStart, u32 sectorSize, u64 arg4, u64 arg5, u64 arg6)
{
	void (*cdvdLoad)(u64, void*, u32, u32, u64, u64, u64) = (void (*)(u64, void*, u32, u32, u64, u64, u64))0x00163840;

	// Check if loading MP map
  MapLoaderState.LevelBuffer = dest;
  MapLoaderState.SoundBuffer = 0;
  MapLoaderState.Loaded = 0;
	if (MapLoaderState.Enabled && HAS_LOADED_MODULES)
	{
    // load sound wad first
    // Generate sound filename
    sprintf(membuffer, fSound, getMapPathPrefix(), MapLoaderState.MapFileName);
    int filelen = readFileLength(membuffer);
    if (filelen > 0)
      if (beginLoadingSoundWad(dest)) return;

    // if we've reached here, then the sound wad doesn't exist, so load the level wad
    if (beginLoadingLevelWad()) return;
	}

	// Default to cdvd load if the usb load failed
	cdvdLoad(arg0, dest, sectorStart, sectorSize, arg4, arg5, arg6);
}

//------------------------------------------------------------------------------
u32 hookedCheck(void)
{
	u32 (*check)(void) = (u32 (*)(void))0x00163928;
	int r, cmd;

  // delay non-host by some amount of time
  // useful for testing slow load desyncs
  /*
  if (MapLoaderState.LoadingFd < 0 && MapLoaderState.Enabled && !gameAmIHost()) {
    GameSettings* gs = gameGetSettings();
    int loadAfter = gs->GameLoadStartTime + (1000 * 45);
    if (gameGetTime() > loadAfter)
      return check();
 
    check();
    return 1;
  }
  */

	// If the wad is not open then we're loading from cdvd
	if (MapLoaderState.LoadingFd < 0 || !MapLoaderState.Enabled)
    return check();

	// Otherwise check to see if we've finished loading the data from USB
	if (rpcUSBSyncNB(0, &cmd, &r) == 1)
	{
		// If the command is USBREAD close and return
		if (cmd == 0x04)
		{
			DPRINTF("finished reading %d bytes from USB\n", r);
			rpcUSBclose(MapLoaderState.LoadingFd);
			rpcUSBSync(0, NULL, NULL);
			MapLoaderState.LoadingFd = -1;

      if ((MapLoaderState.Loaded & 1) == 0) {
        // finish loading sound wad
        if (*(u32*)0x0021E1EC != 0) {
          ((void (*)())0x0058d5d0)(); // sound_LevelInit()
          ((void (*)())0x0051f7b8)(); // music_LoadCoreBank()
          POKE_U32(0x004ea028, 0);    // nop sound_LevelInit()
          POKE_U32(0x004ea05c, 0);    // nop music_LoadCoreBank()
        } else {
          // 
          ((void (*)())0x0066b170)(); // sound_LevelInit()
          ((void (*)())0x005fec20)(); // music_LoadCoreBank()
          POKE_U32(0x005cfca0, 0);    // nop sound_LevelInit()
          POKE_U32(0x005cfcd4, 0);    // nop music_LoadCoreBank()
        }

        // load level wad
        if (beginLoadingLevelWad()) return;
      }

      // finished loading level +/ sound wads
			return 0;
		}
	}

	// Set bg color to red
	*((vu32*)0x120000e0) = 0x1010B4;

	// Not sure if this is necessary but it doesn't hurt to call the game's native load check
	check();

	// Indicate that we haven't finished loading
	return 1;
}

//------------------------------------------------------------------------------
void hookedLoadingScreen(u64 a0, void * a1, u64 a2, u64 a3, u64 t0, u64 t1, u64 t2)
{
	if (MapLoaderState.Enabled && HAS_LOADED_MODULES && readLevelBgUsb(a1, a3 * 0x800) > 0)
	{

	}
	else
	{
		((void (*)(u64, void *, u64, u64, u64, u64, u64))0x00163808)(a0, a1, a2, a3, t0, t1, t2);
	}
}

//------------------------------------------------------------------------------
void hookedGetTable(u32 startSector, u32 sectorCount, u8 * dest, u32 levelId)
{
	// load table
	((void (*)(u32,u32,void*))0x00159A00)(startSector, sectorCount, dest);

	// Disable when loading menu
	if (levelId == 0)
	{
			MapLoaderState.Enabled = 0;
			return;
	}

	// Check if loading MP map
	if (MapLoaderState.Enabled && HAS_LOADED_MODULES)
	{
		// Disable if map doesn't match
		if (levelId != MapLoaderState.MapId && (levelId - 20) != MapLoaderState.MapId)
		{
			MapLoaderState.Enabled = 0;
			return;
		}

		int fSize = getLevelSizeUsb();
		if (fSize > 0)
		{
				((int*)dest)[7] = (fSize / 0x800) + 1;
		}
		else
		{
				MapLoaderState.Enabled = 0;
				DPRINTF("Error reading level wad from usb\n");
		}
	}
}

//------------------------------------------------------------------------------
void hookedGetMap(u64 a0, void * dest, u32 startSector, u32 sectorCount, u64 t0, u64 t1, u64 t2)
{
	// Check if loading MP map
	if (MapLoaderState.Enabled && HAS_LOADED_MODULES)
	{
		// We hardcode the size because that's the max that deadlocked can hold
		if (readLevelMapUsb(dest, 0x27400))
			return;
	}

	((void (*)(u64, void*,u32,u32,u64,u64,u64))0x00163808)(a0, dest, startSector, sectorCount, t0, t1, t2);
}

//------------------------------------------------------------------------------
void hookedGetHud(u64 a0, void * dest, u32 startSector, u32 sectorCount, u64 t0, u64 t1, u64 t2)
{
	((void (*)(u64, void*,u32,u32,u64,u64,u64))0x00163808)(a0, dest, startSector, sectorCount, t0, t1, t2);
}

//------------------------------------------------------------------------------
void hookedSoundCoreBankLoad(int loc, int offset, SndCompleteProc cb, u64 user_data)
{
	// Check if loading MP map
	if (MapLoaderState.Enabled && HAS_LOADED_MODULES && MapLoaderState.LevelBuffer > 0 && MapLoaderState.SoundBuffer > 0)
	{
    MapLoaderState.SoundLoadCb = cb;
    MapLoaderState.SoundLoadUserData = user_data;
  
    // sound wad finished, pass to Cb
    if (cb && (MapLoaderState.Loaded & 4) == 0) {
      int r = ((int (*)(void*))0x00158400)(MapLoaderState.SoundBuffer);
      DPRINTF("load sound bank from EE %08X to IOP %08X\n", MapLoaderState.SoundBuffer, r);
      MapLoaderState.SoundLoadCb(r, MapLoaderState.SoundLoadUserData);
      MapLoaderState.SoundLoadCb = NULL; // reset
      MapLoaderState.Loaded |= 4;
    }

    // we're loading the sound wad from USB
    return;
	}

  // snd_BankLoadByLoc_CB
	((void (*)(int, int, SndCompleteProc, u64))0x001582a8)(loc, offset, cb, user_data);
}

//------------------------------------------------------------------------------
u64 hookedLoadCdvd(u64 a0, u64 a1, u64 a2, u64 a3, u64 t0, u64 t1, u64 t2)
{
	u64 result = ((u64 (*)(u64,u64,u64,u64,u64,u64,u64))0x00163840)(a0, a1, a2, a3, t0, t1, t2);

	// We try and hook here to just to make sure that after tha game loads
	// We can still load our custom minimap
	hook();

	return result;
}

//------------------------------------------------------------------------------
char* hookedLoadScreenMapNameString(char * dest, char * src)
{
	if (MapLoaderState.Enabled)
		strncpy(dest, MapLoaderState.MapName, 32);
	else
		strncpy(dest, src, 32);
	return dest;
}

//------------------------------------------------------------------------------
char* hookedLoadScreenModeNameString(char * dest, char * src)
{
	int i = 0, j = 0;

	// if we're loading a custom map
	// and that map has an exclusive gamemode
	// save map name as gamemode
  if (patchStateContainer.SelectedCustomMapId > 0) {
    if (customMapDefs[patchStateContainer.SelectedCustomMapId-1].ForcedCustomModeId < 0) {
      strncpy(dest, customMapDefs[patchStateContainer.SelectedCustomMapId-1].Name, 32);
      return dest;
    }
  }

	// if custom mode is set
	if (gameConfig.customModeId > 0) {
    for (j = 0; j < dataCustomModes.count; ++j) {
      if (dataCustomModes.items[j].value == gameConfig.customModeId) {
		    strncpy(dest, dataCustomModes.items[j].name, 32);
        break;
      }
    }
  } else {
		strncpy(dest, src, 32);
  }

	return dest;
}

//------------------------------------------------------------------------------
int mapsGetInstallationResult(void)
{
	return LOAD_MODULES_RESULT;
}

//------------------------------------------------------------------------------
int mapsDownloadingModules(void)
{
	return actionState == ACTION_DOWNLOADING_MODULES;
}

//--------------------------------------------------------------------------
int align(int addr, int align)
{
	if (addr % align)
		return addr + (align - (addr % align));
	
	return addr;
}

//------------------------------------------------------------------------------
int mapsAllocateModuleBuffer(void)
{
	if (!USB_FS_MODULE_PTR) {
		USB_FS_MODULE_PTR = malloc(41100);
		if (USB_FS_MODULE_PTR) {
			memset(USB_FS_MODULE_PTR, 0, 41100);
			USB_FS_MODULE_PTR = (void*)align((int)USB_FS_MODULE_PTR, 0x10);
		}
	}
	if (!USB_SRV_MODULE_PTR) {
		USB_SRV_MODULE_PTR = malloc(12000);
		if (USB_SRV_MODULE_PTR) {
			memset(USB_SRV_MODULE_PTR, 0, 12000);
			USB_SRV_MODULE_PTR = (void*)align((int)USB_SRV_MODULE_PTR, 0x10);
		}
	}

	DPRINTF("fs:%08X srv:%08x\n", (u32)USB_FS_MODULE_PTR, (u32)USB_SRV_MODULE_PTR);

	if (!USB_FS_MODULE_PTR)
		return 0;
	if (!USB_SRV_MODULE_PTR)
		return 0;

	return 1;
}

//------------------------------------------------------------------------------
int mapsPromptEnableCustomMaps(void)
{
	MapClientRequestModulesMessage request = { 0, 0 };
	if (uiShowYesNoDialog("Enable Custom Maps", "Are you sure?") == 1)
	{
		if (!mapsAllocateModuleBuffer())
			return -1;

		// request irx modules from server
		request.Module1Start = (u32)USB_FS_MODULE_PTR;
		request.Module2Start = (u32)USB_SRV_MODULE_PTR;
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_REQUEST_MAP_IRX_MODULES, sizeof(MapClientRequestModulesMessage), &request);
		actionState = ACTION_DOWNLOADING_MODULES;
		return 1;
	}

	return 0;
}

//------------------------------------------------------------------------------
void onMapLoaderOnlineMenu(void)
{
	u32 bgColorDownload = 0x70000000;

	if (actionState == ACTION_DOWNLOADING_MODULES)
	{
		// disable input
		padDisableInput();

		// render background
		//gfxScreenSpaceBox(0.2, 0.35, 0.6, 0.3, bgColorDownload);

		// flash color
		//u32 downloadColor = 0x80808080;
		//int gameTime = ((gameGetTime()/100) % 15);
		//if (gameTime > 7)
		//	gameTime = 15 - gameTime;
		//downloadColor += 0x101010 * gameTime;

		// render text
		//gfxScreenSpaceText(SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.5, 1, 1, downloadColor, "Downloading modules, please wait...", -1, 4);
	}
	else if (actionState == ACTION_MODULES_INSTALLED)
	{
		// enable input
		padEnableInput();

		//uiShowOkDialog("Custom Maps", "Custom maps have been enabled.");

		actionState = ACTION_NONE;
		LOAD_MODULES_RESULT = 1;
	}
	else if (actionState == ACTION_NEW_MAPS_UPDATE)
	{
		// enable input
		padEnableInput();
		
		uiShowOkDialog("Custom Maps", "New updates are available. Please download them at https://rac-horizon.com");
		actionState = ACTION_MODULES_INSTALLED;
		LOAD_MODULES_RESULT = 2;
	}
	else if (actionState == ACTION_ERROR_LOADING_MODULES)
	{
		// enable input
		padEnableInput();
		
		uiShowOkDialog("Custom Maps", "There was an error loading the custom map modules.");
		actionState = ACTION_NONE;
		LOAD_MODULES_RESULT = -1;
	}
#if MAPDOWNLOADER
	else if (DownloadState.Enabled == 2)
	{
		padEnableInput();
		DownloadState.Enabled = 0;
	}
	else if (DownloadState.Enabled == 1)
	{
		// disable input
		padDisableInput();

    gfxScreenSpaceBox(0.2, 0.35, 0.6, 0.125, colorBlack);
    gfxScreenSpaceBox(0.2, 0.45, 0.6, 0.05, colorContentBg);

		sprintf(membuffer, "Downloading %s... %.02f kbps", DownloadState.MapName, (float)DownloadState.TotalDownloaded / (1024.0 * (DownloadState.Ticks / 60.0)));
    gfxScreenSpaceText(SCREEN_WIDTH * 0.22, SCREEN_HEIGHT * 0.4, 1, 1, colorText, membuffer, -1, 3);

		float w = (float)DownloadState.TotalDownloaded / (float)DownloadState.TotalSize;
		gfxScreenSpaceBox(0.2, 0.45, 0.6 * w, 0.05, colorRed);

		if (padGetButtonDown(0, PAD_CIRCLE) > 0) {
			DPRINTF("canceling\n");
			DownloadState.Cancel = 1;
			DownloadState.Enabled = 2;
		}

		if (!netGetLobbyServerConnection()) {
			DownloadState.Enabled = 2;
		}
	}
#endif
	else if (initialized == 2)
	{
		if (!mapsAllocateModuleBuffer())
		{
			printf("failed to allocation module buffers\n");
			initialized = 1;
		}

		// request irx modules from server
		MapClientRequestModulesMessage request = { 0, 0 };
		request.Module1Start = (u32)USB_FS_MODULE_PTR;
		request.Module2Start = (u32)USB_SRV_MODULE_PTR;
		netSendCustomAppMessage(NET_DELIVERY_CRITICAL, netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_REQUEST_MAP_IRX_MODULES, sizeof(MapClientRequestModulesMessage), &request);
		actionState = ACTION_DOWNLOADING_MODULES;
		initialized = 1;
	}

	return;
}

//------------------------------------------------------------------------------
void hook(void)
{
	// 
	u32 * hookLoadAddr = (u32*)0x005CFB48;
	u32 * hookCheckAddr = (u32*)0x005CF9B0;
	u32 * hookLoadingScreenAddr = (u32*)0x00705554;
	u32 * hookTableAddr = (u32*)0x00159B20;
	u32 * hookMapAddr = (u32*)0x00557580;
	u32 * hookHudAddr = (u32*)0x0053F970;
  u32 * hookSoundCoreBank = (u32*)0x005FEC74;
	u32 * hookLoadCdvdAddr = (u32*)0x00163814;
	u32 * hookLoadScreenMapNameStringAddr = (u32*)0x007055B4;
	u32 * hookLoadScreenModeNameStringAddr = (u32*)0x0070583C;

  if (isInGame()) {
    hookLoadAddr = (u32*)0x004e9ed0;
    hookCheckAddr = (u32*)0x004e9d38;
    hookLoadingScreenAddr = (u32*)0x00081000;
    hookTableAddr = (u32*)0x00159B20;
    hookSoundCoreBank = (u32*)0x0051f80c;
    hookLoadCdvdAddr = (u32*)0x00163814;
    hookLoadScreenMapNameStringAddr = (u32*)0x00081000;
    hookLoadScreenModeNameStringAddr = (u32*)0x00081000;
  }

	// Load modules
	u32 * hookLoadModulesAddr = (u32*)0x00161364;

	// For some reason we can't load the IRX modules whenever we want
	// So here we hook into when the game uses rpc calls
	// This triggers when entering the online profile select, leaving profile select, and logging out.
	//if (!initialized || *hookLoadModulesAddr == 0x0C054632)
	//	*hookLoadModulesAddr = 0x0C000000 | ((u32)(&loadHookFunc) / 4);

	// Install hooks
	if (!initialized || *hookLoadAddr == 0x0C058E10)
	{
		*hookTableAddr = 0x0C000000 | ((u32)(&hookedGetTable) / 4);
		*hookLoadingScreenAddr = 0x0C000000 | ((u32)&hookedLoadingScreen / 4);
		*hookLoadCdvdAddr = 0x0C000000 | ((u32)&hookedLoadCdvd / 4);
		*hookCheckAddr = 0x0C000000 | ((u32)(&hookedCheck) / 4);
		*hookLoadAddr = 0x0C000000 | ((u32)(&hookedLoad) / 4);
    *hookSoundCoreBank = 0x0C000000 | ((u32)(&hookedSoundCoreBankLoad) / 4);
	}

	// These get hooked after the map loads but before the game starts
	if (!initialized || *hookMapAddr == 0x0C058E02)
	{
		*hookMapAddr = 0x0C000000 | ((u32)(&hookedGetMap) / 4);
	}

	if (!initialized || *hookLoadScreenMapNameStringAddr == 0x0C046A7B)
	{
		*hookLoadScreenMapNameStringAddr = 0x0C000000 | ((u32)(&hookedLoadScreenMapNameString) / 4);
		*hookLoadScreenModeNameStringAddr = 0x0C000000 | ((u32)(&hookedLoadScreenModeNameString) / 4);
	}
}

//------------------------------------------------------------------------------
void runMapLoader(void)
{
  // prevent refreshing maps when redownloading patch in game (debug)
  if (!isInMenus() && !initialized) return;

	// 
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SET_MAP_OVERRIDE, &onSetMapOverride);
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_SENT_MAP_IRX_MODULES, &onServerSentMapIrxModules);
  netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_SENT_CMAPS_GLOBAL_VERSION, onServerSendMapVersion);
#if MAPDOWNLOADER
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_INITIATE_DOWNLOAD_MAP_RESPONSE, &onServerSentMapInitiated);
	netInstallCustomMsgHandler(CUSTOM_MSG_ID_SERVER_DOWNLOAD_MAP_CHUNK_REQUEST, &onServerSentMapChunk);
#endif

	// hook irx module loading 
	hook();

#if MAPDOWNLOADER
	if (DownloadState.Enabled)
		++DownloadState.Ticks;
#endif

  if (isUnloading && customMapDefs) {
    free(customMapDefs);
    customMapDefs = NULL;
  }

	// 
	if (!initialized)
	{
		initialized = 1;

		// set map loader defaults
		MapLoaderState.Enabled = 0;
		MapLoaderState.CheckState = 0;

    if (!customMapDefs) {
      customMapDefs = malloc(sizeof(CustomMapDef_t) * MAX_CUSTOM_MAP_DEFINITIONS);
      DPRINTF("alloc custom maps defs %08X\n", customMapDefs);
    }

		// install on login
		if (LOAD_MODULES_RESULT == 0)
		{
			initialized = 2;
		}
    else if (HAS_LOADED_MODULES)
    {
      // check if host fs exists
      useHost = 1;
      if (!readGlobalVersion(&mapsLocalGlobalVersion)) {
        useHost = 0;
        readLocalGlobalVersion();
      }
        
      // refresh map list
      refreshCustomMapList();
    }
	}

	// force map id to current map override if in staging
	if (MapLoaderState.Enabled == 1 && !isInGame())
	{
		GameSettings * settings = gameGetSettings();
		if (settings && settings->GameLoadStartTime > 0)
		{
			settings->GameLevel = MapLoaderState.MapId;
		}

    // reset before we load
    if (isInMenus()) {
      MapLoaderState.LevelBuffer = NULL;
      MapLoaderState.SoundBuffer = NULL;
      MapLoaderState.SoundLoadCb = NULL;
    }
	}

  //
  if (actionState == ACTION_REFRESHING_MAPLIST) {
    
    gfxScreenSpaceBox(0.2, 0.35, 0.6, 0.125, colorBlack);
    gfxScreenSpaceBox(0.2, 0.45, 0.6, 0.05, colorContentBg);

		sprintf(membuffer, "Scanning USB drive for custom maps...");
    gfxScreenSpaceText(SCREEN_WIDTH * 0.22, SCREEN_HEIGHT * 0.4, 1, 1, colorText, membuffer, -1, 3);

		float w = (float)customMapDefCount / (float)MAX_CUSTOM_MAP_DEFINITIONS;
		gfxScreenSpaceBox(0.2, 0.45, 0.6 * w, 0.05, colorRed);
  }

  // dzo always use hostfs
  if (PATCH_INTEROP->Client == CLIENT_TYPE_DZO) useHost = 1;
}

//------------------------------------------------------------------------------
int mapReadCustomMapExtraData(void* dst, int len)
{
  //
  int currentCustomModeId = gameConfig.customModeId;
  if (MapLoaderState.MapFileName[0] && currentCustomModeId > 0) {
    char buffer[1024];
    char filepath[256];
    snprintf(filepath, sizeof(filepath), fVersion, getMapPathPrefix(), MapLoaderState.MapFileName);

    int read = readFile(filepath, buffer, 0, sizeof(buffer));
    if (read < sizeof(CustomMapVersionFileDef_t))
      return 0;

    CustomMapVersionFileDef_t customMapVersion;
    memcpy(&customMapVersion, buffer, sizeof(customMapVersion));

    int i;
    for (i = 0; i < customMapVersion.ExtraDataCount; ++i) {
      short modeId = *(short*)((u32)buffer + 0x30 + 8*i);
      if (modeId == currentCustomModeId) {
        short extraDataLen = *(short*)((u32)buffer + 0x32 + 8*i);
        int extraDataOffset = *(int*)((u32)buffer + 0x34 + 8*i);
        int readLen = (extraDataLen < len) ? extraDataLen : len;

        // check if we already read data
        if ((extraDataOffset+extraDataLen) < sizeof(buffer)) {
          memcpy(dst, &buffer[extraDataOffset], extraDataLen);
        } else if (readFile(filepath, dst, extraDataOffset, readLen) != readLen) {
          return 0;
        }

        DPRINTF("read %d bytes for extra data\n", read);
        return 1;
      }
    }
  }

  return 0;
}
