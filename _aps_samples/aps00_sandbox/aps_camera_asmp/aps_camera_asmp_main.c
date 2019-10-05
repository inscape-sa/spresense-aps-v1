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

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct v_buffer {
  uint32_t             *start;
  uint32_t             length;
} v_buffer_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static v_buffer_t *buffers_video;
static unsigned int n_buffers;

struct nximage_data_s g_nximage_aps_asmp =
{
  NULL,          /* hnx */
  NULL,          /* hbkgd */
  0,             /* xres */
  0,             /* yres */
  false,         /* havpos */
  { 0 },         /* sem */
  0              /* exit code */
};

static int camera_prepare(int                fd,
                          enum v4l2_buf_type type,
                          uint32_t           buf_mode,
                          uint32_t           pixformat,
                          uint16_t           hsize,
                          uint16_t           vsize,
                          uint8_t            buffernum)
{
  int ret;
  int cnt;
  uint32_t fsize;
  struct v4l2_format         fmt = {0};
  struct v4l2_requestbuffers req = {0};
  struct v4l2_buffer         buf = {0};
  v_buffer_t *buffers;

  /* VIDIOC_REQBUFS initiate user pointer I/O */

  req.type   = type;
  req.memory = V4L2_MEMORY_USERPTR;
  req.count  = buffernum;
  req.mode   = buf_mode;
  
  ret = ioctl(fd, VIDIOC_REQBUFS, (unsigned long)&req);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_REQBUFS: errno = %d\n", errno);
      return ret;
    }

  /* VIDIOC_S_FMT set format */
  fmt.type                = type;
  fmt.fmt.pix.width       = hsize;
  fmt.fmt.pix.height      = vsize;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = pixformat;

  ret = ioctl(fd, VIDIOC_S_FMT, (unsigned long)&fmt);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_S_FMT: errno = %d\n", errno);
      return ret;
    }
    buffers_video = malloc(sizeof(v_buffer_t) * buffernum);
    buffers = buffers_video;

  if (!buffers)
    {
      printf("Out of memory\n");
      return ret;
    }
  fsize = IMAGE_YUV_SIZE;
  for (n_buffers = 0; n_buffers < buffernum; ++n_buffers)
    {
      buffers[n_buffers].length = fsize;

      /* Note: VIDIOC_QBUF set buffer pointer. */
      /*       Buffer pointer must be 32bytes aligned. */

      buffers[n_buffers].start  = memalign(32, fsize);
      if (!buffers[n_buffers].start)
        {
          printf("Out of memory\n");
          return ret;
        }
    }

  for (cnt = 0; cnt < n_buffers; cnt++)
    {
      memset(&buf, 0, sizeof(v4l2_buffer_t));
      buf.type = type;
      buf.memory = V4L2_MEMORY_USERPTR;
      buf.index = cnt;
      buf.m.userptr = (unsigned long)buffers[cnt].start;
      buf.length = buffers[cnt].length;

      ret = ioctl(fd, VIDIOC_QBUF, (unsigned long)&buf);
      if (ret)
        {
          printf("Fail QBUF %d\n", errno);
          return ret;;
        }
    }

  /* VIDIOC_STREAMON start stream */
  ret = ioctl(fd, VIDIOC_STREAMON, (unsigned long)&type);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_STREAMON: errno = %d\n", errno);
      return ret;
    }

  return OK;
}

static void free_buffer(v_buffer_t *buffers, uint8_t bufnum)
{
  uint8_t cnt;
  if (buffers)
    {
      for (cnt = 0; cnt < bufnum; cnt++)
        {
          if (buffers[cnt].start)
            {
              free(buffers[cnt].start);
            }
        }

      free(buffers);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static int apsamp_capdata_lock(int v_fd, struct v4l2_buffer *p_buf)
{
  int ret;
  /* Note: VIDIOC_DQBUF acquire capture data. */
  memset(p_buf, 0, sizeof(v4l2_buffer_t));
  p_buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  p_buf->memory = V4L2_MEMORY_USERPTR;
  ret = ioctl(v_fd, VIDIOC_DQBUF, (unsigned long)p_buf);
  return ret;
}

static int apsamp_capdata_release(int v_fd, struct v4l2_buffer *p_buf)
{
  int ret;
  /* Note: VIDIOC_QBUF reset released buffer pointer. */
  ret = ioctl(v_fd, VIDIOC_QBUF, (unsigned long)p_buf);
  return ret;
}

static int camera_main_loop(int v_fd)
{
  int ret;
  struct v4l2_buffer buf;
  uint32_t loop = DEFAULT_REPEAT_NUM;
  while (loop-- > 0)
    {
      ret = apsamp_capdata_lock(v_fd, &buf);
      if (ret)
        {
          printf("Fail DQBUF %d\n", errno);
          return ret;
        }

      /* Convert YUV color format to RGB565 */
      apsamp_main_yuv2rgb((void *)buf.m.userptr, buf.bytesused);
      printf("|%d| BUF -> %p\n", loop, buf.m.userptr);
      apsamp_nximage_image(g_nximage_aps_asmp.hbkgd, (void *)buf.m.userptr);

      ret = apsamp_capdata_release(v_fd, &buf);
      if (ret)
        {
          printf("Fail QBUF %d\n", errno);
          return ret;
        }
    }
  return 0;
}

static int apsamp_camera_init(int *p_v_fd)
{
  int ret;
  int v_fd;

  /* select capture mode */
  ret = apsamp_nximage_initialize();
  if (ret < 0)
    {
      printf("camera_main: Failed to get NX handle: %d\n", errno);
      return ERROR;
    }

  ret = video_initialize("/dev/video");
  if (ret != 0)
    {
      printf("ERROR: Failed to initialize video: errno = %d\n", errno);
      goto errout_with_nx;
    }

  v_fd = open("/dev/video", 0);
  if (v_fd < 0)
    {
      printf("ERROR: Failed to open video.errno = %d\n", errno);
      goto errout_with_video_init;
    }

  /* Prepare VIDEO_CAPTURE */
  ret = camera_prepare(v_fd,
                       V4L2_BUF_TYPE_VIDEO_CAPTURE,
                       V4L2_BUF_MODE_RING,
                       V4L2_PIX_FMT_UYVY,
                       VIDEO_HSIZE_QVGA,
                       VIDEO_VSIZE_QVGA,
                       VIDEO_BUFNUM);
  if (ret < 0)
    {
      goto errout_with_buffer;
    }
  
  *p_v_fd = v_fd;
  return OK;

errout_with_buffer:
  close(v_fd);
  free_buffer(buffers_video, VIDEO_BUFNUM);

errout_with_video_init:
  video_uninitialize();

errout_with_nx:
  nx_close(g_nximage_aps_asmp.hnx);
  return ERROR;
}

static void apsamp_camera_fini(int *p_v_fd)
{
  close(*p_v_fd);
  free_buffer(buffers_video, VIDEO_BUFNUM);
  video_uninitialize();
  nx_close(g_nximage_aps_asmp.hnx);
}


int aps_camera_asmp_main(int argc, char *argv[])
{
  mptask_t mptask;
  mpmutex_t mutex;
  mpshm_t shm_yuv;
  mpshm_t shm_rgb;
  mpmq_t mq;
  uint32_t msgdata;
  int data = 0x1234;
  int ret, wret;
  char *buf_yuv;
  char *buf_rgb;
  int v_fd;
  int loop;
  
  ret = apsamp_camera_init(&v_fd);
  if (ret)
    {
      printf("camera_main: Failed at init\n");
      return ERROR;
    }

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

  ret = mpmutex_init(&mutex, APS00_SANDBOX_APS_CAMERA_ASMPKEY_MUTEX);
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

  ret = mpmq_init(&mq, APS00_SANDBOX_APS_CAMERA_ASMPKEY_MQ, mptask_getcpuid(&mptask));
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
  ret = mpshm_init(&shm_yuv, APS00_SANDBOX_APS_CAMERA_ASMPKEY_SHM_YUV, SHMSIZE_IMAGE_YUV_SIZE);
  if (ret < 0)
    {
      err("mpshm_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&mptask, &shm_yuv);
  if (ret < 0)
    {
      err("mptask_binobj(shm) failure. %d\n", ret);
      return ret;
    }

  /* Map shared memory to virtual space */
  buf_yuv = mpshm_attach(&shm_yuv, 0);
  if (!buf_yuv)
    {
      err("mpshm_attach() failure.\n");
      return ret;
    }
  message("attached at %08x\n", (uintptr_t)buf_yuv);
  memset(buf_yuv, 0, SHMSIZE_IMAGE_YUV_SIZE);

  ret = mpshm_init(&shm_rgb, APS00_SANDBOX_APS_CAMERA_ASMPKEY_SHM_RGB, SHMSIZE_IMAGE_RGB_SIZE);
  if (ret < 0)
    {
      err("mpshm_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&mptask, &shm_rgb);
  if (ret < 0)
    {
      err("mptask_binobj(shm) failure. %d\n", ret);
      return ret;
    }

  /* Map shared memory to virtual space */
  buf_rgb = mpshm_attach(&shm_rgb, 0);
  if (!buf_rgb)
    {
      err("mpshm_attach() failure.\n");
      return ret;
    }
  message("attached at %08x\n", (uintptr_t)buf_rgb);
  memset(buf_rgb, 0, SHMSIZE_IMAGE_RGB_SIZE);

  /* Run worker */
  ret = mptask_exec(&mptask);
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
      message("camera_main_loop: send = ID %d, data=%x\n", MSG_ID_APS00_SANDBOX_APS_CAMERA_ASMP, data);
      /* Send command to worker */
      ret = mpmq_send(&mq, MSG_ID_APS00_SANDBOX_APS_CAMERA_ASMP, (uint32_t) &data);
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
      message("Worker[%d] response: ID = %d, data = %x\n",
              loop, ret, *((int *)msgdata));      
    }

  /* Show worker copied data */
  /* Send command to worker */
  ret = mpmq_send(&mq, MSG_ID_APS00_SANDBOX_APS_CAMERA_EXIT, (uint32_t) &data);
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
  message("Worker response: ID = %d, data = %x\n",
          ret, *((int *)msgdata));


  /* Lock mutex for synchronize with worker after it's started */

  mpmutex_lock(&mutex);

  message("Worker said: %s\n", buf_yuv);

  mpmutex_unlock(&mutex);

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
  apsamp_camera_fini(&v_fd);

  mpshm_detach(&shm_yuv);
  mpshm_destroy(&shm_yuv);
  mpshm_detach(&shm_rgb);
  mpshm_destroy(&shm_rgb);
  mpmutex_destroy(&mutex);
  mpmq_destroy(&mq);

  return 0;
}
