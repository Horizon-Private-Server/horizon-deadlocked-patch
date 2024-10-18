EE_LIBS = -ldl
EE_OBJS = main.o config.o path.o ../shared/map.o ../shared/spawner.o ../shared/pathfind.o ../shared/maputils.o ../shared/mobs/mob.o
EE_ELF = hub.elf
EE_BIN = hub.bin
EE_INCS := -I../../../common -I../../include -I../shared/include -Iinclude
EE_LDFLAGS := -DMOB_ZOMBIE -DMOB_SWARMER -fno-builtin -nostdlib -nostartfiles -L.

# set to DEBUG or RELEASE
BUILD_CONFIG := DEBUG

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DMOB_CORN
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DRELEASE -DMOB_CORN
endif

.PHONY: setup

all: setup $(EE_BIN) clean
	# payload and definition
	cp -rf $(CURDIR)/hub.bin $(CURDIR)/bin/code.0008.bin
	bin2codedef $(CURDIR)/bin/code.0008.bin 01EF0000 $(CURDIR)/bin/code.0008.def 004A7D98

	cp -rf $(CURDIR)/static/*.* $(CURDIR)/bin/
	if [ -d "/out" ]; then cp -rf $(CURDIR)/bin/* /out; fi
	
setup:
	mkdir -p $(CURDIR)/bin

clean:
	rm -f *.irx *.o ../shared/*.o ../shared/mobs/*.o

include $(CURDIR)/../../../Makefile.pref
include $(CURDIR)/../../../Makefile.eeglobal
include ${CURDIR}/../../../Makefile.rules
