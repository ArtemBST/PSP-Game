#include <pspkernel.h>
#include <pspdebug.h>

PSP_MODULE_INFO("PSP Hello World", 0, 1, 1);

#define printf pspDebugScreenPrintf

int main() {
    pspDebugScreenInit();
    printf("Hello, PSP World!\n");
    printf("Project started by Artem!\n");

    while(1) {
        // Главный цикл программы
    }

    return 0;
}