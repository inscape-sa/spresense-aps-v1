#include <errno.h>

#include <asmp/types.h>
#include <asmp/mpshm.h>
#include <asmp/mpmutex.h>
#include <asmp/mpmq.h>

#include "asmp.h"
#include "include/aps_camera_asmp.h"
#include "include/config_image_asmp.h"

#define ASSERT(cond) if (!(cond)) wk_abort()

static const char helloworld[] = "Hello, ASMP World!";

static char *strcopy(char *dest, const char *src)
{
  char *d = dest;
  while (*src) *d++ = *src++;
  *d = '\0';

  return dest;
}

int main(void)
{
  mpmutex_t mutex;
  mpshm_t shm;
  mpmq_t mq;
  uint32_t msgdata;
  char *buf;
  int msgid;
  int ret;

  /* Initialize MP Mutex */
  ret = mpmutex_init(&mutex, APS00_SANDBOX_APS_CAMERA_ASMPKEY_MUTEX);
  ASSERT(ret == 0);

  /* Initialize MP message queue,
   * On the worker side, 3rd argument is ignored.
   */

  ret = mpmq_init(&mq, APS00_SANDBOX_APS_CAMERA_ASMPKEY_MQ, 0);
  ASSERT(ret == 0);

  /* Initialize MP shared memory */
  ret = mpshm_init(&shm, APS00_SANDBOX_APS_CAMERA_ASMPKEY_SHM, SHMSIZE_IMAGE_YUV_SIZE);
  ASSERT(ret == 0);

  /* Map shared memory to virtual space */
  buf = (char *)mpshm_attach(&shm, 0);
  ASSERT(buf);

  /* Receive message from supervisor */
  while (1) 
    {
    msgid = mpmq_receive(&mq, &msgdata);
    if (msgid == MSG_ID_APS00_SANDBOX_APS_CAMERA_ASMP)
      {
        /* Copy hello message to shared memory */
        mpmutex_lock(&mutex);
        strcopy(buf, helloworld);
        mpmutex_unlock(&mutex);

        apsamp_main_yuv2rgb((void *)msgdata, SHMSIZE_IMAGE_YUV_SIZE);

        /* Send done message to supervisor */
        ret = mpmq_send(&mq, msgid, msgdata);
        ASSERT(ret == 0);
      }
    else if (msgid == MSG_ID_APS00_SANDBOX_APS_CAMERA_EXIT)
      {
        /* Send done message to supervisor */
        ret = mpmq_send(&mq, msgid, 0xcafec001);
        ASSERT(ret == 0);
        break;
      }
    else
      {
        ASSERT(msgid);
      }
    }

  /* Free virtual space */
  mpshm_detach(&shm);
  return 0;
}

