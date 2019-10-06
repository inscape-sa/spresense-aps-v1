#include <sdk/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <debug.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>

#include <asmp/asmp.h>
#include <asmp/mptask.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>
#include <asmp/mpmutex.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/fs/mkfatfs.h>
#include "video/video.h"

#include <sys/ioctl.h>
#include <sys/boardctl.h>
#include <sys/mount.h>

#include <arch/chip/pm.h>
#include <arch/board/board.h>
#include <arch/chip/cisif.h>

#include "nximage.h"
#include "config_image.h"
#include "config_basic.h"
#include "color_proc_maincore.h"
#include "color_camera_ctrl.h"

#include "aps_camera_asmp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MAX_TASKS (1)
/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct apsamp_task_s {
  mptask_t mptask[MAX_TASKS];
  mpmutex_t mutex[MAX_TASKS];
  mpmq_t mq[MAX_TASKS];
  mpshm_t shm[MAX_TASKS];
  char *buf[MAX_TASKS];
  int cpuid[MAX_TASKS];
  int wret[MAX_TASKS];
} apsamp_task;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int apsamp_send(mpmq_t *p_mq, int id, uint32_t send);
static int apsamp_receive(mpmq_t *p_mq, uint32_t *p_recv);
static int apsamp_task_init_and_exec(apsamp_task *ptaskset, int taskid);
static void apsamp_task_fini(apsamp_task *ptaskset, int taskid);

/****************************************************************************
 * main
 ****************************************************************************/
int aps_camera_asmp_main(int argc, char *argv[])
{
  uint32_t recv;
  int ret;

  int v_fd;
  int loop;
  int taskid = 0;
  
  struct v4l2_buffer buf;
  apsamp_task taskset;

  ret = apsamp_camera_init(&v_fd);
  if (ret)
    {
      printf("camera_main: Failed at init\n");
      return ERROR;
    }

  for (taskid = 0; taskid < MAX_TASKS; taskid++)
    {
      apsamp_task_init_and_exec(&taskset, taskid);
    } 
  
  taskid = 0;
  for (loop = 0; loop < DEFAULT_REPEAT_NUM; loop++)
    {
      if ((ret = apsamp_capdata_lock(v_fd, &buf) != 0))
        {
          printf("Fail DQBUF %d\n", errno);
          return ERROR;
        }

      /* Convert YUV color format to RGB565 */
      printf("|%02d| BUF -> %p\n", loop, buf.m.userptr);
      //apsamp_main_yuv2rgb((void *)buf.m.userptr, buf.bytesused);

      /* communication ASMP sub core*/
      apsamp_send(&taskset.mq[taskid], MSG_ID_APS00_SANDBOX_APS_CAMERA_ASMP, (uint32_t)buf.m.userptr);
      apsamp_receive(&taskset.mq[taskid], &recv);
      /* Lock mutex for synchronize with worker after it's started */
      mpmutex_lock(&taskset.mutex[taskid]);
      message("|--|Worker said: %s\n", taskset.buf[taskid]);
      mpmutex_unlock(&taskset.mutex[taskid]);

      apsamp_nximage_image(g_nximage_aps_asmp.hbkgd, (void *)buf.m.userptr);

      if ((ret = apsamp_capdata_release(v_fd, &buf)) != 0)
        {
          printf("Fail QBUF %d\n", errno);
          return ERROR;
        }
    }

  for (taskid = 0; taskid < MAX_TASKS; taskid++)
    { 
      /* Show worker copied data */
      /* Send command to worker */
      apsamp_send(&taskset.mq[taskid], MSG_ID_APS00_SANDBOX_APS_CAMERA_EXIT, 0xdeadbeef);
      apsamp_receive(&taskset.mq[taskid], &recv);
      apsamp_task_fini(&taskset, taskid);
    }

  /* Finalize all of MP objects */
  apsamp_camera_fini(&v_fd);

  return 0;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/***************************************************************
 * Task Messaging API
 ***************************************************************/
static int apsamp_send(mpmq_t *p_mq, int id, uint32_t send)
{
  int ret;

  message("camera_main_loop: send = ID %d, data=%08x\n", id, send);
  /* Send command to worker */
  ret = mpmq_send(p_mq, id, send);
  if (ret < 0)
    {
      err("mpmq_send() failure. %d\n", ret);
      return ret;
    }
  return OK;
}

static int apsamp_receive(mpmq_t *p_mq, uint32_t *p_recv)
{
  int id;
  id = mpmq_receive(p_mq, p_recv);
  if (id < 0)
    {
      err("mpmq_recieve() failure. %d\n", id);
      return id;
    }
  message("Worker response: ID = %d, data = %08x\n", id, *p_recv); 
  return OK;    
}

/***************************************************************
 * Task Generating API
 ***************************************************************/
static int apsamp_task_init_and_exec(apsamp_task *ptaskset, int taskid)
{
  int ret;

  /* clear Worker Return code */
  ptaskset->wret[taskid] = -1;

  ret = mptask_init(&ptaskset->mptask[taskid], WORKER_FILE);
  if (ret != 0)
    {
      err("mptask_init() failure. %d\n", ret);
      return ret;
    }

  ret = mptask_assign(&ptaskset->mptask[taskid]);
  if (ret != 0)
    {
      err("mptask_assign() failure. %d\n", ret);
      return ret;
    }

  /* taskid */
  ptaskset->cpuid[taskid] = mptask_getcpuid(&ptaskset->mptask[taskid]);
  message("attached at CPU-%d\n", ptaskset->cpuid[taskid]);

  /* Initialize MP mutex and bind it to MP task */
  ret = mpmutex_init(&ptaskset->mutex[taskid], APS00_SANDBOX_APS_CAMERA_ASMPKEY_MUTEX);
  if (ret < 0)
    {
      err("mpmutex_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&ptaskset->mptask[taskid], &ptaskset->mutex[taskid]);
  if (ret < 0)
    {
      err("mptask_bindobj(mutex) failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP message queue with assigned CPU ID, and bind it to MP task */

  ret = mpmq_init(&ptaskset->mq[taskid], APS00_SANDBOX_APS_CAMERA_ASMPKEY_MQ, mptask_getcpuid(&ptaskset->mptask[taskid]));
  if (ret < 0)
    {
      err("mpmq_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&ptaskset->mptask[taskid], &ptaskset->mq[taskid]);
  if (ret < 0)
    {
      err("mptask_bindobj(mq) failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP shared memory and bind it to MP task */
  ret = mpshm_init(&ptaskset->shm[taskid], APS00_SANDBOX_APS_CAMERA_ASMPKEY_SHM, SHMSIZE_IMAGE_YUV_SIZE);
  if (ret < 0)
    {
      err("mpshm_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&ptaskset->mptask[taskid], &ptaskset->shm[taskid]);
  if (ret < 0)
    {
      err("mptask_binobj(shm) failure. %d\n", ret);
      return ret;
    }

  /* Map shared memory to virtual space */
  ptaskset->buf[taskid] = mpshm_attach(&ptaskset->shm[taskid], 0);
  if (!ptaskset->buf[taskid])
    {
      err("mpshm_attach() failure.\n");
      return ret;
    }
  message("attached at %08x\n", (uintptr_t)ptaskset->buf[taskid]);
  memset(ptaskset->buf[taskid], 0, SHMSIZE_IMAGE_YUV_SIZE);

  /* Run worker */
  ret = mptask_exec(&ptaskset->mptask[taskid]);
  if (ret < 0)
    {
      err("mptask_exec() failure. %d\n", ret);
      return ret;
    }

  return OK;
}

static void apsamp_task_fini(apsamp_task *ptaskset, int taskid)
{
  int ret, wret;

  /* Destroy worker */
  wret = -1;
  ret = mptask_destroy(&ptaskset->mptask[taskid], false, &wret);
  if (ret < 0)
    {
      err("mptask_destroy() failure. %d\n", ret);
      return;
    }
  message("Worker exit status = %d\n", wret);
  ptaskset->wret[taskid] = wret;
  mpshm_detach(&ptaskset->shm[taskid]);
  mpshm_destroy(&ptaskset->shm[taskid]);
  
    {
      mpmutex_destroy(&ptaskset->mutex[taskid]);
      mpmq_destroy(&ptaskset->mq[taskid]);
    }

  return;
}