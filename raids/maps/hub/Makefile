EE_LIBS = -ldl
EE_OBJS = main.o config.o path.o skillpoint_vendors.o
EE_OBJS += ../../../common/common.o
EE_OBJS += ../shared/map.o ../shared/gate.o ../shared/spawner.o ../shared/messager.o ../shared/mover.o ../shared/controller.o ../shared/npc.o ../shared/pathfind.o ../shared/maputils.o ../shared/mobs/mob.o
EE_ELF = hub.elf
EE_BIN = hub.bin
EE_INCS := -I../../../common -I../../include -I../shared/include -Iinclude
EE_LDFLAGS := -Werror -DMOB_ZOMBIE -DMOB_SWARMER -DGATE -fno-builtin -nostdlib -nostartfiles -L.

# set to DEBUG or RELEASE
BUILD_CONFIG := DEBUG

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DMOB_CORN
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DRELEASE -DMOB_CORN
endif

EE_DEFS += -DSPEED_SKILL_MOBY_OCLASS=0x1BC5 # Courtney Gears NPC
EE_DEFS += -DHEALTH_SKILL_MOBY_OCLASS=0x2124 # Big Al NPC
EE_DEFS += -DDAMAGE_SKILL_MOBY_OCLASS=0x000A # Clank NPC

.PHONY: setup

all: setup $(EE_BIN) clean
	# payload and definition
	cp -rf $(CURDIR)/hub.bin $(CURDIR)/bin/code.0008.bin
	bin2codedef $(CURDIR)/bin/code.0008.bin 01EF0000 $(CURDIR)/bin/code.0008.def 004A7D98

	cp -rf $(CURDIR)/static/*.* $(CURDIR)/bin/
	if [ -d "/out" ]; then cp -rf $(CURDIR)/bin/* /out/raids_hub/Code/rc4-NTSC; fi
	
setup:
	mkdir -p $(CURDIR)/bin

clean:
	rm -f *.irx *.o ../shared/*.o ../shared/mobs/*.o

include $(CURDIR)/../../../Makefile.pref
include $(CURDIR)/../../../Makefile.eeglobal
include ${CURDIR}/../../../Makefile.rules
