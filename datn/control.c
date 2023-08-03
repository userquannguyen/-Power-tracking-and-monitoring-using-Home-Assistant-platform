#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"

#include "control.h"


void GPIOInit(void)
{
    esp_rom_gpio_pad_select_gpio(RELAY1);
    gpio_set_direction(RELAY1,GPIO_MODE_INPUT_OUTPUT);
}

void GPIOOff(int relay)
{
    if(gpio_get_level(relay) == 0)
    {
    gpio_set_level(relay, 1);
    printf("GPIO %d OFF\n",relay);
    }
    else if(gpio_get_level(relay) == 1)
    printf("GPIO %d OFF Already\n",relay);
}

void GPIOOn(int relay)
{
    if(gpio_get_level(relay) == 1)
    {
    gpio_set_level(relay, 0);
    printf("GPIO %d ON\n",relay);
    }
    else if(gpio_get_level(relay) == 0)
    printf("GPIO %d ON Already\n",relay);
}

int CheckState(int relay)
{
    if(gpio_get_level(relay) == 0) 
    return 1;
    else
    return 0;
}