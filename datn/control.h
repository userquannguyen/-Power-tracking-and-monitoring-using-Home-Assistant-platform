#ifndef CONTROL_H_
#define CONTROL_H_


void GPIOInit(void);

void GPIOOn(int relay);
void GPIOOff(int relay);

int CheckState(int relay);

#define RELAY1 26
#define RELAY2 18
#define RELAY3 19
#define RELAY4 23

#endif