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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct apsamp_task_s {
  mptask_t mptask;
  mpmutex_t mutex;
  mpshm_t shm_yuv;
  mpshm_t shm_rgb;
  mpmq_t mq;

  char *buf_yuv;
  char *buf_rgb;
} apsamp_task;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int apsamp_send(mpmq_t *p_mq, int id, uint32_t *p_send);
static int apsamp_receive(mpmq_t *p_mq, uint32_t *p_recv);
static int apsamp_task_init(apsamp_task *ptaskset);
static void apsamp_task_fini(apsamp_task *ptaskset);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int camera_main_loop(int v_fd)
{
  int ret;
  struct v4l2_buffer buf;
  int loop;

  for (loop = 0; loop < DEFAULT_REPEAT_NUM; loop++)
    {
      if (ret = apsamp_capdata_lock(v_fd, &buf))
        {
          printf("Fail DQBUF %d\n", errno);
          return ret;
        }

      /* Convert YUV color format to RGB565 */
      apsamp_main_yuv2rgb((void *)buf.m.userptr, buf.bytesused);
      printf("|%d| BUF -> %p\n", loop, buf.m.userptr);
      apsamp_nximage_image(g_nximage_aps_asmp.hbkgd, (void *)buf.m.userptr);

      if (ret = apsamp_capdata_release(v_fd, &buf))
        {
          printf("Fail QBUF %d\n", errno);
          return ret;
        }
    }
  return 0;
}

int aps_camera_asmp_main(int argc, char *argv[])
{
  uint32_t send;
  uint32_t recv;
  int ret, wret;

  int v_fd;
  int loop;
  
  apsamp_task taskset;

  ret = apsamp_camera_init(&v_fd);
  if (ret)
    {
      printf("camera_main: Failed at init\n");
      return ERROR;
    }
  apsamp_task_init(&taskset);

  /* Run worker */
  ret = mptask_exec(&taskset.mptask);
  if (ret < 0)
    {
      err("mptask_exec() failure. %d\n", ret);
      return ret;
    }

  ret = camera_main_loop(v_fd);
  if (ret) {
      printf("camera_main_loop: Failed at init\n");
      return ERROR;
    }
  for (loop = 0; loop < 10; loop++)
    {
      apsamp_send(&taskset.mq, MSG_ID_APS00_SANDBOX_APS_CAMERA_ASMP, &send);
      apsamp_receive(&taskset.mq, &recv);
    }
  
  /* Lock mutex for synchronize with worker after it's started */
  mpmutex_lock(&taskset.mutex);
  message("Worker said: %s\n", taskset.buf_yuv);
  mpmutex_unlock(&taskset.mutex);
 
  /* Show worker copied data */
  /* Send command to worker */
  apsamp_send(&taskset.mq, MSG_ID_APS00_SANDBOX_APS_CAMERA_EXIT, &send);
  apsamp_receive(&taskset.mq, &recv);

  /* Destroy worker */
  wret = -1;
  ret = mptask_destroy(&taskset.mptask, false, &wret);
  if (ret < 0)
    {
      err("mptask_destroy() failure. %d\n", ret);
      return ret;
    }

  message("Worker exit status = %d\n", wret);

  /* Finalize all of MP objects */
  apsamp_camera_fini(&v_fd);

  apsamp_task_fini(&taskset);
  return 0;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/***************************************************************/
/* Task Messaging API
/***************************************************************/
static int apsamp_send(mpmq_t *p_mq, int id, uint32_t *p_send)
{
  int ret;

  message("camera_main_loop: send = ID %d, data=%x\n", id, *p_send);
  /* Send command to worker */
  ret = mpmq_send(p_mq, id, (uint32_t) &p_send);
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
  message("Worker response: ID = %d, data = %x\n", id, *p_recv); 
  return OK;    
}

/***************************************************************/
/* Task Generating API
/***************************************************************/
static int apsamp_task_init(apsamp_task *ptaskset)
{
  int ret;
  
/* Initialize MP task */
  ret = mptask_init(&ptaskset->mptask, WORKER_FILE);
  if (ret != 0)
    {
      err("mptask_init() failure. %d\n", ret);
      return ret;
    }

  ret = mptask_assign(&ptaskset->mptask);
  if (ret != 0)
    {
      err("mptask_assign() failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP mutex and bind it to MP task */

  ret = mpmutex_init(&ptaskset->mutex, APS00_SANDBOX_APS_CAMERA_ASMPKEY_MUTEX);
  if (ret < 0)
    {
      err("mpmutex_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&ptaskset->mptask, &ptaskset->mutex);
  if (ret < 0)
    {
      err("mptask_bindobj(mutex) failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP message queue with assigned CPU ID, and bind it to MP task */

  ret = mpmq_init(&ptaskset->mq, APS00_SANDBOX_APS_CAMERA_ASMPKEY_MQ, mptask_getcpuid(&ptaskset->mptask));
  if (ret < 0)
    {
      err("mpmq_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&ptaskset->mptask, &ptaskset->mq);
  if (ret < 0)
    {
      err("mptask_bindobj(mq) failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP shared memory and bind it to MP task */
  ret = mpshm_init(&ptaskset->shm_yuv, APS00_SANDBOX_APS_CAMERA_ASMPKEY_SHM_YUV, SHMSIZE_IMAGE_YUV_SIZE);
  if (ret < 0)
    {
      err("mpshm_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&ptaskset->mptask, &ptaskset->shm_yuv);
  if (ret < 0)
    {
      err("mptask_binobj(shm) failure. %d\n", ret);
      return ret;
    }

  /* Map shared memory to virtual space */
  ptaskset->buf_yuv = mpshm_attach(&ptaskset->shm_yuv, 0);
  if (!ptaskset->buf_yuv)
    {
      err("mpshm_attach() failure.\n");
      return ret;
    }
  message("attached at %08x\n", (uintptr_t)ptaskset->buf_yuv);
  memset(ptaskset->buf_yuv, 0, SHMSIZE_IMAGE_YUV_SIZE);

  ret = mpshm_init(&ptaskset->shm_rgb, APS00_SANDBOX_APS_CAMERA_ASMPKEY_SHM_RGB, SHMSIZE_IMAGE_RGB_SIZE);
  if (ret < 0)
    {
      err("mpshm_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&ptaskset->mptask, &ptaskset->shm_rgb);
  if (ret < 0)
    {
      err("mptask_binobj(shm) failure. %d\n", ret);
      return ret;
    }

  /* Map shared memory to virtual space */
  ptaskset->buf_rgb = mpshm_attach(&ptaskset->shm_rgb, 0);
  if (!ptaskset->buf_rgb)
    {
      err("mpshm_attach() failure.\n");
      return ret;
    }
  message("attached at %08x\n", (uintptr_t)ptaskset->buf_rgb);
  memset(ptaskset->buf_rgb, 0, SHMSIZE_IMAGE_RGB_SIZE);

  return OK;
}

static void apsamp_task_fini(apsamp_task *ptaskset)
{
  mpshm_detach(&ptaskset->shm_yuv);
  mpshm_destroy(&ptaskset->shm_yuv);
  mpshm_detach(&ptaskset->shm_rgb);
  mpshm_destroy(&ptaskset->shm_rgb);
  mpmutex_destroy(&ptaskset->mutex);
  mpmq_destroy(&ptaskset->mq);
  return;
}