#include <sdk/config.h>
#include <stdio.h>
#include <string.h>

/* Include worker header */
#include "aps_template_asmp.h"
#include "worker_ctrl.h"

/* Worker ELF path */
#define WORKER_FILE "/mnt/spif/aps_template_asmp"

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

#define message(format, ...)    printf(format, ##__VA_ARGS__)
#define err(format, ...)        fprintf(stderr, format, ##__VA_ARGS__)

/** Start Programs */
extern "C" int aps_template_asmp_main(int argc, char *argv[])
{
  WorkerCtrl *pwc;
  uint32_t msgdata;
  int data = 0x1234;
  int ret;
  void *buf;

  pwc = new WorkerCtrl(WORKER_FILE);
  /* Initialize MP mutex and bind it to MP task */
  if (!pwc->getReady()) {
    err("WorkerCtrl() constructor failure.\n");
    return -1;
  }

  ret = pwc->initMutex(APS_TEMPLATE_ASMPKEY_MUTEX);
  if (ret < 0) {
    err("initMutex(mutex) failure. %d\n", ret);
    return ret;
  }

  ret = pwc->initMq(APS_TEMPLATE_ASMPKEY_MQ);
  if (ret < 0) {
    err("initMq(shm) failure. %d\n", ret);
    return ret;
  }

  buf = pwc->initShm(APS_TEMPLATE_ASMPKEY_SHM, APS_TEMPLATE_ASMPKEY_SHM_SIZE);
  if (buf == NULL) {
    err("initShm() failure. %d\n", ret);
    return ret;
  }
  message("attached at %08x\n", (uintptr_t)buf);

  ret = pwc->execTask();
  if (ret < 0) {
    err("execTask() failure. %d\n", ret);
    return ret;
  }

  for (int loop = 0; loop < 10; loop++) {
    /* Send command to worker */
    ret = pwc->send((uint8_t)MSG_ID_APS_TEMPLATE_ASMP_ACT, (uint32_t) &data);
    if (ret < 0) {
      err("send() failure. %d\n", ret);
      return ret;
    }

    /* Wait for worker message */
    ret = pwc->receive((uint32_t *)&msgdata);
    if (ret < 0) {
      err("recieve() failure. %d\n", ret);
      return ret;
    }
    message("Worker response: ID = %d, data = %x\n",
            ret, *((int *)msgdata));
              
    /* Lock mutex for synchronize with worker after it's started */
    pwc->lock();
    message("Worker said: %s\n", buf);
    pwc->unlock();
  }

  /* Send command to worker */
  ret = pwc->send((uint8_t)MSG_ID_APS_TEMPLATE_ASMP_EXIT, (uint32_t) &data);
  if (ret < 0) {
    err("send() failure. %d\n", ret);
    return ret;
  }

  /* Wait for worker message */
  ret = pwc->receive((uint32_t *)&msgdata);
  if (ret < 0) {
    err("recieve() failure. %d\n", ret);
    return ret;
  }
  message("Worker response: ID = %d, data = %x\n",
          ret, *((int *)msgdata));

  
  /* Destroy worker */
  ret = pwc->destroyTask();
  if (ret < 0) {
    err("destroyTask() failure. %d\n", ret);
    return ret;
  }
  message("Worker exit status = %d\n", ret);

  /* Finalize all of MP objects */
  delete pwc;
  return 0;
}
