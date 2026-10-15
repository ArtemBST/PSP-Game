TARGET = HelloPSP
OBJS = main.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Hello PSP by Artem

PSPSDK=$(shell psp-config --psp-prefix)
include $(PSPSDK)/lib/build.mak
