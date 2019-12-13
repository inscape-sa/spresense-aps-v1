
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
static bool app_create_audio_sub_system(void);
static void app_attention_callback(const ErrorAttentionParam *attparam);
static bool app_open_file_dir(void);
static bool app_close_file_dir(void);
/* Sub-Core(DSP) Interface [via Message-Queue] */
static bool printAudCmdResult(uint8_t command_code, AudioResult& result);
static bool app_power_on(void);
static bool app_power_off(void);

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

  /* First, initialize the shared memory and memory utility used by AudioSubSystem. */
  if (!app_init_libraries())
  {
    printf("Error: init_libraries() failure.\n");
    return 1;
  }
  /* Next, Create the features used by AudioSubSystem. */
  if (!app_create_audio_sub_system()) {
    printf("Error: act_audiosubsystem() failure.\n");
    return 1;
  }

  /* Open directory of recording file. */
  if (!app_open_file_dir()) {
    printf("Error: app_open_file_dir() failure.\n");
    return 1;
  }

  /* On and after this point, AudioSubSystem must be active.
   * Register the callback function to be notified when a problem occurs.
   */

  /* Change AudioSubsystem to Ready state so that I/O parameters can be changed. */
   if (!app_power_on()) {
    printf("Error: app_power_on() failure.\n");
    return 1;
  }

  /* Change AudioSubsystem to PowerOff state. */
  if (!app_power_off()) {
    printf("Error: app_power_off() failure.\n");
    return 1;
  }

  /* Close directory of recording file. */
  if (!app_close_file_dir()) {
      printf("Error: app_close_contents_dir() failure.\n");
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

/** app_create_audio_sub_system
 * - Create Audio Manager (= DSP)
 * - Init MIC-FrontEnd CTRL
 * - Create Recorder CTRL
 * - Create Audio-Capture CTRL
 */
static bool app_create_audio_sub_system(void)
{
  bool result = false;

  /* Create manager of AudioSubSystem. */
  AudioSubSystemIDs ids;
  ids.app         = MSGQ_AUD_APP;
  ids.mng         = MSGQ_AUD_MGR;
  ids.player_main = 0xFF;
  ids.player_sub  = 0xFF;
  ids.micfrontend = MSGQ_AUD_FRONTEND;
  ids.mixer       = 0xFF;
  ids.recorder    = MSGQ_AUD_RECORDER;
  ids.effector    = 0xFF;
  ids.recognizer  = 0xFF;
  AS_CreateAudioManager(ids, app_attention_callback);

  /* Create Frontend. */
  AsCreateMicFrontendParams_t frontend_create_param;
  frontend_create_param.msgq_id.micfrontend = MSGQ_AUD_FRONTEND;
  frontend_create_param.msgq_id.mng         = MSGQ_AUD_MGR;
  frontend_create_param.msgq_id.dsp         = MSGQ_AUD_PREDSP;
  frontend_create_param.pool_id.input       = S0_INPUT_BUF_POOL;
  frontend_create_param.pool_id.output      = S0_NULL_POOL;
  frontend_create_param.pool_id.dsp         = S0_PRE_APU_CMD_POOL;
  AS_CreateMicFrontend(&frontend_create_param, NULL);

  /* Create Recorder. */
  AsCreateRecorderParams_t recorder_create_param;
  recorder_create_param.msgq_id.recorder      = MSGQ_AUD_RECORDER;
  recorder_create_param.msgq_id.mng           = MSGQ_AUD_MGR;
  recorder_create_param.msgq_id.dsp           = MSGQ_AUD_DSP;
  recorder_create_param.pool_id.input         = S0_INPUT_BUF_POOL;
  recorder_create_param.pool_id.output        = S0_ES_BUF_POOL;
  recorder_create_param.pool_id.dsp           = S0_ENC_APU_CMD_POOL;
  result = AS_CreateMediaRecorder(&recorder_create_param, NULL);
  if (!result)
    {
      printf("Error: AS_CreateMediaRecorder() failure. system memory insufficient!\n");
      return false;
    }

  /* Create Capture feature. */
  AsCreateCaptureParam_t capture_create_param;
  capture_create_param.msgq_id.dev0_req  = MSGQ_AUD_CAP;
  capture_create_param.msgq_id.dev0_sync = MSGQ_AUD_CAP_SYNC;
  capture_create_param.msgq_id.dev1_req  = 0xFF;
  capture_create_param.msgq_id.dev1_sync = 0xFF;
  result = AS_CreateCapture(&capture_create_param);
  if (!result)
    {
      printf("Error: As_CreateCapture() failure. system memory insufficient!\n");
      return false;
    }

  return true;
}

/** app_attention_callback
 * - it will be called, when DPS is in not-good status.
 */
static void app_attention_callback(const ErrorAttentionParam *attparam)
{
  printf("Attention!! %s L%d ecode %d subcode %d\n",
          attparam->error_filename,
          attparam->line_number,
          attparam->error_code,
          attparam->error_att_sub_code);
}

/** app_open_file_dir
 * - open file DIR at "/mnt/sd0/REC"
 */
static bool app_open_file_dir(void)
{
  DIR *dirp;
  int ret;
  const char *name = RECFILE_ROOTPATH;

  dirp = opendir("/mnt");
  if (!dirp)
    {
      printf("opendir err(errno:%d)\n",errno);
      return false;
    }
  ret = mkdir(name, 0777);
  if (ret != 0)
    {
      if(errno != EEXIST)
        {
          printf("mkdir err(errno:%d)\n",errno);
          return false;
        }
    }
  s_recorder_info.file.dirp = dirp;
  return true;
}

/** app_close_file_dir
 * - close file DIR for current recording
 * "/mnt/sd0/REC"
 */
static bool app_close_file_dir(void)
{
  closedir(s_recorder_info.file.dirp);
  return true;
}

/*********************************
 * Communication API 
 * from Main-Core to Sub-Core(DSP)
 *********************************/
/* this func print any Attention, 
 * when Warning/Error Replry from Sub-Core will be reached. */
static bool printAudCmdResult(uint8_t command_code, AudioResult& result)
{
  if (AUDRLT_ERRORRESPONSE == result.header.result_code) {
    printf("Command code(0x%x): AUDRLT_ERRORRESPONSE:"
           "Module id(0x%x): Error code(0x%x)\n",
            command_code,
            result.error_response_param.module_id,
            result.error_response_param.error_code);
    return false;
  }
  else if (AUDRLT_ERRORATTENTION == result.header.result_code) {
    printf("Command code(0x%x): AUDRLT_ERRORATTENTION\n", command_code);
    return false;
  }
  return true;
}

/** app_power_on
 * 
 */
static bool app_power_on(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_POWERON;
  command.header.command_code  = AUDCMD_POWERON;
  command.header.sub_code      = 0x00;
  command.power_on_param.enable_sound_effect = AS_DISABLE_SOUNDEFFECT;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

/** app_power_off
 * 
 */
static bool app_power_off(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_POWEROFF_STATUS;
  command.header.command_code  = AUDCMD_SETPOWEROFFSTATUS;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}
