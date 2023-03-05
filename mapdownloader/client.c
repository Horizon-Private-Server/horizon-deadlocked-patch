#include <tamtypes.h>
#include <libdl/stdio.h>
#include <libdl/stdlib.h>
#include <libdl/string.h>
#include <libdl/pad.h>

#include "db.h"

#define BUF_SIZE            (1024 * 2)

struct RTCommSock;

typedef void (*RTCommSock_Destructor_func)(struct RTCommSock* this, int a1);
typedef void* (*RTCommSock_DNSLookup_func)(struct RTCommSock* this, char * hostname, void * memoryContextBase, u32* result);
typedef int (*RTCommSock_DNSLookupNonBlockingQuery_func)(struct RTCommSock* this, void ** svAddr);
typedef int (*RTCommSock_UnserializeAddr_func)(struct RTCommSock* this);
typedef int (*RTCommSock_AddrAsString_func)(struct RTCommSock* this, void* svAddr, char * str, int maxLen);
typedef int (*RTCommSock_Connect_func)(struct RTCommSock* this, void* svAddr, int port);
typedef int (*RTCommSock_WaitForConnect_func)(struct RTCommSock* this);
typedef int (*RTCommSock_Send_func)(struct RTCommSock* this, char* buf, int len, u64* sendCount);
typedef int (*RTCommSock_Recv_func)(struct RTCommSock* this, char* buf, int bufSize, int* recvLen, int* isFinished);
typedef int (*RTCommSock_Close_func)(struct RTCommSock* this, int forceClose);
typedef void (*RTCommSock_SetNonBlocking_func)(struct RTCommSock* this, int nonBlocking);
typedef int (*RTCommSock_GetNonBlocking_func)(struct RTCommSock* this);
typedef void (*RTCommSock_SetSynchronous_func)(struct RTCommSock* this, int synchronous);
typedef int (*RTCommSock_GetSynchronous_func)(struct RTCommSock* this);
typedef int (*RTCommSock_GetErrNo_func)(struct RTCommSock* this);
typedef void (*RTCommSock_SetErrNo_func)(struct RTCommSock* this, int err);

struct RTCommSockVTable
{
    void * FUNC_00;
    void * FUNC_04;
    RTCommSock_Destructor_func Destructor;
    RTCommSock_DNSLookup_func DNSLookup;
    RTCommSock_DNSLookup_func DNSLookupBlocking;
    RTCommSock_DNSLookup_func DNSLookupNonBlocking;
    RTCommSock_DNSLookupNonBlockingQuery_func DNSLookupNonBlockingQuery;
    RTCommSock_UnserializeAddr_func UnserializeAddr;
    RTCommSock_AddrAsString_func AddrAsString;
    RTCommSock_Connect_func Connect;
    RTCommSock_WaitForConnect_func WaitForConnect;
    RTCommSock_Send_func Send;
    RTCommSock_Recv_func Recv;
    RTCommSock_Close_func Close;
    RTCommSock_SetNonBlocking_func SetNonBlocking;
    RTCommSock_GetNonBlocking_func GetNonBlocking;
    RTCommSock_SetSynchronous_func SetSynchronous;
    RTCommSock_GetSynchronous_func GetSynchronous;
    RTCommSock_GetErrNo_func GetErrNo;
    RTCommSock_SetErrNo_func SetErrNo;
};

typedef struct RTCommSock
{
  int ErrNo;
  void * MemoryContext;
  struct RTCommSockVTable* VTable;
  void * RTCommCID;
  char OutBuf[28];
  char HostnameBeingLookedUp[64];
  int NonBlocking;
  u8* ScratchBuffer;
  void * SVAddrVTable;
  long unsigned int Addr[2];
  int Synchronous;
} RTCommSock_t;

const char* HOSTNAME = "cdn.rac-horizon.com";
const char* HOSTNAME2 = "ratchetdl-prod.pdonline.scea.com";

const int PORT = 80;

void* memoryContext = NULL;
RTCommSock_t* commSock = NULL;

long comm_startup(void)
{
  ((long (*)(void))0x01ea7408)();
}

void comm_update(void)
{
  ((void (*)(void))0x01ea7538)();
}

void client_createCommSock(void * this, void * memoryContextBase)
{
  return ((void* (*)(void*, void*))0x01ef4e64)(this, memoryContextBase);
}

int client_connect(void)
{
  DPRINTF("connecting to %s...\n", HOSTNAME);
  char buf[64];
  
  u32 result = 0;

  void* lookupResult = commSock->VTable->DNSLookupBlocking(commSock, HOSTNAME, memoryContext, &result);
  if (!result || !lookupResult)
    return -1;

  //*(u32*)((u32)lookupResult + 0x8) = 0xCFE20F34;

  int connectResult = commSock->VTable->Connect(commSock, lookupResult, PORT);
  if (!connectResult)
    return -2;

  commSock->VTable->AddrAsString(commSock, lookupResult, buf, sizeof(buf));
  DPRINTF("connected %08X %d %s\n", (u32)lookupResult, connectResult, buf);
  return 0;
}

int client_close(void)
{
  DPRINTF("disconnecting...\n");
  
  if (!commSock->VTable->Close(commSock, 1))
    return -1;

  return 0;
}

int client_get(char * path, char * output, int output_size)
{
  int i,j=0;
  char buffer[BUF_SIZE + 1]; /* +1 so we can add null terminator */
  char encoded_path[512];
  int sendCount = 0;
  int len = 0;

  // encode path
  int path_len = strlen(path);
  for (i = 0; i < path_len; ++i) {
    if (path[i] == ' ') {
      encoded_path[j++] = '%';
      encoded_path[j++] = '2';
      encoded_path[j++] = '0';
    } else {
      encoded_path[j++] = path[i];
    }
  }
  encoded_path[j] = '\0';

  sprintf(buffer, "GET %s HTTP/1.1\r\nUser-Agent: PS2HorizonClient\r\nAccept: */*\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n", encoded_path, HOSTNAME);
  int sendResult = commSock->VTable->Send(commSock, buffer, strlen(buffer), &sendCount);
  DPRINTF("sent res:%d cnt:%d\n", sendResult, sendCount);

  len = client_recv(output, output_size);
  DPRINTF("recv len:%d\n", len);

  return len;
}

int client_recv(char * output, int output_size)
{
  int isFinished = 0;
  int len = 0;
  int recvResult = 1;
  int waits = 0;
  
  while (!len && recvResult) {
    // update logic
    if ((waits % 10000) == 9999) {
      uiRunCallbacks();
    }
    if ((waits % 1000) == 500) {
      comm_update();
    }

    if (dbStateCancel)
      return -1;

    recvResult = commSock->VTable->Recv(commSock, output, output_size, &len, &isFinished);
    ++waits;
  }

  DPRINTF("recv res:%d len:%d finished:%d waits:%d\n", recvResult, len, isFinished, waits);
  return len;
}

int client_init(void)
{
  *(u32*)0x0016e4c8 = 1024 * 16;

  if (!memoryContext) {
    DPRINTF("mem %08X\n", ((u32 (*)(void))0x01f07100)() );
    memoryContext = ((void* (*)(void))0x01f07100)(); //malloc(0x10420);
  }

  if (!commSock) {

    comm_startup();
    comm_update();

    commSock = malloc(0xa0);
    client_createCommSock(commSock, memoryContext);
    if (commSock->VTable) {
      commSock->VTable->SetNonBlocking(commSock, 0);
      commSock->VTable->SetSynchronous(commSock, 3);
    }
    DPRINTF("created comm sock at %08X\n", commSock);
  }

  return 0;
}

void client_destroy(void)
{
  if (memoryContext) {
    //free(memoryContext);
    memoryContext = NULL;
  }

  if (commSock) {
    free(commSock);
    commSock = NULL;
  }
}
