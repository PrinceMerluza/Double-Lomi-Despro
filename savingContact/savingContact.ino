#include <SoftwareSerial.h>
SoftwareSerial SIM900(10, 11);
 
char incoming_char = 0;
char x = 0;

void setup(){
	delay(2000);
	Serial.begin(19200);
	delay(100);
	SIM900.begin(19200);
	delay(5000);

	
	
	
}


void loop(){

	while(SIM900.available() > 0){
		incoming_char=SIM900.read(); //Get the character from the cellular serial port.
		Serial.print(incoming_char); //Print the incoming character to the terminal.
	}
	
	if(x == 0){
		SIM900.print("AT+CPBW=1,\"\",129,\"Last Test\"\r");
		delay(100);
		x = 1;
	}
	
}
