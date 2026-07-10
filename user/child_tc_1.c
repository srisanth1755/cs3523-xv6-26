#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int pid = fork();

  if(pid == 0){
    write(1, "child\n", 6);
    pause(20);
    write(1, "child again\n", 12);
    exit(0);
  } else {
    pause(5); 
    int c = getchildsyscount(pid);
    printf("child syscall count: %d\n", c);
    wait(0);
  }

  exit(0);
}
