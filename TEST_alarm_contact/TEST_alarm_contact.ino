int doorOpenLimit = 10000;
boolean doorOpen = false;
unsigned long curTimeA = 0;
unsigned long prevTimeA = 0;
unsigned long curTimeD = 0;
unsigned long prevTimeD = 0;

void setup() {
  pinMode(2, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  curTimeA = millis();
  curTimeD = millis();
  
  if(analogRead(A0) >= 1000){
    doorOpen = true;
    prevTimeA = curTimeA;
  }else{
    if((unsigned long)((curTimeA - prevTimeA) > 2000)){
      doorOpen = false;
      prevTimeA = curTimeA;
    }
  }
  if(doorOpen){
    if((unsigned long)((curTimeD - prevTimeD) > 5000)){
      Serial.println("BUKAS BUI!!");
    }
     digitalWrite(2, HIGH);
      delay(500);
      digitalWrite(2, LOW);
      delay(500);
  }else{
    Serial.println("SARADO BUI!!");
    prevTimeD = curTimeD;
    digitalWrite(2, LOW);
  }
  //Serial.println();
//  Serial.print("VALUE: ");
  //Serial.println(analogRead(A0));

}
