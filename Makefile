TARGET = RetroRepairShop
OBJS = main.o

# Название игры и иконка
PSP_EBOOT_TITLE = Retro Repair Shop 2D
PSP_EBOOT_ICON = ICON0.PNG

# ВАЖНО: Добавляем библиотеку oslib и ее зависимости
# -losl (сама библиотека), -lpng -lz (для картинок), -lm (математика)
LIBS = -losl -lpng -lz -lm -lpspgum -lpspgu -lpsprtc -lpspaudiolib -lpspaudio

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

EXTRA_TARGETS = EBOOT.PBP
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
