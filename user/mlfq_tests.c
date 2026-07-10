#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void cpu() {
    while(1);
}

void syscallheavy() {
    while(1)
        getpid();
}

void interactive() {
    while(1)
        pause(1);
}

int
main()
{
    int p1 = fork();
    if(p1 == 0) cpu();

    int p2 = fork();
    if(p2 == 0) syscallheavy();

    int p3 = fork();
    if(p3 == 0) interactive();

    pause(100);

    struct mlfqinfo i1, i2, i3;

    getmlfqinfo(p1, &i1);
    getmlfqinfo(p2, &i2);
    getmlfqinfo(p3, &i3);

    printf("--> SC-MLFQ Demonstration <--\n");
    printf("CPU-bound Level: %d\n", i1.level);
    printf("Syscall-heavy Level: %d\n", i2.level);
    printf("Interactive Level: %d\n", i3.level);

    kill(p1);
    kill(p2);
    kill(p3);

    exit(0);
}