void setup() {
	Serial.begin(19200); // for serial monitor
	while(!Serial);
	Serial1.begin(19200); // for GSM shield
	Serial1.print("\r");
	while(!Serial1);
	Serial.println("AAA");
	delay(100);
}

boolean x = true;

void loop() {
	if(x){
		Serial1.println("AT+CSQ");
		delay(100);
		x = false;
	}
	while(Serial1.available() > 0){
		char inchar = Serial1.read();
		Serial.print(inchar);
	}
}
