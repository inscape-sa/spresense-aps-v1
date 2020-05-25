#include <sdk/config.h>
#include <stdio.h>


char *gpio_set_39_input[]= {"gpio","conf","39","-m","0","-i"};
char *gpio_set_29_input[]= {"gpio","conf","29","-m","0","-i"};
char *gpio_set_39_read[]= {"gpio","read","39"};
char *gpio_set_29_read[]= {"gpio","read","29"};

char *gpio_set_46_write_on[]= {"gpio","write","46", "1"};
char *gpio_set_47_write_on[]= {"gpio","write","47", "1"};
char *gpio_set_46_write_off[]= {"gpio","write","46", "0"};
char *gpio_set_47_write_off[]= {"gpio","write","47", "0"};

extern int gpio_main(int argc, char **argv);
int gpio_test_main(int argc, char *argv[])
{
  int count;
  gpio_main(6, gpio_set_39_input);
  gpio_main(6, gpio_set_29_input);

  while(1) {
    int mode = count % 4;

    printf("GPIO 39 -->> ");
    gpio_main(3, gpio_set_39_read);
    printf("GPIO 29 -->> ");
    gpio_main(3, gpio_set_29_read);
    sleep(1);
    
    switch (mode) {
      case 0:
        gpio_main(4, gpio_set_46_write_on);
        gpio_main(4, gpio_set_47_write_on);
        break;
      case 1:
        gpio_main(4, gpio_set_46_write_on);
        gpio_main(4, gpio_set_47_write_off);
        break;
      case 2:
        gpio_main(4, gpio_set_46_write_off);
        gpio_main(4, gpio_set_47_write_off);
        break;
      case 3:
        gpio_main(4, gpio_set_46_write_off);
        gpio_main(4, gpio_set_47_write_on);
        break;
      default:
        break;
    }
    count++;
  }
  return 0;
}
