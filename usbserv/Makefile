#  _____     ___ ____
#   ____|   |    ____|     
#  |     ___|   |____      
#                          
#  ------------------------------------------------------------------------

IOP_BIN = usbserv.irx
IOP_OBJS = usbserv.o imports.o

IOP_INCS += -I$(PS2SDK)/iop/include
IOP_CFLAGS += -Wall -Os -D_IOP -fno-builtin $(DEBUG_FLAGS) -DDEBUG1
IOP_LDFLAGS += -s

all: $(IOP_BIN)

clean:
	-rm -f *.o *.irx

include $(PS2SDK)/Defs.make
include Rules.make

