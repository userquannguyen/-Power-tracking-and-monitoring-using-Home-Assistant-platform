#ifndef WIFI_H_
#define WIFI_H_

#define STA_SSID        "myssid"
#define STA_PASSWORD    "mypsw"
#define STA_IP          "192.168.43.51"
#define STA_GW          "192.168.43.1"
#define STA_MASK        "255.255.255.0"
#define MAX_RETRY       10

#define AP_SSID         "ESPTEST"
#define AP_PASSWORD     "12345678"
#define AP_MAX_CONN     4
#define AP_CHANNEL      0

void WIFI_INIT();
void AP_INIT_IP(char *ssid, char *password, char *ip, char *gw, char *nmask);
void AP_INIT(char *ssid, char *password);
void STA_INIT_IP(char *ssid, char *password, char *ip, char *gw, char *nmask);
void STA_INIT(char *ssid, char *password);
void WIFI_START(wifi_mode_t mode);

#endif
