#include <SoftwareSerial.h>
#include <Wire.h>
#include "rgb_lcd.h"
SoftwareSerial SIM900(10, 11);
 
char incoming_char = 0;
int x = 0;
String test = "", tmp = "";

unsigned long mill = 0;
unsigned long prevDisp = 0;
int displayRefresh = 100;
rgb_lcd lcd;

void setup()
{ 
	Serial.begin(19200);
	while(!Serial);
	SIM900.begin(19200); // for GSM shield
	while(!SIM900);
	//SIM900power();  // turn on shield
	delay(5000);  // give time to log on to network.

	SIM900.print("AT+CMGF=1\r");  // set SMS mode to text
	delay(100);
	SIM900.print("AT+CNMI=2,2,0,0,0\r"); 
	// blurt out contents of new SMS upon receipt to the GSM shield's serial out

	lcd.begin(16, 2);
	lcd.setRGB(0,255,255);
	delay(1000);  // turn off module
}


void loop()
{
	mill = millis();
	// Now we simply display any text that the GSM shield sends out on the serial monitor
	while(SIM900.available() > 0){
		incoming_char=SIM900.read(); //Get the character from the cellular serial port.
		test += incoming_char;
		delay(50);
		x = 1;
	}
	if(x == 1){
		tmp = test;
		tmp.trim();
		x = 0;
		if(tmp.substring(0,4)== "+CMT"){
			Serial.print("test: ");
			Serial.print(tmp);
			tmp = tmp.substring(52);
			tmp.trim();
			Serial.print("tmp: ");
			Serial.print(tmp);
			tmp.trim();
			x = 2;
		}
		Serial.println(tmp);
		Serial.print("X: ");
		Serial.println(x);
	}
	
	if(x == 2 && (unsigned long)(mill - prevDisp) > displayRefresh){
		lcd.setCursor(0, 0);
		lcd.print(tmp);	
		prevDisp = mill;
		x = 0;
	}
	
	test = "";
}