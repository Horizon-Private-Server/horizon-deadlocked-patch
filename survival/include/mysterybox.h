#ifndef SURVIVAL_MYSTERY_BOX_H
#define SURVIVAL_MYSTERY_BOX_H

#include <tamtypes.h>
#include <libdl/moby.h>
#include <libdl/math.h>
#include <libdl/time.h>
#include <libdl/player.h>
#include <libdl/math3d.h>

#define MYSTERY_BOX_OCLASS                      (0x2635)
#define MYSTERY_BOX_COST                        (10000)
#define PLAYER_MYSTERY_BOX_COOLDOWN_TICKS       (30)

enum MysteryBoxEventType {
	MYSTERY_BOX_EVENT_SPAWN,
  MYSTERY_BOX_EVENT_ACTIVATE,
  MYSTERY_BOX_EVENT_GIVE_PLAYER,
};

enum MysteryBoxState {
	MYSTERY_BOX_STATE_IDLE,
	MYSTERY_BOX_STATE_OPENING,
	MYSTERY_BOX_STATE_CYCLING_ITEMS,
	MYSTERY_BOX_STATE_DISPLAYING_ITEM,
	MYSTERY_BOX_STATE_BEFORE_CLOSING,
	MYSTERY_BOX_STATE_CLOSING,
	MYSTERY_BOX_STATE_HIDDEN,
};

enum MysteryBoxItem {
	MYSTERY_BOX_ITEM_WEAPON_MOD,
	MYSTERY_BOX_ITEM_ACTIVATE_POWER,
	MYSTERY_BOX_ITEM_UPGRADE_WEAPON,
	MYSTERY_BOX_ITEM_DREAD_TOKEN,
	MYSTERY_BOX_ITEM_INVISIBILITY_CLOAK,
	MYSTERY_BOX_ITEM_REVIVE_TOTEM,
	//MYSTERY_BOX_ITEM_SUPER_JUMP,
	MYSTERY_BOX_ITEM_RESET_GATES,
	MYSTERY_BOX_ITEM_TEDDY_BEAR,
  MYSTERY_BOX_ITEM_COUNT
};

struct MysteryBoxPVar
{
  VECTOR SpawnpointPosition;
  VECTOR SpawnpointRotation;
  enum MysteryBoxItem Item;
  int Random;
  int ActivatedTime;
  int StateChangedAtTime;
  int ActivatedByPlayerId;
  int RoundHidden;
};

void mboxSpawn(void);
void mboxInit(void);

#endif // SURVIVAL_MYSTERY_BOX_H
