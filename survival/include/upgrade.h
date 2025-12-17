#ifndef SURVIVAL_UPGRADE_H
#define SURVIVAL_UPGRADE_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/sound.h>

#define UPGRADE_MOBY_OCLASS				(0x01F9)
#define UPGRADE_PICKUP_RADIUS			(4)
#define UPGRADE_TOKEN_COST				(1)
#define UPGRADE_MAX_USES  				(15)
#define PLAYER_UPGRADE_COOLDOWN_TICKS					(15)

enum UpgradeType {
	UPGRADE_HEALTH,
	UPGRADE_SPEED,
	UPGRADE_DAMAGE,
	UPGRADE_MEDIC,
	UPGRADE_VENDOR,
  UPGRADE_PICKUPS,
  UPGRADE_CRIT,
	UPGRADE_COUNT
};

struct UpgradePVar {
	enum UpgradeType Type;
  int Uses;
  int TexId;
	struct PartInstance* Particles[4];
};

int UpgradeMax[UPGRADE_COUNT];

#endif // SURVIVAL_UPGRADE_H
