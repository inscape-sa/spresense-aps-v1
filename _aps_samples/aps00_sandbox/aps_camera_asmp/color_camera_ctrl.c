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

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/fs/mkfatfs.h>
#include "video/video.h"

#include <arch/chip/pm.h>
#include <arch/board/board.h>
#include <arch/chip/cisif.h>

#include "nximage.h"
#include "config_image.h"
#include "config_basic.h"
#include "color_proc_maincore.h"
#include "color_camera_ctrl.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
v_buffer_t *buffers_video;
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

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int camera_prepare(int             fd,
                enum v4l2_buf_type type,
                uint32_t           buf_mode,
                uint32_t           pixformat,
                uint16_t           hsize,
                uint16_t           vsize,
                uint8_t            buffernum)
{
  int ret;
  int cnt;
  static unsigned int n_buffers;
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

  for (n_buffers = 0; n_buffers < buffernum; ++n_buffers)
    {
      buffers[n_buffers].length = IMAGE_YUV_SIZE;

      /* Note: VIDIOC_QBUF set buffer pointer. */
      /*       Buffer pointer must be 32bytes aligned. */

      buffers[n_buffers].start  = memalign(32, IMAGE_YUV_SIZE);
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
          return ret;
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

void free_buffer(v_buffer_t *buffers, uint8_t bufnum)
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
int apsamp_camera_init(int *p_v_fd)
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

void apsamp_camera_fini(int *p_v_fd)
{
  close(*p_v_fd);
  free_buffer(buffers_video, VIDEO_BUFNUM);
  video_uninitialize();
  nx_close(g_nximage_aps_asmp.hnx);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int apsamp_capdata_lock(int v_fd, struct v4l2_buffer *p_buf)
{
  int ret;
  /* Note: VIDIOC_DQBUF acquire capture data. */
  memset(p_buf, 0, sizeof(v4l2_buffer_t));
  p_buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  p_buf->memory = V4L2_MEMORY_USERPTR;
  ret = ioctl(v_fd, VIDIOC_DQBUF, (unsigned long)p_buf);
  return ret;
}

int apsamp_capdata_release(int v_fd, struct v4l2_buffer *p_buf)
{
  int ret;
  /* Note: VIDIOC_QBUF reset released buffer pointer. */
  ret = ioctl(v_fd, VIDIOC_QBUF, (unsigned long)p_buf);
  return ret;
}
