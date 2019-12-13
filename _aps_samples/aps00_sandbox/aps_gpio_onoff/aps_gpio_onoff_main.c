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

/* BUTTON DEF */
#define GPIO_BUTTON_L (39)
#define GPIO_BUTTON_R (29)
#define GPIO_LED_L    (47)
#define GPIO_LED_R    (46)

/* Arduino Compat Layer */
#define INPUT         (true)
#define OUTPUT        (false)
#define HIGH          (1)
#define LOW           (0)

/* prototype definitions */
static void pinMode(int pin, bool mode);

/* private variables */
int cnt;

static void setup(void)
{
  /* setup GPIO */
  pinMode(GPIO_BUTTON_L, INPUT);
  pinMode(GPIO_BUTTON_R, INPUT);
  pinMode(GPIO_LED_L, OUTPUT);
  pinMode(GPIO_LED_R, OUTPUT);

  /* Clear Counter */
  cnt = 0;
}

static void loop(void)
{
  int inputLeft = LOW;
  int inputRight = LOW;

  board_gpio_write(GPIO_LED_L, LOW);
  board_gpio_write(GPIO_LED_R, LOW);
  sleep(1);
  
  inputRight = board_gpio_read(GPIO_BUTTON_R);
  inputLeft = board_gpio_read(GPIO_BUTTON_L);
  board_gpio_write(GPIO_LED_R, inputRight);
  board_gpio_write(GPIO_LED_L, inputLeft);
  sleep(1);

  cnt++;
  if (cnt >= LOOP_MAX) {
    while(1);
      /* NOP */
  }
}

/**
 * public -- MAIN
 */
int aps_gpio_onoff_main(int argc, char *argv[])
{
  setup();
  while (1) {
    loop();
  }
  /* NEVER REACHED --- */
  return 0;
}

/**
 * private -- Arduino Compat
 */
static void pinMode(int pin, bool input_enable)
{
  int ret;
  ret = board_gpio_config(pin, 0, input_enable, false, PIN_FLOAT);
  if (ret != 0) {
    printf("ERROR:board_gpio_config(%d)", pin);
    while(1)
      ; /* HALT */
  }
}