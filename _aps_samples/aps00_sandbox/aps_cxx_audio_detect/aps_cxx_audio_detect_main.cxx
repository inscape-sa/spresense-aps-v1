
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

/* include API for MultiCore Management */
#include "worker_ctrl.h"
#include "aps_cxx_audio_detect.h"

/* Worker ELF path */
#define WORKER_FILE "/mnt/spif/aps_cxx_audio_detect"

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
#define DSPBIN_PATH "/mnt/sd0/BIN"
#define USE_MIC_CHANNEL_NUM  2

/* For FIFO. */
#define READ_SIMPLE_FIFO_SIZE (3072 * USE_MIC_CHANNEL_NUM)
#define SIMPLE_FIFO_FRAME_NUM 60
#define SIMPLE_FIFO_BUF_SIZE  (READ_SIMPLE_FIFO_SIZE * SIMPLE_FIFO_FRAME_NUM)

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
static bool app_finalize_libraries(void);
static bool app_create_audio_sub_system(void);
static void app_deact_audio_sub_system(void);
static bool app_init_simple_fifo(void);
static void app_pop_simple_fifo(void);
static void app_attention_callback(const ErrorAttentionParam *attparam);
static bool app_open_file_dir(void);
static bool app_close_file_dir(void);
/* Sub-Core(DSP) Interface [via Message-Queue] */
/*** REACTION **/
static bool printAudCmdResult(uint8_t command_code, AudioResult& result);
static void outputDeviceCallback(uint32_t size);
/*** ACTION **/
static bool app_power_on(void);
static bool app_power_off(void);
static bool app_init_mic_gain(void);
static bool app_set_clkmode(int clk_mode);
static bool app_set_recorder_status(void);
static bool app_set_ready(void);
static bool app_init_recorder_lpcm(sampling_rate_e sampling_rate,
                                  channel_type_e ch_type,
                                  bitwidth_e bitwidth);
static bool app_set_recording_param_lpcm(sampling_rate_e sampling_rate,
                                         channel_type_e ch_type,
                                         bitwidth_e bitwidth);
static bool app_init_micfrontend(uint8_t preproc_type, const char *dsp_name);
static bool app_start_recorder_wav(void);
static bool app_stop_recorder_wav(void);
static void app_recorde_process(uint32_t rec_time);
/*** FILE ACCESS **/
static bool app_open_output_file_wav(void);
static void app_close_output_file_wav(void);
static bool app_update_wav_file_size(void);
static bool app_write_wav_header(void);
static bool app_init_wav_header(void);
/** ASMP **/
static int test_subcore_ctrl(void);

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
  int ret;
  
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

  /* On and after this point, AudioSubSystem must be active.
   * Register the callback function to be notified when a problem occurs.
   */

  /* Change AudioSubsystem to Ready state so that I/O parameters can be changed. */
   if (!app_power_on()) {
    printf("Error: app_power_on() failure.\n");
    return 1;
  }

  /* Initialize simple fifo. */
  if (!app_init_simple_fifo()) {
    printf("Error: app_init_simple_fifo() failure.\n");
    return false;
  }

  /* Set the initial gain of the microphone to be used. */
  if (!app_init_mic_gain()) {
    printf("Error: app_init_mic_gain() failure.\n");
    return 1;
  }

  /* Set audio clock mode. */
  // sampling_rate_e sampling_rate = SAMPLING_RATE_48K;
  if (!app_set_clkmode(AS_CLKMODE_NORMAL)) {
    printf("Error: app_set_clkmode() failure.\n");
    return 1;
  }

  /* Set recorder operation mode. */
  if (!app_set_recorder_status()) {
    printf("Error: app_set_recorder_status() failure.\n");
    return 1;
  }

  /* Initialize recorder. */
  if (!app_init_recorder_lpcm(SAMPLING_RATE_48K,
                              CHAN_TYPE_STEREO,
                              BITWIDTH_16BIT)) {
    printf("Error: app_init_recorder() failure.\n");
    return 1;
  }

  /* Initialize MicFrontend. */
  if (!app_init_micfrontend(AsMicFrontendPreProcThrough,
                            "/mnt/sd0/BIN/PREPROC")) {
    printf("Error: app_init_micfrontend() failure.\n");
    return 1;
  }

  /** -- TEST CODE START -- */
  ret = test_subcore_ctrl();
  if (ret < 0) {
    printf("Error: test_subcore_ctrl failure.\n");
    return 1;
  }
  /** -- TEST CODE END -- */

  /* Start recorder operation. */
  if (!app_start_recorder_wav()) {
    printf("Error: app_start_recorder() failure.\n");
    return 1;
  }

  /* Running... */
  printf("Running time is %d sec\n", RECORDER_REC_TIME);
  {
    /* Timer Start */
    time_t start_time;
    time_t cur_time;
    time(&start_time);

    do {
        /** Check the FIFO every 100 ms and fill if there is space. 
         * 100ms => 4800points(L+R, 4byte * 4800 = 19200byte)
         */
        usleep(100 * 1000);
        app_pop_simple_fifo();

    } while((time(&cur_time) - start_time) < RECORDER_REC_TIME);
  }

  /* Stop recorder operation. */
  if (!app_stop_recorder_wav()) {
    printf("Error: app_stop_recorder() failure.\n");
    return 1;
  }

  /* Return the state of AudioSubSystem before voice_call operation. */
  if (!app_set_ready()) {
    printf("Error: app_set_ready() failure.\n");
    return 1;
  }

  /* Change AudioSubsystem to PowerOff state. */
  if (!app_power_off()) {
    printf("Error: app_power_off() failure.\n");
    return 1;
  }
  
  /* finalize the shared memory and memory utility used by AudioSubSystem. */
  app_deact_audio_sub_system();
  if (!app_finalize_libraries()) {
    printf("Error: finalize_libraries() failure.\n");
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

/** app_finalize_libraries
 * - Setup Shared-Memory for DSP (on SubCore)
 * - Setup MessageManager for DSP
 */
static bool app_finalize_libraries(void)
{
  /* Finalize MessageLib. */
  MsgLib::finalize();
  /* Destroy static pools. */
  MemMgrLite::Manager::destroyStaticPools(AUDIO_SECTION);
  /* Finalize memory manager. */
  MemMgrLite::Manager::finalize();

  /* Destroy shared memory. */
  int ret;
  ret = mpshm_detach(&s_shm);
  if (ret < 0) {
    printf("Error: mpshm_detach() failure. %d\n", ret);
    return false;
  }

  ret = mpshm_destroy(&s_shm);
  if (ret < 0) {
    printf("Error: mpshm_destroy() failure. %d\n", ret);
    return false;
  }

  if (s_container_format != NULL) {
    delete s_container_format;
    s_container_format = NULL;
  }

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

/** app_deact_audio_sub_system
 * - delete all object that be created in create_audio_subsystem. 
 */
static void app_deact_audio_sub_system(void)
{
  AS_DeleteAudioManager();
  AS_DeleteMediaRecorder();
  AS_DeleteMicFrontend();
  AS_DeleteCapture();
}

/** app_init_simple_fifo
 * - init SimpleFifo Manager (it's defined in includes). 
 */
static bool app_init_simple_fifo(void)
{
  if (CMN_SimpleFifoInitialize(&s_recorder_info.fifo.handle,
                               s_recorder_info.fifo.fifo_area,
                               SIMPLE_FIFO_BUF_SIZE, NULL) != 0)
    {
      printf("Error: Fail to initialize simple FIFO.");
      return false;
    }
  CMN_SimpleFifoClear(&s_recorder_info.fifo.handle);

  s_recorder_info.fifo.output_device.simple_fifo_handler =
    (void*)(&s_recorder_info.fifo.handle);
  s_recorder_info.fifo.output_device.callback_function = outputDeviceCallback;

  return true;
}

/** test_print_buf
 * - output write_buffer (Received WAV-DATA)
 * (head+0byte to head+63byte)
 */
static void test_print_buf (int size, uint8_t *buf)
{
  int pos;
  for (pos = 0; pos < 64; pos++) {
    printf("%02x ", s_recorder_info.fifo.write_buf[pos]);
    if ((pos % 16) == 0) {
      printf("\n");
    }
  }
  printf("\n");
}

/** app_pop_simple_fifo
 * - pop recorded sound from Simple-FIFO
 * - In app_write_output_file_wav, 
 *   Copy FIFO data and Write data in File.  
 */
static void app_pop_simple_fifo(void)
{
  size_t occupied_simple_fifo_size =
    CMN_SimpleFifoGetOccupiedSize(&s_recorder_info.fifo.handle);
  uint32_t output_size = 0;

  while (occupied_simple_fifo_size > 0) {
    output_size = (occupied_simple_fifo_size > READ_SIMPLE_FIFO_SIZE) ?
      READ_SIMPLE_FIFO_SIZE : occupied_simple_fifo_size;
    do {        
      int ret;
      if (output_size == 0 || 
        CMN_SimpleFifoGetOccupiedSize(&s_recorder_info.fifo.handle) == 0) {
        break;
      }
      if (CMN_SimpleFifoPoll(&s_recorder_info.fifo.handle,
                            (void*)s_recorder_info.fifo.write_buf,
                            output_size) == 0) {
        printf("ERROR: Fail to get data from simple FIFO.\n");
        break;
      }
      printf("GET_WAV:%dbyte(on 0x%08x)\n", output_size, s_recorder_info.fifo.write_buf);
      test_print_buf(output_size, s_recorder_info.fifo.write_buf);
      s_recorder_info.file.size += output_size;
    } while(0);
    occupied_simple_fifo_size -= output_size;
  }
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

/** outputDeviceCallback
 * - do nothing.
 */
static void outputDeviceCallback(uint32_t size)
{
    /* do nothing */
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

/** app_init_mic_gain
 * - set all MIC Gain 0dB. 
 */
static bool app_init_mic_gain(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_INITMICGAIN;
  command.header.command_code  = AUDCMD_INITMICGAIN;
  command.header.sub_code      = 0;
  command.init_mic_gain_param.mic_gain[0] = 0;
  command.init_mic_gain_param.mic_gain[1] = 0;
  command.init_mic_gain_param.mic_gain[2] = 0;
  command.init_mic_gain_param.mic_gain[3] = 0;
  command.init_mic_gain_param.mic_gain[4] = 0;
  command.init_mic_gain_param.mic_gain[5] = 0;
  command.init_mic_gain_param.mic_gain[6] = 0;
  command.init_mic_gain_param.mic_gain[7] = 0;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

/** app_set_clkmode
 * setup NORMAL MODE (it's for under 192kbps)
 */
static bool app_set_clkmode(int clk_mode)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SETRENDERINGCLK;
  command.header.command_code  = AUDCMD_SETRENDERINGCLK;
  command.header.sub_code      = 0x00;
  command.set_renderingclk_param.clk_mode = clk_mode;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

/** app_set_recorder_status
 * - setup INPUT=MIC, OUTPUT=RAM
 */
static bool app_set_recorder_status(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_RECORDER_STATUS;
  command.header.command_code  = AUDCMD_SETRECORDERSTATUS;
  command.header.sub_code      = 0x00;
  command.set_recorder_status_param.input_device          = AS_SETRECDR_STS_INPUTDEVICE_MIC;
  command.set_recorder_status_param.input_device_handler  = 0x00;
  command.set_recorder_status_param.output_device         = AS_SETRECDR_STS_OUTPUTDEVICE_RAM;
  command.set_recorder_status_param.output_device_handler = &s_recorder_info.fifo.output_device;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

/** app_set_recorder_READY status
 * - it's pair with app_set_recorder_status
 */
static bool app_set_ready(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_READY_STATUS;
  command.header.command_code  = AUDCMD_SETREADYSTATUS;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

/** app_init_recorder_lpcm
 * - setup recording hardware for WAV format
 */
static bool app_init_recorder_lpcm(sampling_rate_e sampling_rate,
                                  channel_type_e ch_type,
                                  bitwidth_e bitwidth)
{
  // it support only "codec_type = AS_CODECTYPE_LPCM"
  if (!app_set_recording_param_lpcm(sampling_rate,
                                    ch_type,
                                    bitwidth)) {
    printf("Error: app_set_recording_param() failure.\n");
    return false;
  }

  AudioCommand command;
  command.header.packet_length = LENGTH_INIT_RECORDER;
  command.header.command_code  = AUDCMD_INITREC;
  command.header.sub_code      = 0x00;
  command.recorder.init_param.sampling_rate  = s_recorder_info.file.sampling_rate;
  command.recorder.init_param.channel_number = s_recorder_info.file.channel_number;
  command.recorder.init_param.bit_length     = s_recorder_info.file.bitwidth;
  snprintf(command.recorder.init_param.dsp_path, AS_AUDIO_DSP_PATH_LEN, "%s", DSPBIN_PATH);
  /* app_init_recorder_wav */
  command.recorder.init_param.codec_type = AS_CODECTYPE_LPCM;
  s_recorder_info.file.format_type = FORMAT_TYPE_WAV;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

static bool app_set_recording_param_lpcm(sampling_rate_e sampling_rate,
                                         channel_type_e ch_type,
                                         bitwidth_e bitwidth)
{
  s_recorder_info.file.codec_type = AS_CODECTYPE_LPCM;
  switch(sampling_rate) {
  case SAMPLING_RATE_8K:
    s_recorder_info.file.sampling_rate = AS_SAMPLINGRATE_8000;
    break;
  case SAMPLING_RATE_16K:
    s_recorder_info.file.sampling_rate = AS_SAMPLINGRATE_16000;
    break;
  case SAMPLING_RATE_48K:
    s_recorder_info.file.sampling_rate = AS_SAMPLINGRATE_48000;
    break;
  case SAMPLING_RATE_192K:
    s_recorder_info.file.sampling_rate = AS_SAMPLINGRATE_192000;
    break;
  default:
    printf("Error: Invalid sampling rate(%d)\n", sampling_rate);
    return false;
  }

  switch(ch_type) {
  case CHAN_TYPE_MONO:
    s_recorder_info.file.channel_number = AS_CHANNEL_MONO;
    break;
  case CHAN_TYPE_STEREO:
    s_recorder_info.file.channel_number = AS_CHANNEL_STEREO;
    break;
  case CHAN_TYPE_4CH:
    s_recorder_info.file.channel_number = AS_CHANNEL_4CH;
    break;
  case CHAN_TYPE_6CH:
    s_recorder_info.file.channel_number = AS_CHANNEL_6CH;
    break;
  case CHAN_TYPE_8CH:
    s_recorder_info.file.channel_number = AS_CHANNEL_8CH;
    break;
  default:
    printf("Error: Invalid channel type(%d)\n", ch_type);
    return false;
  }

  switch (bitwidth) {
  case BITWIDTH_16BIT:
    s_recorder_info.file.bitwidth = AS_BITLENGTH_16;
    break;
  case BITWIDTH_24BIT:
    s_recorder_info.file.bitwidth = AS_BITLENGTH_24;
    break;
  case BITWIDTH_32BIT:
    s_recorder_info.file.bitwidth = AS_BITLENGTH_32;
    break;
  default:
    printf("Error: Invalid bit width(%d)\n", bitwidth);
    return false;
  }

  return true;
}

/** app_init_micfrontend
 * - Request Loading DSP-codec on Sub-Core
 */
static bool app_init_micfrontend(uint8_t preproc_type,
                                 const char *dsp_name)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_INIT_MICFRONTEND;
  command.header.command_code  = AUDCMD_INIT_MICFRONTEND;
  command.header.sub_code      = 0x00;
  command.init_micfrontend_param.ch_num       = s_recorder_info.file.channel_number;
  command.init_micfrontend_param.bit_length   = s_recorder_info.file.bitwidth;
  command.init_micfrontend_param.samples      = getCapSampleNumPerFrame(s_recorder_info.file.codec_type,
                                                                        s_recorder_info.file.sampling_rate);
  command.init_micfrontend_param.preproc_type = preproc_type;
  snprintf(command.init_micfrontend_param.preprocess_dsp_path,
           AS_PREPROCESS_FILE_PATH_LEN,
           "%s", dsp_name);
  command.init_micfrontend_param.data_dest = AsMicFrontendDataToRecorder;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

/** app_start_recorder_wav
 * - 
 */
static bool app_start_recorder_wav(void)
{
  s_recorder_info.file.size = 0;
  CMN_SimpleFifoClear(&s_recorder_info.fifo.handle);

  AudioCommand command;
  command.header.packet_length = LENGTH_START_RECORDER;
  command.header.command_code  = AUDCMD_STARTREC;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

/** app_stop_recorder_wav
 * - 
 */
static bool app_stop_recorder_wav(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_STOP_RECORDER;
  command.header.command_code  = AUDCMD_STOPREC;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (!printAudCmdResult(command.header.command_code, result)) {
    return false;
  }
  CMN_SimpleFifoClear(&s_recorder_info.fifo.handle);
  return true;
}

/*********************************
 * Task Control API 
 * from Main-Core to Sub-Core(DSP)
 *********************************/
/** Start Programs */
static int test_subcore_ctrl(void)
{
  WorkerCtrl *pwc;
  uint32_t msgdata;
  int data = 0x1234;
  int ret;
  void *buf;

  pwc = new WorkerCtrl(WORKER_FILE);
  /* Initialize MP mutex and bind it to MP task */
  if (!pwc->getReady()) {
    printf("WorkerCtrl() constructor failure.\n");
    return -1;
  }

  ret = pwc->initMutex(APS_CXX_AUDIO_DETECTKEY_MUTEX);
  if (ret < 0) {
    printf("initMutex(mutex) failure. %d\n", ret);
    return ret;
  }

  ret = pwc->initMq(APS_CXX_AUDIO_DETECTKEY_MQ);
  if (ret < 0) {
    printf("initMq(shm) failure. %d\n", ret);
    return ret;
  }

  buf = pwc->initShm(APS_CXX_AUDIO_DETECTKEY_SHM, APS_CXX_AUDIO_DETECTKEY_SHM_SIZE);
  if (buf == NULL) {
    printf("initShm() failure. %d\n", ret);
    return ret;
  }
  printf("attached at %08x\n", (uintptr_t)buf);

  ret = pwc->execTask();
  if (ret < 0) {
    printf("execTask() failure. %d\n", ret);
    return ret;
  }

  for (int loop = 0; loop < 10; loop++) {
    /* Send command to worker */
    ret = pwc->send((uint8_t)MSG_ID_APS_CXX_AUDIO_DETECT_ACT, (uint32_t) &data);
    if (ret < 0) {
      printf("send() failure. %d\n", ret);
      return ret;
    }

    /* Wait for worker message */
    ret = pwc->receive((uint32_t *)&msgdata);
    if (ret < 0) {
      printf("recieve() failure. %d\n", ret);
      return ret;
    }
    printf("Worker response: ID = %d, data = %x\n",
            ret, *((int *)msgdata));
              
    /* Lock mutex for synchronize with worker after it's started */
    pwc->lock();
    printf("Worker said: %s\n", buf);
    pwc->unlock();
  }

  /* Send command to worker */
  ret = pwc->send((uint8_t)MSG_ID_APS_CXX_AUDIO_DETECT_EXIT, (uint32_t) &data);
  if (ret < 0) {
    printf("send() failure. %d\n", ret);
    return ret;
  }

  /* Wait for worker message */
  ret = pwc->receive((uint32_t *)&msgdata);
  if (ret < 0) {
    printf("recieve() failure. %d\n", ret);
    return ret;
  }
  printf("Worker response: ID = %d, data = %x\n",
          ret, *((int *)msgdata));

  
  /* Destroy worker */
  ret = pwc->destroyTask();
  if (ret < 0) {
    printf("destroyTask() failure. %d\n", ret);
    return ret;
  }
  printf("Worker exit status = %d\n", ret);

  /* Finalize all of MP objects */
  delete pwc;
  return 0;
}
