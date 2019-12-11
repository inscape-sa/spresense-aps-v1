#include <sdk/config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

/* includes for GPIO */
#include <arch/board/board.h>
#include <arch/chip/pin.h>

/* EXEC times */
#define LOOP_MAX      (20)
#define LED_OFF       (0)

/* BUTTON DEF */
#define GPIO_BUTTON_L (29)
#define GPIO_BUTTON_R (39)
#define GPIO_LED_L    (46)
#define GPIO_LED_R    (47)

int aps_gpio_onoff_main(int argc, char *argv[])
{
  int ret;
  int pin;
  int cnt;

  /* GPIO input settings */
  pin = GPIO_BUTTON_L;
  ret = board_gpio_config(pin, 0, true, false, PIN_FLOAT);
  if (ret != 0) {
    printf("ERROR:board_gpio_config(%d)", pin);
    return -1;
  }
  pin = GPIO_BUTTON_R;
  ret = board_gpio_config(pin, 0, true, false, PIN_FLOAT);
  if (ret != 0) {
    printf("ERROR:board_gpio_config(%d)", pin);
    return -1;
  }

  /* GPIO output settings */
  pin = GPIO_LED_R;
  ret = board_gpio_config(pin, 0, false, false, PIN_FLOAT);
  if (ret != 0) {
    printf("ERROR:board_gpio_config(%d)", pin);
    return -1;
  }
  pin = GPIO_LED_L;
  ret = board_gpio_config(pin, 0, false, false, PIN_FLOAT);
  if (ret != 0) {
    printf("ERROR:board_gpio_config(%d)", pin);
    return -1;
  }

  for (cnt = 0; cnt < LOOP_MAX; cnt++) {
    int inputL = LED_OFF;
    int inputR = LED_OFF;
  
    board_gpio_write(GPIO_LED_L, LED_OFF);
    board_gpio_write(GPIO_LED_R, LED_OFF);
    sleep(1);
    
    inputL = board_gpio_read(GPIO_BUTTON_L);
    inputR = board_gpio_read(GPIO_BUTTON_R);
    board_gpio_write(GPIO_LED_L, inputL);
    board_gpio_write(GPIO_LED_R, inputR);
    sleep(1);
  }
  return 0;

}
