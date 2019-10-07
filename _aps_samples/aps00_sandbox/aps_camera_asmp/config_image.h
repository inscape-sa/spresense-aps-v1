#ifndef __APPS_EXAMPLES_CONFIG_IMAGE_H
#define __APPS_EXAMPLES_CONFIG_IMAGE_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Note: Buffer size must be multiple of 32. */
#define IMAGE_YUV_SIZE     (320*240*2) /* QVGA YUV422 */
#define VIDEO_BUFNUM       (3)
#define DEFAULT_REPEAT_NUM (300)
#ifndef CONFIG_EXAMPLES_CAMERA_LCD_DEVNO
#  define CONFIG_EXAMPLES_CAMERA_LCD_DEVNO 0
#endif
#define itou8(v) ((v) < 0 ? 0 : ((v) > 255 ? 255 : (v)))

#endif /* __APPS_EXAMPLES_CONFIG_IMAGE_H */