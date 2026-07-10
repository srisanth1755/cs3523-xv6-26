#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main()
{
    int pid = fork();

    if(pid == 0) {
        while(1) {
            getpid();  
        }
    } else {
        pause(100);

        struct mlfqinfo info;
        getmlfqinfo(pid, &info);

        printf("Syscall-heavy Process:\n");
        printf("Final Level: %d\n", info.level);

        kill(pid);
    }

    exit(0);
}