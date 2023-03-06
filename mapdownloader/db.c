#include <tamtypes.h>
#include <libdl/stdio.h>
#include <libdl/stdlib.h>
#include <libdl/string.h>
#include <libdl/ui.h>

#define NEWLIB_PORT_AWARE
#include <io_common.h>

#include "db.h"
#include "rpc.h"

#define BUF_SIZE                  (1460)
#define DOWNLOAD_CHUNK_SIZE       (1024 * 16)
#define USB_WRITE_BLOCK_SIZE      (256)

const char * MASS_DIR_PATH = "dl";
const char * MASS_BINARY_PATH = "dl/%s.%s";

const char * INDEX_PATH = "/downloads/maps/index.txt";
const char * MAP_IMAGE_PATH = "/downloads/maps/assets/images/%s.jpg";
const char * MAP_BINARY_PATH = "/downloads/maps/assets/binaries/%s.%s";
const char * MAP_VERSION_EXT = "version";
const char * MAP_WAD_EXT = "wad";
const char * MAP_BG_EXT = "bg";
const char * MAP_MAP_EXT = "map";

char dlBuffer1[BUF_SIZE];
char dlBuffer2[BUF_SIZE];
char dlBuffer3[BUF_SIZE];

char dbStateTitleStr[64];
char dbStateDescStr[64];
int dbStateDownloadTotal = 0;
int dbStateDownloadAmount = 0;
int dbStateTimeStarted = 0;
int dbStateActive = 0;
int dbStateCancel = 0;

int client_connect(void);
int client_close(void);
int client_get(char * path, char * output, int output_size);
int client_recv(char * output, int output_size, int* bytes_recvd);
int db_read_local_version(char * map_filename);

int parse_http_response_content_length(char* buf)
{
  int contentLength = 0;

  // read content length
  char* substrbuf = strstr(buf, "Content-Length: ");
  if (substrbuf)
    sscanf(substrbuf, "Content-Length: %d", &contentLength);

  return contentLength;
}

int parse_http_response_content_type(char* buf, char* output)
{
  int contentLength = 0;

  // read content length
  char* substrbuf = strstr(buf, "Content-Type: ");
  if (!substrbuf)
    return -1;

  return sscanf(substrbuf, "Content-Type: %s\r\n", output) == 1;
}

int parse_http_response_content(char* buf, int buf_size, char* output, int output_size)
{
  int contentLength = 0;

  // read content length
  char* substrbuf = strstr(buf, "\r\n\r\n");
  if (!substrbuf)
    return -1;

  int content_size = buf_size - ((substrbuf - buf) + 4);
  int read = (content_size < output_size) ? content_size : output_size;
  memcpy(output, substrbuf + 4, read);
  return read;
}

int parse_http_response(char* buf, int buf_size, int* response_code, int* content_length, char* output_content, int output_content_size, char* output_content_type, int output_content_type_size)
{
  *response_code = 0;
  *content_length = 0;
  if (!buf)
    return -1;

  // initialize buffers
  memset(output_content_type, 0, output_content_type_size);

  // read response code
  sscanf(buf, "HTTP/1.1 %d", response_code);
  if (*response_code != 200)
    return 0;

  // read content
  *content_length = parse_http_response_content_length(buf);
  parse_http_response_content_type(buf, output_content_type);
  return parse_http_response_content(buf, buf_size, output_content, (*content_length < output_content_size) ? *content_length : output_content_size);
}

int http_get(char * path, char * output, int output_length, downloadCallback_func callback)
{
  int i;
  int response, content_length;
  int total_read = 0;

  char* buffer = dlBuffer1;
  char* content_type_buffer = dlBuffer2;

  // connect
  if (client_connect() < 0) {
    DPRINTF("unable to connect to db\n");
    return -1;
  }

  // GET
  int read = client_get(path, buffer, BUF_SIZE);
  if (read <= 0) {
    DPRINTF("db returned empty response\n");
    client_close();
    return -2;
  }

  // parse response
  int content_read_length = parse_http_response(buffer, read, &response, &content_length, output, output_length, content_type_buffer, BUF_SIZE);
  if (response != 200) {
    DPRINTF("db returned %d\n", response);
    client_close();
    return -3;
  }

  // read rest
  total_read += content_read_length;
  output += content_read_length;
  if (callback)
    callback(total_read, content_length);
  while (total_read < content_length) {

    // 
    int left = content_length - total_read;
    if (left > BUF_SIZE)
      left = BUF_SIZE;
    else if (left <= 0)
      break;

    int finished = client_recv(output, left, &content_read_length);
    if (content_read_length < 0) {
      DPRINTF("failed to finish reading content %d/%d (%s)\n", total_read, content_length, output);
      client_close();
      return -4;
    }

    //output += content_read_length;
    total_read += content_read_length;
    if (callback)
      callback(total_read, content_length);
    DPRINTF("read %d/%d\n", total_read, content_length);

    if (finished) {
      DPRINTF("recv finished\n");
      break;
    }
  }

  if (callback)
    callback(total_read, content_length);

  client_close();
  return total_read;
}

int block_write(int fd, void* buf, int len)
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
		int size = len - rPos;
		if (size > USB_WRITE_BLOCK_SIZE)
			size = USB_WRITE_BLOCK_SIZE;

		// Try to write to usb
		if ((r = rpcUSBwrite(fd, buf, size)) != 0)
		{
			rpcUSBSync(0, NULL, NULL);
			DPRINTF("error writing to file %d\n", r);
			//rpcUSBclose(fd);
			//rpcUSBSync(0, NULL, NULL);
			//fd = -1;
			return 0;
		}

		rpcUSBSync(0, NULL, &r);
		rPos += r;
		buf += r;

		if (r != size)
			break;
	}

  return rPos;
}

int http_download(char * path, char* file_path, downloadCallback_func callback)
{
  int i = 0;
  int fd = -1;
  int response = 0, content_length = 0;
  int total_read = 0, total_write = 0;

  dbStateTimeStarted = gameGetTime();
  dbStateDownloadAmount = 0;
  dbStateDownloadTotal = 0;

  char* buffer = dlBuffer1;
  char* content_buffer = dlBuffer2;
  char* content_type_buffer = dlBuffer3;
  
  // connect
  if (client_connect() < 0) {
    DPRINTF("unable to connect to db\n");
    goto error;
  }

  // GET
  int read = client_get(path, buffer, BUF_SIZE);
  DPRINTF("get %d\n", read);
  if (read <= 0) {
    DPRINTF("db returned empty response\n");
    goto error;
  }

  // parse response
  int content_read_length = parse_http_response(buffer, read, &response, &content_length, content_buffer, BUF_SIZE, content_type_buffer, BUF_SIZE-1);
  if (response != 200) {
    DPRINTF("db returned %d\n", response);
    goto error;
  }

  // open output file
  DPRINTF("pre open\n");
  rpcUSBopen(file_path, FIO_O_CREAT | FIO_O_WRONLY | FIO_O_TRUNC);
	rpcUSBSync(0, NULL, &fd);
  if (fd < 0) {
    DPRINTF("unable to open %s\n", file_path);
    goto error;
  }

  // read rest
  DPRINTF("write %d to %d\n", content_read_length, fd);
  
  dbStateDownloadTotal = content_length;
  dbStateDownloadAmount = content_read_length;
  int w = block_write(fd, content_buffer, content_read_length);
  if (w <= 0) {
    DPRINTF("failed to write at %d size:%d\n", 0, content_read_length);
    goto error;
  }
  total_write += w;
  DPRINTF("write end %d\n", w);
  total_read += content_read_length;
  if (callback)
    callback(total_read, content_length);
  
  int x = 0;
  int y = 0; //scr_getY();
  while (total_read < content_length) {

    uiRunCallbacks();
    if (dbStateCancel) {
      goto error;
    }

    // 
    int left = content_length - total_read;
    if (left > BUF_SIZE)
      left = BUF_SIZE;
    else if (left <= 0)
      break;

    DPRINTF("recv start\n");
    memset(content_buffer, 0, BUF_SIZE);
    int recvResult = client_recv(content_buffer, left, &content_read_length);
    DPRINTF("recv end ret:%d read:%d\n", recvResult, content_read_length);
    if (content_read_length < 0 || recvResult < 0) {
      DPRINTF("failed to finish reading content %d/%d (%s)\n", total_read, content_length, content_buffer);
      goto error;
    }

    if (content_read_length == 0)
      break;

    dbStateDownloadAmount += content_read_length;

    DPRINTF("%d write %d to %d\n", x, content_read_length, fd);
    //lseek(fd, total_write, SEEK_SET);
    w = block_write(fd, content_buffer, content_read_length);
    //w = content_read_length;
    if (w <= 0) {
      DPRINTF("failed to write at %d size:%d\n", total_write, content_read_length);
      goto error;
    }

    total_write += w;
    DPRINTF("%d write end %d\n", x, w);

    total_read += content_read_length;
    DPRINTF("%d read %d/%d\n", x, total_read, content_length);
    if (callback)
      callback(total_read, content_length);

    if (recvResult > 0) {
      DPRINTF("recv finished\n");
      break;
    }
    ++x;
  }

  if (callback)
    callback(total_read, content_length);

  DPRINTF("finished %d/%d\n", total_read, content_length);
  client_close();
  rpcUSBclose(fd);
  rpcUSBSync(0, NULL, NULL);
  return total_read;

error: ;
  client_close();
  if (fd >= 0) {
    rpcUSBclose(fd);
    rpcUSBSync(0, NULL, NULL);
  }
  return -1;
}

int parse_db(struct DbIndex* db)
{
  int i;
  char content_buffer[2048];

  // init
  memset(db, 0, sizeof(struct DbIndex));

  // get index
  DPRINTF("querying index...\n");
  int read = http_get(INDEX_PATH, content_buffer, sizeof(content_buffer)-1, NULL);
  if (read <= 0) {
    DPRINTF("unable to query index\n");
    return -1;
  }

  // parse index
  char * content = content_buffer;
  sscanf(content, "%d", &db->GlobalVersion);
  content = strstr(content, "\n") + 1;
  i = 0;
  while (i < MAX_DB_INDEX_ITEMS)
  {
    if (3 != sscanf(content, "%[^\n|]|%[^\n|]|%d\n", db->Items[i].Name, db->Items[i].Filename, &db->Items[i].RemoteVersion))
      break;
    
    content = strstr(content, "\n") + 1;
    ++i;
  }
  db->ItemCount = i;

  // print
  DPRINTF("GlobalVersion: %d\n", db->GlobalVersion);
  DPRINTF("Maps: %d\n", db->ItemCount);
  for (i = 0; i < db->ItemCount; ++i) {
    DPRINTF("\t%s (version %d)\n", db->Items[i].Name, db->Items[i].RemoteVersion);
  }
  
  // read local version
  for (i = 0; i < db->ItemCount; ++i) {
    db->Items[i].LocalVersion = db_read_local_version(db->Items[i].Filename);
  }

  return 0;
}

int db_read_local_version(char * map_filename)
{
  char file_path[128];
  char buf[4];
  int version = 0;
  int fd = -1;
  int ret = 0;

  // construct path
  snprintf(file_path, sizeof(file_path), MASS_BINARY_PATH, map_filename, MAP_VERSION_EXT);

  // open
  rpcUSBopen(file_path, FIO_O_RDONLY);
	rpcUSBSync(0, NULL, &fd);
  if (fd < 0) {
    DPRINTF("unable to open %s\n", file_path);
    return -1;
  }

	// Go back to start of file
	rpcUSBseek(fd, 0, SEEK_SET);
	rpcUSBSync(0, NULL, NULL);

  // read version
	if (rpcUSBread(fd, (void*)buf, sizeof( int ) ) != 0)
	{
		DPRINTF( "error reading from %s\n", file_path );
		rpcUSBclose(fd);
		rpcUSBSync(0, NULL, NULL);
		return -1;
	}
	rpcUSBSync(0, NULL, NULL);

  version = *(int*)buf;
  DPRINTF( "%s => %d\n", file_path, version );

	// Close file
	rpcUSBclose(fd);
	rpcUSBSync(0, NULL, NULL);
  return version;
}

int db_check_mass_dir(void)
{
  return 0;
  DPRINTF("check\n");
  io_stat_t st = {0};
  int ret = 0;

  // if dir not exist then try mkdir
  int getstat = rpcUSBgetstat(MASS_DIR_PATH, &st);
  rpcUSBSync( 0, NULL, &ret );
  if ( ret < 0 ) {
    DPRINTF("getstat failed with %d\n", ret);

    if ( rpcUSBmkdir(MASS_DIR_PATH) != 0 ) {
      rpcUSBSync( 0, NULL, &ret );
      DPRINTF("mkdir failed with %d\n", ret);
      return -1;
    }

    rpcUSBSync( 0, NULL, &ret );
    if ( ret < 0 ) {
      DPRINTF("2 mkdir failed with %d\n", ret);
      return -1;
    }
  
    return 0;
  }

  return 0;
}

void download_db_item_callback(int bytes_downloaded, int total_bytes)
{
#if DEBUG
  //scr_printf("%.2f%%\n", 100.0 * (bytes_downloaded / (float)total_bytes));
#else

  //if (padGetButtonDown(0, PAD_SQUARE)) {
  //  DPRINTF("%.2f%%\n", 100.0 * (bytes_downloaded / (float)total_bytes));
  //}

  //int y = scr_getY();
  //scr_printf("%.2f%%\n", 100.0 * (bytes_downloaded / (float)total_bytes));
  //scr_setXY(0, y);
#endif
}

int download_db_item(struct DbIndexItem* item)
{
  char url_path[512];
  char file_path[512];
  int fd = -1;
  if (!item)
    return -1;

  // delete version file if it exists
  snprintf(file_path, 511, MASS_BINARY_PATH, item->Filename, MAP_VERSION_EXT);
  //rpcUSBremove(file_path);
	//rpcUSBSync(0, NULL, NULL);

  // download wad
  snprintf(file_path, 511, MASS_BINARY_PATH, item->Filename, MAP_WAD_EXT);
  snprintf(url_path, 511, MAP_BINARY_PATH, item->Filename, MAP_WAD_EXT);
  strncpy(dbStateDescStr, "Map Data", sizeof(dbStateDescStr));
  DPRINTF("%s\n", file_path);
  int wadRead = http_download(url_path, file_path, &download_db_item_callback);

  if (wadRead > 0) {
    // download map
    snprintf(file_path, 511, MASS_BINARY_PATH, item->Filename, MAP_MAP_EXT);
    snprintf(url_path, 511, MAP_BINARY_PATH, item->Filename, MAP_MAP_EXT);
    strncpy(dbStateDescStr, "Minimap", sizeof(dbStateDescStr));
    DPRINTF("%s\n", file_path);
    int mapRead = http_download(url_path, file_path, &download_db_item_callback);

    // download bg
    snprintf(file_path, 511, MASS_BINARY_PATH, item->Filename, MAP_BG_EXT);
    snprintf(url_path, 511, MAP_BINARY_PATH, item->Filename, MAP_BG_EXT);
    strncpy(dbStateDescStr, "Loading Screen", sizeof(dbStateDescStr));
    DPRINTF("%s\n", file_path);
    int bgRead = http_download(url_path, file_path, &download_db_item_callback);
    
    // write version
    snprintf(file_path, 511, MASS_BINARY_PATH, item->Filename, MAP_VERSION_EXT);
    DPRINTF("%s\n", file_path);
    rpcUSBopen(file_path, FIO_O_WRONLY | FIO_O_TRUNC | FIO_O_CREAT);
	  rpcUSBSync(0, NULL, &fd);
    if (fd < 0) {
      DPRINTF("unable to open version\n");
      return -2;
    }
    block_write(fd, &item->RemoteVersion, sizeof(int));
    rpcUSBclose(fd);
    rpcUSBSync(0, NULL, NULL);

    return 0;
  }
  
  return -1;
}

void db_write_global_maps_version(int version)
{
  char file_path[512];
  int fd = -1;

  snprintf(file_path, 511, "%s/version", MASS_DIR_PATH);
  DPRINTF("%s\n", file_path);
  rpcUSBopen(file_path, FIO_O_WRONLY | FIO_O_TRUNC | FIO_O_CREAT);
  rpcUSBSync(0, NULL, &fd);
  if (fd < 0) {
    DPRINTF("unable to open version\n");
    return -2;
  }
  block_write(fd, &version, sizeof(int));
  rpcUSBclose(fd);
  rpcUSBSync(0, NULL, NULL);
}
