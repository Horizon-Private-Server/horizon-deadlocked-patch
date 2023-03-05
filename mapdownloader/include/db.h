#ifndef _DB_H_
#define _DB_H_

#define MAX_DB_INDEX_ITEMS                              (128)
#define DB_INDEX_ITEM_NAME_MAX_LEN                      (128)
#define DB_INDEX_ITEM_FILENAME_MAX_LEN                  (128)

#ifdef DEBUG
    #define DPRINTF(fmt, ...)       \
        printf("%s"fmt, "", ##__VA_ARGS__);
#else
    #define DPRINTF(fmt, ...) 
#endif

struct DbIndexItem
{
  int RemoteVersion;
  int LocalVersion;
  char Name[DB_INDEX_ITEM_NAME_MAX_LEN];
  char Filename[DB_INDEX_ITEM_FILENAME_MAX_LEN];
};

struct DbIndex
{
  int GlobalVersion;
  int ItemCount;
  struct DbIndexItem Items[MAX_DB_INDEX_ITEMS]; 
};

typedef void (*downloadCallback_func)(int bytes_downloaded, int total_bytes);

int db_check_mass_dir(void);
int parse_db(struct DbIndex* db);
int download_db_item(struct DbIndexItem* item);
void db_write_global_maps_version(int version);

extern char dbStateTitleStr[64];
extern char dbStateDescStr[64];
extern int dbStateDownloadTotal;
extern int dbStateDownloadAmount;
extern int dbStateTimeStarted;
extern int dbStateActive;
extern int dbStateCancel;

#endif // _DB_H_
