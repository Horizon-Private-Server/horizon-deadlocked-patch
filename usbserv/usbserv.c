
#include <loadcore.h>
#include <stdio.h>
#include <sysclib.h>
#include <sysmem.h>
#include <thbase.h>
#include <thevent.h>
#include <intrman.h>
#include <sifcmd.h>
#include <sifman.h>
#include <thsemap.h>
#include <errno.h>
#include <io_common.h>
//#include <ioman.h>
#include <iomanX.h>
#include <intrman.h>


#define USBSERV_BUFSIZE (1024 * 8)
#define USBSERV_RPC_BUFSIZE (USBSERV_BUFSIZE + 8)

#define MODNAME "usbserv"
IRX_ID(MODNAME, 1, 0);

int usbserv_io_sema; 	// IO semaphore

static int rpc_tidS_0A10;

static SifRpcDataQueue_t rpc_qdS_0A10 __attribute__((aligned(64)));
static SifRpcServerData_t rpc_sdS_0A10 __attribute__((aligned(64)));

static u8 usbserv_rpc_buf[USBSERV_RPC_BUFSIZE] __attribute__((aligned(64)));

static struct {		
	int rpc_func_ret;
	int usbserv_version;
} rpc_stat __attribute__((aligned(64)));

int (*rpc_func)(void);

void dummy(void) {}
int  rpcUSBopen(void);
int _rpcUSBopen(void *rpc_buf);
int  rpcUSBwrite(void);
int _rpcUSBwrite(void *rpc_buf);
int  rpcUSBread(void);
int _rpcUSBread(void *rpc_buf);
int  rpcUSBseek(void);
int _rpcUSBseek(void *rpc_buf);
int  rpcUSBclose(void);
int _rpcUSBclose(void *rpc_buf);
int rpcUSBremove(void);
int _rpcUSBremove(void *rpc_buf);
int rpcUSBgetstat(void);
int _rpcUSBgetstat(void *rpc_buf);
int rpcUSBmkdir(void);
int _rpcUSBmkdir(void *rpc_buf);
int  rpcUSBdopen(void);
int _rpcUSBdopen(void *rpc_buf);
int  rpcUSBdclose(void);
int _rpcUSBdclose(void *rpc_buf);
int  rpcUSBdread(void);
int _rpcUSBdread(void *rpc_buf);

// rpc command handling array
void *rpc_funcs_array[16] = {
    (void *)dummy,
    (void *)rpcUSBopen,
    (void *)rpcUSBwrite,
    (void *)rpcUSBclose,
    (void *)rpcUSBread,
    (void *)rpcUSBseek,
    (void *)rpcUSBremove,
    (void *)rpcUSBgetstat,
    (void *)rpcUSBmkdir,
    (void *)rpcUSBdopen,
    (void *)rpcUSBdclose,
    (void *)rpcUSBdread,
    (void *)dummy,
    (void *)dummy,
    (void *)dummy,
    (void *)dummy
};

typedef struct g_openParam {  // size = 1024
	int flags;				  // 
	char filename[256];	  // 0
	u8 pad[12];
} g_openParam_t;

typedef struct g_writeParam { // size =
	int fd;					  // 0
	int size;				  // 
	void* buf;
  unsigned int unalignedDataLen;
  unsigned char unalignedData[64];
} g_writeParam_t;

typedef struct g_closeParam { // size = 16
	int fd;	  				  // 0
	u8 pad[12];
} g_closeParam_t;

typedef struct g_readParam { // size =
	int fd;					  // 0
	void * buf;
	int size;				  // 
  void * read;
} g_readParam_t;

typedef struct g_seekParam { // size =
	int fd;					  // 0
	int offset;
	int whence;
	u8 pad[4];
} g_seekParam_t;

typedef struct g_removeParam {  // size = 256
	char filename[256];	  // 0
} g_removeParam_t;

typedef struct g_getstatParam {  // size = 272
	char filename[256];	  // 0
  void* st; // 260
	u8 pad[12];
} g_getstatParam_t;

typedef struct g_mkdirParam {  // size = 256
	char dirpath[256];	  // 0
} g_mkdirParam_t;

typedef struct g_dopenParam {  // size = 256
	char dirpath[256];	  // 0
} g_dopenParam_t;

typedef struct g_dcloseParam { // size = 16
	int fd;	  				  // 0
	u8 pad[12];
} g_dcloseParam_t;

typedef struct g_dreadParam { // size = 16
	int fd;	  				  // 0
  void* dirent; // 4
  char pad[8];
} g_dreadParam_t;

char filepath[262];

//--------------------------------------------------------------
void *cb_rpc_S_0A10(u32 fno, void *buf, int size)
{
	// Rpc Callback function
	
	if (fno >= 16)
		return (void *)&rpc_stat;

	// Get function pointer
	rpc_func = (void *)rpc_funcs_array[fno];
	
	// Call needed rpc func
	rpc_stat.rpc_func_ret = rpc_func();
	
	return (void *)&rpc_stat;
}

//--------------------------------------------------------------
void thread_rpc_S_0A10(void* arg)
{
	if (!sceSifCheckInit())
		sceSifInit();

	sceSifInitRpc(0);
	sceSifSetRpcQueue(&rpc_qdS_0A10, GetThreadId());
	sceSifRegisterRpc(&rpc_sdS_0A10, 0x80033a10, (void *)cb_rpc_S_0A10, &usbserv_rpc_buf, NULL, NULL, &rpc_qdS_0A10);
	sceSifRpcLoop(&rpc_qdS_0A10);
}

//-------------------------------------------------------------- 
int start_RPC_server(void)
{
	iop_thread_t thread_param;
	register int thread_id;	
			
 	thread_param.attr = TH_C;
 	thread_param.thread = (void *)thread_rpc_S_0A10;
 	thread_param.priority = 0x68;
 	thread_param.stacksize = 0x1000;
 	thread_param.option = 0;
			
	thread_id = CreateThread(&thread_param);
	rpc_tidS_0A10 = thread_id;
		
	StartThread(thread_id, 0);
	
	return 0;
}

//-------------------------------------------------------------- 
int rpcUSBopen(void)
{
	return _rpcUSBopen(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBopen(void *rpc_buf)
{
	int fd;
	g_openParam_t *eP = (g_openParam_t *)rpc_buf;	

	WaitSema(usbserv_io_sema);
	
	sprintf(filepath, "%s", eP->filename);
	fd = open(filepath, eP->flags);	
#if DEBUG
	printf("opening %s,%d -> %d\n", filepath, eP->flags, fd);
#endif
	
	SignalSema(usbserv_io_sema);
		
	return fd;
}

//-------------------------------------------------------------- 
int rpcUSBwrite(void)
{
	return _rpcUSBwrite(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBwrite(void *rpc_buf)
{
  SifRpcReceiveData_t rdata;
	int read, ret, left, pos, fd;
	g_writeParam_t *eP = (g_writeParam_t *)rpc_buf;	
			
  fd = eP->fd;
  left = eP->size;
  read = 0;

	WaitSema(usbserv_io_sema);
	
  if (eP->unalignedDataLen) {
    ret = write(fd, (void*)eP->unalignedData, eP->unalignedDataLen);
    if (ret < 0)
      goto exit;
    
    read += ret;
    left -= eP->unalignedDataLen;
  }

  pos = (int)(eP->buf + eP->unalignedDataLen);
  while (left) {
    int writelen = left;
    if (left > USBSERV_BUFSIZE)
      left = USBSERV_BUFSIZE;

		SifRpcGetOtherData(&rdata, (void *)pos, rpc_buf, writelen, 0);
    ret = write(fd, (void*)rpc_buf, writelen);
#if DEBUG
	printf("writing %d,%d -> %d\n", eP->fd, eP->size, ret);
#endif
    if (ret != writelen) {
      if (ret > 0)
        read += ret;

      goto exit;
    }

    left -= writelen;
    pos  += writelen;
    read += writelen;
  }

	//r = write(eP->fd, (void *)(eP->buf + eP->unalignedDataLen), eP->size);

exit: ;
	SignalSema(usbserv_io_sema);
	
	return read;
}

//-------------------------------------------------------------- 
int rpcUSBclose(void)
{
	return _rpcUSBclose(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBclose(void *rpc_buf)
{
	int r;
	g_closeParam_t *eP = (g_closeParam_t *)rpc_buf;	

	WaitSema(usbserv_io_sema);
					
	r = close(eP->fd);
#if DEBUG
	printf("close %d -> %d\n", eP->fd, r);
#endif

	SignalSema(usbserv_io_sema);
	
	return r;
}

//-------------------------------------------------------------- 
int rpcUSBread(void)
{
	return _rpcUSBread(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBread(void *rpc_buf)
{
	int r = 0, rPos = 0, blockSize;
	g_readParam_t *eP = (g_readParam_t *)rpc_buf;
	SifDmaTransfer_t dmaStruct;
	void * eedata = eP->buf;
  void * eeread = eP->read;
	int intStatus, status = -1;
	int len = eP->size;
	int fd = eP->fd;
			
	WaitSema(usbserv_io_sema);
	
	while (rPos < len)
	{
		// Clamp read size by buffer size
		blockSize = len - rPos;
		if (blockSize > USBSERV_BUFSIZE)
			blockSize = USBSERV_BUFSIZE;

		r = read(fd, rpc_buf, blockSize);
#if DEBUG
		printf("READ %d: POS: %d, BLOCK: %d -> %d (error: %d)\n", len, rPos, blockSize, r, r <= 0);
#endif

		// Error
		if (r <= 0)
			break;

		// Send back to EE
		dmaStruct.src = (void *)rpc_buf;
		dmaStruct.dest = eedata;
		dmaStruct.size = r;
		dmaStruct.attr = 0;

		CpuSuspendIntr(&intStatus);
		status = sceSifSetDma(&dmaStruct, 1);
		CpuResumeIntr(intStatus);

    // wait
    DelayThread(100);
    while (status >= 0 && sceSifDmaStat(status) >= 0)
      DelayThread(100);

		// Increment
		eedata += r;
		rPos += r;

    if (eeread) {

      // Send back to EE
      dmaStruct.src = (void *)&rPos;
      dmaStruct.dest = (void*)eeread;
      dmaStruct.size = 4;
      dmaStruct.attr = 0;

      CpuSuspendIntr(&intStatus);
      status = sceSifSetDma(&dmaStruct, 1);
      CpuResumeIntr(intStatus);
    }

		// 
		if (r != blockSize)
			break;
	}

	SignalSema(usbserv_io_sema);
	
	return rPos;
}

//-------------------------------------------------------------- 
int rpcUSBseek(void)
{
	return _rpcUSBseek(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBseek(void *rpc_buf)
{
	int r;
	g_seekParam_t *eP = (g_seekParam_t *)rpc_buf;	
			
	WaitSema(usbserv_io_sema);
	
	r = lseek(eP->fd, eP->offset, eP->whence);
#if DEBUG
	printf("seeking %d,%d,%d -> %d\n", eP->fd, eP->offset, eP->whence, r);
#endif
	
	SignalSema(usbserv_io_sema);
	
	return r;
}

//-------------------------------------------------------------- 
int rpcUSBremove(void)
{
	return _rpcUSBremove(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBremove(void *rpc_buf)
{
	int fd;
	g_removeParam_t *eP = (g_removeParam_t *)rpc_buf;	

	WaitSema(usbserv_io_sema);
	
	sprintf(filepath, "%s", eP->filename);
	fd = remove(filepath);	
#if DEBUG
	printf("removing %s -> %d\n", filepath, fd);
#endif
	
	SignalSema(usbserv_io_sema);
		
	return fd;
}

//-------------------------------------------------------------- 
int rpcUSBgetstat(void)
{
	return _rpcUSBgetstat(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBgetstat(void *rpc_buf)
{
	int ret;
	g_getstatParam_t *eP = (g_getstatParam_t *)rpc_buf;
  iox_stat_t st;
	SifDmaTransfer_t dmaStruct;
	int intStatus, status = -1;

	WaitSema(usbserv_io_sema);
	
	sprintf(filepath, "%s", eP->filename);
	ret = getstat(filepath, &st);	
#if DEBUG
	printf("getstat %s -> %d\n", filepath, ret);
#endif

  // Send back to EE
  dmaStruct.src = (void *)&st;
  dmaStruct.dest = eP->st;
  dmaStruct.size = sizeof(io_stat_t);
  dmaStruct.attr = 0;

  CpuSuspendIntr(&intStatus);
  status = sceSifSetDma(&dmaStruct, 1);
  CpuResumeIntr(intStatus);

	// Wait
	while (status >= 0 && sceSifDmaStat(status) >= 0)
		DelayThread(100);

	SignalSema(usbserv_io_sema);
		
	return ret;
}

//-------------------------------------------------------------- 
int rpcUSBmkdir(void)
{
	return _rpcUSBmkdir(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBmkdir(void *rpc_buf)
{
	int ret;
	g_mkdirParam_t *eP = (g_mkdirParam_t *)rpc_buf;	

	WaitSema(usbserv_io_sema);
	
	sprintf(filepath, "%s", eP->dirpath);
	ret = mkdir(filepath, 0);	
#if DEBUG
	printf("mkdir %s -> %d\n", filepath, ret);
#endif
	
	SignalSema(usbserv_io_sema);
		
	return ret;
}

//-------------------------------------------------------------- 
int rpcUSBdopen(void)
{
	return _rpcUSBdopen(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBdopen(void *rpc_buf)
{
	int fd;
	g_dopenParam_t *eP = (g_dopenParam_t *)rpc_buf;	

	WaitSema(usbserv_io_sema);
	
	sprintf(filepath, "%s", eP->dirpath);
	fd = dopen(filepath);	
#if DEBUG
	printf("opening %s -> %d\n", filepath, fd);
#endif
	
	SignalSema(usbserv_io_sema);
		
	return fd;
}

//-------------------------------------------------------------- 
int rpcUSBdclose(void)
{
	return _rpcUSBdclose(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBdclose(void *rpc_buf)
{
	int r;
	g_dcloseParam_t *eP = (g_dcloseParam_t *)rpc_buf;	

	WaitSema(usbserv_io_sema);
					
	r = dclose(eP->fd);
#if DEBUG
	printf("dclose %d -> %d\n", eP->fd, r);
#endif

	SignalSema(usbserv_io_sema);
	
	return r;
}

//-------------------------------------------------------------- 
int rpcUSBdread(void)
{
	return _rpcUSBdread(&usbserv_rpc_buf);
}

//-------------------------------------------------------------- 
int _rpcUSBdread(void *rpc_buf)
{
	int r;
	g_dreadParam_t *eP = (g_dreadParam_t *)rpc_buf;	
  iox_dirent_t dirent;
	SifDmaTransfer_t dmaStruct;
	int intStatus, status = -1;

	WaitSema(usbserv_io_sema);
					
	r = dread(eP->fd, &dirent);
#if DEBUG
	printf("dread %d -> %d \"%s\"\n", eP->fd, r, dirent.name);
#endif

  // Send back to EE
  dmaStruct.src = (void *)&dirent;
  dmaStruct.dest = eP->dirent;
  dmaStruct.size = sizeof(iox_dirent_t);
  dmaStruct.attr = 0;

  CpuSuspendIntr(&intStatus);
  status = sceSifSetDma(&dmaStruct, 1);
  CpuResumeIntr(intStatus);

	// Wait
	while (status >= 0 && sceSifDmaStat(status) >= 0)
		DelayThread(100);

	SignalSema(usbserv_io_sema);
	
	return r;
}

//-------------------------------------------------------------------------
int _start(int argc, char** argv)
{			
	iop_sema_t smp;
				
	SifInitRpc(0);
			
	// Starting usbserv Remote Procedure Call server	
	start_RPC_server();
	
	smp.attr = 1;
	smp.initial = 1;
	smp.max = 1;
	smp.option = 0;
	usbserv_io_sema = CreateSema(&smp);	
						
	return MODULE_RESIDENT_END;
}
