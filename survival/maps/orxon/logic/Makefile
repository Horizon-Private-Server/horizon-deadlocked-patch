EE_LIBS = -ldl
EE_OBJS = main.o config.o path.o gaspath.o wraith.o surge.o ../../shared/gate.o gas.o power.o ../../shared/mysterybox.o ../../shared/pathfind.o ../../shared/maputils.o ../../shared/mobs/mob.o
EE_ELF = logic.elf
EE_BIN = logic.bin
EE_INCS := -I../../../../common -I../../../include -I../../shared/include -Iinclude
EE_LDFLAGS := -DMOB_ZOMBIE -DMOB_EXECUTIONER -DMOB_TREMOR -DMAP_WRAITH -DMAP_SURGE -DGATE -DGAS -DPOWER -fno-builtin -nostdlib -nostartfiles -L.

# set to DEBUG or RELEASE
BUILD_CONFIG := RELEASE

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DNOGAS -DSURGE_IDLE -DWRAITH_IDLE
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := 
endif

all: $(EE_BIN) clean-temp

clean-temp:
	rm -f *.irx *.o ../../shared/*.o ../../shared/mobs/*.o

clean:
	rm -f *.irx *.o *.bin *.elf ../../shared/*.o ../../shared/mobs/*.o

include $(CURDIR)/../../../../Makefile.pref
include $(CURDIR)/../../../../Makefile.eeglobal
include ${CURDIR}/../../../../Makefile.rules
