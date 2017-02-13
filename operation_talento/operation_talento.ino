void setup() {
  // put your setup code here, to run once:
	Serial1.begin(19200); // for GSM shield
	while(!Serial1);
	delay(100);
}


void loop() {
	Serial1.println("AT+CMGF=1");
	delay(100);
	Serial1.print("AT+CMGS=\"+639262543854\"\r");
	delay(100);
	Serial1.println("Sardeña to. Open minded ka ba sa business. Reply ka naman");
	delay(500);
	Serial1.println((char)26);
	delay(500);
	for(char i = 0; i < 60; i++)
		delay(1000);
}
