#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>

PSP_MODULE_INFO("RetroRepairShop", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* Системные колбэки выхода */
int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, NULL);
    if (thid >= 0) sceKernelStartThread(thid, 0, NULL);
    return thid;
}

/* Структура консоли для ремонта */
typedef struct {
    int id;
    char name[32];
    char issue[64];
    int repair_x; // Координаты поломки на столе
    int repair_y;
} ConsoleLevel;

// Список всех 16 консолей
ConsoleLevel levels[17] = {
    {1, "Game Boy Classic", "Clean battery contacts", 30, 10},
    {2, "Dendy / Famicom", "Blow cartridge slot", 25, 12},
    {3, "SEGA Mega Drive", "Fix D-Pad buttons", 20, 15},
    {4, "PlayStation 1", "Adjust CD laser lens", 28, 11},
    {5, "Game Boy Color", "Replace screen lens", 32, 9},
    {6, "SEGA Dreamcast", "Replace RTC battery", 26, 13},
    {7, "PSone", "Fix power jack", 29, 10},
    {8, "PlayStation 2", "Lubricate DVD tray", 24, 14},
    {9, "Xbox Original", "Replace thermal paste", 22, 12},
    {10, "Game Boy Advance SP", "Fix hinge & battery", 31, 8},
    {11, "PlayStation Portable", "Replace analog stick", 27, 11},
    {12, "PlayStation 3", "Clean cooling fan", 21, 15},
    {13, "Xbox 360", "Replace DVD drive belt", 23, 13},
    {14, "Nintendo DS", "Calibrate touchscreen", 30, 10},
    {15, "Nintendo Wii", "Fix Sensor Bar port", 25, 12},
    {16, "PlayStation 4", "Deep dust cleaning", 20, 14},
    {17, "Xbox One", "Repair the disk drive", 29, 10}
};

int main(void) {
    pspDebugScreenInit();
    setup_callbacks();

    SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    int current_level = 0;
    int cursor_x = 20;
    int cursor_y = 10;
    int fix_progress = 0;
    int last_buttons = 0;

    while (1) {
        pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf("==================================================\n");
        pspDebugScreenPrintf("             RETRO REPAIR SHOP 2D                 \n");
        pspDebugScreenPrintf("==================================================\n\n");

        ConsoleLevel *lvl = &levels[current_level];

        pspDebugScreenPrintf(" LEVEL %d/17: %s\n", lvl->id, lvl->name);
        pspDebugScreenPrintf(" Issue: %s\n", lvl->issue);
        pspDebugScreenPrintf(" Fix Progress: [");
        
        int bars = fix_progress / 10;
        for (int i = 0; i < 10; i++) {
            if (i < bars) pspDebugScreenPrintf("#");
            else pspDebugScreenPrintf(".");
        }
        pspDebugScreenPrintf("] %d%%\n\n", fix_progress);

        // Простой рабочий стол 2D
        for (int y = 5; y < 18; y++) {
            pspDebugScreenPrintf("  ");
            for (int x = 5; x < 55; x++) {
                if (x == cursor_x && y == cursor_y) {
                    pspDebugScreenPrintf("+"); // Курсор мастера (отвертка)
                } else if (x == lvl->repair_x && y == lvl->repair_y) {
                    pspDebugScreenPrintf("X"); // Точка поломки
                } else if (x >= 15 && x <= 45 && y >= 8 && y <= 16) {
                    pspDebugScreenPrintf("="); // Корпус консоли
                } else {
                    pspDebugScreenPrintf(" ");
                }
            }
            pspDebugScreenPrintf("\n");
        }

        pspDebugScreenPrintf("\n Controls: D-Pad = Move Tool | X = Repair | START = Exit\n");

        sceCtrlReadBufferPositive(&pad, 1);
        int pressed = pad.Buttons & ~last_buttons;
        last_buttons = pad.Buttons;

        if (pad.Buttons & PSP_CTRL_START) break;

        // Движение курсора
        if ((pad.Buttons & PSP_CTRL_UP) && cursor_y > 5) cursor_y--;
        if ((pad.Buttons & PSP_CTRL_DOWN) && cursor_y < 17) cursor_y++;
        if ((pad.Buttons & PSP_CTRL_LEFT) && cursor_x > 5) cursor_x--;
        if ((pad.Buttons & PSP_CTRL_RIGHT) && cursor_x < 54) cursor_x++;

        // Механика чистки / ремонта
        if (pad.Buttons & PSP_CTRL_CROSS) {
            // Если курсор наведен на точку поломки
            if (abs(cursor_x - lvl->repair_x) <= 2 && abs(cursor_y - lvl->repair_y) <= 1) {
                fix_progress += 2; // Ремонтируем!
                if (fix_progress >= 100) {
                    fix_progress = 0;
                    current_level++;
                    if (current_level >= 16) {
                        // Победа в игре!
                        pspDebugScreenClear();
                        pspDebugScreenPrintf("\n\n  CONGRATULATIONS! YOU FIXED ALL 16 CONSOLES!\n");
                        pspDebugScreenPrintf("  YOU ARE THE ULTIMATE RETRO MASTER!\n");
                        sceKernelDelayThread(5000000);
                        break;
                    }
                }
            }
        }

        sceKernelDelayThread(30000); // Гладкие 30 FPS
    }

    sceKernelExitGame();
    return 0;
}
