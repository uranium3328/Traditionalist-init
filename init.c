#include "header.h"
#include <unistd.h>
#include <sys/reboot.h> // Нужен для правильного выключения в конце

void handle_signal(int sig) {
    printf("\nTRAD: Received signal SHTD %d. Shutting down gracefully...\n", sig);
    system("./sh.init.shutdown");
    
    // Вместо exit(0), который роняет ядро в Kernel Panic:
    printf("TRAD: Powering off...\n");
    sync(); // Сбрасываем кэш дисков на всякий случай
    reboot(RB_POWER_OFF); 
}

int main (void) {
    // Настраиваем обработку сигналов
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal); // Добавим еще и SIGTERM
    int status;
    
    printf("TRAD: Successfully init started!\n");

    // 1. Монтирование proc
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        perror("TRAD: Failed to mount /proc via C");
    } else {
        printf("TRAD: /proc mounted successfully via C.\n");
    }

    // 2. Монтирование sysfs (Исправлены логи)
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0) {
        perror("TRAD: Failed to mount /sys via C");
    } else {
        printf("TRAD: /sys mounted successfully via C.\n");
    }

    // 3. Создаем директории для устройств
    mkdir("/dev/pts", 0755);
    mkdir("/dev/shm", 1777);

    // 4. Монтирование devpts
    if (mount("devpts", "/dev/pts", "devpts", 0, NULL) != 0) {
        perror("TRAD: Failed to mount /dev/pts via C");
    } else {
        printf("TRAD: /dev/pts mounted successfully via C.\n");
    }

    // 5. Монтирование tmpfs (Добавлена точка с запятой)
    if (mount("tmpfs", "/dev/shm", "tmpfs", 0, NULL) != 0) {
        perror("TRAD: Failed to mount /dev/shm via C");
    } else {
        printf("TRAD: /dev/shm mounted successfully via C.\n");
    }

    // Запуск основного скрипта инициализации сервисов
    printf("TRAD: Running sh.init...\n");
    status = system("./sh.init");

    if (status == -1) {
        printf("TRAD: ERROR: Failed to execute sh.init\n");
    } else {
        printf("TRAD: Script finished with status: %d\n", status);
    }

    printf("TRAD: Wait for the user's signals...\n");

    // Правильный сбор зомби-процессов для PID 1
    while(1) {
        // wait(NULL) заблокирует процесс, пока есть живые дети.
        // Если детей вообще нет, он вернет -1. Чтобы не грузить CPU, спим 1 секунду.
        if (wait(NULL) == -1) {
            sleep(1); 
        }
    }

    return 0;
}
