#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void cpu_task() {
    while(1);
}

void interactive_task() {
    while(1) {
        pause(1);
    }
}

int
main()
{
    int pid1 = fork();
    if(pid1 == 0)
        cpu_task();

    int pid2 = fork();
    if(pid2 == 0)
        interactive_task();

    pause(100);

    struct mlfqinfo info1, info2;

    getmlfqinfo(pid1, &info1);
    getmlfqinfo(pid2, &info2);

    printf("CPU Process Level: %d\n", info1.level);
    printf("Interactive Process Level: %d\n", info2.level);

    kill(pid1);
    kill(pid2);

    exit(0);
}