//--------------PIN----------------------
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

#define DHTPIN 15

#define BATTERYPIN 36 //Analog

//--------------Config----------------------

#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

char daysOfTheWeek[7][12] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};


const String PINSIM = "****";
//---------------------------------------------

#define SerialAT  Serial1

RTC_DS3231 rtc;
