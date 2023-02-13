#ifndef SURVIVAL_UPGRADE_H
#define SURVIVAL_UPGRADE_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

#define UPGRADE_MOBY_OCLASS				(0x70)
#define UPGRADE_PICKUP_RADIUS			(4)
#define UPGRADE_TOKEN_COST				(1)
#define PLAYER_UPGRADE_COOLDOWN_TICKS					(60)

enum UpgradeType {
	UPGRADE_HEALTH,
	UPGRADE_SPEED,
	UPGRADE_DAMAGE,
	UPGRADE_MEDIC,
	UPGRADE_VENDOR,
	UPGRADE_COUNT
};

enum UpgradeEventType {
	UPGRADE_EVENT_SPAWN,
	UPGRADE_EVENT_DESTROY,
	UPGRADE_EVENT_PICKUP
};

struct UpgradePVar {
	enum UpgradeType Type;
	struct PartInstance* Particles[4];
};

struct UpgradeSpawnEventArgs
{
	enum UpgradeType Type;
};

struct UpgradeDestroyedEventArgs
{
	
};

struct UpgradePickupEventArgs
{
	int PickedUpByPlayerId;
};

int UpgradeMax[UPGRADE_COUNT];

void upgradeTick(void);
void upgradeInitialize(void);
int upgradeCreate(VECTOR position, VECTOR rotation, enum UpgradeType upgradeType);
void upgradePickup(Moby* moby, int pickedUpByPlayerId);

#endif // SURVIVAL_UPGRADE_H