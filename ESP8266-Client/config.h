static uint8_t PEER[] {0x84, 0x0D, 0x8E, 0xD2, 0x31, 0xE4};

#define BATTERYPIN A0
#define DHTPIN 13
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define timeDeepSleep 1 // in hour