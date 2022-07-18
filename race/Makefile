
EE_ELF = race-11184.elf
EE_BIN = race-11184.bin
EE_OBJS = main.o game.o scoreboard.o utils.o config.o net.o bezier.o
EE_LIBS = -ldl

EE_INCS := -I../common
EE_LDFLAGS := -fno-builtin -nostdlib -nostartfiles -L. -L../libdl/lib

# set to DEBUG or RELEASE
BUILD_CONFIG := DEBUG

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := 
endif

all: $(EE_BIN) clean copy

clean:
	rm -f *.irx *.o

include $(CURDIR)/../Makefile.pref
include $(CURDIR)/../Makefile.eeglobal
include ${CURDIR}/../Makefile.rules

