/*---------------------------------------
|			Termometer SMS				|
|		    	By Xmow					|			
|			gammatroniques.fr			|
----------------------------------------*/
#include <Arduino.h>
#include "function.h"

void setup() {
	setupScreen();
	startModem();
	setupEspNow();
	setRTC_GSM();
	delay(1000);
	displayTemp();
}

void loop() {
	loopTemp();
	loopGSM();
	sendBatteryAlert();
	weeklySMS();
	checkButton();
	delay(50);
}
