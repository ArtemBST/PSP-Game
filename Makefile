TARGET = driftmania
OBJS = main.o

# Подключаем твою кастомную иконку
PSP_EBOOT_ICON = ICON0.PNG

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
