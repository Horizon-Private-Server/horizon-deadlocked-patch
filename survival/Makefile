
EE_ELF = survival-11184.elf
EE_BIN = survival-11184.bin
EE_OBJS = main.o mob.o upgrade.o drop.o utils.o config.o demonbell.o
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

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DMOB_CORN -DDISABLE_SPAWNING -DBENCHMARK1 -DMANUAL_DROP_SPAWN -DMANUAL_SPAWN -DDROPS -DUPGRADES -DDEMONBELL -DFIXEDTARGET1 -DMOB_NO_DAMAGE1 -DMOB_NO_MOVE1 -DPAYDAY  -DVENDOR_OCLASS=0x263A -DBIGAL_OCLASS=0x2124 -DSHARED_BOLTS -DLOG_STATS2
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DMOB_CORN -DAUTOSTART -DVENDOR_OCLASS=0x263A -DBIGAL_OCLASS=0x2124 -DSHARED_BOLTS -DDROPS -DUPGRADES -DDEMONBELL
endif

.PHONY: maps

all: $(EE_BIN) maps clean copy

maps:
	cd maps/orxon && $(MAKE)

clean:
	rm -f *.irx *.o
	cd maps/orxon && $(MAKE) clean
 
include $(CURDIR)/../Makefile.pref
include $(CURDIR)/../Makefile.eeglobal
include ${CURDIR}/../Makefile.rules

