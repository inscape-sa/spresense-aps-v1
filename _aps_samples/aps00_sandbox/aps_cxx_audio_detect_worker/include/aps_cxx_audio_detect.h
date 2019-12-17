/* MP object keys. Must be synchronized with supervisor. */

#define APS_CXX_AUDIO_DETECTKEY_SHM         (80)
#define APS_CXX_AUDIO_DETECTKEY_MQ          (81)
#define APS_CXX_AUDIO_DETECTKEY_MUTEX       (82)

#define MSG_ID_APS_CXX_AUDIO_DETECT_INIT   (1)
#define MSG_ID_APS_CXX_AUDIO_DETECT_EXIT   (2)
#define MSG_ID_APS_CXX_AUDIO_DETECT_ACT    (3)

#define MSG_ID_APS_CXX_AUDIO_DETECT_ACK    (0)
#define MSG_ID_APS_CXX_AUDIO_DETECT_NAK    (127)

#define APS_CXX_AUDIO_DETECTKEY_SHM_SIZE   (128 * 1024) /* 128KB(single-Tile)*/