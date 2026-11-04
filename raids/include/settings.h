#ifndef RAIDS_SETTINGS_H
#define RAIDS_SETTINGS_H

#include "game.h"

struct RaidsGetSettingsRequest
{
  u32 DestAddress;
  u32 DestHasFlagAddress;
};

struct RaidsSaveSettingsRequest
{
  RaidsUserSettings_t Settings;
  u32 AckAddress;
  int Revision;
} __attribute__((packed));

typedef char RaidsGetSettingsSizeCheck[sizeof(struct RaidsGetSettingsRequest) == 8 ? 1 : -1];
typedef char RaidsSaveSettingsSizeCheck[sizeof(struct RaidsSaveSettingsRequest) == 11 ? 1 : -1];

extern RaidsUserSettings_t raidsUserSettings;

void raidsSettingsOpen(void);
void raidsSettingsSave(void);
void raidsSettingsTick(void);
void raidsSettingsDraw(void);
void raidsSettingsMapInit(void);

#endif // RAIDS_SETTINGS_H
