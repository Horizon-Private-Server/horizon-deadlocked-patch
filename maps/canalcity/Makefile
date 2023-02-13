EE_OBJS = main.o
EE_ELF = canalcity.elf
EE_BIN = canalcity.bin
EE_LIBS = -ldl
EE_INCS := -I../../common
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

all: $(EE_BIN) clean
	mkdir -p $(CURDIR)/bin
	cp $(CURDIR)/static/*.* $(CURDIR)/bin/

	# payload and definition
	cp $(CURDIR)/$(EE_BIN) $(CURDIR)/bin/code.0008.bin
	bin2codedef $(CURDIR)/bin/code.0008.bin 0062F028 $(CURDIR)/bin/code.0008.def 004A7D98


clean:
	rm -f *.irx *.o

include $(CURDIR)/../../Makefile.pref
include $(CURDIR)/../../Makefile.eeglobal
include ${CURDIR}/../../Makefile.rules
