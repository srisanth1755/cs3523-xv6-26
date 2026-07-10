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
    int cpu_pid = fork();
    if(cpu_pid == 0)
        cpu_task();

    int int_pid = fork();
    if(int_pid == 0)
        interactive_task();

    struct mlfqinfo info;

    printf("Monitoring starvation...\n");

    for(int i = 0; i < 10; i++){
        pause(50);
        getmlfqinfo(cpu_pid, &info);
        printf("Time %d00 ticks -> CPU Level: %d | Times Scheduled: %d\n",
               i+1, info.level, info.times_scheduled);
    }

    kill(cpu_pid);
    kill(int_pid);
    exit(0);
}