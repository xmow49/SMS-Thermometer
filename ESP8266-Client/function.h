#include <WifiEspNow.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include "DHT.h"
#include "config.h"

void printReceivedMessage(const uint8_t mac[6], const uint8_t* buf, size_t count, void* cbarg) {
  Serial.printf("Message from %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  for (int i = 0; i < count; ++i) {
    Serial.print(static_cast<char>(buf[i]));
  }
  Serial.println();
}

typedef struct struct_message {
  int t;
  int h;
  int b;

} struct_message;
struct_message myData;


void setupEspNow(){
  Serial.begin(115200);
  Serial.setTimeout(2000);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESPNOW", nullptr, 3);
  WiFi.softAPdisconnect(false);

  Serial.print("MAC address of this node is ");
  Serial.println(WiFi.softAPmacAddress());

  bool ok = WifiEspNow.begin();
  if (!ok) {
    Serial.println("WifiEspNow.begin() failed");
    ESP.restart();
  }

  WifiEspNow.onReceive(printReceivedMessage, nullptr);

  ok = WifiEspNow.addPeer(PEER);
  if (!ok) {
    Serial.println("WifiEspNow.addPeer() failed");
    ESP.restart();
  }
}

int t;
int h;
int b;
int percent;
int TempClient(){
  t = dht.readTemperature();
  Serial.println(t);
  return t;
}
int HumClient(){
  h = dht.readHumidity();
  return h;
}

int getBattery(){
  b = analogRead(BATTERYPIN);
  Serial.println(b);
  percent = map(b, 0, 1024, 0, 100);
  return percent;
}

void sendTemp(){
  delay(500);
  dht.begin();
	delay(500);
	
	myData.t = TempClient();
	myData.h = HumClient();
	myData.b = getBattery();

  myData.t = TempClient();
	myData.h = HumClient();
	myData.b = getBattery();
	delay(500);
	WifiEspNow.send(PEER, (uint8_t *) &myData, sizeof(myData));
	ESP.deepSleep(timeDeepSleep*3600000000);

}