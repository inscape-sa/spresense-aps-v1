#ifndef __APPS_EXAMPLES_COLOR_PROC_MAINCORE_H
#define __APPS_EXAMPLES_COLOR_PROC_MAINCORE_H

#include <errno.h>

#include <asmp/types.h>
#include <asmp/mpshm.h>
#include <asmp/mpmutex.h>
#include <asmp/mpmq.h>

#include "asmp.h"

void apsamp_main_yuv2rgb(void *buf, int size);

#endif /* __APPS_EXAMPLES_COLOR_PROC_MAINCORE_H */