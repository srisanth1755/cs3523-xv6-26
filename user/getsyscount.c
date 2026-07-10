#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int before = getsyscount();

  // make some syscalls
  getpid();
  getpid();
  //write(1, "hi\n", 3);

  int after = getsyscount();

  printf("Syscalls before: %d\n", before);
  printf("Syscalls after : %d\n", after);
  printf("Difference     : %d\n", after - before);

  exit(0);
}
