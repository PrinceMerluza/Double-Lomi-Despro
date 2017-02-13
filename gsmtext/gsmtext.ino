#include <SoftwareSerial.h>
SoftwareSerial SIM900(7, 8);
 
char incoming_char=0;
char x = 0;
String test = "", tmp = "";
 
void setup()
{
  pinMode(2, OUTPUT);
  
  delay(1000);
  Serial.begin(19200); // for serial monitor
  while(!Serial);
  SIM900.begin(19200); // for GSM shield
  while(!SIM900);
  //SIM900power();  // turn on shield
  delay(5000);  // give time to log on to network.
 
  SIM900.print("AT+CMGF=1\r");  // set SMS mode to text
  delay(100);
  SIM900.print("AT+CNMI=2,2,0,0,0\r"); 
  // blurt out contents of new SMS upon receipt to the GSM shield's serial out
  delay(100);
  Serial.println("FINISH SETUP");
}
 
void SIM900power()
// software equivalent of pressing the GSM shield "power" button
{
  digitalWrite(9, HIGH);
  delay(1000);
  digitalWrite(9, LOW);
  delay(7000);
}
 
void sendSMS(String message)
{
  SIM900.println("AT + CMGS = \"+639262543854\"");                                     // recipient's mobile number, in international format
  delay(100);
  SIM900.println(message);        // message to send
  delay(100);
  SIM900.println((char)26);                       // End AT command with a ^Z, ASCII code 26
  delay(100); 
  SIM900.println();
  delay(5000);                                     // give module time to send SMS                                  // turn off module
}
 
void lightOn(){
	digitalWrite(2, HIGH);
}
void lightOff(){
	digitalWrite(2, LOW);
}

void loop()
{
  // Now we simply display any text that the GSM shield sends out on the serial monitor
  while(SIM900.available() > 0)
  {
    incoming_char=SIM900.read(); //Get the character from the cellular serial port.
	test += incoming_char;
    Serial.print(incoming_char); //Print the incoming character to the terminal.
	x = 1;
  }
  tmp = test;
  if(x == 1){
	tmp.trim();
	tmp = tmp.substring(0,4);
	x = 0;
  }
	
  if(tmp.substring(0,4)== "+CMT"){
		test = test.substring(56);
		test.trim();
		//sendSMS(test);
		Serial.println("MESSAGE: " + test);
		if(test == "open"){
			lightOn();
		}else if(test == "close"){
			lightOff();
		}
	}
	test = "";
}