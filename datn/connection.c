#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "wifi.h"
#include "mqtt.h"
#include "bh1750.h"
#include "control.h"
#include "pzem004t.h"
#include "main.h"

static const char *TAG1 = "WIFI";
static const char *TAG = "MQTT";

static void mqtt_app_start(void);

//For Wi-Fi Config
wifi_config_t wifi_sta_config;
wifi_config_t wifi_ap_config;
static int retry_cnt =  0;
uint8_t start  = 0;
uint8_t cout = 0;

//For MQTT config
uint8_t MQTT_CONNEECTED = 0;


//For PZEM Data to Send
uint8_t buffer[8] = {0xF8, CMD_RIR, 0X00, 0X00, 0X00, 0X09, 0x00, 0x00}; 
/*
    0xF8 la dia chi mac dinh khi su dung 1 PZEM, code hi su dung 1 module PZEM nen khong can su dung code SetAddress
    CMD_RIR la code doc du lieu tu Hoding Register
    0x00,0x00 la dia chi thanh ghi bat dau doc, 0x0000 la dia chi cua thanh ghi luu tru gia tri dien ap
    0x0009 la so so luong thanh ghi can doc
*/
uint8_t respone[25] = {0};


/*                  
                                            #################################################################
                                            #                             _`				#
                                            #                          _ooOoo_				#
                                            #                         o8888888o				#
                                            #                         88" . "88				#
                                            #                         (| -_- |)				#
                                            #                         O\  =  /O				#
                                            #                      ____/`---'\____				#
                                            #                    .'  \\|     |//  `.			#
                                            #                   /  \\|||  :  |||//  \			#
                                            #                  /  _||||| -:- |||||_  \			#
                                            #                  |   | \\\  -  /'| |   |			#
                                            #                  | \_|  `\`---'//  |_/ |			#
                                            #                  \  .-\__ `-. -'__/-.  /			#
                                            #                ___`. .'  /--.--\  `. .'___			#
                                            #             ."" '<  `.___\_<|>_/___.' _> \"".			#
                                            #            | | :  `- \`. ;`. _/; .'/ /  .' ; |		#
                                            #            \  \ `-.   \_\_`. _.'_/_/  -' _.' /		#
                                            #=============`-.`___`-.__\ \___  /__.-'_.'_.-'=================#
                                                                    `=--=-'                    



                                                    _.-/`)
                                                    // / / )
                                                .=// / / / )
                                                //`/ / / / /
                                                // /     ` /
                                            ||         /
                                                \\       /
                                                ))    .'
                                                //    /
                                                    /
*/

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                //ESP_LOGI(TAG1, "WIFI_EVENT_STA_START");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_CONNECTED:
                //ESP_LOGI(TAG1, "WIFI_EVENT_STA_CONNECTED");
                ESP_LOGI(TAG1, "Wi-Fi Connected\n");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG1, "disconnected: Retrying Wi-Fi\n");
                if (retry_cnt++ < MAX_RETRY)
                {
                    esp_wifi_connect();
                }
                else
                {
                ESP_LOGI(TAG1, "Max Retry Failed: Wi-Fi Connection\n");
                ESP_LOGI(TAG1,"Restarting Now");
                esp_restart();
                }
                break;
        }
    } 
    else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t* event = event_data;
                ESP_LOGI(TAG1, "Station connected with IP: "IPSTR", GW: "IPSTR", Mask: "IPSTR".",
                    IP2STR(&event->ip_info.ip),
                    IP2STR(&event->ip_info.gw),
                    IP2STR(&event->ip_info.netmask)); //IP2STR: Convert an IP number into it’s string representation.
                ESP_LOGI(TAG1, "Got IP: Starting MQTT Client\n");
                mqtt_app_start();
                break;
            }
        }
    }
}

static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    switch (event_id) {
        case IP_EVENT_AP_STAIPASSIGNED: {
            ip_event_ap_staipassigned_t* event = event_data;
            ESP_LOGI(TAG1, "SoftAP client connected with IP: "IPSTR".",
                IP2STR(&event->ip));
            break;
        }
    }
}

void WIFI_INIT() {
    // NVS: Required by WiFi Driver
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init()); 
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void AP_INIT_IP(char *ssid, char *password, char *ip, char *gw, char *nmask) {
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();
    
    esp_netif_ip_info_t IP_settings_ap;
    IP_settings_ap.ip.addr=ipaddr_addr(ip);
    IP_settings_ap.netmask.addr=ipaddr_addr(nmask);
    IP_settings_ap.gw.addr=ipaddr_addr(gw);
    esp_netif_dhcps_stop(esp_netif_ap);
    esp_netif_set_ip_info(esp_netif_ap, &IP_settings_ap);
    esp_netif_dhcps_start(esp_netif_ap);
 
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &wifi_ap_event_handler, NULL, NULL));

    strcpy((char *)wifi_ap_config.ap.ssid, ssid); 
    strcpy((char *)wifi_ap_config.ap.password, password);
    wifi_ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_ap_config.ap.max_connection = AP_MAX_CONN;
    wifi_ap_config.ap.channel = AP_CHANNEL;
    wifi_ap_config.ap.ssid_hidden = 0;
}

void AP_INIT(char *ssid, char *password) {
    esp_netif_create_default_wifi_ap();
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &wifi_ap_event_handler, NULL, NULL));

    strcpy((char *)wifi_ap_config.ap.ssid, ssid); 
    strcpy((char *)wifi_ap_config.ap.password, password);
    wifi_ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_ap_config.ap.max_connection = 4;
}

void STA_INIT_IP(char *ssid, char *password, char *ip, char *gw, char *nmask) {

    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

    esp_err_t ret = esp_netif_dhcpc_stop(esp_netif_sta);
    if(ret == ESP_OK) ESP_LOGI(TAG1, "esp_netif_dhcpc_stop OK");
    else ESP_LOGI(TAG1, "esp_netif_dhcpc_stop ERROR");

    esp_netif_ip_info_t IP_settings_sta;
    IP_settings_sta.ip.addr=ipaddr_addr(ip); //ipaddr_addr: return IP addr as order
    IP_settings_sta.netmask.addr=ipaddr_addr(nmask);
    IP_settings_sta.gw.addr=ipaddr_addr(gw);
    esp_netif_set_ip_info(esp_netif_sta, &IP_settings_sta);
  
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL, NULL));
    
    strcpy((char *)wifi_sta_config.sta.ssid, ssid); 
    strcpy((char *)wifi_sta_config.sta.password, password);
}

void STA_INIT(char *ssid, char *password) {
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL, NULL));
        
    strcpy((char *)wifi_sta_config.sta.ssid, ssid);
    strcpy((char *)wifi_sta_config.sta.password, password);
}

void WIFI_START(wifi_mode_t mode) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
   
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
  
    if (mode==WIFI_MODE_APSTA || mode==WIFI_MODE_STA) ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_sta_config));
    if (mode==WIFI_MODE_APSTA || mode==WIFI_MODE_AP) ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));
    
    ESP_ERROR_CHECK(esp_wifi_start());
}

/*


        MQTT START HERE



*/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        MQTT_CONNEECTED=1;
        
        /*msg_id = esp_mqtt_client_subscribe(client, TOPIC1, QoS_level_0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);*/

        msg_id = esp_mqtt_client_subscribe(client, TOPIC2, QoS_level_1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        /*msg_id = esp_mqtt_client_subscribe(client, TOPIC3, QoS_level_1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);*/

        msg_id = esp_mqtt_client_subscribe(client, TOPIC10, QoS_level_1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        GPIOOff(RELAY1);
        esp_mqtt_client_publish(client, TOPIC3, "OFF", 0, 1, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        MQTT_CONNEECTED=0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

// Check he thong ON hay OFF
        if (strncmp(event -> topic, TOPIC2, event -> topic_len) == 0)
        {
            char status[event -> data_len + 1]; // tao chuoi de chua du lieu tu event -> data
            strncpy(status, event -> data, event -> data_len); // copy du lieu tu data[] sang status[]
            status[event -> data_len] = '\0'; //dam bao chuoi ket thuc bang ki tu NULL
            if(strcmp(status, "ON") == 0)
            {
                GPIOOn(RELAY1);
                start = 0;
                esp_mqtt_client_publish(client, TOPIC3, "ON", 0, 1, 1);
            }
            else if(strcmp(status, "OFF") == 0)
            {
                GPIOOff(RELAY1);
                esp_mqtt_client_publish(client, TOPIC3, "OFF", 0, 1, 1);
            }
        }

        

//Check Reset
        if ((strncmp(event -> topic, TOPIC10, event -> topic_len)  == 0) && (MQTT_CONNEECTED == 1))
        {
            char status[event -> data_len + 1];
            strncpy(status, event -> data, event -> data_len);
            status[event -> data_len] = '\0';
            if(strcmp(status, "PRESSED") == 0) 
                pzem004t_reset();


        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_mqtt_client_handle_t client = NULL;
static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqttConfig = {
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .broker.address.uri = MQTT_URI,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASSWRD
    };
        
    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, &mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}
/*


            SEND DATA


*/
void Lux_Publisher_Task(void *params)
{
    while (true)
    {
        if (CheckState(RELAY1) == 1)
        {
            float lux_val;
            char lux_str[10];
            if(MQTT_CONNEECTED)
            {
                lux_val = bh1750_read();
                snprintf(lux_str, sizeof(lux_str), "%.1f", lux_val);
                esp_mqtt_client_publish(client, TOPIC1, lux_str, 0, 0, 0);
                //printf("Lux value is %.1f\n",lux_val);
                ESP_LOGI(TAG, "Lux value is %.1f\n",lux_val);
            }         
        }
        else if (CheckState(RELAY1) == 0)
        {
            ESP_LOGI(TAG, "Waiting to Turn ON\n");
        }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}


void Pzem_Publisher_Task(void *params)
{
    while(1)
    {
        switch(start)
        {
            case 0:
            {
            if(CheckState(RELAY1) == 1)
            {
                updateValues(buffer,respone, sizeof(buffer), sizeof(respone));
                if((respone[5] | respone[6] | respone [7] | respone[8]) == 0x00 )
                {
                /*snprintf(str, sizeof(str), "%.1f", Voltage);
                esp_mqtt_client_publish(client, TOPIC4, str, 0, 0, 0);
                esp_mqtt_client_publish(client, TOPIC5, "0.0", 0, 0, 0);
                esp_mqtt_client_publish(client, TOPIC6, "0.0", 0, 0, 0);
                esp_mqtt_client_publish(client, TOPIC7, "0.0", 0, 0, 0);
                snprintf(str, sizeof(str), "%.1f", Frequency);
                esp_mqtt_client_publish(client, TOPIC8, str, 0, 0, 0);*/
                }

                else if((respone[5] | respone[6] | respone [7] | respone[8]) != 0x00 )
                {
                /*snprintf(str, sizeof(str), "%.1f", Voltage);
                esp_mqtt_client_publish(client, TOPIC4, str, 0, 0, 0);
                esp_mqtt_client_publish(client, TOPIC5, "0.0", 0, 0, 0);
                esp_mqtt_client_publish(client, TOPIC6, "0.0", 0, 0, 0);
                esp_mqtt_client_publish(client, TOPIC7, "0.0", 0, 0, 0);
                snprintf(str, sizeof(str), "%.1f", Frequency);
                esp_mqtt_client_publish(client, TOPIC8, str, 0, 0, 0);*/
                start = 1;
                }
            }
            else if(CheckState(RELAY1) == 0){}
            vTaskDelay(1000/portTICK_PERIOD_MS);
            }
            break;

            case 1:
            {
            if(CheckState(RELAY1) == 1)
            {
                //float PF = powerfactor();
                char str[10];
                if(MQTT_CONNEECTED)
                {
                    updateValues(buffer,respone, sizeof(buffer), sizeof(respone));
                    float Voltage = voltage();
                    float Current = current();
                    float Power = power();
                    float Energy = energy();
                    float Frequency = frequency();
                    snprintf(str, sizeof(str), "%.1f", Voltage);
                    esp_mqtt_client_publish(client, TOPIC4, str, 0, 0, 1);
                    snprintf(str, sizeof(str), "%.1f", Current*1000);
                    esp_mqtt_client_publish(client, TOPIC5, str, 0, 0, 1);
                    snprintf(str, sizeof(str), "%.1f", Power);
                    esp_mqtt_client_publish(client, TOPIC6, str, 0, 0, 1);
                    snprintf(str, sizeof(str), "%.3f", Energy);
                    ESP_LOGI(TAG,"0x%02x%02x %02x%02x",respone[15],respone[16],respone[13],respone[14]);
                    esp_mqtt_client_publish(client, TOPIC7, str, 0, 0, 1);
                    snprintf(str, sizeof(str), "%.1f", Frequency);
                    esp_mqtt_client_publish(client, TOPIC8, str, 0, 0, 1);
                    
                }
            }
            else if(CheckState(RELAY1) == 0){}
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            break;
        }
    }
}

/*
void CheckRELAY2(void);
void CheckRELAY3(void);
void CheckRELAY4(void);

void CheckTask(void)
{
    GPIOOn(RELAY1);
    GPIOOn(RELAY2);
    GPIOOn(RELAY3);
    GPIOOn(RELAY4);
    vTaskSuspend(myTask1Handle); vTaskSuspend(myTask1Handle);
    updateValues(buffer,respone, sizeof(buffer), sizeof(respone));
    float Voltage = voltage();
    float Current = current();
    float Power = power();
    if( (220 <= Voltage && Voltage <= 235 ) | (Current != 0) | (14 <= Power && Power <= 16))
    {
        esp_mqtt_client_publish(client, TOPIC11, "Everything is OK",0,0,0);
    }
    else
    {
        CheckRELAY2();
        CheckRELAY3();
        CheckRELAY4();
}
    GPIOOff(RELAY1);
    GPIOOff(RELAY2);
    GPIOOff(RELAY3);
    GPIOOff(RELAY4);
    vTaskResume(myTask1Handle);
    vTaskResume(myTask2Handle);
}

void CheckRELAY2(void)
{
    GPIOOn(RELAY2);
    GPIOOff(RELAY3); 
    //esp_mqtt_client_publish(client, TOPIC21, "OFF", 0, 1, 1);
    GPIOOff(RELAY4);
    //esp_mqtt_client_publish(client, TOPIC23, "OFF", 0, 1, 1);
    updateValues(buffer,respone, sizeof(buffer), sizeof(respone));
    float Voltage = voltage();
    float Current = current();
    float Power = power();
    if( (220 <= Voltage && Voltage <= 235 ) | (Current != 0) | (2.5 <= Power && Power <= 3.5))
    {
        esp_mqtt_client_publish(client, TOPIC12, "OK",0,0,0);
    }
    else
        esp_mqtt_client_publish(client, TOPIC15, "Problem",0,0,0);
}

void CheckRELAY3(void)
{
    GPIOOn(RELAY3);
    GPIOOff(RELAY2);
    GPIOOff(RELAY4);
    updateValues(buffer,respone, sizeof(buffer), sizeof(respone));
    float Voltage = voltage();
    float Current = current();
    float Power = power();
    if( (220 <= Voltage && Voltage <= 235 ) | (Current != 0) | (4.5 <= Power && Power <= 5.5))
    {
        esp_mqtt_client_publish(client, TOPIC13, "OK",0,0,0);
    }
    else
        esp_mqtt_client_publish(client, TOPIC16, "Problem",0,0,0);
}

void CheckRELAY4(void)
{
    GPIOOn(RELAY4);
    GPIOOff(RELAY2);
    GPIOOff(RELAY3);
    updateValues(buffer,respone, sizeof(buffer), sizeof(respone));
    float Voltage = voltage();
    float Current = current();
    float Power = power();
    if( (220 <= Voltage && Voltage <= 235 ) | (Current != 0) | (6.5 <= Power && Power <= 7.5))
    {
        esp_mqtt_client_publish(client, TOPIC14, "OK",0,0,0);
    }
    else
        esp_mqtt_client_publish(client, TOPIC17, "Problem",0,0,0);
}
*/
