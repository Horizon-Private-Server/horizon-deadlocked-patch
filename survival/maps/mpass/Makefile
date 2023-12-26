EE_LIBS = -ldl
EE_OBJS = main.o config.o path.o bigalpath.o bigal.o statue.o ../shared/upgrade.o ../shared/drop.o ../shared/gate.o ../shared/mysterybox.o ../shared/pathfind.o ../shared/maputils.o ../shared/bankbox.o ../shared/mobs/mob.o 
EE_ELF = mpass.elf
EE_BIN = mpass.bin
EE_INCS := -I../../../common -I../../include -I../shared/include -Iinclude
EE_LDFLAGS := -DMOB_ZOMBIE -DMOB_TREMOR -DMOB_SWARMER -DMOB_REAPER -DMOB_REACTOR -DGATE -fno-builtin -nostdlib -nostartfiles -L.

# set to DEBUG or RELEASE
BUILD_CONFIG := RELEASE

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG -DMOB_CORN -DMOB_DAMAGETYPES=0x40 -DBIGAL_IDLE -DPRINT_JOINTS_ -DDEBUGPATH_ -DDEBUGMOVE -DDEBUGMBOX_
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := -DRELEASE -DMOB_CORN -DMOB_DAMAGETYPES=0x40
endif

.PHONY: setup

all: setup $(EE_BIN) clean
	# payload and definition
	cp $(CURDIR)/mpass.bin $(CURDIR)/bin/code.0008.bin
	bin2codedef $(CURDIR)/bin/code.0008.bin 01EF0000 $(CURDIR)/bin/code.0008.def 004A7D98

	cp $(CURDIR)/static/*.* $(CURDIR)/bin/
	
setup:
	mkdir -p $(CURDIR)/bin

clean:
	rm -f *.irx *.o ../shared/*.o ../shared/mobs/*.o

include $(CURDIR)/../../../Makefile.pref
include $(CURDIR)/../../../Makefile.eeglobal
include ${CURDIR}/../../../Makefile.rules
