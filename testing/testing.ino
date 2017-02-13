#include <SoftwareSerial.h>
SoftwareSerial SIM900(7, 8);
 
char incoming_char = 0;
char x = 0;
char y = 0;
String test = "";

void setup(){
	delay(100);
	Serial.begin(19200); // for serial monitor
	while(!Serial);
	SIM900.begin(19200); // for GSM shield
	while(!SIM900);
	delay(3000);
	
}


void loop(){

	while(SIM900.available() > 0){
		incoming_char=SIM900.read(); //Get the character from the cellular serial port.
		test += incoming_char; //Print the incoming character to the terminal.
		y++;
		if(y > 10){
			Serial.println(test);
			test = "";
		}
	}

	if(x == 0){
		SIM900.println("AT+CPBR=1");
		x = 1;
	}
	
}