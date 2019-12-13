
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

#include "memutils/os_utils/chateau_osal.h"
#include "memutils/simple_fifo/CMN_SimpleFifo.h"
#include "memutils/memory_manager/MemHandle.h"
#include "memutils/memory_manager/MemManager.h"
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


/** ---
 * This Code is in NameSpace "MemMgrLite" 
 * (it's same as memory_manager).
 */
using namespace MemMgrLite;

/** --- 
 * Section number of memory layout is
 * used in "include/mem_layout.h" 
 * and Communication via DSP(Sub-Core) */
#define AUDIO_SECTION   SECTION_NO0

/** --- parameter settings */
#define MAX_PATH_LENGTH 128
#define RECFILE_ROOTPATH "/mnt/sd0/REC"
#define DSPBIN_PATH "/mnt/sd0/BIN"
#define USE_MIC_CHANNEL_NUM  2

/* For FIFO. */
#define READ_SIMPLE_FIFO_SIZE (3072 * USE_MIC_CHANNEL_NUM)
#define SIMPLE_FIFO_FRAME_NUM 60
#define SIMPLE_FIFO_BUF_SIZE  (READ_SIMPLE_FIFO_SIZE * SIMPLE_FIFO_FRAME_NUM)

/* For line buffer mode. */
#define STDIO_BUFFER_SIZE 4096

/* Recording time(sec). */
#define RECORDER_REC_TIME 10

/** --- enum Definitions */
enum codec_type_e
{
  CODEC_TYPE_MP3 = 0,
  CODEC_TYPE_LPCM,
  CODEC_TYPE_OPUS,
  CODEC_TYPE_NUM
};

enum sampling_rate_e
{
  SAMPLING_RATE_8K = 0,
  SAMPLING_RATE_16K,
  SAMPLING_RATE_48K,
  SAMPLING_RATE_192K,
  SAMPLING_RATE_NUM
};

enum channel_type_e
{
  CHAN_TYPE_MONO = 0,
  CHAN_TYPE_STEREO,
  CHAN_TYPE_4CH,
  CHAN_TYPE_6CH,
  CHAN_TYPE_8CH,
  CHAN_TYPE_NUM
};

enum bitwidth_e
{
  BITWIDTH_16BIT = 0,
  BITWIDTH_24BIT,
  BITWIDTH_32BIT
};

enum microphone_device_e
{
  MICROPHONE_ANALOG = 0,
  MICROPHONE_DIGITAL,
  MICROPHONE_NUM
};

enum format_type_e
{
  FORMAT_TYPE_RAW = 0,
  FORMAT_TYPE_WAV,
  FORMAT_TYPE_OGG,
  FORMAT_TYPE_NUM
};

/** --- Structures */
struct recorder_fifo_info_s
{
  CMN_SimpleFifoHandle        handle;
  AsRecorderOutputDeviceHdlr  output_device;
  uint32_t                    fifo_area[SIMPLE_FIFO_BUF_SIZE/sizeof(uint32_t)];
  uint8_t                     write_buf[READ_SIMPLE_FIFO_SIZE];
};

struct recorder_file_info_s
{
  uint32_t  sampling_rate;
  uint8_t   channel_number;
  uint8_t   bitwidth;
  uint8_t   codec_type;
  uint16_t  format_type;
  uint32_t  size;
  DIR       *dirp;
  FILE      *fd;
};

struct recorder_info_s
{
  struct recorder_fifo_info_s  fifo;
  struct recorder_file_info_s  file;
};

/** --- Prototype Definitions */
static bool app_init_libraries(void);

/** --- Static Variables */

/* For share memory. */
static mpshm_t s_shm;
/* For Audio Processing */
static recorder_info_s s_recorder_info;
static WavContainerFormat* s_container_format = NULL;
static WAVHEADER  s_wav_header;

/*****************************************************************
 * Public Functions 
 *****************************************************************/
/** Start Programs */
extern "C" int aps_cxx_audio_detect_main(int argc, char *argv[])
{
  printf("Start aps_cxx_audio_detect\n");
  if (!app_init_libraries())
  {
    printf("Error: init_libraries() failure.\n");
    return 1;
  }
  
  printf("SUCCESS - exit App.\n");
  return 0;
}

/*****************************************************************
 * Static Functions 
 *****************************************************************/
/** app_init_libraries
 * - Setup Shared-Memory for DSP (on SubCore)
 * - Setup MessageManager for DSP
 */
static bool app_init_libraries(void)
{
  int ret;
  uint32_t addr = AUD_SRAM_ADDR;

  /* Initialize shared memory.*/
  ret = mpshm_init(&s_shm, 1, 1024 * 128 * 2);
  if (ret < 0) {
    printf("Error: mpshm_init() failure. %d\n", ret);
    return false;
  }

  ret = mpshm_remap(&s_shm, (void *)addr);
  if (ret < 0) {
    printf("Error: mpshm_remap() failure. %d\n", ret);
    return false;
  }

  /* Initalize MessageLib. */
  err_t err = MsgLib::initFirst(NUM_MSGQ_POOLS, MSGQ_TOP_DRM);
  if (err != ERR_OK) {
    printf("Error: MsgLib::initFirst() failure. 0x%x\n", err);
    return false;
  }

  err = MsgLib::initPerCpu();
  if (err != ERR_OK) {
    printf("Error: MsgLib::initPerCpu() failure. 0x%x\n", err);
    return false;
  }

  void* mml_data_area = translatePoolAddrToVa(MEMMGR_DATA_AREA_ADDR);
  err = Manager::initFirst(mml_data_area, MEMMGR_DATA_AREA_SIZE);
  if (err != ERR_OK) {
    printf("Error: Manager::initFirst() failure. 0x%x\n", err);
    return false;
  }

  err = Manager::initPerCpu(mml_data_area, static_pools, pool_num, layout_no);
  if (err != ERR_OK) {
    printf("Error: Manager::initPerCpu() failure. 0x%x\n", err);
    return false;
  }

  /* Create static memory pool of VoiceCall. */
  const uint8_t sec_no = AUDIO_SECTION;
  const NumLayout layout_no = MEM_LAYOUT_RECORDER;
  void* work_va = translatePoolAddrToVa(S0_MEMMGR_WORK_AREA_ADDR);
  const PoolSectionAttr *ptr  = &MemoryPoolLayouts[AUDIO_SECTION][layout_no][0];
  err = Manager::createStaticPools(sec_no,
                                   layout_no,
                                   work_va,
                                   S0_MEMMGR_WORK_AREA_SIZE,
                                   ptr);
  if (err != ERR_OK) {
    printf("Error: Manager::createStaticPools() failure. %d\n", err);
    return false;
  }

  if (s_container_format != NULL) {
    return false;
  }
  s_container_format = new WavContainerFormat();

  return true;
}
