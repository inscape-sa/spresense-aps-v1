#ifndef __APPS_EXAMPLES_PROFILE_INFO_H
#define __APPS_EXAMPLES_PROFILE_INFO_H

#include <sdk/config.h>
#include <stdio.h>
#include <string.h>
#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/clock.h>

#define ENABLE_PROFILING

/***************************************************************
 * Profiling API
 ***************************************************************/
static inline void print_tick(struct timespec *get_p_ts)
{
#ifdef ENABLE_PROFILING
  clock_systimespec(get_p_ts);
  printf("|--| %d.%09d sec\n", get_p_ts->tv_sec, get_p_ts->tv_nsec);
#endif /* ENABLE_PROFILING */
}

static inline void diff_tick(struct timespec *start_p_ts, struct timespec *end_p_ts, int num_of_frame)
{
#ifdef ENABLE_PROFILING
  double fps = (double)num_of_frame;
  double time;
  int sec = end_p_ts->tv_sec - start_p_ts->tv_sec;
  int nsec = end_p_ts->tv_nsec - start_p_ts->tv_nsec;
  if (nsec < 0) {
    sec -= 1;
    nsec += 1000000000;
  }
  time = ((double)sec) + (((double)nsec) * 0.000000001);
  fps /= time;
  printf("|--| DIFF = %d.%09dsec, %4.4lf[FPS]\n", sec, nsec, fps);


#endif /* ENABLE_PROFILING */
}

#endif /* __APPS_EXAMPLES_PROFILE_INFO_H */