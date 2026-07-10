#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int pid = fork();

  if(pid == 0){
    pause(20);
    exit(0);
  } else {
    pause(5);
    int c = getchildsyscount(99999); // PID does not exist..so kept 9999
    printf("Invalid PID case: %d\n", c);
    wait(0);
  }

  exit(0);
}
