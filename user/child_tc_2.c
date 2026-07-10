#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int c = getchildsyscount(1234);  
  printf("No children case: %d\n", c);
  exit(0);
}
