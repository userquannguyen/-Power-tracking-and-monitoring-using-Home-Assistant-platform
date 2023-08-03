#ifndef PZEM004T_H_
#define PZEM004T_H_

#define PZEM_DEFAULT_ADDR   0xF8

#define PZEM_BAUD_RATE      9600
#define PZEM_DATA_BITS      UART_DATA_8_BITS
#define PZEM_STOP_BITS      UART_STOP_BITS_1
#define PZEM_PARITY         UART_PARITY_DISABLE
#define PZEM_UART_TX_PIN    17
#define PZEM_UART_RX_PIN    16

#define UART_BUF_SIZE 1024

#define REG_VOLTAGE         0x0000
#define REG_CURRENT_L       0x0001
#define REG_CURRENT_H       0x0002
#define REG_POWER_L         0x0003
#define REG_POWER_H         0x0004
#define REG_ENERGY_L        0x0005
#define REG_ENERGY_H        0x0006
#define REG_FREQ            0x0007 
#define REG_PF              0x0008 
#define REG_ALARM           0x0009

/*
*    \param CMD_RHR: Read Holding Register 
*    \param CMD_RIR: Read Input Register
*    \param CMD_WSR: Write Single Register
*    \param CMD_CALIB: Calibration
*    \param CMD_RST: Reset Energy
*/
#define CMD_RHR             0x03    // CMD_RHR: Read Holding Register 
#define CMD_RIR             0x04    // CMD_RIR: Read Input Register
#define CMD_WSR             0x06    // CMD_WSR: Write Single Register
#define CMD_CALIB           0x41    // CMD_CALIB: Calibration
#define CMD_RST             0x42    // CMD_RST: Reset Energy

void pzem004t_init(void);
void pzem004t_reset(void);

bool updateValues(uint8_t *buf, uint8_t *res, size_t buflen, size_t reslen);

float voltage(); //return Voltage value
float current(); //return Current value
float energy(); //return Energy value
float power(); //return Power value
float frequency(); //return Frequency value
float powerfactor(); //return PF value


#endif