#include <errno.h>

#include <asmp/types.h>
#include <asmp/mpshm.h>
#include <asmp/mpmutex.h>
#include <asmp/mpmq.h>

#include "asmp.h"
#include "include/aps_multicore.h"
#include "include/debugring.h"

#define ASSERT(cond) if (!(cond)) wk_abort()

static const char msgEnqueue[] = "Enqueue on ASMP World!";
static const char msgDequeue[] = "Dequeue on ASMP World!";
static const char msgGoodbye[] = "Goodbye, ASMP World!";
static const char msgNop[] = "  -> NOP in ASMP World :p";

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
  int ret;

  char sendbuf[] = "ASMP SEND";
  char recvbuf[64];

  /* Initialize MP Mutex */

  ret = mpmutex_init(&mutex, APS_MULTICOREKEY_MUTEX);
  ASSERT(ret == 0);

  /* Initialize MP message queue,
   * On the worker side, 3rd argument is ignored.
   */

  ret = mpmq_init(&mq, APS_MULTICOREKEY_MQ, 0);
  ASSERT(ret == 0);

  /* Initialize MP shared memory */

  ret = mpshm_init(&shm, APS_MULTICOREKEY_SHM, 1024);
  ASSERT(ret == 0);

  /* Map shared memory to virtual space */

  buf = (char *)mpshm_attach(&shm, 0);
  ASSERT(buf);

  init_debugring(((void*)buf + (64 * 1024)), &mutex, DQUEUE_SLAVE);

  /* Receive message from supervisor */
  while(1) {
    ret = mpmq_receive(&mq, &msgdata);
    ASSERT(ret == MSG_ID_APS_MULTICORE);

    if (msgdata == QUEUEITEM_ID_REQ_ENQUEUE) {
      /* Copy hello message to shared memory */
      enqueue_debugring(0x1234, sendbuf, 10);
      mpmutex_lock(&mutex);
      strcopy(buf, msgEnqueue);
      mpmutex_unlock(&mutex);
      ret = mpmq_send(&mq, MSG_ID_APS_MULTICORE, (unsigned int)&msgdata);
      ASSERT(ret == 0); 
    } else if (msgdata == QUEUEITEM_ID_REQ_DEQUEUE) {
      sDebugRingItem getItem;
      /* Copy hello message to shared memory */
      ret = dequeue_debugring(&getItem, recvbuf, 64);
      /* Copy hello message to shared memory */
      mpmutex_lock(&mutex);
      if (ret < 0) {
        /* Empty*/
        buf[0] = '\0';
      } else {
        /* Get Item */
        strcopy(buf, recvbuf);
      }
      mpmutex_unlock(&mutex);
      msgdata = 0x2345;
      ret = mpmq_send(&mq, MSG_ID_APS_MULTICORE, msgdata);
      ASSERT(ret == 0); 
    } else if (msgdata == QUEUEITEM_ID_REQ_QUIT) {    
      /* Copy hello message to shared memory */
      mpmutex_lock(&mutex);
      strcopy(buf, msgGoodbye);
      mpmutex_unlock(&mutex);  
      msgdata = 0x2345;
      ret = mpmq_send(&mq, MSG_ID_APS_MULTICORE, (unsigned int)&msgdata);
      ASSERT(ret == 0); 
      break;
    } else {
      mpmutex_lock(&mutex);
      strcopy(buf, msgNop);
      buf[0] = 'A' + (msgdata - 0xA);
      mpmutex_unlock(&mutex);  
      msgdata = 0x3456;
      ret = mpmq_send(&mq, MSG_ID_APS_MULTICORE, &msgdata);
      ASSERT(ret == 0);
      /* NOP */
    }
  }
  mpshm_detach(&shm);
  /* Send done message to supervisor */

  return 0;
}

