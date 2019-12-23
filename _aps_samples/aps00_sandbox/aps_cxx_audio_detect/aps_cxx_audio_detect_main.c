/** --- Included Files */
#include <sdk/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <asmp/mpshm.h>
#include <sys/stat.h>

#include "aps_cxx_audio_detect_class.h"

extern int aps_cxx_audio_detect_class(int argc, char *argv[]);
int aps_cxx_audio_detect_main(int argc, char *argv[])
{
    return aps_cxx_audio_detect_class(argc, argv);
}