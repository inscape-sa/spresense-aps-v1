
#include <sdk/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/fs/mkfatfs.h>
#include "video/video.h"

#include <nuttx/lcd/lcd.h>
#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "nximage.h"
#include "config_image.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static void nximage_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool more, FAR void *arg);
static void nximage_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg);

/****************************************************************************
 * Public Data
 ****************************************************************************/
/* Background window call table */
const struct nx_callback_s g_nximagecb_aps_asmp =
{
  nximage_redraw,   /* redraw */
  nximage_position  /* position */
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nximage_image
 *
 * Description:
 *   Put the NuttX logo in the center of the display.
 *
 ****************************************************************************/

void apsamp_nximage_image(NXWINDOW hwnd, FAR const void *image)
{
  FAR struct nxgl_point_s origin;
  FAR struct nxgl_rect_s dest;
  FAR const void *src[CONFIG_NX_NPLANES];
  int ret;

  origin.x = 0;
  origin.y = 0;

  /* Set up the destination to whole LCD screen */

  dest.pt1.x = 0;
  dest.pt1.y = 0;
  dest.pt2.x = g_nximage_aps_asmp.xres - 1;
  dest.pt2.y = g_nximage_aps_asmp.yres - 1;

  src[0] = image;

  ret = nx_bitmap((NXWINDOW)hwnd, &dest, src, &origin,
                  g_nximage_aps_asmp.xres * sizeof(nxgl_mxpixel_t));
  if (ret < 0)
    {
      printf("nximage_image: nx_bitmapwindow failed: %d\n", errno);
    }
}

/****************************************************************************
 * Name: apsamp_nximage_initialize
 *
 * Description:
 *   Put the NuttX logo in the center of the display.
 *
 ****************************************************************************/
int apsamp_nximage_initialize(void)
{
  FAR NX_DRIVERTYPE *dev;
  nxgl_mxpixel_t color;
  int ret;

  /* Initialize the LCD device */

  printf("nximage_initialize: Initializing LCD\n");
  ret = board_lcd_initialize();
  if (ret < 0)
    {
      printf("nximage_initialize: board_lcd_initialize failed: %d\n", -ret);
      return ERROR;
    }

  /* Get the device instance */

  dev = board_lcd_getdev(CONFIG_EXAMPLES_CAMERA_LCD_DEVNO);
  if (!dev)
    {
      printf("nximage_initialize: board_lcd_getdev failed, devno=%d\n",
             CONFIG_EXAMPLES_CAMERA_LCD_DEVNO);
      return ERROR;
    }

  /* Turn the LCD on at 75% power */

  (void)dev->setpower(dev, ((3*CONFIG_LCD_MAXPOWER + 3)/4));

  /* Then open NX */

  printf("nximage_initialize: Open NX\n");
  g_nximage_aps_asmp.hnx = nx_open(dev);
  if (!g_nximage_aps_asmp.hnx)
    {
      printf("nximage_initialize: nx_open failed: %d\n", errno);
      return ERROR;
    }

  /* Set background color to black */

  color = 0;
  nx_setbgcolor(g_nximage_aps_asmp.hnx, &color);
  ret = nx_requestbkgd(g_nximage_aps_asmp.hnx, &g_nximagecb_aps_asmp, NULL);
  if (ret < 0)
    {
      printf("nximage_initialize: nx_requestbkgd failed: %d\n", errno);
      nx_close(g_nximage_aps_asmp.hnx);
      return ERROR;
    }

  while (!g_nximage_aps_asmp.havepos)
    {
      (void) sem_wait(&g_nximage_aps_asmp.sem);
    }
  printf("nximage_initialize: Screen resolution (%d,%d)\n",
         g_nximage_aps_asmp.xres, g_nximage_aps_asmp.yres);

  return 0;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nximage_redraw
 *
 * Description:
 *   NX re-draw handler
 *
 ****************************************************************************/
static void nximage_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool more, FAR void *arg)
{
  ginfo("hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd, rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");
}

/****************************************************************************
 * Name: nximage_position
 *
 * Description:
 *   NX position change handler
 *
 ****************************************************************************/
static void nximage_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg)
{
  /* Report the position */

  ginfo("hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
        hwnd, size->w, size->h, pos->x, pos->y,
        bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);

  /* Have we picked off the window bounds yet? */

  if (!g_nximage_aps_asmp.havepos)
    {
      /* Save the background window handle */

      g_nximage_aps_asmp.hbkgd = hwnd;

      /* Save the window limits */

      g_nximage_aps_asmp.xres = bounds->pt2.x + 1;
      g_nximage_aps_asmp.yres = bounds->pt2.y + 1;

      g_nximage_aps_asmp.havepos = true;
      sem_post(&g_nximage_aps_asmp.sem);
      ginfo("Have xres=%d yres=%d\n", g_nximage_aps_asmp.xres, g_nximage_aps_asmp.yres);
    }
}
