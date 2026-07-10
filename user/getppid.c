#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("My PID: %d\n", getpid2());
  printf("My Parent PID: %d\n", getppid());
  exit(0);
}
