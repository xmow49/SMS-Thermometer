//--------------PIN----------------------
//---------SIM800L PINS------------
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

//-----------other pins----------------
#define SerialAT  Serial1  //Serial for SIM800L

#define DHTPIN 15

#define BATTERYPIN 36 //Analog
const int BatteryMin = 2767; //Min Value
const int BatteryMax = 3647; //Max Value

#define BUTTONPIN 12

#define POWERPIN 33 //Input 5v of usb

//------------------OLED SCREEN-----------------

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//--------------Config----------------------

#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

char daysOfTheWeek[7][12] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};


const String PINSIM = "1234"; //CHANGE SIM PIN CODE!!

RTC_DS3231 rtc; //set RTC Module
//---------------------Variables----------------

String dateTempClient; //the date when receive temp from client
String timeTempClient; //the time when receive temp from client

String tmpCommand; //text of received SMS

String dateGSM, hourGSM; //date/time text from SIM800L
String tmpRTCMessage; //message of SIM800L for get date/time

String msgSMSinfos; //temporary SMS for send it

String strPhoneAlert; //phone numbre for temperature alert
String msgAlertTemp; //temporary SMS alert for send it

String tempGSM, tempSMS; //raw message from SIM800L

String tempSignalQuality;
String tempOperator;

float TempClient = 200; //temperature from client : 200 = no temperature received
float HumClient = 200; //humidity from client
int BatteryClient = 200; //battery from client

int smsInfoHour; //get from EEPROM, its the hour when the info sms will send
int smsInfoDay;  //get from EEPROM, its the day of week when the info sms will send

int TempAlert; //temperature threshold for alert
long PhoneAlert; //phone number for alert

const long unsigned intervalCheckHour = 60000; // 60 000 ms -> 1min : interval for check the hour
unsigned long previousMillisCheckHour = 0;

const long unsigned intervalCheckTemp = 1800000; // 1 800 000ms -> 30min : interval for check temp for the alert
unsigned long previousMillisCheckTemp = 0;

bool smsAlertSend = false; //temporary value for sms
bool BatteryAlert = false; //temporary value for sms
