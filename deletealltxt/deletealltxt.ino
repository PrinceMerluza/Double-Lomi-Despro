#include <SoftwareSerial.h>
SoftwareSerial SIM900(7, 8);
 

void setup()
{
  SIM900.begin(19200); // for GSM shield
  //SIM900power();  // turn on shield
  delay(5000);  // give time to log on to network.
 
  SIM900.print("AT+CMGD=0,4\r");  // set SMS mode to text
  delay(100);

}
 

void loop()
{

}