EE_LIBS = -ldl
EE_OBJS = common.o main.o config.o path.o soulcollector.o hackerorb.o ../shared/stackables.o ../shared/upgrade.o ../shared/drop.o ../shared/stackbox.o ../shared/mysterybox.o ../shared/pathfind.o ../shared/maputils.o ../shared/mobs/mob.o
EE_ELF = valix.elf
EE_BIN = valix.bin
EE_INCS := -I../../../common -I../../include -I../shared/include -Iinclude
EE_LDFLAGS := -DMOB_ZOMBIE -DMOB_LEVIATHAN -DMOB_REAPER -DMOB_TREMOR -DMOB_SWARMER -DSWARMER_ORANGE -fno-builtin -nostdlib -nostartfiles -L.

# set to DEBUG or RELEASE
BUILD_CONFIG := RELEASE

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DMOB_CORN -DDEBUGMBOX_ -DDEBUGPATH -DDEBUGSBOX_ -DDEBUGMOVE_
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DRELEASE -DMOB_CORN
endif

.PHONY: setup

all: setup $(EE_BIN) clean
	# payload and definition
	cp $(CURDIR)/valix.bin $(CURDIR)/bin/survival_valix.code

	if [ -d "/out" ]; then cp -r $(CURDIR)/bin/* /out/; fi
	
setup:
	mkdir -p $(CURDIR)/bin

clean:
	rm -f *.irx *.o ../shared/*.o ../shared/mobs/*.o

include $(CURDIR)/../../../Makefile.pref
include $(CURDIR)/../../../Makefile.eeglobal
include ${CURDIR}/../../../Makefile.rules
