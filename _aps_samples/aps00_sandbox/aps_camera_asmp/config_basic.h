#ifndef __APPS_EXAMPLES_CONFIG_BASIC_H
#define __APPS_EXAMPLES_CONFIG_BASIC_H

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

#define message(format, ...)    printf(format, ##__VA_ARGS__)
#define err(format, ...)        fprintf(stderr, format, ##__VA_ARGS__)

/**
 * Configuration for ASMP 
 */
/* Include worker header */
#include "aps_camera_asmp.h"
/* Worker ELF path */
#define WORKER_FILE "/mnt/spif/aps_camera_asmp"

#endif /* __APPS_EXAMPLES_CONFIG_BASIC_H */