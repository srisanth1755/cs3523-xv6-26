#include "kernel/types.h"
#include "user/user.h"
#include "user/user.h"

int main(void)
{
  int pid = fork();

  if (pid == 0) {
    pause(50);
    exit(0);
  } else {
    printf("Alive children: %d\n", getnumchild());
    wait(0);
  }

  exit(0);
}
