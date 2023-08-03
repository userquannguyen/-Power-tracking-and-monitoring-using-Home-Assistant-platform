#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "sdkconfig.h"
#include "esp_log.h"

#include "esp_rom_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "driver/uart.h"
#include "mqtt_client.h"

#include "bh1750.h"
#include "pzem004t.h"
#include "wifi.h"
#include "mqtt.h"
#include "control.h"
#include "main.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

TaskHandle_t myTask1Handle = NULL;
TaskHandle_t myTask2Handle = NULL;
/*void LuxTask(void *pvParameters)
{
    float lux_val;
    while(1){
    lux_val = bh1750_read();
    printf("Lux value is %.1f\n",lux_val);
    vTaskDelay(3000/portTICK_PERIOD_MS);
    }
}

void Pzem004tTask(void *pvParameters)
{
    while(1){
    printf("Dumb = 0000\n");
    updateValues(buffer,respone, 8, 25);
    float Voltage = voltage();
    float Current = current();
    printf("Voltage = %.1f\n", Voltage);
    printf("Current = %.1f\n", Current);
    //pzem004t_reset();
    vTaskDelay(5000/portTICK_PERIOD_MS);
}
}
*/


void app_main()
{
    GPIOInit();
    bh1750_init();
    pzem004t_init();
    WIFI_INIT();
    STA_INIT_IP(STA_SSID, STA_PASSWORD, STA_IP, STA_GW, STA_MASK);
    WIFI_START(WIFI_MODE_STA);
    xTaskCreate(Pzem_Publisher_Task, "Pzem_Publisher_Task", 1024 * 5, NULL, 5, &myTask1Handle);
    xTaskCreate(Lux_Publisher_Task, "Lux_Publisher_Task", 1024 * 5, NULL, 5, &myTask2Handle);

//xTaskCreate( task1, "task1" , 4096 , NULL , 5 , &myTask2Handle); 
//xTaskCreate( LuxTask, "task2" , 4096 , NULL , 5 , &myTask1Handle);
//xTaskCreate( Pzem004tTask, "task3" , 4096 , NULL , 5 , &myTask3Handle);  
}