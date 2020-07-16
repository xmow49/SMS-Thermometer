#include <EEPROM.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <SPI.h>
#include "DHT.h"
#include <RTClib.h>
#include "config.h"
#include "bitmap.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

String getArgs(String data, char separator, int index)
{
	//Function for separate String with special characters
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

//-----------------Temp---------------------
float TempServer()
{
	float t = dht.readTemperature(); //get temperature
	int ntry = 0;
	while(isnan(t) && ntry < 3){//try to get temperature
		ntry ++;
		t = dht.readTemperature();
		delay(500);
	}
	if(isnan(t)){
		t = 200; //If no temperature, set to 200 for the error SMS
	}
	return t;
}
float HumServer()
{
	float h = dht.readHumidity(); //get humidity
	int ntry = 0;
	while(isnan(h) && ntry < 3){//try to get humidity
		ntry ++;
		h = dht.readHumidity();
		delay(500);
	}
	if(isnan(h)){
		h = 200; //If no humidity, set to 200 for the error SMS
	}
	return h;
}
int getBattery()
{
	float tempValue = 0;
	for (int n = 0; n < 10; n++)
	{
		tempValue += analogRead(BATTERYPIN);
		delay(10);
	}
	tempValue = tempValue / 10;

	float b = ((tempValue - BatteryMin) / (BatteryMax - BatteryMin)) * 100; //mettre en pourcentage
	if (b > 100) //max is 100%
		b = 100;

	else if (b < 0) //min is 0%
		b = 0;
	return b;
}

bool alertTempClient()
{
	//Check if the temperature is smaller than threshold
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
	//Check if the temperature is smaller than threshold
	if (TempServer() <= TempAlert && TempServer() != 200)
	{
		return true;
	}
	else
	{
		return false;
	}
}

int getSignal(){
	//get GSM signal but without percentage
	SerialAT.readString(); //clear the Serial
	SerialAT.println("AT+CSQ"); //ask the SIM800L the GSM signal
	delay(1000);
	tempSignalQuality = SerialAT.readString(); //read response
	tempSignalQuality.remove(0,15); //remove the beginning of the raw message
	tempSignalQuality.remove(2); ////remove the end of the raw message
	int s = tempSignalQuality.toInt(); //convert to int
	return s; // 0-31
}
int getSignalQuality(){
	SerialAT.readString(); //clear the Serial
	SerialAT.println("AT+CSQ");
	delay(2000);
	tempSignalQuality = SerialAT.readString();
	tempSignalQuality.remove(0,15);
	tempSignalQuality.remove(2);
	float s = tempSignalQuality.toInt();
	s = ((s - 0) / (31 - 0)) * 100; //set in percentage
	return s; //0-100
}
//---------------Formatted Time--------------------
void displayBoot(int state)
{
	//setup display to write
	display.clearDisplay();
	display.setFont(&FreeSans9pt7b);
	display.setTextSize(1); 
	display.setTextColor(WHITE);
	//print message depending the state of boot
	switch (state)
	{
	case 0:
		display.setCursor(20,37);
		display.println("Demarrage");
		display.display();
		break;
	case 1:
		display.setCursor(20,27);
		display.println("Demarrage");
		display.setCursor(25,47);
		display.println("de la SIM");
		display.display();
		break;
	case 2:
		display.setCursor(50,37);
		display.println("OK");
		display.display();
		break;
	case 3:
		display.setCursor(30,27);
		display.println("Reglage");
		display.setCursor(25,47);
		display.println("de l\'heure...");
		display.display();
		break;
	case 4:
		display.setCursor(5,27);
		display.println("Demarrage du");
		display.setCursor(20,47);
		display.println("mode SMS");
		display.display();
		break;
	case 5:
		display.setCursor(5,27);
		display.println("SMS en cours");
		display.setCursor(25,47);
		display.println("d'envoi ...");
		display.display();
		break;
	case 6:
		display.setCursor(10,37);
		display.println("Redemarrage");
		display.display();
		break;
	case 7:
		display.setCursor(5,27);
		display.println("Probleme lors");
		display.setCursor(5,47);
		display.println("du demarrage");
		display.display();
	default:
		break;
	}
}

void setRTC_GSM()
{
	displayBoot(3);
	SerialAT.readString(); //clear serial
	SerialAT.println("AT+CCLK?"); //ask the time
	delay(100);//wait response 
	tmpRTCMessage = SerialAT.readString(); //get response
	tmpRTCMessage.remove(0, 19); //remove the beginning of the message
	tmpRTCMessage.remove(17); //remove the end of the message

	dateGSM = getArgs(tmpRTCMessage, ',', 0); //get full date message : ex: 01/02/2020
	hourGSM = getArgs(tmpRTCMessage, ',', 1); //get full time message : ex: 18:32

	int setYear = getArgs(dateGSM, '/', 0).toInt() + 2000; //convert and set year
	int setMonth = getArgs(dateGSM, '/', 1).toInt(); //convert and set month
	int setDate = getArgs(dateGSM, '/', 2).toInt(); //convert and set date

	int setHour = getArgs(hourGSM, ':', 0).toInt(); //convert and set hour
	int setMinute = getArgs(hourGSM, ':', 1).toInt(); //convert and set minute
	int setSecond = getArgs(hourGSM, ':', 2).toInt(); //convert and set second

	if(setMonth == 1 && setDate == 1 && setYear == 2004){
		Serial.println("GSM ERROR");
	}
	else{
		rtc.adjust(DateTime(setYear, setMonth, setDate, setHour, setMinute, setSecond)); //save all in the RTC module
		Serial.println("RTC is updated by GSM"); //display in Serial
		//Serial.println(formattedTime());
		//Serial.println(formattedDate());
	}

	displayBoot(2);
}

String formattedTime()
{
	rtc.begin();
	DateTime now = rtc.now(); //get time
	delay(10);
	int tmpH = now.hour(); //set temp variable
	delay(10);
	int tmpMi = now.minute(); //set temp variable
	
	if (String(tmpH) == "165")
	{
		//RTC ERROR
		return "ERREUR";
	}
	else
	{
		if(tmpH > 23 || tmpH < 0 || tmpMi > 59 || tmpH < 0){
			setRTC_GSM();
		}
		String strTmpH, strTmpMi; //String for the return

		if (tmpH <= 9)
			strTmpH = "0" + String(tmpH); //add a '0' for 0-9 hours
		else
			strTmpH = tmpH;

		if (tmpMi <= 9)
			strTmpMi = "0" + String(tmpMi); //add a '0' for 0-9 minutes
		else
			strTmpMi = tmpMi;

		return strTmpH + ":" + String(strTmpMi); //return with layout
	}
}
String formattedDate()
{
	rtc.begin();
	DateTime now = rtc.now(); //get time
	delay(10);
	int tmpD = now.day(); //set temp variable
	delay(10);
	int tmpMo = now.month(); //set temp variable
	delay(10);
	int tmpY = now.year(); //set temp variable

	if (String(tmpD) == "165")
	{
		//RTC ERROR
		return "ERREUR";
	}
	else
	{
		if(tmpD > 31 || tmpD < 0 || tmpMo > 12 || tmpMo < 1){
			setRTC_GSM();
		}
		String strTmpD, strTmpMo; //String for the return

		if (tmpD <= 9)
			strTmpD = "0" + String(tmpD); //add a '0' for 0-9 days
		else
			strTmpD = tmpD;

		if (tmpMo <= 9)
			strTmpMo = "0" + String(tmpMo); //add a '0' for 0-9 months
		else
			strTmpMo = tmpMo;

		return strTmpD + "/" + strTmpMo + "/" + tmpY; //return with layout
	}
}

void sendSms(String num, String msg)
{
	//Send an SMS
	Serial.println("Sending text message...");
	SerialAT.print("AT+CMGF=1\r"); // start SMS mode
	delay(500); //wait the sim800l
	SerialAT.print("AT+CMGS=\"" + num + "\"\r"); //send the phone number
	delay(100);
	SerialAT.print(msg + char(26) + "\r"); //send message and 'char(26)' for the end of message
	delay(100);
	Serial.println("Text send"); //show in serial
}
void sendWeekly()
{
	//It s the function who send automatically sms
	//----------convert Int to String for the SMS-------------
	String strTempClient = String(TempClient); 
	strTempClient.remove(4);
	String strHumClient = String(HumClient);
	strHumClient.remove(4);
	String strBatteryClient = String(BatteryClient);

	volatile float tmpValue = TempServer();
	String strTempServer = String(tmpValue);
	strTempServer.remove(4);//keep one digit after the decimal point
	String strHumServer = String(HumServer());
	strHumServer.remove(4);//keep one digit after the decimal point
	String strBatteryServer = String(getBattery());
	//--------------------------------------------------------
	if (TempClient != 200 && HumClient != 200 && tmpValue != 200)
	{
		//If we have all temperature
		msgSMSinfos =
			"Rapport des Temperatures\r"
			"Temperature A: " + strTempClient + " C\r"
			"Humidite A: " + strHumClient + "%\r"
			"Batterie A: " + strBatteryClient + "%\r"			
			"Date A: " + timeTempClient + "\r" + dateTempClient + "\r\r"

			"Temperature B: " + strTempServer + " C\r"
			"Humidite B: " + strHumServer + "%\r"
			"Batterie B: " + strBatteryServer + "%\r"
			"Date B: " + formattedTime() + "\r" + formattedDate();
	}
	else if (TempClient == 200 && HumClient == 200 && tmpValue != 200)
	{
		//If we dont have the temp of client
		msgSMSinfos =
			"Rapport des Temperatures:\r"
			"Capteur A: Aucune Donnee\r\r"

			"Temperature B: " + strTempServer + " C\r"
			"Humidite B: " + strHumServer + "%\r"
			"Batterie B: " + strBatteryServer + "%\r"
			"Date B: " + formattedTime() + "\r" + formattedDate();
	}
	else if (TempClient != 200 && HumClient != 200 && tmpValue == 200)
	{
		//If we have temp only from client
		msgSMSinfos =
			"Rapport des Temperatures:\r"
			"Temperature A: " + strTempClient + " C\r"
			"Humidite A: " + strHumClient + "%\r"
			"Batterie A: " + strBatteryClient + "%\r"
			"Date A: " + timeTempClient + "\r" + dateTempClient + "\r\r"

			"Temperature B: \r" +
			"Aucune Donnee\r";
	}
	else
	{
		//If we have no temperature
		msgSMSinfos =
			"Rapport des Temperatures:\r"
			"Erreur de comuication\r"
			"avec les thermometres\r"
			"Date: \r" + formattedTime() + "\r" + formattedDate();
	}
	EEPROM.get(4, PhoneAlert); //get phone number from EEPROM
	strPhoneAlert = "+33" + String(PhoneAlert).substring(0); //structure the phone number, CHANGE '+33' BY THE YOUR CONTRY CODE
	sendSms(strPhoneAlert, msgSMSinfos); //send the SMS
}

//---------------Screen--------------------

void setupScreen(){
	//if oled screen is ok
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
		Serial.println("Display ERROR");
}

void displayTemp()
{
	previousMillisCheckHour = millis();
	//setup display to write
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(1);
	display.setFont(&FreeSansBold12pt7b);

	volatile float tmpValue = TempServer();
	if (tmpValue != 200)
	{
		//get Temperature
		String strTempServer = String(tmpValue);
		strTempServer.remove(4);
		//print it
		display.setCursor(0, 20);
		display.println(strTempServer + " C");
		display.drawCircle(49, 5, 2, WHITE);
	}
	else
	{
		//ERROR
		display.setCursor(0, 20);
		display.println(".... C");
		display.drawCircle(49, 5, 2, WHITE);
	}

	tmpValue = HumServer();
	if (tmpValue != 200)
	{
		//get Humidity
		String strHumServer = String(tmpValue);
		strHumServer.remove(4);
		//print it
		display.setCursor(0, 40);
		display.println(strHumServer + " %");
	}
	else
	{
		//ERROR
		display.setCursor(0, 40);
		display.println(".... %");
	}

	//get battery level and display bitmap depending the level
	int battery = getBattery();
	if(battery > 75){
		display.drawBitmap(88, 0, BattFull, BattHeight, BattWidth, WHITE);
	}
	else if(battery > 50 && battery <= 75){
		display.drawBitmap(88, 0, Batt75, BattHeight, BattWidth, WHITE);
	}
	else if(battery > 25 && battery <= 50){
		display.drawBitmap(88, 0, Batt50, BattHeight, BattWidth, WHITE);
	}
	else if(battery > 10 && battery <= 25){
		display.drawBitmap(88, 0, Batt25, BattHeight, BattWidth, WHITE);
	}
	else{
		display.drawBitmap(88, 0, Batt0, BattHeight, BattWidth, WHITE);
	}
	
	//get signal level and display bitmap depending the level
	int signal = getSignal();
	if(signal > 26){
		display.drawBitmap(88, 20, SignalFull, SignalHeight, SignalWidth, WHITE);
	}
	else if(signal > 21 && signal <= 26){
		display.drawBitmap(88, 20, Signal4, SignalHeight, SignalWidth, WHITE);
	}
	else if(signal > 16 && signal <= 21){
		display.drawBitmap(88, 20, Signal3, SignalHeight, SignalWidth, WHITE);
	}
	else if(signal > 11 && signal <= 16){
		display.drawBitmap(88, 20, Signal2, SignalHeight, SignalWidth, WHITE);
	}	
	else if(signal > 6 && signal <= 11){
		display.drawBitmap(88, 20, Signal1, SignalHeight, SignalWidth, WHITE);
	}else{
		display.drawBitmap(88, 20, Signal0, SignalHeight, SignalWidth, WHITE);
	}
	
	//setup display for write
	display.setFont(&FreeSans9pt7b);
	display.setTextSize(1); 
	display.setTextColor(WHITE);
	
	//get date
	display.setCursor(0, 60);
	String shortDate = formattedDate();
	shortDate.remove(8,10); //keep 2 didgit of year : 2020 -> 20
	display.println(shortDate);

	//print time
	display.setCursor(80, 60);
	display.println(formattedTime());
	display.display();
}

void checkButton(){
	if(digitalRead(BUTTONPIN)){//When button is press
		volatile int time = 0;
		while(digitalRead(BUTTONPIN)){
			time++;
			delay(10);
		}
		if(time < 200){
			Serial.println("Button pressed"); //debug
			//setup display for write
			display.clearDisplay();
			display.setFont(&FreeSans9pt7b);
			//print message
			display.setCursor(10,27);
			display.println("Recuperation");
			display.setCursor(10,47);
			display.println("des valeurs ...");
			display.display();
			//display temp valules
			displayTemp();
		}else if(time >= 200 && time < 500){
			displayBoot(5);
			delay(500);
			sendWeekly();
			delay(5000);
			displayTemp();
		}else if (time > 500){
			displayBoot(6);
			delay(1000);
			ESP.restart();
		}
	}
}

void turnoffDisplay(){
	display.clearDisplay();
	display.display();
}
//------------------------ESP-NOW----------------------
typedef struct createMsg
{
	//structure the received message
	float t; //temperature
	float h; //humidity
	int b; //battery

} createMsg;

createMsg msg;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
	memcpy(&msg, incomingData, sizeof(msg));
	TempClient = msg.t;
	HumClient = msg.h;
	BatteryClient = msg.b;

	dateTempClient = formattedDate();
	timeTempClient = formattedTime();
	Serial.println("Temp recevied from client:");
	Serial.println(TempClient);
	Serial.println(HumClient);
	Serial.println(BatteryClient);
}

void setupEspNow()
{
	WiFi.mode(WIFI_STA); //setup wifi un access point
	if (esp_now_init() != ESP_OK) //ERROR
	{
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	esp_now_register_recv_cb(OnDataRecv); //set the function
	dht.begin(); //setup DHT22
	pinMode(BUTTONPIN, INPUT); //Button pin
	pinMode(POWERPIN, INPUT); //Power usb pin
	rtc.begin(); //setup RTC module
	//---------------EEPROM SETTINGS------------
	EEPROM.begin(10);
	TempAlert = EEPROM.read(0);
	smsInfoHour = EEPROM.read(1);
	smsInfoDay = EEPROM.read(2);
	EEPROM.get(4, PhoneAlert);

	if(digitalRead(POWERPIN) == 0)
		BatteryAlert = true;
}

//---------------------------GSM-----------------------------
String numSMS(String tmp)
{
	//get the phone number of the sender of the SMS, from the serial message of SIM800l 
	tmp.remove(0, 46);
	tmp.remove(12);
	return tmp;
}
String msgSMS(String tmp)
{
	//Get content of the SMS
	tmp.remove(0, 87);
	tmp.remove(tmp.length() - 8, tmp.length());
	return tmp;
}


void clearSMS()
{
	SerialAT.print("AT+CMGD=1,4\r"); // clear all SMS on the SIM800L
}

String getOperator(){
	SerialAT.readString();
	SerialAT.println("AT+COPS?");
	delay(2000);
	tempOperator = SerialAT.readString();
	tempOperator.remove(0,23);
	tempOperator = getArgs(tempOperator, '\"', 0);
	return tempOperator;

}

void startModem()
{
	SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX); //setup Serial to the SIM800L
	Serial.begin(115200); //setup serial to the USB
	Serial.println("Starting Modem...");
	displayBoot(0);
	//---------Power the SIM800L module------------
	pinMode(MODEM_PWKEY, OUTPUT);
	pinMode(MODEM_RST, OUTPUT);
	pinMode(MODEM_POWER_ON, OUTPUT);
	
	digitalWrite(MODEM_PWKEY, LOW);
	digitalWrite(MODEM_RST, HIGH);
	digitalWrite(MODEM_POWER_ON, HIGH);
	//--------------------------------------------
	delay(5000); //wait start of the SIM800L
	Serial.println("Unlocking SIM...");
	displayBoot(1);
	SerialAT.print("AT+CPIN=" + PINSIM + "\r"); //unlock sim with pin
	delay(2000);
	Serial.println("Starting SMS Mode...");
	displayBoot(4);
	SerialAT.print("AT+CMGF=1\r"); // Start SMS
	delay(5000);
	Serial.println("Clearing SMS ...");
	clearSMS(); //Clear SMS
	delay(2000);
	Serial.println("Modem OK"); //send ok to computer
	displayBoot(2);
	delay(200);
	
}

void sendInfo()
{
	//It s the function who send temperature and humidity sms
	//----------convert Int to String for the SMS-------------
	String strTempClient = String(TempClient); 
	strTempClient.remove(4);//keep one digit after the decimal point
	String strHumClient = String(HumClient);
	strHumClient.remove(4);//keep one digit after the decimal point
	String strBatteryClient = String(BatteryClient);

	volatile float tmpValue = TempServer();
	String strTempServer = String(tmpValue);
	strTempServer.remove(4);//keep one digit after the decimal point
	String strHumServer = String(HumServer());
	strHumServer.remove(4);//keep one digit after the decimal point
	String strBatteryServer = String(getBattery());

	String strFormattedTime = formattedTime();
	String strFormattedDate = formattedDate();
	//--------------------------------------------------------

	if (TempClient != 200 && HumClient != 200 && tmpValue != 200)
	{
		//If we have all temperature
		msgSMSinfos =
			"Infos:\r"
			"Temperature A: " + strTempClient + " C\r"
			"Humidite A: " + strHumClient + "%\r"
			"Batterie A: " + strBatteryClient + "%\r"
			"Date A: " + timeTempClient + "\r" + dateTempClient + "\r\r"

			"Temperature B: " + strTempServer + " C\r"
			"Humidite B: " + strHumServer + "%\r"
			"Batterie B: " + strBatteryServer + "%\r"
			"Date B: " + strFormattedTime + "\r" + strFormattedDate;
	}
	
	else if (TempClient == 200 && HumClient == 200 && tmpValue != 200)
	{
		//If we dont have the temp of client
		msgSMSinfos =
			"Infos:\r"
			"Capteur A: Aucune Donnee\r\r"

			"Temperature B: " + strTempServer + " C\r"
			"Humidite B: " + strHumServer + "%\r"
			"Batterie B: " + strBatteryServer + "%\r"
			"Date B: " + strFormattedTime + "\r" + strFormattedDate;
	}
	else if (TempClient != 200 && HumClient != 200 && tmpValue == 200)
	{
		//If we have temp only from client
		msgSMSinfos =
			"Infos:\r"
			"Temperature A: " + strTempClient + " C\r"
			"Humidite A: " + strHumClient + "%\r"
			"Batterie A: " + strBatteryClient + "%\r"
			"Date A: " + timeTempClient + "\r" + dateTempClient + "\r\r"

			"Temperature B: \r" +
			"Aucune Donnee\r";
	}	
	else
	{
		//If we have no temperature
		msgSMSinfos =
			"Infos:\r"
			"Erreur de comuication\r"
			"avec les thermometres"
			"Date: \r" + strFormattedTime + "\r" + strFormattedDate;
	}
	sendSms(numSMS(tempSMS), msgSMSinfos); //send the SMS with the message
}

void loopGSM()
{
	if (SerialAT.available()) //when SIM800L send Serial message
	{
		tempGSM = SerialAT.readString(); //save the message
		//Serial.println(tempGSM); //Debug the Serial of SIM800L
		if (tempGSM.indexOf("+CMTI: \"SM\"") > 0) //check if is a SMS
		{
			Serial.println("Message recu");
			SerialAT.print("AT+CMGL=\"REC UNREAD\"\r"); //ask SMS
		}
		if (tempGSM.indexOf("+CMGL: 1,\"REC UNREAD\"") > 0) // if we have recived a SMS
		{
			Serial.println("Lecture du message...");
			tempSMS = tempGSM; //save the response
			//setRTC_GSM();
			clearSMS(); //clear SMS
			tmpCommand = msgSMS(tempSMS); //get the content of the SMS
			
			tmpCommand = getArgs(tmpCommand,'+',0);
			tmpCommand.trim();

			tmpCommand.toLowerCase(); //set all SMS in lower case
			Serial.println("Message recu: " + tmpCommand); //show in serial

			//-------------------MSG Restart-------------------------------
			if (tmpCommand == "restart" || tmpCommand == "reboot")
			{
				//restart the ESP
				sendSms(numSMS(tempSMS), "OK, Restarting...");
				delay(1000);
				ESP.restart();
			}
			//----------------------MSG TEMP-------------------------------
			else if (tmpCommand == "t" || tmpCommand == "temperature" || tmpCommand == "temperatures" || tmpCommand == "info" || tmpCommand == "infos")
			{
				sendInfo(); //send temperature infos
			}
			//----------------------MSG Hello-------------------------------
			else if (tmpCommand == "bonjour" || tmpCommand == "coucou" || tmpCommand == "cc") //hello message
			{
				sendSms(numSMS(tempSMS), "Bonjour, BIP BIP\rJe suis un thermometre");
			}
			//------------------------MSG SET TEMP--------------------------------------
			else if (tmpCommand.indexOf("reglage-temp") != -1)
			{
				if (tmpCommand.length() > 12) //if we have an argument
				{
					tmpCommand.remove(0, 13); //remove the beginning of the SMS
					if ((TempAlert = getArgs(tmpCommand, ' ', 0).toInt())) //convert the temperature to int
					{
						Serial.println("Temp Alert set");
						EEPROM.write(0, TempAlert); //save in the EEPROM
						EEPROM.commit();
						delay(200);

						msgSMSinfos = "Changement Reussi, \nNouvelle Temperature: " + String(TempAlert) + "C"; 
						sendSms(numSMS(tempSMS), msgSMSinfos);//send an SMS confimation
					}
					else
					{
						//error, the String to int return an error
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Alert de Temperature: " +
							TempAlert;
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					// we dont have a valid argument
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-temp <temperature>\r"
						"Exemple:\r"
						"reglage-temp 5";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}
			//---------------------------SET TEL------------------------------
			else if (tmpCommand.indexOf("reglage-tel") != -1)
			{
				
				if (tmpCommand.length() > 11) // if we have an argument
				{
					tmpCommand.remove(0, 12); //remove the beginning of the SMS
					if ((PhoneAlert = getArgs(tmpCommand, ' ', 0).toInt())) //convert to long
					{
						Serial.println("Phone number Alert set");

						Serial.println(PhoneAlert);

						EEPROM.put(4, PhoneAlert); //save the phone number alert
						EEPROM.commit();
						delay(200);

						EEPROM.get(4, PhoneAlert);
						strPhoneAlert = "0" + String(PhoneAlert);
						msgSMSinfos = "Changement Reussi, \nNouveau Numero d'alerte: " + strPhoneAlert; //send the sms
						sendSms(numSMS(tempSMS), msgSMSinfos);
						delay(5000);
					
						EEPROM.get(4, PhoneAlert); //get phone number from EEPROM
						strPhoneAlert = "+33" + String(PhoneAlert).substring(0); //structure the phone number, CHANGE '+33' BY THE YOUR CONTRY CODE
						msgSMSinfos = "Le 0" + String(PhoneAlert) + " est maintenant le numero par defaut pour les alertes hebdomadaires";
						sendSms(strPhoneAlert, msgSMSinfos); //send the SMS
					}
					else
					{
						//error, the String to long return an error
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Numero d'alerte: " +
							String(PhoneAlert);
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					// we dont have a valid argument
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Reglage du destinataire des alertes"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-tel <numero de tel>\r"
						"Exemple:\r"
						"reglage-tel 0601020304";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}
			//------------------------------MSG SETDATE---------------------------------------------
			else if (tmpCommand.indexOf("reglage-date") != -1)
			{
				if (tmpCommand.length() > 12) //if we have an argument
				{
					tmpCommand.remove(0, 13);//remove the beginning of the SMS
					int setDate; //temporary value of date
					int setMonth; //temporary value of month
					int setYear; //temporary value of year
					if ((setDate = getArgs(tmpCommand, ' ', 0).toInt()) && (setMonth = getArgs(tmpCommand, ' ', 1).toInt()) and (setYear = getArgs(tmpCommand, ' ', 2).toInt()))
					{
						if (setDate > 31 || setDate < 1 || setMonth > 12 || setMonth < 1 || setYear < 2000 || setYear > 2200)
						{
							//if we have not valid date
							msgSMSinfos =
								"Une Erreur c'est produite :-(\r"
								"Heure Actuel :" +
								formattedTime() + "\r" + formattedDate();
							sendSms(numSMS(tempSMS), msgSMSinfos);
						}
						else
						{
							//if its is ok
							Serial.println("Date is set");
							DateTime now = rtc.now();
							rtc.adjust(DateTime(setYear, setMonth, setDate, now.hour(), now.minute(), 0));
							msgSMSinfos =
								"Changement Reussi\r"
								"Nouvelle Date: " +
								formattedTime() + "\r" + formattedDate();
							sendSms(numSMS(tempSMS), msgSMSinfos);
						}
					}
					else
					{
						//error, the String to int return an error
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Heure Actuel :" +
							formattedTime() + "\r" + formattedDate();
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					// if we have not valid argument
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-date <jour> <mois> <annee>\r"
						"Exemple:\r"
						"reglage-date 25 12 2020";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}
			//------------------------------------MSG SETHOUR-------------------------------
			else if (tmpCommand.indexOf("reglage-heure") != -1)
			{
				if (tmpCommand.length() > 13) //if we have an argument
				{
					tmpCommand.remove(0, 14); //remove the beginning of the SMS
					int setHour; //temporary value of hour
					int setMinute; //temporary value of minute
					if ((setHour = getArgs(tmpCommand, ' ', 0).toInt()) && (setMinute = getArgs(tmpCommand, ' ', 1).toInt()))
					{
						if (setHour > 23 || setHour < 0 || setMinute > 60 || setMinute < 0)
						{
							//if we have not valid time
							msgSMSinfos =
								"Une Erreur c'est produite :-(\r"
								"Heure Actuel :" +
								formattedTime() + "\r" + formattedDate();
							sendSms(numSMS(tempSMS), msgSMSinfos);
						}
						else
						{
							//if its is ok
							Serial.println("Hour is set");
							DateTime now = rtc.now();
							rtc.adjust(DateTime(now.year(), now.month(), now.day(), setHour, setMinute, 0));
							msgSMSinfos =
								"Changement Reussi\r"
								"Nouvelle Heure: " +
								formattedTime() + "\r" + formattedDate();
							sendSms(numSMS(tempSMS), msgSMSinfos);
						}
					}
					else
					{
						//error, the String to int return an error
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Heure Actuel :" +
							formattedTime() + "\r" + formattedDate();
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					// if we have not valid argument
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-heure <heure> <minutes>\r"
						"Exemple:\r"
						"reglage-heure 16 03";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}
			//--------------------------MSG SET WEEKLY TIME ALERT---------------------
			else if (tmpCommand.indexOf("reglage-info") != -1){
				if (tmpCommand.length() > 12) //if we have an argument
				{
					tmpCommand.remove(0, 13); //remove the beginning of the SMS
					if ((smsInfoHour = getArgs(tmpCommand, ' ', 0).toInt()) && (smsInfoDay = getArgs(tmpCommand, ' ', 1).toInt())) //convert the args to int
					{
						if(smsInfoHour < 0 || smsInfoHour > 23 || smsInfoDay < 0 || smsInfoDay > 6){
							smsInfoHour = EEPROM.read(1);
							smsInfoDay = EEPROM.read(2);
							msgSMSinfos =
								"Une Erreur c'est produite :-(\r"
								"Merci d'entrer des valeurs valide\r"
								"Heure pour l'info de Temperature: " + String(smsInfoHour) + "h\r"
								"Jour pour l'info de Temperature: " + String(daysOfTheWeek[smsInfoDay]);
							sendSms(numSMS(tempSMS), msgSMSinfos);
						}
						else{
							Serial.println("Time Alert set");
							EEPROM.write(1, smsInfoHour); //save in the EEPROM
							EEPROM.write(2, smsInfoDay); 
							EEPROM.commit();
							delay(200);

							msgSMSinfos = 
							"Changement Reussi:\r"
							"Heure pour l'info :  " + String(smsInfoHour) + "h\r" +
							"Jour pour l'info  de Temperature: " + String(daysOfTheWeek[smsInfoDay]);
							sendSms(numSMS(tempSMS), msgSMSinfos);//send an SMS confimation
						}

					}
					else
					{
						//error, the String to int return an error
						smsInfoHour = EEPROM.read(1);
						smsInfoDay = EEPROM.read(2);
						
						msgSMSinfos =
							"Une Erreur c'est produite :-(\r"
							"Heure pour l'info de Temperature: " + String(smsInfoHour) + "h\r"
							"Jour pour l'info de Temperature: " + String(daysOfTheWeek[smsInfoDay]);
						sendSms(numSMS(tempSMS), msgSMSinfos);
					}
				}
				else
				{
					// we dont have a valid argument
					msgSMSinfos =
						"La demande n'est pas complete\r"
						"Veuillez respecter cette syntaxe:\r"
						"reglage-info <heure> <jour>\r"
						"Exemple:\r"
						"reglage-info 10 6";

					sendSms(numSMS(tempSMS), msgSMSinfos);
				}
			}
			else if (tmpCommand == "config"){
				
				TempAlert = EEPROM.read(0);
				smsInfoHour = EEPROM.read(1);
				EEPROM.get(4, PhoneAlert);
				smsInfoDay = EEPROM.read(2);

				msgSMSinfos =
				"Configuration actuelle:\r"
				"Temperature d'alerte: " + String(TempAlert) + "C\r"
				"Numero pour l'alerte: 0" + String(PhoneAlert) + "\r"
				"Heure: " + formattedTime() + "\r"
				"Date: " + formattedDate() + "\r"
				"Jour pour l'info: " + daysOfTheWeek[smsInfoDay] + "\r"
				"Heure pour l'info: " + smsInfoHour + "h\r"
				"Signal Reseau: " + String(getSignalQuality()) + "%\r"
				"Operateur: " + getOperator();
				
				sendSms(numSMS(tempSMS), msgSMSinfos);
			}else if(tmpCommand == "thermometre" || tmpCommand == "help" || tmpCommand == "?" || tmpCommand == "aide"){
					msgSMSinfos =
					"Thermometre SMS\r"
					"Creation: Dorian\r"
					"Version: 1.5 (07/2020)\r"
					"Heure: " + formattedTime() + "\r"
					"Date: " + formattedDate() + "\r"
					"Temperature actuelle: " + TempServer() + " C\r"
					"Temperature\r"
					"    -envoyez \"t\" \r"
					"Reglage de l'heure:\r"
					"    -envoyez \"reglage-heure\"\r"
					"Reglage de la date:\r"
					"    -envoyez \"reglage-date\"\r"
					"Reglage de la temperature d'alerte:\r"
					"    -envoyez \"reglage-temp\"\r"
					"Reglage du numero de telephone:\r"
					"    -envoyez \"reglage-tel\"\r"
					"Reglage des alertes hebdomadaires:\r"
					"    -envoyez \"reglage-info\"\r";
					sendSms(numSMS(tempSMS), msgSMSinfos);
			}
			else
			{
				msgSMSinfos = 
				"Commande non valide";
				sendSms(numSMS(tempSMS), msgSMSinfos);
			}
		}
	}
}

void autoReboot()
{
	DateTime now = rtc.now(); //get time
	Serial.println("check AutoReboot"); //show in Serial
	if (now.hour() == 0 && now.minute() == 0) // if the time is 00:00
	{
		delay(3000);
		Serial.println("AutoReboot");
		ESP.restart(); //restart 
	}
	if(millis() == 86400000)//86 400 000 ms -> 24h
	{
		delay(3000);
		Serial.println("AutoReboot");
		ESP.restart(); //restart 
	}
}

void weeklySMS()
{
	if (millis() - previousMillisCheckHour > intervalCheckHour) //check every minutes
	{
		Serial.println("check time");
		previousMillisCheckHour = millis();
		DateTime now = rtc.now(); //get time
		if (now.hour() == smsInfoHour && now.minute() == 0 && now.dayOfTheWeek() == smsInfoDay) //if it is good time
			sendWeekly(); //sent info temp
		
		bool b = digitalRead(POWERPIN);
		b = digitalRead(POWERPIN);
		if(now.hour() > 8 && now.hour() < 22 && b)
			displayTemp();
		else
			turnoffDisplay();
	}
}

void loopTemp()
{
	if (millis() - previousMillisCheckTemp > intervalCheckTemp)
	{
		autoReboot();
		displayTemp();
		previousMillisCheckTemp = millis();
		Serial.print("check Temp: ");
		Serial.println(TempAlert);

		if (smsAlertSend == 0)
		{

			if (alertTempClient() || alertTempServer())
			{

				//----------convert Int to String for the SMS-------------
				String strTempClient = String(TempClient);
				strTempClient.remove(4);//keep one digit after the decimal point
				String strHumClient = String(HumClient);
				strHumClient.remove(4);//keep one digit after the decimal point
				String strBatteryClient = String(BatteryClient);

				String strTempServer = String(TempServer());
				strTempServer.remove(4);//keep one digit after the decimal point
				String strHumServer = String(HumServer());
				strHumServer.remove(4);//keep one digit after the decimal point
				String strBatteryServer = String(getBattery());

				String strFormattedTime = formattedTime();
				String strFormattedDate = formattedDate();
				//--------------------------------------------------------

				if (TempClient != 200 && HumClient != 200 && TempServer() != 200)
				{
					msgAlertTemp =
						"ALERT TEMPERATURE:\r"
						"Temperature A: " + strTempClient + " C\r"
						"Humidite A: " + strHumClient + "%\r"
						"Batterie A: " + strBatteryClient + "%\r"
						"Date A: " + timeTempClient + "\r" + dateTempClient + "\r\r"

						"Temperature B: " + strTempServer + " C\r"
						"Humidite B: " + strHumServer + "%\r"
						"Batterie B: " + strBatteryServer + "%\r"
						"Date B: " + formattedTime() + "\r" + formattedDate();
				}
				else if (TempClient == 200 && HumClient == 200 && TempServer() != 200)
				{
					msgAlertTemp =
						"ALERT TEMPERATURE:\r"
						"Capteur A: Aucune Donnee\r\r"

						"Temperature B: " + strTempServer + " C\r"
						"Humidite B: " + strHumServer + "%\r"
						"Batterie B: " + strBatteryServer + "%\r"
						"Date B: " + formattedTime() + "\r" + formattedDate();
				}
				else if (TempClient != 200 && HumClient != 200 && TempServer() == 200)
				{
					msgAlertTemp =
						"ALERT TEMPERATURE:\r"
						"Temperature A: " + strTempClient + " C\r"
						"Humidite A: " + strHumClient + "%\r"
						"Batterie A: " + strBatteryClient + "%\r"
						"Date A: " + timeTempClient + "\r" + dateTempClient + "\r\r"

						"Temperature B: \r" +
						"Aucune Donnee\r";
				}

				EEPROM.get(4, PhoneAlert);
				strPhoneAlert = "+33" + String(PhoneAlert).substring(0);

				sendSms(strPhoneAlert, msgAlertTemp);
				smsAlertSend = true;
			}
			else
			{
				smsAlertSend = false;
			}
		}

		else
		{
			Serial.println("SMS already send");
		}
	}
}

void sendBatteryAlert()
{
	bool b = digitalRead(POWERPIN);
	if (b == false && BatteryAlert == false)
	{
		display.clearDisplay();
		display.setCursor(15,37);
		display.println("Sur Batterie");
		display.display();
		msgAlertTemp = "Alerte:\r"
					   "Coupure de courant,\r"
					   "le thermometre passe sur les batteries\r"
					   "qui ont une autonomie de plusieurs heures";
		EEPROM.get(4, PhoneAlert);
		strPhoneAlert = "+33" + String(PhoneAlert).substring(0);

		sendSms(strPhoneAlert, msgAlertTemp);
		BatteryAlert = true;
		delay(5000);
		sendWeekly();
		displayTemp();
	}
	else if (BatteryAlert && b)
	{
		display.clearDisplay();
		display.setCursor(20,37);
		display.println("En Charge...");
		display.display();

		msgAlertTemp = "Info:\r"
					   "Le courant est de retour,\r"
					   "Le thermometre est de nouveau\r"
					   "alimente par le secteur";
		EEPROM.get(4, PhoneAlert);
		strPhoneAlert = "+33" + String(PhoneAlert).substring(0);

		sendSms(strPhoneAlert, msgAlertTemp);
		BatteryAlert = false;
		delay(5000);
		sendWeekly();
		displayTemp();
	}
}
