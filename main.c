#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <stdio.h>

/* Перенаправляем стандартный printf на экран PSP */
#define printf pspDebugScreenPrintf

PSP_MODULE_INFO("Driftmania", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int exit_callback(int arg1, int arg2, void *arg) {
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
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, NULL);
    return thid;
}

int main(void) {
    setup_callbacks();
    
    // Инициализируем экран отладки для вывода текста
    pspDebugScreenInit();
    
    // Инициализация контроллера PSP
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    
    SceCtrlData pad;

    int menu_state = 0; // 0 - Главное меню, 1 - Сама игра
    int blink_timer = 0; // Таймер для мигания текста

    while (1) {
        sceCtrlReadBufferPositive(&pad, 1);
        
        // Переводим курсор в левый верхний угол перед каждой перерисовкой экрана
        pspDebugScreenSetXY(0, 0);
        
        if (menu_state == 0) {
            printf("\n\n");
            printf("  ========================================\n");
            printf("            D R I F T M A N I A          \n");
            printf("            PSP Retro Arcade Racing      \n");
            printf("  ========================================\n\n\n");
            
            // Эффект мигания надписи "Press START" каждые ~30 кадров
            if ((blink_timer / 30) % 2 == 0) {
                printf("       > PRESS START TO CONTINUE <       \n");
            } else {
                printf("                                         \n");
            }
            
            printf("\n\n\n     Project by Artem (c) 2026\n");

            // Проверяем нажатие кнопки START
            if (pad.Buttons & PSP_CTRL_START) {
                menu_state = 1; // Переходим в режим игры
            }
        } 
        else if (menu_state == 1) {
            printf("\n\n");
            printf("  [ GAME ENGINE RUNNING ]\n\n");
            printf("  Engine started! Get ready to drift.\n");
            printf("  Press SELECT to return to menu.\n");
            
            if (pad.Buttons & PSP_CTRL_SELECT) {
                menu_state = 0; // Возврат в меню
            }
        }

        blink_timer++;
        
        // Ожидание вертикальной развертки (60 кадров в секунду)
        sceDisplayWaitVblankStart();
    }

    return 0;
}
