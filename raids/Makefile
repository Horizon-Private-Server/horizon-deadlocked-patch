
EE_ELF = raids-11184.elf
EE_BIN = raids-11184.bin
EE_OBJS = main.o utils.o config.o ../common/common.o
EE_LIBS = -ldl

EE_INCS := -I../common
EE_LDFLAGS := -fno-builtin -nostdlib -nostartfiles -L. -L../libdl/lib

# set to DEBUG or RELEASE
BUILD_CONFIG := DEBUG

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.
#		DISABLE_SPAWNING			Disables round/spawn logic.
#		MOB_NO_MOVE						All mobs have 0 speed
#		MOB_NO_DAMAGE					All mobs have 0 damage
#		VENDOR_OCLASS					Configures which moby class to look for when finding the map's vendor.
#														Useful for testing on maps without a vendor.
#		AUTOSTART							Enables logic to start each round when the previous round ends.
#														When this is disabled, the host must manually start the round.
#		MOB_CORN							Enables spawning mob limbs (corn) on death.
#		PAYDAY								Mobs give a lot more bolts.
#		QUICK_SPAWN						When set will spawn zombies as quickly as possible near the player.
#		LOG_STATS							With DEBUG enabled, will output game state information to the console.
#		MANUAL_SPAWN					When enabled, allows player to spawn mob at feet by pressing DPAD DOWN.
#		DROPS									When enabled, randomly spawns special drops when a mob is killed.
#		UPGRADES							When enabled, spawns the player upgrade mobies around the map.
#		PRINT_MOB_COMPLEXITY	When enabled, draws "complexity" number as sum of drawn mobs

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DMOB_CORN
#EE_DEFS += -DDEBUG_PERKS
EE_DEFS += -DDROPS -DUPGRADES -DDEMONBELL -DSHARED_BOLTS -DSHARED_XP
EE_DEFS += -DVENDOR_OCLASS=0x263A -DBIGAL_OCLASS=0x2124 -DPRESTIGEMACHINE_OCLASS=0x2643
EE_DEFS += -DMANUAL_DROP_SPAWN
#EE_DEFS += -DAUTOSTART
EE_DEFS += -DDISABLE_SPAWNING
EE_DEFS += -DMANUAL_SPAWN
#EE_DEFS += -DDEBUG_SOUNDS
#EE_DEFS += -DSTARTROUND=25
#EE_DEFS += -DMOB_INVINCIBLE
EE_DEFS += -DMOB_NO_DAMAGE
#EE_DEFS += -DMOB_NO_MOVE
#EE_DEFS += -DAUTOSTART
EE_DEFS += -DPAYDAY
#EE_DEFS += -DBENCHMARK
#EE_DEFS += -DPRINT_MOB_COMPLEXITY
#EE_DEFS += -DFIXEDTARGET
#EE_DEFS += -DJOINT_TEST
#EE_DEFS += -DLOG_STATS
#EE_DEFS += -DLOG_STATS2
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DRELEASE -DMOB_CORN -DAUTOSTART -DVENDOR_OCLASS=0x263A -DBIGAL_OCLASS=0x2124 -DPRESTIGEMACHINE_OCLASS=0x2643 -DDROPS -DUPGRADES -DDEMONBELL -DSHARED_BOLTS -DSHARED_XP
#EE_DEFS += -DSTARTROUND=100
#EE_DEFS += -DMOB_NO_DAMAGE
#EE_DEFS += -DTRAILER
endif

.PHONY: all maps hub

all: mode maps clean

mode: $(EE_BIN) copy

maps: hub

hub: 
	cd maps/hub && $(MAKE)

clean: 
	rm -f *.irx *.o
	cd maps/hub && $(MAKE) clean
 
include $(CURDIR)/../Makefile.pref
include $(CURDIR)/../Makefile.eeglobal
include ${CURDIR}/../Makefile.rules

