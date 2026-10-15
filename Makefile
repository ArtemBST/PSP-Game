TARGET = driftmania
OBJS = main.o

# Название игры и иконка
PSP_EBOOT_TITLE = Driftmania
PSP_EBOOT_ICON = ICON0.PNG

# ВАЖНО: Приказываем сборщику упаковать файлы в EBOOT.PBP
EXTRA_TARGETS = EBOOT.PBP

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
