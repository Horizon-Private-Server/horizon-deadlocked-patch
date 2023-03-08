EE_LIBS = -ldl
EE_OBJS = data.o
EE_ELF = data.elf
EE_SYMS = data.syms
EE_BIN = data.bin
EE_INCS := -I../../../../common
EE_LDFLAGS := -fno-builtin -nostdlib -nostartfiles -L. -L../libdl/lib

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
	rm -f *.irx *.o

clean:
	rm -f *.irx *.o *.bin *.elf

include $(CURDIR)/../../../../Makefile.pref
include $(CURDIR)/../../../../Makefile.eeglobal
include ${CURDIR}/../../../../Makefile.rules
