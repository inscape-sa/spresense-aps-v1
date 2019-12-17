#include <errno.h>
#include <asmp/types.h>
#include <asmp/mpshm.h>
#include <asmp/mpmutex.h>
#include <asmp/mpmq.h>

#include "asmp.h"
#include "include/aps_cxx_audio_detect.h"

/* --- Parameters --- */
static const char helloworld[] = "Hello, ASMP World!";

/* --- define typed --- */
typedef struct _s_task_resource {
  mpmutex_t mutex;
  mpshm_t shm;
  mpmq_t mq;
  char *buf;
} TaskResource;

/* prototype definition */
/** MACRO **/
#define ASSERT(cond) if (!(cond)) wk_abort()

/** TASK Control **/
static int send(TaskResource *pt, uint8_t id, uint32_t value);
static int receive(TaskResource *pt, uint32_t *pvalue);
static void task_lock(TaskResource *pt);
static void task_unlock(TaskResource *pt);
static void fini_task_resource(TaskResource *pt);
static int init_task_resource(TaskResource *pt);
/** Comodity API **/
static char *strcopy(char *dest, const char *src);

/*****************************************************************
 * Public Functions 
 *****************************************************************/
/** Start Programs */
int main(void)
{
  TaskResource t;
  uint32_t msgdata;
  char *buf;
  int ret;
  int req;

  /* Initialize MP Mutex */
  ret = init_task_resource(&t);
  ASSERT(ret == 0);
  buf = t.buf;

  do {
    uint8_t nak = 0;
    uint8_t retcode;
    /* Receive message from supervisor */
    req = receive(&t, &msgdata);

    switch (req) {
    case MSG_ID_APS_CXX_AUDIO_DETECT_INIT:
      /* NOP */
      break;
    case MSG_ID_APS_CXX_AUDIO_DETECT_ACT:
      /* Copy hello message to shared memory */
      task_lock(&t);
      strcopy(buf, helloworld);
      task_unlock(&t);
      break;  
    case MSG_ID_APS_CXX_AUDIO_DETECT_EXIT:
      msgdata = 0xABCD;
      break;
    default:
      /* NOP */
      break;
    }
 
    if (!nak) {
      retcode = MSG_ID_APS_CXX_AUDIO_DETECT_ACK;
    } else {
      retcode = MSG_ID_APS_CXX_AUDIO_DETECT_NAK;
    }
    /* Send done message to supervisor */
    ret = send(&t, retcode, msgdata);
    ASSERT(ret == 0);
  } while (req != MSG_ID_APS_CXX_AUDIO_DETECT_EXIT);

  fini_task_resource(&t);

  return 0;
}

/*****************************************************************
 * Static Functions 
 *****************************************************************/
/** send message via MQ
 * - send id(uint8_t) and value(uint32_t)
 * - ARG = TaskResource 
 */
static int send(TaskResource *pt, uint8_t id, uint32_t value)
{
  int ret;
  ret = mpmq_send(&pt->mq, id, value);
  return ret;
}

/** receive message via MQ
 * - receive value(uint32_t) [ret = id(uint8_t)]
 * - ARG = TaskResource 
 */
static int receive(TaskResource *pt, uint32_t *pvalue)
{
  int ret;
  ret = mpmq_receive(&pt->mq, pvalue);
  return ret;
}

/** task_lock <-> Mutex
 * - mutex with TaskResource
 */
static void task_lock(TaskResource *pt)
{
  mpmutex_lock(&pt->mutex);
}

/** task_unlock <-> Mutex
 * - mutex with TaskResource
 */
static void task_unlock(TaskResource *pt)
{
  mpmutex_unlock(&pt->mutex);
}

/** init_task_resource
 * - ARG = TaskResource
 * - Init mutex, mq, shm 
 */
static int init_task_resource(TaskResource *pt)
{
  int ret;
  /* Initialize MP Mutex */
  ret = mpmutex_init(&pt->mutex, APS_CXX_AUDIO_DETECTKEY_MUTEX);
  if (ret != 0) {
    return -1;
  }

  /* Initialize MP message queue,
   * On the worker side, 3rd argument is ignored.
   */

  ret = mpmq_init(&pt->mq, APS_CXX_AUDIO_DETECTKEY_MQ, 0);
  if (ret != 0) {
    return -1;
  }

  /* Initialize MP shared memory */

  ret = mpshm_init(&pt->shm, APS_CXX_AUDIO_DETECTKEY_SHM, APS_CXX_AUDIO_DETECTKEY_SHM_SIZE);
  if (ret != 0) {
    return -1;
  }

  /* Map shared memory to virtual space */
  pt->buf = (char *)mpshm_attach(&pt->shm, 0);
  if (pt->buf == NULL) {
    return -1;
  }  
  return 0;
}

/** fini_task_resource
 * - ARG = TaskResource
 * - Fini shm
 */
static void fini_task_resource(TaskResource *pt)
{
  mpshm_detach(&pt->shm);
}

/** strcopy
 * - copy src -> dst
 */
static char *strcopy(char *dest, const char *src)
{
  char *d = dest;
  while (*src) *d++ = *src++;
  *d = '\0';

  return dest;
}