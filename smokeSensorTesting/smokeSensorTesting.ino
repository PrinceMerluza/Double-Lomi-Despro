void setup() {
	
}

void loop() {
	
}

void checkGasSensor(){
	if(analogRead(pinForSensor) > gasValAvg + GAS_SENSOR_THRESHOLD){
		thereIsFire();
	}
}