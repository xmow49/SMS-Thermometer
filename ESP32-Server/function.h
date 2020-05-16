#include <EEPROM.h>
#include <esp_now.h>
#include <WiFi.h>
#include "DHT.h"
#include <RTClib.h>
#include "config.h"
//-------------------String----------------------
String msgSMSinfos = "";

String strDebug = "";
int TempAlert;
long PhoneAlert;
String strPhoneAlert;

//-----------------Temp---------------------
int TempClient = 200;
int HumClient = 200;
int BatteryClient = 200;


int TempServer()
{
	float t = dht.readTemperature();
	return (int)t;
}
int HumServer()
{
	float h = dht.readHumidity();
	return (int)h;
}

int getBattery()
{
	int b = analogRead(BATTERYPIN);
	Serial.println(b);
	int percent = map(b, 0, 4095, 0, 100);
	return percent;
}

bool alertTempClient()
{
	if (TempClient <= TempAlert && TempClient != 200)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool alertTempServer()
{
	if (TempServer() <= TempAlert && TempServer() != 200)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//----------------Formated Time-------------------
String tmpD = "00";
String tmpMo = "00";
String tmpY = "0000";
String tmpH = "00";
String tmpMi = "00";

DateTime now = rtc.now();

String formatedTime()
{
	now = rtc.now();

	tmpH = now.hour();
	tmpMi = now.minute();

	if (now.hour() <= 9)
	{
		tmpH = "0" + tmpH;
	}
	if (now.minute() <= 9)
	{
		tmpMi = "0" + tmpMi;
	}

	return tmpH + ":" + tmpMi;
}
String formatedDate()
{
	now = rtc.now();
	tmpD = "";
	tmpD = now.day();
	tmpMo = now.month();
	tmpY = now.year();

	if (now.day() <= 9)
	{
		tmpD = "0" + tmpD;
	}
	if (now.month() <= 9)
	{
		tmpMo = "0" + tmpMo;
	}

	return tmpD + "/" + tmpMo + "/" + tmpY;
}
//------------------------ESP-NOW----------------------
typedef struct createMsg
{
	int t;
	int h;
	int b;

} createMsg;

createMsg msg;
String dateTempClient;
String timeTempClient;
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{

	memcpy(&msg, incomingData, sizeof(msg));
	TempClient = msg.t;
	HumClient = msg.h;
	BatteryClient = msg.b;

	dateTempClient = formatedDate();
	timeTempClient = formatedTime();
	Serial.println("Temp recevied from client");
}

void setupEspNow()
{
	WiFi.mode(WIFI_STA);
	if (esp_now_init() != ESP_OK)
	{
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	esp_now_register_recv_cb(OnDataRecv);
	dht.begin();
	EEPROM.begin(10);
	TempAlert = EEPROM.read(0);
}

//---------------------------GSM-----------------------------
String numSMS(String tmp)
{
	tmp.remove(0, 46);
	tmp.remove(12);
	return tmp;
}
String msgSMS(String tmp)
{
	tmp.remove(0, 87);
	tmp.remove(tmp.length() - 8, tmp.length());
	return tmp;
}

String tempSerial, tempGSM, tempSMS;

void sendSms(String num, String msg)
{
	Serial.println("Sending text message...");
	SerialAT.print("AT+CMGF=1\r");
	delay(1000);
	SerialAT.print("AT+CMGS=\"" + num + "\"\r"); 
	delay(100);
	SerialAT.print(msg + char(26) + "\r");
	delay(100);
	Serial.println("Text send");

void clearSMS()
{
	SerialAT.print("AT+CMGD=1,4\r");
}

void startModem()
{
	SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
	Serial.begin(115200);
	Serial.println("Starting Modem...");
	pinMode(MODEM_PWKEY, OUTPUT);
	pinMode(MODEM_RST, OUTPUT);
	pinMode(MODEM_POWER_ON, OUTPUT);
	digitalWrite(MODEM_PWKEY, LOW);
	digitalWrite(MODEM_RST, HIGH);
	digitalWrite(MODEM_POWER_ON, HIGH);
	delay(5000);
	Serial.println("Unlocking SIM...");
	SerialAT.print("AT+CPIN=" + PINSIM + "\r");
	delay(2000);
	Serial.println("Starting SMS Mode...");
	SerialAT.print("AT+CMGF=1\r"); // Lance le mode SMS
	delay(5000);
	Serial.println("Clearing SMS ...");
	clearSMS();
	delay(2000);
	Serial.println("Modem OK");
	rtc.begin();
}
String strTempClient;
String strHumClient;
String strBatteryClient;

String strTempServer;
String strHumServer;
String strBatteryServer;
String tmpCommand;

int setHour;
int setMinute;

int setDate;
int setMonth;
int setYear;
String getArgs(String data, char separator, int index)
{
	int found = 0;
	int strIndex[] = {0, -1};
	int maxIndex = data.length() - 1;

	for (int i = 0; i <= maxIndex && found <= index; i++)
	{
		if (data.charAt(i) == separator || i == maxIndex)
		{
			found++;
			strIndex[0] = strIndex[1] + 1;
			strIndex[1] = (i == maxIndex) ? i + 1 : i;
		}
	}

	return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void sendWeekly()
{
	strTempClient = TempClient;
	strHumClient = HumClient;
	strBatteryClient = BatteryClient;

	strTempServer = TempServer();
	strHumServer = HumServer();
	strBatteryServer = getBattery();

	if (TempClient != 0 && HumClient != 0)
	{
		msgSMSinfos =
			"Infos:\r"
			"Temperature 1: " +
			strTempClient + " C\r"
							"Humidite 1: " +
			strHumClient + "%\r"
						   "Batterie 1: " +
			strBatteryClient + "%\r"
							   "Date 1: " +
			timeTempClient + "\r" + dateTempClient + "\r\r"

													 "Temperature 2: " +
			strTempServer + " C\r"
							"Humidite 2: " +
			strHumServer + "%\r"
						   "Batterie 2: " +
			strBatteryServer + "%\r"

							   "Date 2: " +
			formatedTime() + "\r" + formatedDate();
	}
	else
	{
		msgSMSinfos =
			"Infos:\r"
			"Capteur 1: Aucune Donnes.\r\r"

			"Temperature 2: " +
			strTempServer + " C\r"
							"Humidite 2: " +
			strHumServer + "%\r"
						   "Batterie 2: " +
			strBatteryServer + "%\r"

							   "Date 2: " +
			formatedTime() + "\r" + formatedDate();
	}

	EEPROM.get(2, PhoneAlert);	
	strPhoneAlert = "+33" + String(PhoneAlert).substring(0);

	sendSms(strPhoneAlert, msgSMSinfos);
}

void sendInfo()
{
	strTempClient = TempClient;
	strHumClient = HumClient;
	strBatteryClient = BatteryClient;

	strTempServer = TempServer();
	strHumServer = HumServer();
	strBatteryServer = getBattery();

	if (TempClient != 200 && HumClient != 200)
	{
		msgSMSinfos =
			"Infos:\r"
			"Temperature 1: " +
			strTempClient + " C\r"
							"Humidite 1: " +
			strHumClient + "%\r"
						   "Batterie 1: " +
			strBatteryClient + "%\r"
							   "Date 1: " +
			timeTempClient + "\r" + dateTempClient + "\r\r"

													 "Temperature 2: " +
			strTempServer + " C\r"
							"Humidite 2: " +
			strHumServer + "%\r"
						   "Batterie 2: " +
			strBatteryServer + "%\r"

							   "Date 2: " +
			formatedTime() + "\r" + formatedDate();
	}
	else
	{
		msgSMSinfos =
			"Infos:\r"
			"Capteur 1: Aucune Donnes\r\r"

			"Temperature 2: " +
			strTempServer + " C\r"
							"Humidite 2: " +
			strHumServer + "%\r"
						   "Batterie 2: " +
			strBatteryServer + "%\r"

							   "Date 2: " +
			formatedTime() + "\r" + formatedDate();
	}

	sendSms(numSMS(tempSMS), msgSMSinfos);
}

void loopGSM()
{
	
	if (SerialAT.available())
	{
		tempGSM = SerialAT.readString();
		//Serial.println(tempGSM); //Debug
		if (tempGSM.indexOf("+CMTI: \"SM\"") > 0)
		{
			Serial.println("Message recu");
			SerialAT.print("AT+CMGL=\"REC UNREAD\"\r");
		}
		if (tempGSM.indexOf("+CMGL: 1,\"REC UNREAD\"") > 0)
		{
			Serial.println("Lecture du message...");
			tempSMS = tempGSM;
			clearSMS();
			delay(1000);

			tmpCommand = msgSMS(tempSMS);
			Serial.println("Message recu: " + tmpCommand);

			//-------------------MSG Restart-------------------------------
			if (tmpCommand == "restart")
			{
				sendSms(numSMS(tempSMS), "OK, Restarting...");
				delay(5000);
				ESP.restart();
			}
			//----------------------MSG TEMP------------------------------------------------------------------------
			else if (tmpCommand == "t" || tmpCommand == "T" || tmpCommand == "temperature" || tmpCommand == "Temperature" || tmpCommand == "Temperatures" || tmpCommand == "temperatures")
			{
				sendInfo();
			}
			//------------------------MSG SET TEMP--------------------------------------
			
			else if (tmpCommand.indexOf("reglage-temp") != -1)
			{
				if (tmpCommand.length() > 12)
				{
					tmpCommand.remove(0, 13);
					if ((TempAlert = getArgs(tmpCommand, ' ', 0).toInt()))
					{
						Serial.println("Temp Alert set");
						EEPROM.write(0, TempAlert);
						EEPROM.commit();
						delay(200);

						msgSMSinfos = "Changement Reussi, \nNouvelle Temperature:  " + String(TempAlert) + "C";
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
					else
					{
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Alert de Temperature: " +
							TempAlert;
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-temp <temperature>\r"
						"Remplacez <temperature> par \r"
						"la temperature d'alert voulue.";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}
			//---------------------------SET TEL------------------------------
			else if (tmpCommand.indexOf("reglage-tel") != -1)
			{
				if (tmpCommand.length() > 11)
				{
					tmpCommand.remove(0, 12);
					if ((PhoneAlert = getArgs(tmpCommand, ' ', 0).toInt()))
					{
						Serial.println("Phone number Alert set");
						
						Serial.println(PhoneAlert);

						EEPROM.put(2, PhoneAlert);
						EEPROM.commit();
						delay(200);

						EEPROM.get(2, PhoneAlert);	
						strPhoneAlert = "0" + String(PhoneAlert);
						msgSMSinfos = "Changement Reussi, \nNouveau Numero d'alerte: " + strPhoneAlert;
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
					else
					{
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Numero d'alerte: " +
							String(PhoneAlert);
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-tel <numero de tel>\r"
						"Remplacez <numero de tel> par \r"
						"le numero de tel contenant 10 chiffres";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}

			//------------------------------MSG SETDATE---------------------------------------------
			else if (tmpCommand.indexOf("reglage-date") != -1)
			{
				if (tmpCommand.length() > 12)
				{
					tmpCommand.remove(0, 13);
					if ((setDate = getArgs(tmpCommand, ' ', 0).toInt()) and (setMonth = getArgs(tmpCommand, ' ', 1).toInt()) and (setYear = getArgs(tmpCommand, ' ', 2).toInt()))
					{
						Serial.println("Date is set");

						now = rtc.now();
						rtc.adjust(DateTime(setYear, setMonth, setDate, now.hour(), now.minute(), 0));
						msgSMSinfos =
							"Changement Reussi\r"
							"Nouvelle Date: " +
							formatedTime() + "\r" + formatedDate();
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
					else
					{
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Heure Actuel :" +
							formatedTime() + "\r" + formatedDate();
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-date <jour> <mois> <annee>\r"
						"Remplacez <jour> <mois> <annee> par \r"
						"la date actuel.";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}
			//------------------------------------MSH SETHOUR-------------------------------
			else if (tmpCommand.indexOf("reglage-heure") != -1)
			{
				if (tmpCommand.length() > 13)
				{
					tmpCommand.remove(0, 14);
					if ((setHour = getArgs(tmpCommand, ' ', 0).toInt()) and (setMinute = getArgs(tmpCommand, ' ', 1).toInt()))
					{

						Serial.println("Hour is set");

						now = rtc.now();
						rtc.adjust(DateTime(now.year(), now.month(), now.day(), setHour, setMinute, 0));
						msgSMSinfos =
							"Changement Reussi\r"
							"Nouvelle Heure: " +
							formatedTime() + "\r" + formatedDate();
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
					else
					{
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Heure Actuel :" +
							formatedTime() + "\r" + formatedDate();
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-heure <heure> <minutes>\r"
						"Remplacez <heure> et <minutes> par \r"
						"l'heure actuel.";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}

			else
			{
				msgSMSinfos =
					"Thermometre GMS\r"
					"Fabrique par Dorian\r"
					"Version: 1.0 (04/2020)\r"
					"Heure: " +
					formatedTime() + "\r"
									 "Date: " +
					formatedDate() + "\r"
									 "Temperature actuel: " +
					TempServer() + " C\r"
								   "Plus d'info: repondez \"t\" \r"
								   "Reglage de l'heure:\r"
								   		"    repondez \"reglage-heure\"\r" 
								   
								   "Reglage de la date:\r"
								   		"    repondez \"reglage-date\"\r"
										   
								   "Reglage de la temperature d'alerte:\r"
								   		"    repondez \"reglage-temp\"\r"

								   "Reglage du numero de telephone:\r"
								   		"    repondez \"reglage-tel\"";
				sendSms(numSMS(tempSMS), msgSMSinfos);
			}
		}
	}
}

void autoReboot()
{
	now = rtc.now();
	if (now.hour() == 0 && now.minute() == 0)
	{
		Serial.println("AutoReboot");
		ESP.restart();
	}
}

void weeklySMS()
{
	now = rtc.now();
	//6
	if (now.dayOfTheWeek() == 6 && now.hour() == 18 && now.minute() == 0)
	{
		sendWeekly();
	}
}

String msgAlertTemp;
long previousMillis = 0;
long interval = 60 * 1000;
bool smsAlertSend = false;
void loopTemp()
{

	weeklySMS();
	autoReboot();
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis > interval)
	{
		previousMillis = currentMillis;
		Serial.print("Check Temp: ");
		Serial.println(TempAlert);

		if (smsAlertSend == 0)
		{

			if (alertTempClient() || alertTempServer())
			{
				strTempClient = TempClient;
				strHumClient = HumClient;
				strBatteryClient = BatteryClient;

				strTempServer = TempServer();
				strHumServer = HumServer();
				strBatteryServer = getBattery();

				if (TempClient != 200 && HumClient != 200)
				{
					msgAlertTemp =
						"ALERT DE TEMPERATURE:\r"
						"Temperature 1: " +
						strTempClient + " C\r"
										"Humidite 1: " +
						strHumClient + "%\r"
									   "Batterie 1: " +
						strBatteryClient + "%\r"
										   "Date 1: " +
						timeTempClient + "\r" + dateTempClient + "\r\r"

						"Temperature 2: " +
						strTempServer + " C\r"
										"Humidite 2: " +
						strHumServer + "%\r"
									   "Batterie 2: " +
						strBatteryServer + "%\r"

										   "Date 2: " +
						formatedTime() + "\r" + formatedDate();
				}
				else
				{
					msgAlertTemp =
						"ALERT DE TEMPERATURE:\r"
						"Capteur 1: Aucune Donnes\r\r"

						"Temperature 2: " +
						strTempServer + " C\r"
										"Humidite 2: " +
						strHumServer + "%\r"
									   "Batterie 2: " +
						strBatteryServer + "%\r"

										   "Date 2: " +
						formatedTime() + "\r" + formatedDate();
				}

				EEPROM.get(2, PhoneAlert);	
				strPhoneAlert = "+33" + String(PhoneAlert).substring(0);

				sendSms(strPhoneAlert, msgAlertTemp);
				smsAlertSend = true;
			}else{
				smsAlertSend = false;
			}


		}

		else
		{
			Serial.println("SMS already send");
		}
	}
}
