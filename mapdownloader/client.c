#include <tamtypes.h>
#include <libdl/stdio.h>
#include <libdl/stdlib.h>
#include <libdl/string.h>
#include <libdl/pad.h>
#include <libdl/net.h>

#include "db.h"

#define BUF_SIZE            (1460)
#define CLIENTDATA          ((void*)0x001b26c0)

const char* HOSTNAME = "cdn.rac-horizon.com";

const int PORT = 80;

static u8	net_buf[2048] __attribute__( ( aligned( 64 ) ) );

int inetClientTCPID = -1;

long comm_startup(void)
{
  ((long (*)(void))0x01ea7408)();
}

void comm_update(void)
{
  ((void (*)(void))0x01ea7538)();
}

int client_connect(void)
{
  int ret = 0;
  inetParam_t param;

  DPRINTF("connecting to %s...\n", HOSTNAME);
  
  memset( &param, 0, sizeof( inetParam_t ) );
	param.type = 1;
	param.local_port = -1;
	ret = inetName2Address( CLIENTDATA, net_buf, 0, &param.remote_addr, HOSTNAME, 0, 0 );
	if ( ret < 0 ) {
		DPRINTF( "%s, %d, inetName2Address : DNS failed : %s, ret = %d\n", __FILE__, __LINE__, HOSTNAME, ret );
		return -1;
	}
	param.remote_port = 80;

	inetClientTCPID = inetCreate( CLIENTDATA, net_buf, &param );
	if ( inetClientTCPID < 0 ) {
		DPRINTF( "%s, %d, inetCreate() failed.(%d)\n", __FILE__, __LINE__, inetClientTCPID );
		return -2;
	}

	ret = inetOpen( CLIENTDATA, net_buf, inetClientTCPID, -1 );
	if ( ret < 0 ) {
		DPRINTF( "inetOpen() failed.(%d)\n", ret );
		client_close();
    return -3;
	}

  //DPRINTF("connected %08X %d %s\n", (u32)lookupResult, connectResult, buf);
  return 0;
}

int client_close(void)
{
  int ret = 0;
  if (inetClientTCPID < 0)
    return 0;

  DPRINTF("disconnecting...\n");
  
	ret = inetClose( CLIENTDATA, net_buf, inetClientTCPID, -1 );
	if ( ret < 0 ) {
		DPRINTF( "inetClose() failed.(%d)\n", ret );
	}
	else {
		DPRINTF( "inetClose() done.\n" );
	}

  inetClientTCPID = -1;
  return 0;
}

int client_get(char * path, char * output, int output_size)
{
  int i,j=0;
  int ret, flags;
  char getcmd[BUF_SIZE + 1]; /* +1 so we can add null terminator */
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

  sprintf(getcmd, "GET %s HTTP/1.1\r\nUser-Agent: PS2HorizonClient\r\nAccept: */*\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n", encoded_path, HOSTNAME);
  
  flags = 0;
	ret = inetSend( CLIENTDATA, net_buf, inetClientTCPID, getcmd, strlen( getcmd ), &flags, -1 );
	if ( ret < 0 ) {
		DPRINTF( "inetSend() failed.(%d)\n", ret );
		return -1;
	}

  ret = client_recv(output, output_size, &len);
  DPRINTF("recv ret:%d len:%d\n", ret, len);

  return len;
}

int client_recv(char * output, int output_size, int * bytes_recvd)
{
  int flags = 0;
  int ret = 0;
  
  ret = inetRecv( CLIENTDATA, net_buf, inetClientTCPID, output, output_size, &flags, -1 );
  if ( ret < 0 ) {
    DPRINTF( "inetRecv() failed.(%d)\n", ret );
    if (bytes_recvd)
      *bytes_recvd = 0;
    return ret;
  } else if (!ret) { DPRINTF("empty recv\n"); }


  DPRINTF("recv ret:%d finished:%d\n", ret, flags & 4);
  if (bytes_recvd)
    *bytes_recvd = ret;
    
  if (flags & 4)
    return 1;

  return 0;
}

int client_init(void)
{
  return 0;
}

void client_destroy(void)
{
  
}
