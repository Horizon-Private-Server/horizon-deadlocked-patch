EE_LIBS = -ldl
EE_OBJS = main.o config.o path.o gaspath.o wraith.o surge.o gas.o power.o ../shared/upgrade.o ../shared/drop.o ../shared/gate.o ../shared/mysterybox.o ../shared/pathfind.o ../shared/maputils.o ../shared/mobs/mob.o
EE_ELF = orxon.elf
EE_BIN = orxon.bin
EE_INCS := -I../../../common -I../../include -I../shared/include -Iinclude
EE_LDFLAGS := -DMOB_ZOMBIE -DMOB_EXECUTIONER -DMOB_TREMOR -DMAP_WRAITH -DMAP_SURGE -DGATE -DGAS -DPOWER -fno-builtin -nostdlib -nostartfiles -L.

# set to DEBUG or RELEASE
BUILD_CONFIG := RELEASE

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DNOGAS -DSURGE_IDLE -DWRAITH_IDLE1 -DMOB_CORN
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DMOB_CORN
endif

.PHONY: setup

all: setup $(EE_BIN) clean
	# payload and definition
	cp $(CURDIR)/orxon.bin $(CURDIR)/bin/code.0008.bin
	bin2codedef $(CURDIR)/bin/code.0008.bin 01EF0000 $(CURDIR)/bin/code.0008.def 004A7D98

	cp $(CURDIR)/static/*.* $(CURDIR)/bin/
	
setup:
	mkdir -p $(CURDIR)/bin

clean:
	rm -f *.irx *.o ../shared/*.o ../shared/mobs/*.o

include $(CURDIR)/../../../Makefile.pref
include $(CURDIR)/../../../Makefile.eeglobal
include ${CURDIR}/../../../Makefile.rules
