#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pzem004t.h"

static const char *TAG = "pzem004t";
const uart_port_t UART_PORT = UART_NUM_1;
/*
    The command format of the master reads the measurement: 
    Slave Address + 0x04 + Register Address High Byte + Register Address Low Byte + Number 
of Registers High Byte + Number of Registers Low Byte + CRC Check High Byte + CRC Check 
Low Byte.
*/
/*
    The command format of the master to reset the slave's energy:
    Slave address + 0x42 + CRC check high byte + CRC check low byte.
*/

uint8_t rst[4] = {0xF8, 0x42, 0x00, 0x00}; //reset command
uint8_t rstres[4] = {0};


bool updateValuesflag = false;

//Tinh CRC16, PZEM su dung gia tri 0xA001 de tinh
uint16_t calculateCRC16(uint8_t *data, uint16_t length) {
    //Bang CRC duoc tinh toan truoc, giup giam thoi gian tinh toan
static const uint16_t crc16Table[] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};
    uint16_t crc = 0xFFFF; // Giá trị khởi tạo của CRC là 0xFFFF

    for (int i = 0; i < length; ++i) {
        crc = (crc >> 8) ^ crc16Table[(crc ^ data[i]) & 0xFF];
    }

    return crc;
}

void setCRC(uint8_t *buf, uint16_t len)
{
    if(len <= 2)
    {printf("Buffer size ERROR");
    return;}
    uint16_t crc;
    crc = calculateCRC16(buf, len - 2); //CRC cua data

    buf[len - 2] = crc & 0xFF; //CRC Low Byte
    buf[len - 1] = (crc >> 8) & 0xFF; //CRC High Byte
}

//Cac gia tri can doc
struct 
{
   float voltage;
   float current;
   float power;
   float energy;
   float frequency;
   float pf;
   uint16_t alarm;
} currentValues;

//Khoi tao chuc nang UART
void pzem004t_init(void) {    
    uart_config_t uart_config;                                        
        uart_config.baud_rate = 9600;
        ESP_LOGI(TAG,"Baud rate = %d",uart_config.baud_rate);
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.source_clk = UART_SCLK_DEFAULT;
        uart_config.rx_flow_ctrl_thresh = 122;
        ESP_LOGI(TAG, "Init Done");
    
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,PZEM_UART_TX_PIN,PZEM_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 10, NULL, 0));
}

/*
*    \param buf: data transmit buffer
*    \param res: data receive buffer
*    \param buflen: length of data transmit buffer
*    \param reslen: length of data receive buffer
*/
bool pzem004t_read_data(uint8_t *buf, uint8_t *res, uint16_t buflen, uint16_t reslen) {
    int ret = -1;
    setCRC(buf,buflen);
    ret = uart_write_bytes(UART_PORT, (const char *)buf, buflen);
    if (ret == -1) {
        printf("Write buffer failed\n");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Đợi 10ms cho phép module PZEM-004T phản hồi
    ret = uart_read_bytes(UART_PORT, res, reslen, 1000/portTICK_PERIOD_MS); // Đọc dữ liệu từ UART
    if (ret == -1) {
        printf("Read buffer failed\n");
        return false;
    } else {
        printf("Read buffer successfully: %d\n", ret);
        return true;
    }
    uint16_t crc = calculateCRC16(res,sizeof(reslen) - 2);
    uint16_t received_crc = ( ((uint16_t)res[reslen - 1] << 8) | ((uint16_t)res[reslen - 2]) );
    if (crc == received_crc)
    {
        return true;
    }
    else return false;
}

void pzem004t_reset(void)
{
    if(pzem004t_read_data(rst, rstres, 4, 4) && (memcmp(rst,rstres,4) == 0))
    {
        printf("PZEM004T Reset Sucessfully\n");
    }
    else 
        printf("Reset Failed\n");
}


/*
*    \param buf: data transmit buffer
*    \param res: data receive buffer
*    \param buflen: length of data transmit buffer
*    \param reslen: length of data receive buffer
*/
bool updateValues(uint8_t *buf, uint8_t *res, size_t buflen, size_t reslen)
{
    if(pzem004t_read_data(buf, res,  buflen, reslen))
    {
            currentValues.voltage = ((uint32_t)res[3] << 8 | // Raw voltage in 0.1V
                                    (uint32_t)res[4])/10.0;

            currentValues.current = ((uint32_t)res[5] << 8 | // Raw current in 0.001A
                                    (uint32_t)res[6] |
                                    (uint32_t)res[7] << 24 |
                                    (uint32_t)res[8] << 16) / 1000.0;

            currentValues.power =   ((uint32_t)res[9] << 8 | // Raw power in 0.1W
                                    (uint32_t)res[10] |
                                    (uint32_t)res[11] << 24 |
                                    (uint32_t)res[12] << 16) / 10.0;

            currentValues.energy =  ((uint32_t)res[13] << 8 | // Raw Energy in 1kWh
                                    (uint32_t)res[14] |
                                    (uint32_t)res[15] << 24 |
                                    (uint32_t)res[16] << 16) / 1000.0;

            currentValues.frequency=((uint32_t)res[17] << 8 | // Raw Frequency in 0.1Hz
                                    (uint32_t)res[18]) / 10.0;

            currentValues.pf =      ((uint32_t)res[19] << 8 | // Raw pf in 0.01
                                    (uint32_t)res[20]) / 100.0;

            currentValues.alarm =  ((uint32_t)res[21] << 8 | // Raw alarm value
                                    (uint32_t)res[22]);
            updateValuesflag = true;
    }
    
    else{ ESP_LOGD(TAG, "Read Values Failed\n");
          updateValuesflag = false;         }
    return updateValuesflag;
}

float voltage()
{
    if(updateValuesflag)
    {
        return currentValues.voltage;
    }
    else 
    {    printf("Update Voltage Value Failed\n");
        return 0;
    }
}

float current()
{
    if(updateValuesflag)
    {
        return currentValues.current;
    }
    else 
    {
        printf("Update Current Value Failed\n");
        return 0;
    }
}

float energy()
{
    if(updateValuesflag)
    {
        return currentValues.energy;
    }
    else 
    {
        printf("Update Energy Value Failed\n");
        return 0;
    }
}

float power()
{
    if(updateValuesflag)
    {
        return currentValues.power;
    }
    else 
    {
        printf("Update Power Value Failed\n");
        return 0;
    }
}

float frequency()
{
    if(updateValuesflag)
    {
        return currentValues.frequency;
    }
    else 
    {
        printf("Update Frequency Value Failed\n");
        return 0;
    }
}

float powerfactor()
{
    if(updateValuesflag)
    {
        return currentValues.pf;
    }
    else 
    {
        printf("Update PF Value Failed\n");
        return 0;
    }
}




