EE_LIBS = -ldl
EE_OBJS = main.o ../../shared/gate.o gas.o power.o ../../shared/mysterybox.o ../../shared/maputils.o
EE_ELF = logic.elf
EE_BIN = logic.bin
EE_INCS := -I../../../../common -I../../../include -I../../shared/include
EE_LDFLAGS := -fno-builtin -nostdlib -nostartfiles -L.

# set to DEBUG or RELEASE
BUILD_CONFIG := RELEASE

# build flags
# 	DEBUG									Enables debug strings and additional misc debug logic.

ifeq ($(BUILD_CONFIG),DEBUG)
EE_DEFS := -DDEBUG
else ifeq ($(BUILD_CONFIG),RELEASE)
EE_DEFS := 
endif

all: $(EE_BIN) clean-temp

clean-temp:
	rm -f *.irx *.o ../../shared/*.o

clean:
	rm -f *.irx *.o *.bin *.elf ../../shared/*.o

include $(CURDIR)/../../../../Makefile.pref
include $(CURDIR)/../../../../Makefile.eeglobal
include ${CURDIR}/../../../../Makefile.rules
