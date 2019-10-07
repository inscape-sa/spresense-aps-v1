#ifndef __APPS_EXAMPLES_COLOR_CTRL_BASIC_H
#define __APPS_EXAMPLES_COLOR_CTRL_BASIC_H

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct v_buffer {
  uint32_t             *start;
  uint32_t             length;
} v_buffer_t;

extern v_buffer_t *buffers_video;

int apsamp_camera_init(int *p_v_fd, uint8_t buffernum, void* bufferlist[]);
void apsamp_camera_fini(int *p_v_fd);
int apsamp_capdata_lock(int v_fd, struct v4l2_buffer *p_buf);
int apsamp_capdata_release(int v_fd, struct v4l2_buffer *p_buf);
int camera_prepare(int             fd,
                enum v4l2_buf_type type,
                uint32_t           buf_mode,
                uint32_t           pixformat,
                uint16_t           hsize,
                uint16_t           vsize,
                uint8_t            buffernum,
                void*              bufferlist[]
                );
void alloc_buffer(void *bufferlist[], uint8_t bufnum);
void free_buffer(void *bufferlist[], uint8_t bufnum);


#endif /* __APPS_EXAMPLES_COLOR_CTRL_BASIC_H */