// #include "kernel/types.h"
// #include "user/user.h"

// int
// main()
// {
//   volatile unsigned long x = 0;
//   while(1){
//     x++;   // pure CPU
//   }


//   exit(0);
// }
// cpu_bound.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main()
{
    int pid = fork();

    if(pid == 0) {
        while(1) {
        }
    } else {
        pause(100);

        struct mlfqinfo info;
        getmlfqinfo(pid, &info);

        printf("CPU-bound Process:\n");
        printf("Final Level: %d\n", info.level);
        printf("Ticks: %d %d %d %d\n",
               info.ticks[0], info.ticks[1],
               info.ticks[2], info.ticks[3]);

        kill(pid);
    }

    exit(0);
}