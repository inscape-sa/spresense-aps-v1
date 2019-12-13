
/** Included Files */
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

#include "memutils/os_utils/chateau_osal.h"
#include "memutils/simple_fifo/CMN_SimpleFifo.h"
#include "memutils/memory_manager/MemHandle.h"
#include "memutils/message/Message.h"
#include "audio/audio_high_level_api.h"
#include <audio/utilities/wav_containerformat.h>
#include <audio/utilities/frame_samples.h>
#include "include/msgq_id.h"
#include "include/mem_layout.h"
#include "include/memory_layout.h"
#include "include/msgq_pool.h"
#include "include/pool_layout.h"
#include "include/fixed_fence.h"

#include <arch/chip/cxd56_audio.h>

/** Section number of memory layout 
 * to use (in "include/mem_layout.h") */
#define AUDIO_SECTION   SECTION_NO0

/** Parameter definitions */

extern "C" int aps_cxx_audio_detect_main(int argc, char *argv[])
{
  return 0;
}
