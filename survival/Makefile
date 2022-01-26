
EE_ELF = survival-11184.elf
EE_BIN = survival-11184.bin
EE_OBJS = main.o mob.o utils.o config.o
EE_LIBS = -ldl

EE_INCS := -I../common
EE_LDFLAGS := -fno-builtin -nostdlib -nostartfiles -L. -L../libdl/lib

# set to DEBUG or RELEASE
BUILD_CONFIG := RELEASE

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

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DDISABLE_SPAWNING -DMOB_NO_DAMAGE -DMOB_NO_MOVE -DPAYDAY  -DVENDOR_OCLASS=0x20F6 -DBIGAL_OCLASS=0x2124 -DMANUAL_SPAWN
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DMOB_CORN1 -DAUTOSTART -DVENDOR_OCLASS=0x20F6 -DBIGAL_OCLASS=0x2124 -DSHARED_BOLTS
endif

all: $(EE_BIN) clean copy

clean:
	rm -f *.irx *.o

include $(CURDIR)/../Makefile.pref
include $(CURDIR)/../Makefile.eeglobal
include ${CURDIR}/../Makefile.rules

