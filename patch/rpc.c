#include <libdl/string.h>
#include "rpc.h"

#define CMD_NONE  			0x00
#define CMD_USBOPEN		  0x01
#define CMD_USBWRITE		0x02
#define CMD_USBCLOSE		0x03
#define CMD_USBREAD			0x04
#define CMD_USBSEEK			0x05
#define CMD_USBREMOVE   0x06
#define CMD_USBGETSTAT  0x07
#define CMD_USBMKDIR    0x08

#define UNCACHED_SEG(x) \
    ((void *)(((u32)(x)) | 0x20000000))

#define IS_UNCACHED_SEG(x) \
    (((u32)(x)) & 0x20000000)

#define RPCCLIENT_INITED (*(int*)0x000CFF00)

static SifRpcClientData_t * rpcclient = (SifRpcClientData_t*)0x000CFF10;
static int Rpc_Buffer[16] 			__attribute__((aligned(64)));

static struct { 			// size = 256
	int flags;
	char filename[256];	// 0
} openParam __attribute__((aligned(64)));

static struct { 		// size =
	int fd;				// 0
	int size;			// 8
	void * buf;
  unsigned int unalignedDataLen;
  unsigned char unalignedData[64];
} writeParam __attribute__((aligned(64)));

static struct { 		// size = 16
	int fd;				// 0
	u8 pad[12];
} closeParam __attribute__((aligned(64)));

static struct { 		// size =
	int fd;				// 0
	void * buf;		// 4
	int size;			// 
} readParam __attribute__((aligned(64)));

static struct { 		// size =
	int fd;				// 0
	int offset;
	int whence;
} seekParam __attribute__((aligned(64)));

static struct { 			// size = 256
	char filename[256];	// 0
} removeParam __attribute__((aligned(64)));

static struct { 			// size = 272
	char filename[256];	// 0
  struct stat * st;   // 260
  char pad[12];
} getstatParam __attribute__((aligned(64)));

static struct { 			// size = 256
	char dirpath[256];	// 0
} mkdirParam __attribute__((aligned(64)));


// stores command currently being executed on the iop
static unsigned int currentCmd = 0;


//--------------------------------------------------------------
int rpcUSBInit(void)
{
	int ret = 0;

	if (RPCCLIENT_INITED)
		return -2;

	// 
	memset(rpcclient, 0, sizeof(SifRpcClientData_t));

	SifInitRpc(0);
		
	// bind to rpc on iop
	do {
		if ((ret = SifBindRpc(rpcclient, 0x80033a10, 0)) < 0)
			return ret;
			
		if (rpcclient->server == NULL)
			nopdelay();
			
	} while (rpcclient->server == NULL);
 	
	// successfully inited
	RPCCLIENT_INITED = 1;

	return 1;
}

//--------------------------------------------------------------
int rpcUSBReset(void)
{
	RPCCLIENT_INITED = 0;
	rpcclient->server = NULL;
	return 1; 
}

//--------------------------------------------------------------
int rpcUSBopen(char *filename, int flags)
{
	int ret = 0;

	// check lib is inited
	if (!RPCCLIENT_INITED)
		return -1;
			
	// set global variables
	if (filename)
		memcpy(openParam.filename, filename, 1020);
	else
		return -2;	
	
	// Set mode
	openParam.flags = flags;

	if((ret = SifCallRpc(rpcclient, CMD_USBOPEN, SIF_RPC_M_NOWAIT, &openParam, sizeof(openParam), Rpc_Buffer, 4, 0, 0)) != 0) {
		return ret;
	}
			
	currentCmd = CMD_USBOPEN;
	
	return 1;
}

//--------------------------------------------------------------
int rpcUSBwrite(int fd, void *buf, int size)
{
	int ret = 0;
  unsigned int miss = 0;

	// check lib is inited
	if (!RPCCLIENT_INITED)
		return -1;

	if((unsigned int)buf & 0x3F)
	{
		miss = 64 - ((unsigned int)buf & 0x3F);
		if(miss > (unsigned int)size) miss = size;
	} else {
		miss = 0;
	}

	// set global variables
	writeParam.fd = fd;
  writeParam.buf = buf;
	//memcpy(writeParam.buf, buf, size);
	writeParam.size = size;
  writeParam.unalignedDataLen = miss;
	memcpy(writeParam.unalignedData, buf, miss);

	if(!IS_UNCACHED_SEG(buf))
	  SifWriteBackDCache(buf, size);
	 	
	if((ret = SifCallRpc(rpcclient, CMD_USBWRITE, SIF_RPC_M_NOWAIT, &writeParam, sizeof(writeParam), Rpc_Buffer, 4, 0, 0)) != 0) {
		return ret;
	}
			
	currentCmd = CMD_USBWRITE;
	
	return ret;
}

//--------------------------------------------------------------
int rpcUSBclose(int fd)
{
	int ret = 0;

	// check lib is inited
	if (!RPCCLIENT_INITED)
		return -1;
			
	// set global variables
	closeParam.fd = fd;
	 	
	if((ret = SifCallRpc(rpcclient, CMD_USBCLOSE, SIF_RPC_M_NOWAIT, &closeParam, sizeof(closeParam), Rpc_Buffer, 4, 0, 0)) != 0) {
		return ret;
	}
			
	currentCmd = CMD_USBCLOSE;
	
	return 1;
}

//--------------------------------------------------------------
int rpcUSBread(int fd, void *buf, int size)
{
	int ret = 0;

	// check lib is inited
	if (!RPCCLIENT_INITED)
		return -1;
			
	// set global variables
	readParam.fd = fd;
	readParam.buf = buf;
	readParam.size = size;

	SifWriteBackDCache(buf, size);
	 	
	if((ret = SifCallRpc(rpcclient, CMD_USBREAD, SIF_RPC_M_NOWAIT, &readParam, sizeof(readParam), Rpc_Buffer, 4, 0, 0)) != 0) {
		return ret;
	}

	currentCmd = CMD_USBREAD;

	return ret;
}

//--------------------------------------------------------------
int rpcUSBseek(int fd, int offset, int whence)
{
	int ret = 0;

	// check lib is inited
	if (!RPCCLIENT_INITED)
		return -1;
			
	// set global variables
	seekParam.fd = fd;
	seekParam.offset = offset;
	seekParam.whence = whence;
	 	
	if((ret = SifCallRpc(rpcclient, CMD_USBSEEK, SIF_RPC_M_NOWAIT, &seekParam, sizeof(seekParam), Rpc_Buffer, 4, 0, 0)) != 0) {
		return ret;
	}
			
	currentCmd = CMD_USBSEEK;
	
	return 1;
}

//--------------------------------------------------------------
int rpcUSBremove(char *filename)
{
	int ret = 0;

	// check lib is inited
	if (!RPCCLIENT_INITED)
		return -1;
			
	// set global variables
	if (filename)
		memcpy(removeParam.filename, filename, sizeof(removeParam.filename));
	else
		return -2;	
	
	if((ret = SifCallRpc(rpcclient, CMD_USBREMOVE, SIF_RPC_M_NOWAIT, &removeParam, sizeof(removeParam), Rpc_Buffer, 4, 0, 0)) != 0) {
		return ret;
	}
			
	currentCmd = CMD_USBREMOVE;
	
	return 1;
}

//--------------------------------------------------------------
int rpcUSBgetstat(char *filename, struct stat * st)
{
	int ret = 0;

	// check lib is inited
	if (!RPCCLIENT_INITED)
		return -1;
			
	// set global variables
	if (filename)
		memcpy(getstatParam.filename, filename, sizeof(getstatParam.filename));
	else
		return -2;	
	
  getstatParam.st = st;
	if((ret = SifCallRpc(rpcclient, CMD_USBGETSTAT, SIF_RPC_M_NOWAIT, &getstatParam, sizeof(getstatParam), Rpc_Buffer, 4, 0, 0)) != 0) {
		return ret;
	}
			
	currentCmd = CMD_USBGETSTAT;
	
	return ret;
}

//--------------------------------------------------------------
int rpcUSBmkdir(char *dirpath)
{
	int ret = 0;

	// check lib is inited
	if (!RPCCLIENT_INITED)
		return -1;
			
	// set global variables
	if (dirpath)
		memcpy(mkdirParam.dirpath, dirpath, sizeof(mkdirParam.dirpath));
	else
		return -2;	
	
	if((ret = SifCallRpc(rpcclient, CMD_USBMKDIR, SIF_RPC_M_NOWAIT, &mkdirParam, sizeof(mkdirParam), Rpc_Buffer, 4, 0, 0)) != 0) {
		return ret;
	}
			
	currentCmd = CMD_USBMKDIR;
	
	return ret;
}

//--------------------------------------------------------------
int rpcUSBSync(int mode, int *cmd, int *result)
{
	int funcIsExecuting, i;
	
	// check if any functions are registered
	if (currentCmd == CMD_NONE)
		return -1;
	
	// check if function is still processing
	funcIsExecuting = SifCheckStatRpc(rpcclient);
	
	// if mode = 0, wait for function to finish
	if (mode == 0) {
		while (SifCheckStatRpc(rpcclient)) {
			for (i=0; i<100000; i++) ;
		}
		// function has finished
		funcIsExecuting = 0;
	}
	
	// get the function that just finished
	if (cmd)
		*cmd = currentCmd;
	
	// if function is still processing, return 0
	if (funcIsExecuting == 1)
		return 0;
	
	// function has finished, so clear last command
	currentCmd = CMD_NONE;
	
	// get result
	if(result)
		*result = *(int*)Rpc_Buffer;
	
	return 1;
}

//--------------------------------------------------------------
int rpcUSBSyncNB(int mode, int *cmd, int *result)
{
	int funcIsExecuting;
	
	// check if any functions are registered
	if (currentCmd == CMD_NONE)
		return -1;
	
	// check if function is still processing
	funcIsExecuting = SifCheckStatRpc(rpcclient);
	
	// get the current rpc cmd
	if (cmd)
		*cmd = currentCmd;
	
	// if function is still processing, return 0
	if (funcIsExecuting == 1)
		return 0;
	
	// function has finished, so clear last command
	currentCmd = CMD_NONE;
	
	// get result
	if(result)
		*result = *(int*)Rpc_Buffer;
	
	return 1;
}
