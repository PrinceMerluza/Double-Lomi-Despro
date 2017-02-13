//Pressing up will increase x value while pressing down will decrease x value

#include <Wire.h>
#include "rgb_lcd.h"

#define B_UP 2
#define B_DOWN 3

unsigned long mill = 0;
unsigned long prevMill = 0;
unsigned long prevDisp = 0;
int prevStateUp;
int prevStateDown;
int debounce = 100;
int displayRefresh = 100;
int x = 0;
rgb_lcd lcd;

void setup(){
	Serial.begin(9600);
	while(!Serial);
	pinMode(B_UP, INPUT);
	pinMode(B_DOWN, INPUT);
	
	lcd.begin(16, 2);
    lcd.setRGB(0,255,255);

    delay(1000);
}
	

void loop(){
	mill = millis();
	
	if(digitalRead(B_UP) == LOW) prevStateUp = LOW;
	if(digitalRead(B_DOWN) == LOW) prevStateDown = LOW;

	if(digitalRead(B_UP) == HIGH && prevStateUp == LOW && (unsigned long)(mill - prevMill) > debounce){
		x++;
		prevStateUp = HIGH;
		prevMill = mill;
	}
	
	if(digitalRead(B_DOWN) == HIGH && prevStateDown == LOW && (unsigned long)(mill - prevMill) > debounce){
		x--;
		prevStateDown = HIGH;
		prevMill = mill;
	}
	
	if((unsigned long)(mill - prevDisp) > displayRefresh){
		lcd.setCursor(0, 0);
		lcd.print(x);	
		prevDisp = mill;
	}
}