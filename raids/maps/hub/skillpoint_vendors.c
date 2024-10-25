/***************************************************
 * FILENAME :		skillpoint_vendors.c
 * 
 * DESCRIPTION :
 * 		
 * 		
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libdl/dl.h>
#include <libdl/player.h>
#include <libdl/pad.h>
#include <libdl/time.h>
#include <libdl/net.h>
#include <libdl/game.h>
#include <libdl/string.h>
#include <libdl/hud.h>
#include <libdl/math.h>
#include <libdl/math3d.h>
#include <libdl/collision.h>
#include <libdl/stdio.h>
#include <libdl/gamesettings.h>
#include <libdl/dialog.h>
#include <libdl/patch.h>
#include <libdl/ui.h>
#include <libdl/graphics.h>
#include <libdl/color.h>
#include <libdl/utils.h>
#include "module.h"
#include "common.h"
#include "messageid.h"
#include "gate.h"
#include "game.h"
#include "messager.h"
#include "spawner.h"
#include "mover.h"
#include "mob.h"
#include "shared.h"
#include "pathfind.h"
#include "hub.h"

Moby* spVendorMobys[RAIDS_SKILLS_COUNT] = {0,0,0,0};
char* spVendorSkillNames[] = {
  [RAIDS_SKILLS_HEALTH] "Health",
  [RAIDS_SKILLS_DAMAGE] "Damage",
  [RAIDS_SKILLS_SPEED] "Speed",
  [RAIDS_SKILLS_UNUSED] NULL
};
char* spVendorSkillInteractMessages[] = {
  [RAIDS_SKILLS_HEALTH] "\x11 Do you like stew?\x01\x11 Yes",
  [RAIDS_SKILLS_DAMAGE] "\x11 You look weaker than usual, Ratchet.",
  [RAIDS_SKILLS_SPEED] "\x11 Wanna be able to move like this?",
  [RAIDS_SKILLS_UNUSED] NULL
};
char* spVendorSkillGotMessages[] = {
  [RAIDS_SKILLS_HEALTH] "%d Health Skillpoints",
  [RAIDS_SKILLS_DAMAGE] "%d Damage Skillpoints",
  [RAIDS_SKILLS_SPEED] "%d Speed Skillpoints",
  [RAIDS_SKILLS_UNUSED] NULL
};
char spVendorLocalStrBufs[GAME_MAX_LOCALS][64];

//--------------------------------------------------------------------------
void spVendorTick(void)
{
  if (!MapConfig.GetBankFunc || !MapConfig.SendBankAccountToServerFunc) return;

  RaidsPlayerBank_t* bank = MapConfig.GetBankFunc();
  if (!bank || !bank->Account.SkillPoints) return;

  Player* localPlayer = playerGetFromSlot(0);
  if (!localPlayer) return;

  int skillIdx;
  char buf[64];
  for (skillIdx = 0; skillIdx < RAIDS_SKILLS_COUNT; ++skillIdx) {
    if (!spVendorMobys[skillIdx]) continue;
    if (vector_distance(localPlayer->PlayerPosition, spVendorMobys[skillIdx]->Position) < 5) {
      snprintf(spVendorLocalStrBufs[0], sizeof(spVendorLocalStrBufs[0]), spVendorSkillInteractMessages[skillIdx]);
      uiShowPopup(0, spVendorLocalStrBufs[0]);
      if (padGetButtonDown(0, PAD_CIRCLE) > 0) {
        bank->Account.SkillPoints--;
        bank->Account.Skills[skillIdx]++;
        hudHidePopup();
        if (MapConfig.PushSnackFunc) {
          snprintf(buf, sizeof(buf), spVendorSkillGotMessages[skillIdx], bank->Account.Skills[skillIdx]);
          MapConfig.PushSnackFunc(buf, 1, 0);
        }
      }
      break;
    }
  }
}

//--------------------------------------------------------------------------
void spVendorInit(void)
{
  Moby* mobyStart = mobyListGetStart();
#if SPEED_SKILL_MOBY_OCLASS
  spVendorMobys[RAIDS_SKILLS_SPEED] = mobyFindNextByOClass(mobyStart, SPEED_SKILL_MOBY_OCLASS);
#endif
#if DAMAGE_SKILL_MOBY_OCLASS
  spVendorMobys[RAIDS_SKILLS_DAMAGE] = mobyFindNextByOClass(mobyStart, DAMAGE_SKILL_MOBY_OCLASS);
#endif
#if HEALTH_SKILL_MOBY_OCLASS
  spVendorMobys[RAIDS_SKILLS_HEALTH] = mobyFindNextByOClass(mobyStart, HEALTH_SKILL_MOBY_OCLASS);
#endif
}
