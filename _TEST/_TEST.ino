void setup(){	
	Serial.begin(19200);
	delay(100);
	pinMode(11, OUTPUT);
}


void loop(){
	//digitalWrite(11, HIGH);
	Serial.println(analogRead(A2));
	if(analogRead(A2) > 200){
		digitalWrite(11, HIGH);
	}
	delay(1000);
}

