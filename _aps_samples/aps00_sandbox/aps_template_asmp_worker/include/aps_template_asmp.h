/* MP object keys. Must be synchronized with supervisor. */

#define APS_TEMPLATE_ASMPKEY_SHM   1
#define APS_TEMPLATE_ASMPKEY_MQ    2
#define APS_TEMPLATE_ASMPKEY_MUTEX 3

#define MSG_ID_APS_TEMPLATE_ASMP_INIT   (1)
#define MSG_ID_APS_TEMPLATE_ASMP_EXIT   (2)
#define MSG_ID_APS_TEMPLATE_ASMP_ACT    (3)

#define MSG_ID_APS_TEMPLATE_ASMP_ACK    (0)
#define MSG_ID_APS_TEMPLATE_ASMP_NAK    (127)

#define APS_TEMPLATE_ASMPKEY_SHM_SIZE   (128 * 1024) /* 128KB(single-Tile)*/