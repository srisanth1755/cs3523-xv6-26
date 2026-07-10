#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main()
{
    int pid = fork();

    if(pid == 0) {
        while(1); 
    }

    struct mlfqinfo info;

    printf("Monitoring boost behavior...\n");

    for(int i = 0; i < 8; i++){
        pause(50);
        getmlfqinfo(pid, &info);
        printf("Time %d00 ticks -> Level: %d\n", i+1, info.level);
    }

    kill(pid);
    exit(0);
}