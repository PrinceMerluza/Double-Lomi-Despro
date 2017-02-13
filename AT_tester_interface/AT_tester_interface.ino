#include <SoftwareSerial.h>
 
SoftwareSerial mySerial(7,8); // change these paramters depending on your Arduino GSM Shield
 
void setup()
{
  Serial.begin(19200);
  //Serial.println(“Begin”);
  mySerial.begin(19200);
 
}
 
void loop()
{
  if (mySerial.available())
    Serial.write(mySerial.read());
  if (Serial.available())
    mySerial.write(Serial.read());
}
