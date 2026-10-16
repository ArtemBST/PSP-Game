#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <time.h>

PSP_MODULE_INFO("PuzzleGalaxyGame", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* Глобальные переменные (для RetroAchievements) */
int g_game_state = 1;     // 1 - игра идет, 2 - победа
int g_cursor_x = 0;       // Позиция курсора X (0-2)
int g_cursor_y = 0;       // Позиция курсора Y (0-2)
int g_selected_x = -1;    // Выбранный тайл X (-1 если не выбран)
int g_selected_y = -1;    // Выбранный тайл Y
int g_moves = 0;          // Счётчик ходов (для ачивок)
int g_is_won = 0;         // 0 - в процессе, 1 - собрано

#define SIZE 3
int g_board[SIZE][SIZE];

/* Системные функции выхода из игры */
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

// Функция перемешивания поля (гарантирует, что пазл собираемый)
void shuffle_board(void) {
    int expected = 1;
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            if (x == SIZE - 1 && y == SIZE - 1) {
                g_board[y][x] = 0;
            } else {
                g_board[y][x] = expected++;
            }
        }
    }

    // Делаем серию случайных обменов тайлов
    for (int i = 0; i < 15; i++) {
        int x1 = rand() % SIZE;
        int y1 = rand() % SIZE;
        int x2 = rand() % SIZE;
        int y2 = rand() % SIZE;

        int temp = g_board[y1][x1];
        g_board[y1][x1] = g_board[y2][x2];
        g_board[y2][x2] = temp;
    }
}

// Проверка: собран ли пазл правильно
int check_win(void) {
    int expected = 1;
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            if (x == SIZE - 1 && y == SIZE - 1) {
                if (g_board[y][x] != 0) return 0;
            } else {
                if (g_board[y][x] != expected++) return 0;
            }
        }
    }
    return 1;
}

int main(void) {
    pspDebugScreenInit();
    setup_callbacks();

    // Инициализация генератора случайных чисел
    srand(time(NULL));
    shuffle_board(); // Перемешиваем при запуске!

    SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    int last_buttons = 0;

    while (1) {
        pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf("=== PUZZLE GALAXY GAME ===\n");
        pspDebugScreenPrintf("Moves: %d  Status: %s\n\n", g_moves, g_is_won ? "VICTORY!" : "Playing");

        // Отрисовка поля
        for (int y = 0; y < SIZE; y++) {
            pspDebugScreenPrintf("  ");
            for (int x = 0; x < SIZE; x++) {
                if (x == g_cursor_x && y == g_cursor_y) {
                    pspDebugScreenPrintf("[%d] ", g_board[y][x]); // Курсор
                } else if (x == g_selected_x && y == g_selected_y) {
                    pspDebugScreenPrintf("*%d* ", g_board[y][x]); // Выбран для обмена
                } else {
                    pspDebugScreenPrintf(" %d  ", g_board[y][x]);
                }
            }
            pspDebugScreenPrintf("\n");
        }

        pspDebugScreenPrintf("\nControls:\n");
        pspDebugScreenPrintf("D-Pad: Move | Cross (X): Select/Swap\n");
        pspDebugScreenPrintf("START: Exit\n");

        sceCtrlReadBufferPositive(&pad, 1);
        int pressed = pad.Buttons & ~last_buttons;
        last_buttons = pad.Buttons;

        if (pad.Buttons & PSP_CTRL_START) break;

        if (!g_is_won) {
            // Управление курсором
            if ((pressed & PSP_CTRL_UP) && g_cursor_y > 0) g_cursor_y--;
            if ((pressed & PSP_CTRL_DOWN) && g_cursor_y < SIZE - 1) g_cursor_y++;
            if ((pressed & PSP_CTRL_LEFT) && g_cursor_x > 0) g_cursor_x--;
            if ((pressed & PSP_CTRL_RIGHT) && g_cursor_x < SIZE - 1) g_cursor_x++;

            // Выбор плитки и обмен
            if (pressed & PSP_CTRL_CROSS) {
                if (g_selected_x == -1) {
                    g_selected_x = g_cursor_x;
                    g_selected_y = g_cursor_y;
                } else {
                    // Меняем местами
                    int temp = g_board[g_selected_y][g_selected_x];
                    g_board[g_selected_y][g_selected_x] = g_board[g_cursor_y][g_cursor_x];
                    g_board[g_cursor_y][g_cursor_x] = temp;
                    
                    g_moves++;
                    g_selected_x = -1;
                    g_selected_y = -1;

                    if (check_win()) {
                        g_is_won = 1;
                        g_game_state = 2; // Победа для RA
                    }
                }
            }
        }

        sceKernelDelayThread(100000); // 100мс пауза
    }

    sceKernelExitGame();
    return 0;
}
