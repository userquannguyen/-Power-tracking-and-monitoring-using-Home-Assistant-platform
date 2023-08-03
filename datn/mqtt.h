#ifndef MQTT_H_
#define MQTT_H_

#define MQTT_URI "mqtt://192.168.43.50:1883" //mqtt:IP:Port
#define MQTT_USER     "mqtt_person"
#define MQTT_PASSWRD  "mquan123456"   

void Lux_Publisher_Task(void *params);
void Pzem_Publisher_Task(void *params);

//controlswitch
#define TOPIC1   "home/sensor/lux2"

#define TOPIC2   "home/switch/set2"
#define TOPIC3   "home/switch/status2"

//pzem
#define TOPIC4   "home/sensor/pzem/voltage2"     //V
#define TOPIC5   "home/sensor/pzem/current2"     //A
#define TOPIC6   "home/sensor/pzem/power2"       //W
#define TOPIC7   "home/sensor/pzem/energy2"      //Wh
#define TOPIC8   "home/sensor/pzem/frequency2"   //Hz 
#define TOPIC9   "home/sensor/pzem/powerfactor2"
#define TOPIC10  "home/sensor/pzem/reset2"

#define QoS_level_0 0 //Broker/client sẽ gửi dữ liệu đúng một lần, quá trình gửi được xác nhận bởi chỉ giao thức TCP/IP
#define QoS_level_1 1 //Broker/client sẽ gửi dữ liệu với ít nhất một lần xác nhận từ đầu kia
#define QoS_level_2 2 //Broker/client đảm bảo khi gửi dữ liệu thì phía nhận chỉ nhận được đúng một lần


#endif