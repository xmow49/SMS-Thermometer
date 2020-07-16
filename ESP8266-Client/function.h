#include <WifiEspNow.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <DHT.h>
#include "config.h"

void printReceivedMessage(const uint8_t mac[6], const uint8_t *buf, size_t count, void *cbarg)
{
  //Debug:
  /*Serial.printf("Message from %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); //Debug
  for (unsigned int i = 0; i < count; ++i) {
    Serial.print(static_cast<char>(buf[i]));
  }
  Serial.println();*/
}

typedef struct struct_message
{
  float t;
  float h;
  int b;
} struct_message;
struct_message myData;

void setupEspNow()
{
  //Serial.begin(115200); //Debug
  //Serial.setTimeout(2000);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESPNOW", nullptr, 3);
  WiFi.softAPdisconnect(false);

  //Serial.print("MAC address of this node is "); //Debug
  //Serial.println(WiFi.softAPmacAddress());

  bool ok = WifiEspNow.begin();
  if (!ok)
  {
    //Serial.println("WifiEspNow.begin() failed");//Debug
    ESP.restart();
  }

  WifiEspNow.onReceive(printReceivedMessage, nullptr);

  ok = WifiEspNow.addPeer(PEER);
  if (!ok)
  {
    //Serial.println("WifiEspNow.addPeer() failed"); //Debug
    ESP.restart();
  }
}

float TempClient()
{
  float t = dht.readTemperature(); //get temperature
  int ntry = 0;
  while (String(t) == "nan" && ntry < 5)
  {
    ntry++;
    t = dht.readTemperature();
    delay(1000);
  }
  if (String(t) == "nan")
  {
    t = 200; //If no temperature, set to 200 for the error Info
  }
  return t;
}
float HumClient()
{
  float h = dht.readHumidity(); //get humidity
  int ntry = 0;
  while (String(h) == "nan" && ntry < 5)
  {
    ntry++;
    h = dht.readHumidity();
    delay(1000);
  }
  if (String(h) == "nan")
  {
    h = 200; //If no temperature, set to 200 for the error SMS
  }
  return h;
}
int getBattery()
{
  float b = analogRead(BATTERYPIN); //get battery
  Serial.println(b);
  b = (b - 677) / (840 - 677) * 100; //set in percentage
  if (b > 100)
  {
    b = 100;
  }
  else if (b < 0)
  {
    b = 0;
  }
  Serial.println(b);
  return b;
}

void sendTemp()
{
  delay(500);
  dht.begin();
  delay(500);

  //----Get values-----------
  myData.t = TempClient();
  myData.h = HumClient();
  myData.b = getBattery();

  WifiEspNow.send(PEER, (uint8_t *)&myData, sizeof(myData)); //send to the server
  ESP.deepSleep(timeDeepSleep * 3600000000);                 //deep sleep mode to save battery
}
