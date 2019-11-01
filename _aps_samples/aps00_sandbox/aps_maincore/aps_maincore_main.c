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
#include <debugring.h>

/* DEBUG = ON/OFF */
#define _DEBUG_PRINTF_
#ifdef _DEBUG_PRINTF_
#define dbgprintf(f_, ...) printf((f_), ##__VA_ARGS__)
#else
#define dbgprintf(f_, ...) while(0){}
#endif /* _DEBUG_PRINTF_ */

/* for debug memory space */
char test_memory[64 * 1024];

/* test routine */
static void test_debugring(void);

/* main */
int aps_maincore_main(int argc, char *argv[])
{
  int ret;
  printf("Hello, World. by aps_maincore\n");

  ret = init_debugring(test_memory);
  printf("ret(%d) = init_debugring(on 0x%08x)\n", ret, test_memory);
  test_debugring();

  return 0;
}

/** Ring Buffer Test */
static void test_debugring(void)
{
  int ret;
  char set_buf[] = "Test Test Test";
  char set_len = strlen(set_buf) + 1;
  const char get_buf_len = 64;
  char get_buf[get_buf_len];
  sDebugRingItem getitem;

  int loop;
  const int num_test = 100;

  for (loop = 0; loop < num_test; loop++)   
  {
    enqueue_debugring(set_len, set_buf, set_len);

    memset(get_buf, 0, get_buf_len);
    ret = dequeue_debugring(&getitem, get_buf, get_buf_len);
    dbgprintf("dequeue(RET=%d, CPUID=%d, PARAM=0x%x, buf[%s][%s])\n", ret, getitem.cpuid, getitem.param, get_buf, set_buf);

    enqueue_debugring(set_len, set_buf, set_len);
    
    memset(get_buf, 0, get_buf_len);
    ret = dequeue_debugring(&getitem, get_buf, get_buf_len);
    dbgprintf("dequeue(RET=%d, CPUID=%d, PARAM=0x%x, buf[%s][%s])\n", ret, getitem.cpuid, getitem.param, get_buf, set_buf);

    enqueue_debugring(set_len, set_buf, set_len);

    memset(get_buf, 0, get_buf_len);
    ret = dequeue_debugring(&getitem, get_buf, get_buf_len);
    dbgprintf("dequeue(RET=%d, CPUID=%d, PARAM=0x%x, buf[%s][%s])\n", ret, getitem.cpuid, getitem.param, get_buf, set_buf);

    memset(get_buf, 0, get_buf_len);
    ret = dequeue_debugring(&getitem, get_buf, get_buf_len);
    dbgprintf("dequeue(RET=%d, CPUID=%d, PARAM=0x%x, buf[%s][%s])\n", ret, getitem.cpuid, getitem.param, get_buf, set_buf);
  }

  return;
}