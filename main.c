#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <oslib/oslib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

PSP_MODULE_INFO("RetroRepairShop", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* --- СТРУКТУРЫ ДАННЫХ --- */
typedef struct {
    int id;
    char name[32];
    char issue[64];
    int repair_x; 
    int repair_y;
    int req_tool;   
    int reward;    
    int is_complex; 
} ConsoleLevel;

ConsoleLevel levels[18] = {
    {1, "Game Boy NINTENDO", "Clean battery contacts", 30, 10, 2, 15, 0},
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

const char *tool_names[3] = { "[SCREWDRIVER]", "[SOLDERING IRON]", "[ALCOHOL BRUSH]" };
unsigned int qte_buttons[3] = {PSP_CTRL_TRIANGLE, PSP_CTRL_SQUARE, PSP_CTRL_CIRCLE};
const char *qte_names[3] = {"TRIANGLE", "SQUARE", "CIRCLE"};

/* --- СОХРАНЕНИЯ --- */
void save_game(int current_level, int money) {
    FILE *f = fopen("savegame.dat", "wb");
    if (f) {
        fwrite(&current_level, sizeof(int), 1, f);
        fwrite(&money, sizeof(int), 1, f);
        fclose(f);
    }
}

void load_game(int *current_level, int *money) {
    FILE *f = fopen("savegame.dat", "rb");
    if (f) {
        fread(current_level, sizeof(int), 1, f);
        fread(money, sizeof(int), 1, f);
        fclose(f);
    } else {
        *current_level = 0; // Если сейва нет, начинаем с 0
        *money = 0;
    }
}

/* --- СИСТЕМНЫЕ КОЛБЭКИ --- */
int exit_callback(int arg1, int arg2, void *common) { sceKernelExitGame(); return 0; }
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

/* --- ГЛАВНАЯ ФУНКЦИЯ --- */
int main(void) {
    // Инициализация графики OSLib
    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
    oslInitConsole();
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
    float solder_drift_x = 0.0f;
    int qte_active = 0;
    int qte_target_idx = 0;
    int qte_timer = 0;

    // Загружаем сохранение при старте
    load_game(&current_level, &money);

    while (1) {
        // Очищаем экран темно-синим цветом (стол мастера)
        oslClearScreen(RGB(20, 25, 45));

        ConsoleLevel *lvl = &levels[current_level];

        // Верхняя панель
        oslDrawFillRect(0, 0, 480, 30, RGB(40, 50, 80));
        oslDrawString(10, 8, "RETRO REPAIR SHOP 2D");
        
        char buf[100];
        sprintf(buf, "WALLET: $%d", money);
        oslDrawString(350, 8, buf);

        sprintf(buf, "LEVEL %d/18: %s", lvl->id, lvl->name);
        oslDrawString(10, 40, buf);
        oslDrawString(10, 60, lvl->issue);

        sprintf(buf, "Active Tool: %s", tool_names[current_tool]);
        oslDrawString(10, 80, buf);
        
        if (current_tool == lvl->req_tool) oslDrawString(250, 80, "(CORRECT)");
        else oslDrawString(250, 80, "(WRONG TOOL!)");

        // Прогресс-бар
        oslDrawString(10, 110, "Fix Progress:");
        oslDrawFillRect(110, 115, 200, 10, RGB(50, 50, 50)); // Задний фон
        oslDrawFillRect(110, 115, (200 * fix_progress) / 100, 10, RGB(0, 255, 0)); // Зеленый прогресс
        sprintf(buf, "%d%%", fix_progress);
        oslDrawString(320, 110, buf);

        // Рисуем рабочее поле
        for (int y = 5; y < 18; y++) {
            for (int x = 5; x < 55; x++) {
                int draw_x = x * 8 + 10; // Перевод координат в пиксели
                int draw_y = y * 10 + 140;

                if (x == cursor_x && y == cursor_y) {
                    oslDrawString(draw_x, draw_y, "+"); // Курсор
                } else if (x == lvl->repair_x && y == lvl->repair_y) {
                    oslDrawString(draw_x, draw_y, "X"); // Точка ремонта
                } else if (x >= 15 && x <= 45 && y >= 8 && y <= 16) {
                    oslDrawString(draw_x, draw_y, "="); // Стол/плата
                }
            }
        }

        oslDrawString(10, 260, "D-Pad: Move | L/R: Tool | X: Repair | START: Save & Exit");

        // Отрисовка кадра
        oslEndFrame();
        oslSyncFrame();

        // Чтение ввода
        sceCtrlReadBufferPositive(&pad, 1);
        int pressed = pad.Buttons & ~last_buttons;
        last_buttons = pad.Buttons;

        if (pad.Buttons & PSP_CTRL_START) {
            save_game(current_level, money); // Сохраняем перед выходом
            break;
        }

        if (pressed & PSP_CTRL_LTRIGGER) { current_tool--; if (current_tool < 0) current_tool = 2; }
        if (pressed & PSP_CTRL_RTRIGGER) { current_tool++; if (current_tool > 2) current_tool = 0; }

        // Движение курсора
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
                    
                    if (current_tool == 1) { // Мини-игра Пайка
                        solder_drift_x += ((rand() % 3) - 1) * 0.4f;
                        int stick_x = pad.Lx - 128;
                        if (abs(stick_x) > 20) solder_drift_x += (stick_x / 64.0f);
                        if (fabsf(solder_drift_x) <= 2.5f) fix_progress += 2;
                        else { if (fix_progress > 0) fix_progress--; }
                        if (solder_drift_x > 5.0f) solder_drift_x = 5.0f;
                        if (solder_drift_x < -5.0f) solder_drift_x = -5.0f;
                    }
                    else if (lvl->is_complex) { // QTE
                        if (!qte_active) {
                            if (rand() % 15 == 0) { qte_active = 1; qte_target_idx = rand() % 3; qte_timer = 30; }
                            fix_progress += 1;
                        } else {
                            qte_timer--;
                            if (pressed & qte_buttons[qte_target_idx]) { fix_progress += 15; qte_active = 0; }
                            else if (pressed & (PSP_CTRL_TRIANGLE | PSP_CTRL_SQUARE | PSP_CTRL_CIRCLE)) { 
                                if (fix_progress > 5) fix_progress -= 5; qte_active = 0; 
                            }
                            else if (qte_timer <= 0) { 
                                if (fix_progress > 8) fix_progress -= 8; qte_active = 0; 
                            }
                        }
                    }
                    else fix_progress += 2;

                    // Переход на новый уровень
                    if (fix_progress >= 100) {
                        money += lvl->reward;
                        fix_progress = 0;
                        qte_active = 0;
                        solder_drift_x = 0.0f;
                        current_level++;
                        save_game(current_level, money); // АВТОСОХРАНЕНИЕ

                        if (current_level >= 18) {
                            oslClearScreen(RGB(0, 0, 0));
                            oslDrawString(100, 130, "CONGRATULATIONS! YOU FIXED ALL CONSOLES!");
                            oslEndFrame(); oslSyncFrame();
                            sceKernelDelayThread(5000000);
                            break;
                        }
                    }
                } else {
                    if (fix_progress > 0) fix_progress--;
                    if (money > 0 && rand() % 10 == 0) money--;
                }
            }
        } else {
            qte_active = 0;
        }

        sceKernelDelayThread(33000); // 30 FPS
    }

    oslEndGfx();
    oslQuit();
    sceKernelExitGame();
    return 0;
}
