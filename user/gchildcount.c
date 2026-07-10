#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int pid = fork();

  if(pid == 0){
    // child
    for(int i = 0; i < 5; i++){
      write(1, "child running\n", 14);
      pause(10);
    }
    exit(0);
  } else {
    // parent
    pause(10);   // let child make syscalls first
    int c = getchildsyscount(pid);
    printf("Child syscall count: %d\n", c);
    wait(0);
  }

  exit(0);
}
