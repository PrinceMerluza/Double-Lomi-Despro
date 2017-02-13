#include <EEPROM.h>

char arr[3][2][11];
char a;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(19200);
  a = EEPROM.read(1001);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(a);
}
