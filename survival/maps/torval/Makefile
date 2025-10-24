EE_LIBS = -ldl
EE_OBJS = common.o main.o config.o path.o hackerorb.o ../shared/stackables.o ../shared/upgrade.o ../shared/drop.o ../shared/stackbox.o ../shared/mysterybox.o ../shared/pathfind.o ../shared/maputils.o ../shared/mobs/mob.o
EE_ELF = torval.elf
EE_BIN = torval.bin
EE_INCS := -I../../../common -I../../include -I../shared/include -Iinclude
EE_LDFLAGS := -DMOB_ZOMBIE -DMOB_LEVIATHAN -DMOB_REACTOR -DMOB_REAPER -DMOB_EXECUTIONER2 -DMOB_SWARMER -DSWARMER_BLUE -fno-builtin -nostdlib -nostartfiles -L.

# set to DEBUG or RELEASE
BUILD_CONFIG := RELEASE

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DMOB_CORN -DDEBUGMBOX_ -DDEBUGPATH_ -DDEBUGSBOX_ -DDEBUGMOVE_
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DRELEASE -DMOB_CORN
endif

.PHONY: setup

all: setup $(EE_BIN) clean
	# payload and definition
	cp $(CURDIR)/torval.bin $(CURDIR)/bin/survival_torval.code

	if [ -d "/out/survival_torval/build/dl" ]; then cp -r $(CURDIR)/bin/* /out/survival_torval/build/dl; fi
	
setup:
	mkdir -p $(CURDIR)/bin

clean:
	rm -f *.irx *.o ../shared/*.o ../shared/mobs/*.o

include $(CURDIR)/../../../Makefile.pref
include $(CURDIR)/../../../Makefile.eeglobal
include ${CURDIR}/../../../Makefile.rules
