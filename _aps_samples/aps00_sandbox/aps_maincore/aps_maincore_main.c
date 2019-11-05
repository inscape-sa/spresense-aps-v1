#include <sdk/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <debug.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>

/* test routine */
extern void test_debugring(void);

/* main */
int aps_maincore_main(int argc, char *argv[])
{
  int ret;
  printf("Hello, World. by aps_maincore\n");
  return 0;
}
