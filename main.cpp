#include <Arduino.h>
#include "function.h"

void setup() {
	startModem();
	setupEspNow();
	
	Serial.println(formatedTime());
	Serial.println(formatedDate());
	Serial.println(dht.readTemperature());
	
}

void loop() {
	loopTemp();
	loopGSM();
	delay(2000);
}
