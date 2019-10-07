#ifndef __APPS_EXAMPLES_PROFILE_INFO_H
#define __APPS_EXAMPLES_PROFILE_INFO_H

#include <sdk/config.h>
#include <stdio.h>
#include <string.h>
#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/clock.h>

/***************************************************************
 * Profiling API
 ***************************************************************/
static inline void print_tick(void)
{
#if 1
  struct timespec ts;
  clock_systimespec(&ts);
  printf("|--| %d.%06d sec\n", ts.tv_sec, ts.tv_nsec);
#endif
}

#endif /* __APPS_EXAMPLES_PROFILE_INFO_H */