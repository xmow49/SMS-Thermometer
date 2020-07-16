static uint8_t PEER[] {0x84, 0x0D, 0x8E, 0xD2, 0x31, 0xE4}; //MAC Adress of Server

#define BATTERYPIN A0 //Analog pin of battery
#define DHTPIN 13 //pin of DHT22
#define DHTTYPE DHT22 //DHT type

DHT dht(DHTPIN, DHTTYPE); //define DHT

#define timeDeepSleep 1 //Interval to send data in hour
