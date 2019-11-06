#include <sdk/config.h>
#include <stdio.h>
#include <string.h>

#include <asmp/asmp.h>
#include <asmp/mptask.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>
#include <asmp/mpmutex.h>

/** ------------------------
 * for Debug-Rings
 * ------------------------- */
/* Include worker header */
#include "aps_multicore.h"
#include "debugring.h"

/* Worker ELF path */
#define WORKER_FILE "/mnt/spif/aps_multicore"

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

#define message(format, ...)    printf(format, ##__VA_ARGS__)
#define err(format, ...)        fprintf(stderr, format, ##__VA_ARGS__)

int aps_multicore_main(int argc, char *argv[])
{
  mptask_t mptask;
  mpmutex_t mutex;
  mpshm_t shm;
  mpmq_t mq;
  uint32_t msgdata;
  int data = 0x1234;
  char putData[] = "GETDATA";
  int ret, wret;
  char *buf;

  /* Initialize MP task */

  ret = mptask_init(&mptask, WORKER_FILE);
  if (ret != 0)
    {
      err("mptask_init() failure. %d\n", ret);
      return ret;
    }

  ret = mptask_assign(&mptask);
  if (ret != 0)
    {
      err("mptask_assign() failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP mutex and bind it to MP task */

  ret = mpmutex_init(&mutex, APS_MULTICOREKEY_MUTEX);
  if (ret < 0)
    {
      err("mpmutex_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&mptask, &mutex);
  if (ret < 0)
    {
      err("mptask_bindobj(mutex) failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP message queue with assigned CPU ID, and bind it to MP task */

  ret = mpmq_init(&mq, APS_MULTICOREKEY_MQ, mptask_getcpuid(&mptask));
  if (ret < 0)
    {
      err("mpmq_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&mptask, &mq);
  if (ret < 0)
    {
      err("mptask_bindobj(mq) failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP shared memory and bind it to MP task */

  ret = mpshm_init(&shm, APS_MULTICOREKEY_SHM, 1024);
  if (ret < 0)
    {
      err("mpshm_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&mptask, &shm);
  if (ret < 0)
    {
      err("mptask_binobj(shm) failure. %d\n", ret);
      return ret;
    }

  /* Map shared memory to virtual space */

  buf = mpshm_attach(&shm, 0);
  if (!buf)
    {
      err("mpshm_attach() failure.\n");
      return ret;
    }
  message("attached at %08x\n", (uintptr_t)buf);
  memset(buf, 0, 1024);
  init_debugring((((void*)buf) + (64*1024)), &mutex, DQUEUE_MASTER);

  /* Run worker */
  ret = mptask_exec(&mptask);
  if (ret < 0)
    {
      err("mptask_exec() failure. %d\n", ret);
      return ret;
    }

  
  int large_count;
  for (large_count = 0; large_count < 10; large_count++) {
    int loop;
    for (loop = 0; loop < 10; loop++) {
      /* Send command to worker */
      data = QUEUEITEM_ID_REQ_ENQUEUE;
      ret = mpmq_send(&mq, MSG_ID_APS_MULTICORE, (uint32_t)data);
      if (ret < 0)
        {
          err("mpmq_send() failure. %d\n", ret);
          return ret;
        }

      /* Wait for worker message */
      ret = mpmq_receive(&mq, &msgdata);
      if (ret < 0)
        {
          err("mpmq_recieve() failure. %d\n", ret);
          return ret;
        }
      message("Worker response: ID = %d\n", ret);
      /* Lock mutex for synchronize with worker after it's started */
      mpmutex_lock(&mutex);
      message("Worker said: %s\n", buf);
      mpmutex_unlock(&mutex);
    }

    for (loop = 0; loop < 10; loop++) {
      sDebugRingItem getItem;
      int ret;
      char buf[64];
      sDebugRing *pDRing = get_debugring_addr();
      ret = dequeue_debugring(&getItem, buf, 64);
      message("Dequeue on Main<RET=%d|H=%d|T=%d> // cpu#%d, data=0x%x, %s\n",
        ret, pDRing->header.head, pDRing->header.tail,
        getItem.cpuid, getItem.param, buf);
    }

    for (loop = 0; loop < 10; loop++) {
      sDebugRingItem getItem;
      int ret;
      char buf[64];
      sDebugRing *pDRing = get_debugring_addr();
      ret = enqueue_debugring(loop, putData, 8);
      message("Dequeue on Main<RET=%d|H=%d|T=%d> from MainCore\n",
        ret, pDRing->header.head, pDRing->header.tail);
    }

    for (loop = 0; loop < 10; loop++) {
      /* Send command to worker */
      data = QUEUEITEM_ID_REQ_DEQUEUE;
      ret = mpmq_send(&mq, MSG_ID_APS_MULTICORE, (uint32_t)data);
      if (ret < 0)
        {
          err("mpmq_send() failure. %d\n", ret);
          return ret;
        }

      /* Wait for worker message */
      ret = mpmq_receive(&mq, &msgdata);
      if (ret < 0)
        {
          err("mpmq_recieve() failure. %d\n", ret);
          return ret;
        }
      message("Worker response: ID = %d\n", ret);
      /* Lock mutex for synchronize with worker after it's started */
      mpmutex_lock(&mutex);
      message("Worker said: %s\n", buf);
      mpmutex_unlock(&mutex);
    }

    {
      sDebugRingItem getItem;
      sDebugRing *pDRing = get_debugring_addr();
      message("Queue Status on Main<H=%d|T=%d> from MainCore\n",
        pDRing->header.head, pDRing->header.tail);
    }
  }
  {
    int loop;
    for (loop = 0; loop < 1; loop++) {
      /* Send command to worker */
      data = QUEUEITEM_ID_REQ_QUIT;
      ret = mpmq_send(&mq, MSG_ID_APS_MULTICORE, (uint32_t)data);
      if (ret < 0)
        {
          err("mpmq_send() failure. %d\n", ret);
          return ret;
        }

      /* Wait for worker message */
      ret = mpmq_receive(&mq, &msgdata);
      if (ret < 0)
        {
          err("mpmq_recieve() failure. %d\n", ret);
          return ret;
        }
      message("Worker response: ID = %d\n", ret);
      /* Lock mutex for synchronize with worker after it's started */
      mpmutex_lock(&mutex);
      message("Worker said: %s\n", buf);
      mpmutex_unlock(&mutex);
    }
  }
  

  /* Destroy worker */
  wret = -1;
  ret = mptask_destroy(&mptask, false, &wret);
  if (ret < 0)
    {
      err("mptask_destroy() failure. %d\n", ret);
      return ret;
    }

  message("Worker exit status = %d\n", wret);

  /* Finalize all of MP objects */

  mpshm_detach(&shm);
  mpshm_destroy(&shm);
  mpmutex_destroy(&mutex);
  mpmq_destroy(&mq);

  return 0;
}
