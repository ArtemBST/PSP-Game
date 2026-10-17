#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
    int repair_x; 
    int repair_y;
    int req_tool;   // 0 - Отвертка, 1 - Паяльник, 2 - Спирт/Щетка
    int reward;    
    int is_complex; // 1 - активирует QTE-режим для некстгена (PS3, Xbox360, PS4, XOne)
} ConsoleLevel;

ConsoleLevel levels[18] = {
    {1, "Game Boy Classic", "Clean battery contacts", 30, 10, 2, 15, 0},
    {2, "Dendy / Famicom", "Blow cartridge slot", 25, 12, 2, 10, 0},
    {3, "SEGA Mega Drive", "Fix D-Pad buttons", 20, 15, 0, 25, 0},
    {4, "PlayStation 1", "Adjust CD laser lens", 28, 11, 0, 40, 0},
    {5, "Game Boy Color", "Replace screen lens", 32, 9, 0, 20, 0},
    {6, "SEGA Dreamcast", "Replace RTC battery", 26, 13, 1, 50, 0},
    {7, "PSone", "Fix power jack", 29, 10, 1, 45, 0},
    {8, "PlayStation 2", "Lubricate DVD tray", 24, 14, 2, 35, 0},
    {9, "Xbox Original", "Replace thermal paste", 22, 12, 2, 60, 0},
    {10, "Game Boy Advance SP", "Fix hinge & battery", 31, 8, 0, 30, 0},
    {11, "PlayStation Portable", "Replace analog stick", 27, 11, 0, 50, 0},
    {12, "PlayStation 3", "Clean cooling fan", 21, 15, 2, 70, 1}, 
    {13, "Xbox 360", "Replace DVD drive belt", 23, 13, 0, 55, 1},
    {14, "Nintendo DS", "Calibrate touchscreen", 30, 10, 0, 40, 1},
    {15, "Nintendo Wii", "Fix Sensor Bar port", 25, 12, 1, 65, 1},
    {16, "PlayStation 4", "Deep dust cleaning", 20, 14, 2, 90, 1},
    {17, "Xbox One", "Repair the disk drive", 29, 10, 1, 120, 1},
    {18, "PlayStation Vita", "Repair the graphics chip", 31, 20, 2, 100, 0}
};

const char *tool_names[3] = {
    "[SCREWDRIVER]",   
    "[SOLDERING IRON]", 
    "[ALCOHOL BRUSH]"  
};

// Возможные кнопки для QTE
unsigned int qte_buttons[3] = {PSP_CTRL_TRIANGLE, PSP_CTRL_SQUARE, PSP_CTRL_CIRCLE};
const char *qte_names[3] = {"TRIANGLE", "SQUARE", "CIRCLE"};

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
    
    int current_tool = 0; 
    int money = 0;        
    int is_sparking = 0;  
    char spark_char = ' '; 

    // Переменные для мини-игры "Пайка" (Аналоговый стик)
    float solder_drift_x = 0.0f;

    // Переменные для мини-игры "QTE"
    int qte_active = 0;
    int qte_target_idx = 0;
    int qte_timer = 0;

    while (1) {
        pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf("==================================================\n");
        pspDebugScreenPrintf("             RETRO REPAIR SHOP 2D  v3.0           \n");
        pspDebugScreenPrintf("==================================================\n\n");

        ConsoleLevel *lvl = &levels[current_level];

        pspDebugScreenPrintf(" WALLET: $%d | LEVEL %d/17: %s\n", money, lvl->id, lvl->name);
        pspDebugScreenPrintf(" Issue: %s\n", lvl->issue);
        
        pspDebugScreenPrintf(" Active Tool: %s ", tool_names[current_tool]);
        if (current_tool == lvl->req_tool) pspDebugScreenPrintf("(CORRECT)\n");
        else pspDebugScreenPrintf("(WRONG TOOL!)\n");

        // Отображение интерфейса мини-игр
        if ((pad.Buttons & PSP_CTRL_CROSS) && current_tool == lvl->req_tool) {
            if (lvl->req_tool == 1) { // Мини-игра со стиком для паяльника
                pspDebugScreenPrintf(" [MINIGAME] KEEP SOLDER IN CENTER! ");
                int offset_x = (int)solder_drift_x;
                pspDebugScreenPrintf("[");
                for(int i = -5; i <= 5; i++) {
                    if (i == offset_x) pspDebugScreenPrintf("O");
                    else if (i == 0) pspDebugScreenPrintf("|");
                    else pspDebugScreenPrintf(".");
                }
                pspDebugScreenPrintf("]\n");
            } 
            else if (lvl->is_complex && qte_active) { // Мини-игра QTE для сложных консолей
                pspDebugScreenPrintf(" [QTE EVENT] !! PRESS %s NOW !! (Time: %d)   \n", qte_names[qte_target_idx], qte_timer);
            }
            else {
                pspDebugScreenPrintf("\n");
            }
        } else {
            pspDebugScreenPrintf("\n");
        }

        // Шкала прогресса
        pspDebugScreenPrintf(" Fix Progress: [");
        int bars = fix_progress / 10;
        for (int i = 0; i < 10; i++) {
            if (i < bars) pspDebugScreenPrintf("#");
            else pspDebugScreenPrintf(".");
        }
        pspDebugScreenPrintf("] %d%%\n\n", fix_progress);

        // Игровое поле (Рабочий стол)
        is_sparking = 0; 
        for (int y = 5; y < 18; y++) {
            pspDebugScreenPrintf("  ");
            for (int x = 5; x < 55; x++) {
                if ((pad.Buttons & PSP_CTRL_CROSS) && abs(cursor_x - lvl->repair_x) <= 2 && abs(cursor_y - lvl->repair_y) <= 1) {
                    spark_char = (current_tool == lvl->req_tool) ? '*' : '~';
                    if (abs(x - cursor_x) <= 2 && abs(y - cursor_y) <= 1 && (rand() % 3 == 0)) {
                        is_sparking = 1;
                    }
                }

                if (x == cursor_x && y == cursor_y) {
                    pspDebugScreenPrintf("+"); 
                } else if (is_sparking && abs(x - cursor_x) <= 2 && abs(y - cursor_y) <= 1 && rand() % 2 == 0) {
                    pspDebugScreenPrintf("%c", spark_char); 
                } else if (x == lvl->repair_x && y == lvl->repair_y) {
                    pspDebugScreenPrintf("X"); 
                } else if (x >= 15 && x <= 45 && y >= 8 && y <= 16) {
                    pspDebugScreenPrintf("="); 
                } else {
                    pspDebugScreenPrintf(" ");
                }
            }
            pspDebugScreenPrintf("\n");
        }

        pspDebugScreenPrintf("\n Controls: D-Pad=Move | L/R=Tool | X=Repair (Hold) | START=Exit\n");

        sceCtrlReadBufferPositive(&pad, 1);
        int pressed = pad.Buttons & ~last_buttons;
        last_buttons = pad.Buttons;

        if (pad.Buttons & PSP_CTRL_START) break;

        // Смена инструментов (L / R триггеры)
        if (pressed & PSP_CTRL_LTRIGGER) { current_tool--; if (current_tool < 0) current_tool = 2; }
        if (pressed & PSP_CTRL_RTRIGGER) { current_tool++; if (current_tool > 2) current_tool = 0; }

        // Движение курсора (блокируется при пайке, чтобы аналог не дергал курсор)
        if (!(pad.Buttons & PSP_CTRL_CROSS && current_tool == 1)) {
            if ((pad.Buttons & PSP_CTRL_UP) && cursor_y > 5) cursor_y--;
            if ((pad.Buttons & PSP_CTRL_DOWN) && cursor_y < 17) cursor_y++;
            if ((pad.Buttons & PSP_CTRL_LEFT) && cursor_x > 5) cursor_x--;
            if ((pad.Buttons & PSP_CTRL_RIGHT) && cursor_x < 54) cursor_x++;
        }

        // Логика ремонта
        if (pad.Buttons & PSP_CTRL_CROSS) {
            if (abs(cursor_x - lvl->repair_x) <= 2 && abs(cursor_y - lvl->repair_y) <= 1) {
                if (current_tool == lvl->req_tool) {
                    
                    // 1. Мини-игра "Паяльник" (Удержание аналогового стика)
                    if (current_tool == 1) {
                        solder_drift_x += ((rand() % 3) - 1) * 0.4f; // Случайный увод в сторону
                        
                        int stick_x = pad.Lx - 128; // Читаем положение стика PSP
                        if (abs(stick_x) > 20) {
                            solder_drift_x += (stick_x / 64.0f); // Сопротивление игрока
                        }

                        if (fabsf(solder_drift_x) <= 2.5f) {
                            fix_progress += 2; // Прицел удержан
                        } else {
                            if (fix_progress > 0) fix_progress--; // Промах — паяльник жжёт плату
                        }

                        if (solder_drift_x > 5.0f) solder_drift_x = 5.0f;
                        if (solder_drift_x < -5.0f) solder_drift_x = -5.0f;
                    }
                    
                    // 2. Мини-игра QTE (Быстрые кнопки для некстгена)
                    else if (lvl->is_complex) {
                        if (!qte_active) {
                            if (rand() % 15 == 0) { // Шанс появления кнопки
                                qte_active = 1;
                                qte_target_idx = rand() % 3;
                                qte_timer = 30; // 30 кадров на реакцию
                            }
                            fix_progress += 1; // Медленный базовый ремонт
                        } else {
                            qte_timer--;
                            
                            // Проверка нажатия
                            if (pressed & qte_buttons[qte_target_idx]) {
                                fix_progress += 15; // Успешно нажали — большой буст!
                                qte_active = 0;
                            } else if (pressed & (PSP_CTRL_TRIANGLE | PSP_CTRL_SQUARE | PSP_CTRL_CIRCLE)) {
                                if (fix_progress > 5) fix_progress -= 5; // Нажали не то
                                qte_active = 0;
                            } else if (qte_timer <= 0) {
                                if (fix_progress > 8) fix_progress -= 8; // Не успели вовремя
                                qte_active = 0;
                            }
                        }
                    }

                    // 3. Простой ремонт (Отвертка/Щетка на обычных уровнях)
                    else {
                        fix_progress += 2;
                    }

                    // Переход на следующий уровень
                    if (fix_progress >= 100) {
                        money += lvl->reward;
                        fix_progress = 0;
                        qte_active = 0;
                        solder_drift_x = 0.0f;
                        current_level++;

                        if (current_level >= 17) {
                            pspDebugScreenClear();
                            pspDebugScreenPrintf("\n\n  CONGRATULATIONS! YOU FIXED ALL CONSOLES!\n");
                            pspDebugScreenPrintf("  TOTAL EARNED: $%d\n", money);
                            pspDebugScreenPrintf("  YOU ARE THE ULTIMATE RETRO MASTER!\n");
                            sceKernelDelayThread(5000000);
                            break;
                        }
                    }
                } else {
                    // Штраф за ремонт неправильным инструментом
                    if (fix_progress > 0) fix_progress--;
                    if (money > 0 && rand() % 10 == 0) money--;
                }
            }
        } else {
            // Если игрок отпустил Х, QTE сбрасывается, чтобы не висеть бесконечно
            qte_active = 0;
        }

        sceKernelDelayThread(33000); // Ограничитель 30 FPS
    }

    sceKernelExitGame();
    return 0;
}
