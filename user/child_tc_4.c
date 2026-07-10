#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int p1 = fork();
  if(p1 == 0){
    for(int i = 0; i < 2; i++){
      //write(1, "child1\n", 7);
      getpid();
      pause(5);
    }

    exit(0);
  }

  int p2 = fork();
  if(p2 == 0){
    for(int i = 0; i < 5; i++){
      //write(1, "child2\n", 7);
      pause(5);
      getpid();
    }
    exit(0);
  }

  pause(10);
  printf("Child 1 syscall count: %d\n", getchildsyscount(p1));
  printf("Child 2 syscall count: %d\n", getchildsyscount(p2));

  wait(0);
  wait(0);
  exit(0);
}
