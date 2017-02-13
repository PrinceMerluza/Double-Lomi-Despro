/* 
Consolidated versions of PB3 and lcd control
candidate for final
*/
#include <Keypad.h>
#include <Wire.h>
#include <stdio.h>
#include "rgb_lcd.h"

#define MAINMENUOPTIONS 3
#define MAXCONTACTS 15
#define TEXTMESSAGEMAX 30
#define SIM900 Serial1

//SoftwareSerial SIM900(10, 11);

//constnts for sms commands and the truster person in PB
const char* TRUSTKEY = "DoMoLuMe"; 
const char* OPENDOOR = "open"; 
const char* CLOSEDOOR = "close";
const char* STOPBUZZANDALARM = "stopa";
const char* MANUALALARM = "alarm";
char* MESSFORDOOROPEN = "Bumukas ang pinto";
char* MESSFORFIRE = "May leak ang gas. Fire Hotline: 117";

byte index = 0; //index for filling serial input array
byte iForPBList = 1; //index for phonebook list
char inChar = -1; //temp char for incoming char for serial input
byte state = 0;

byte numContacts = 0;

char ATcommand[50] = ""; //commands to be sent to SIM900
char tempNum[12] = ""; //phone number parameter

//first 11 digits of cstring is celphone number. 12th is a letter [a-z] for indexing and ID.
//this makes the technical limit of cp numbers up to 26.
char pbNumberList[MAXCONTACTS][13];

//timing and display
unsigned long mill = 0;
unsigned long prevDisp = 0;
int displayRefresh = 100;
rgb_lcd lcd;

//Keypad setup
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {31, 33, 35, 37}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {23, 25, 27, 29}; //connect to the column pinouts of the keypad
byte screenColor[] = {0, 255, 255};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//MAIN MENU SETUP
const char lcdMainMenu[MAINMENUOPTIONS][17] = {
	"1.Add Contact",
	"2.Del Contact",
	"3.Lst Contacts"
}; 
byte menuState = 0;
/* MENUS TATES AND MEANINGS
//states 0 -main menu.
//1: adding a contact number
//2: delete a number
//3: List Contacts
//4: Messgae (Alert)
//5: SMS receiving
//6: SMS sending
*/

//variable for menu display and position
byte maxPage = 0;
byte curPage = 0;
byte curPos = -1;
byte curOption = 0;

//variables for phone number input
char inputPhoneNum[14] = "";

//vars for message display
byte prevState = 0;
char messageForDisplay[17] = "NaN";

//variables for receiving SMS to SIM900
char textMsg[TEXTMESSAGEMAX]; 
char simInputBuffer[100]; //sim900 output for receiving sms
char smsRecFromNumber[14]; // sms received from number
char smsRecMessage[50]; //the actual sms message
char smsRecWhen[21]; //date time of sms received
char messageToSend[100]; //array for the message to send from SIM900
boolean smsRecTrustedPerson = false;
byte iForSimInputBuffer = 0;
boolean postSim = false;

//variable for door and sensors
boolean isDoorOpen = false;
//pin assignments
char pinForSensor = A0;
char pinForDoorContact = 47;
char pinForRelayDoor = 52;
boolean onDoorOpen = false;
char pinForFan = 50;
char pinForWeng = 48; 
const int ALLOWANCE_FOR_DOOR = 10000; //ms door open until it alarms
const int GAS_SENSOR_THRESHOLD = 300;
const unsigned long GAS_SENSOR_REENABLE_TIMER = 150000; //2.5 minutes
boolean enableSensor = true;
unsigned long timeOfFire = 0;
unsigned long timerResetSensor = 0;
unsigned int ticksForSensorReset = 0;
unsigned long curTDoorContact = 0;
unsigned long prevTDoorContact = 0;
boolean firing = false;
boolean manualAlarm = false;

byte cursor[8] = {
    0b10000,
    0b11000,
    0b11100,
    0b11110,
    0b11110,
    0b11100,
    0b11000,
    0b10000
};

void setup(){
	//serial setup
	Serial.begin(19200); // for serial monitor
	while(!Serial);
	SIM900.begin(19200); // for GSM shield
	while(!SIM900);
	delay(100);
	//power up SIM900. be used on actual thesis (external power supply)
	pinMode(9, OUTPUT); 
	digitalWrite(9,LOW);
	delay(1000);
	digitalWrite(9,HIGH);
	delay(2000);
	digitalWrite(9,LOW);
	delay(10000);
	
	countContacts();
	
	//setup for sensors
	pinMode(pinForDoorContact, INPUT); //reed switch
	pinMode(pinForWeng, OUTPUT); //weng weng
	pinMode(pinForFan, OUTPUT); //wfan
	pinMode(pinForRelayDoor, OUTPUT);
	
	//lcd setup
	lcd.begin(16, 2);
	lcd.setRGB(screenColor[0], screenColor[1], screenColor[2]);
	lcd.createChar(1, cursor);
	
	callMainMenu();
	delay(1000); 
}

void countContacts(){
	Serial.println("Counting");
	numContacts = 0;
	clearNumberList();
	for(byte i = 1; i <= MAXCONTACTS; i++){
		ATcommand[0] = '\0';
		strcpy(ATcommand, "AT+CPBR=");
		addBytToString(ATcommand, i);
		SIM900.println(ATcommand);
		delay(100);
		
		while(SIM900.available() > 0){
			inChar = SIM900.read(); 
			Serial.print(inChar);
			if(inChar == ':'){
				while(inChar != '"'){
					inChar = SIM900.read();
				}
				for(byte ii = 0; ii < 11; ii++){
					tempNum[ii] = SIM900.read();
				}
				strcat(pbNumberList[numContacts],tempNum);
				pbNumberList[numContacts][11] = (char)(i + 96); //put the char ID				
				pbNumberList[numContacts][12] = '\0';
				
				numContacts++;
			}
		}
	}
	Serial.print("Number of Contacts: ");
	Serial.println(numContacts);	
}

//clear the numbers list form the array bNumberList
void clearNumberList(){
	for(byte i = 0; i < MAXCONTACTS; i++){
		pbNumberList[i][0] = '\0';
	}
}

//switch menu to the main menu
void callMainMenu(){
	curPos = 0;
	curOption = 0;
	curPage = 0;
	maxPage = (MAINMENUOPTIONS / 2) + 1;
	menuState = 0;
}

//concatenate a single char to a cstring
void addCharToString(char* srcStr, char a){
    char tmpstr[2];
    tmpstr[0] = a;
    tmpstr[1] = '\0';
 
    strcat(srcStr, tmpstr);
}

//concatenate a byte to a cstring
void addBytToString(char* srcStr, byte a){
	if(a < 10){
		addCharToString(srcStr, char(a + 48));	
	}else{
		addCharToString(srcStr, char((a/10) + 48));
		addCharToString(srcStr, char((a%10) + 48));
	}
}

void showMessage(char* msg){
	messageForDisplay[0] = '\0';
	strcat(messageForDisplay, msg);
	prevState = menuState;
	menuState = 4;
}

//reset vlaues after every state change
void resetStuff(){
	ATcommand[0] = '\0';
	iForPBList = 1;
	state = 0;
}

//when a key from keypad is pressed
void keyPressed(char k){
	Serial.println(k);
	switch(menuState){
	case 0:
	//MAIN MENU INPUT
		if(k == 'D'){ //DOWN button
			curOption++;
			if(curOption >= MAINMENUOPTIONS) curOption = 0;
		}
		if(k == 'C'){ // UP button
			curOption--;
			if(curOption == 255) curOption = MAINMENUOPTIONS - 1;
		}
		if(k == 'A'){ //choose option
			switch(curOption){
			//chose option ADD CONTACT
			case 0: 
				if(numContacts < MAXCONTACTS)
					menuState = 1; 
				else
					showMessage("Reached Max!");
				break;
			//chose option DEL CONTACT
			case 1: 
				menuState = 2; 
				curPos = 0;
				curOption = 0;
				curPage = 0;
				maxPage = (numContacts / 2) + 1;
				break;
			//chose option LST CONTACT
			case 2: 
				menuState = 3;
				curPos = 0;
				curOption = 0;
				curPage = 0;
				maxPage = (numContacts / 2) + 1;	
				break;
			}
		}
		curPage = curOption / 2;
		curPos = curOption % 2;
		break;
	
	case 1:
	//ADD CONTACT INPUT
		if((k >= 48) && (k <= 57)) //if key pressed is numbers 0-9
		addCharToString(inputPhoneNum, k);	
		
		if(k == 'B'){ //erase digit
			if(strlen(inputPhoneNum) > 0)
				inputPhoneNum[strlen(inputPhoneNum)-1] = '\0';
			else
				menuState = 0;
		}
		if(k == 'D'){ //erase everything
			inputPhoneNum[0] = '\0';
		}
		if(k == 'A'){ //accept number
			//input validation
			if((strlen(inputPhoneNum) != 11) || 
			  (inputPhoneNum[0] != '0') ||
			  (inputPhoneNum[1] != '9')){
				showMessage("Invalid number");
			}else{
				//successful input

				strcpy(ATcommand, "AT+CPBW=,\"");
				strcat(ATcommand, inputPhoneNum);
				strcat(ATcommand, "\",129,\"");
				strcat(ATcommand, TRUSTKEY);
				strcat(ATcommand, "\"\r");

				SIM900.print(ATcommand);
				delay(100);
				
				//cleanup
				inputPhoneNum[0] = '\0';
				menuState = 0;
				
				showMessage("Number added");
			}
		}
		break;
	case 2:
	//DELETE NUMBER CONTROLS
		if(k == 'D'){ //DOWN button
			curOption++;
			if(curOption >= numContacts) curOption = 0;
		}
		if(k == 'C'){ // UP button
			curOption--;
			if(curOption == 255) curOption = numContacts - 1;
		}
		if(k == 'B'){ // BACK button to main menu
			callMainMenu();
		}
		if(k == 'A'){ //delete
			strcpy(ATcommand, "AT+CPBW=");
			addBytToString(ATcommand, (byte)(pbNumberList[curOption][11] - 96));
			strcat(ATcommand, "\r");

			SIM900.print(ATcommand);
			delay(100);
			
			showMessage("Deleted Number");
			curOption = 0;
			if(numContacts <= 0) callMainMenu();
		}		
		
		curPage = curOption / 2;
		curPos = curOption % 2;
		break;
	//LIST CONTACT CONTROLS
	case 3:
		if(k == 'D'){ //DOWN button
			curOption++;
			if(curOption >= numContacts) curOption = 0;
		}
		if(k == 'C'){ // UP button
			curOption--;
			if(curOption == 255) curOption = numContacts - 1;
		}
		if(k == 'B'){ // BACK button to main menu
			callMainMenu();
		}
		
		curPage = curOption / 2;
		curPos = curOption % 2;
		break;
	//input for alert message
	case 4:
		//return to previous state also recount the contacts
		if(k == 'C'){
			countContacts();
			menuState = prevState;
		}	
			
		break;
	//for both sending and receiving message. lcd should be disbled
	case 5: break;
	case 6: break;
	}
}

//display for the screen.
void screenDisplay(){
	lcd.clear();
	
	switch(menuState){
	//MAIN MENU DISPLAY
	case 0:
		//screen cursor position
		lcd.setCursor(0, curPos);
		lcd.write(1);
	
		//main menu contents
		lcd.setCursor(1, 0);
		lcd.print(lcdMainMenu[0 + (curPage * 2)]);
		if((curPage != maxPage - 1) || (MAINMENUOPTIONS % 2 != 0)){
			lcd.setCursor(1, 1);
			lcd.print(lcdMainMenu[1 + (curPage * 2)]);
		}
		break;
	//ADD CONTACT DISPLAY
	case 1:
		lcd.setCursor(0, 0);
		lcd.print("Enter number:");
		lcd.setCursor(0, 1);
		lcd.print(inputPhoneNum);
		lcd.setCursor(strlen(inputPhoneNum), 1);
		lcd.print("_");
		break;
	//DEL CONTACTACT DISPLAY
	case 2:
	//LIST CONTACT DISPLAY
	case 3:
		//screen cursor position
		lcd.setCursor(0, curPos);
		lcd.write(1);
	
		//phonue number contents
		lcd.setCursor(1, 0);
		lcd.print(pbNumberList[0 + (curPage * 2)]);
		if((curPage != maxPage - 1) || (numContacts % 2 != 0)){
			lcd.setCursor(1, 1);
			lcd.print(pbNumberList[1 + (curPage * 2)]);
		}
		lcd.setCursor(12,0);
		lcd.print(" ");
		lcd.setCursor(12,1);
		lcd.print(" ");
		break;
	case 4:
		//message ALERT display
		lcd.setCursor(0,0);
		lcd.print(messageForDisplay);
		lcd.setCursor(0, 1);
		lcd.print("press [C]ontinue");
		break;
	case 5: 
		//when receiving. ON HOLD - unusre pa kung iaadd
		break;
	case 6:
		//when sim is sending mesage
		lcd.setCursor(0,0);
		lcd.print("Busy Sending");
		lcd.setCursor(0, 1);
		lcd.print("Please Wait...");
		break;
	}
}

//sending message using SIM900
void sendMessage(char * number, char * message){
	//lock lcd for sending sms
	menuState = 6;
	screenDisplay();
	
	ATcommand[0] = '\0';
			
	SIM900.println("AT+CMGF=1");
	delay(100);
	SIM900.print("AT+CMGS=\"");
	SIM900.print(number);
	SIM900.println("\"\r");
	delay(100);	
	
	Serial.println(number);
	while(SIM900.available() > 0){
		inChar = SIM900.read(); 
		Serial.print(inChar);
	}
	
	if(message> 0)
		SIM900.println(message);
	else
		SIM900.println("BLANK SHIT");
	delay(100); 
	SIM900.println((char)26);
	delay(100);
	
	//stay in this process and wait until message is sent.
	//because not multithread will stop everything including lcd procs and shit
	boolean getOut = false;
	while(true){
		while(SIM900.available() > 0){
			inChar = SIM900.read(); 
			Serial.print(inChar);
			//check for confirmation of sent message
			if(inChar == '+'){delay(1);if(SIM900.read() == 'C'){
				delay(1);if(SIM900.read() == 'M'){delay(1);if(SIM900.read() == 'G'){
				delay(1);if(SIM900.read() == 'S'){delay(1);if(SIM900.read() == ':'){
					getOut = true;
			}}}}}}
		}
		if(getOut) break;
	}
	
	//DEBUG
	Serial.print("SENT THIS SMS: ");
	Serial.println(message);
}

//send to everyone on phonebook
void sendToAll(char * message){
	//send mesage through phonebook
	char tmp_numPart[11];
	char the_number[14];
	for(char i = 0; i < numContacts; i++){
		the_number[0] = '\0'; 
		tmp_numPart[0] = '\0';
		
		//get number from phonebook from 11-digit to +63 format
		strncpy(tmp_numPart, pbNumberList[i] + 1, 10);
		tmp_numPart[10] = '\0';
		strcat(the_number, "+63");
		strcat(the_number, tmp_numPart);
		
		sendMessage(the_number, message);
		delay(100);
		
		//AFter every send to a single person, stop to check if
		//door closes to stop the alarm.
		checkDoorCloseInstance();
	}
	
	callMainMenu(); //retrun from screen lock
}

//when changing the state of the door and texting everyone
void changeDoorAndText(char * who, char * when, boolean toOpenDoor){
	isDoorOpen = toOpenDoor;
	if(toOpenDoor){
		digitalWrite(pinForRelayDoor, HIGH);
		delay(200);
		digitalWrite(pinForRelayDoor, LOW);
	}
	
	
	if(toOpenDoor)
		Serial.println("DOOR BUKAS!");
	else
		Serial.println("DOOR SARA!");
	
	//send mesage through phonebook
	char tmp_numPart[11];
	char the_number[14];

	messageToSend[0] = '\0';
		
	//compose message		
	strcat(messageToSend, who);
	if(toOpenDoor)
		strcat(messageToSend, " opened");
	else
		strcat(messageToSend, " closed");
	strcat(messageToSend, " the door at ");
	strcat(messageToSend, when);
	
	sendToAll(messageToSend);
	delay(100);
	
}

// utility functions for fan and the fan
void stopAlarm(){
	digitalWrite(pinForWeng, LOW);
}

void startAlarm(){
	digitalWrite(pinForWeng, HIGH);
}

void startFan(){
	digitalWrite(pinForFan, HIGH);
}

void stopFan(){
	digitalWrite(pinForFan, LOW);
}

//ALERTS
void thereIsFire(){
	enableSensor = false;
	startAlarm();
	startFan();
	firing = true;
	sendToAll(MESSFORFIRE);
	timeOfFire = millis();
}

void doorOpenTooLong(){
	startAlarm();
}

void checkGasSensor(){
	if(analogRead(pinForSensor) > GAS_SENSOR_THRESHOLD){
		thereIsFire();
	}
}

void checkDoorOpenInstance(){
	if((digitalRead(pinForDoorContact) == LOW) && (isDoorOpen == false)){
		prevTDoorContact = curTDoorContact;
		isDoorOpen = true;
	}
}

void checkDoorCloseInstance(){
	if((digitalRead(pinForDoorContact) == HIGH) && (isDoorOpen)){
		isDoorOpen = false;
		onDoorOpen = false;
		
		if(!firing && !manualAlarm){
			stopAlarm();
			//stopFan();
		}
		
		prevTDoorContact = curTDoorContact;
	}
}

void cstringToLower(char * mess,byte length){
	for(byte i = 0; i < length; i++){
		mess[i] = tolower(mess[i]);
	}
}

void loop(){
	//delta for display
	char key = keypad.getKey();
	mill = millis();
	curTDoorContact = millis();

	// ------------------ INPUT -----------------
	//check for keypad input
	if (key){
		keyPressed(key);
	}
	
	// ---------------- SENSOR EVALUATE --------------
	
	//timer delay after fire to reenable sensor
	if(!enableSensor && ((unsigned long)(millis() - timeOfFire) > GAS_SENSOR_REENABLE_TIMER)){
		enableSensor = true;
	}
	
	//instance when door is opened
	checkDoorOpenInstance();
	//instance when door is closed
	checkDoorCloseInstance();
	//will check if door is opened for too long
	if(isDoorOpen){
		if((unsigned long)(curTDoorContact - prevTDoorContact) >= ALLOWANCE_FOR_DOOR){
			doorOpenTooLong();
			prevTDoorContact = curTDoorContact;
			if(!onDoorOpen){
				sendToAll(MESSFORDOOROPEN);
				onDoorOpen = true;
			}
		}		
	}
	
	//checking gas sensor
	if(((unsigned long)mill > 30000) && enableSensor) {
		checkGasSensor(); 
	}
	
	// ----------------- SIM900 EVALUATE --------------
	//check for SIM900 Activity
	if(SIM900.available()){
		Serial.println(" ---FROM LOOP ---");
		while(SIM900.available() > 0){
			//display to serial monitor
			inChar = SIM900.read();
			Serial.print(inChar);
			
			//checking if text message is received and storing to simInputBuffer
			if(inChar == '+'){delay(1);if(SIM900.read() == 'C'){
				delay(1);if(SIM900.read() == 'M'){delay(1);if(SIM900.read() == 'T'){
					iForSimInputBuffer = 0;
					postSim = true;
					while(SIM900.available() > 0){
						if(iForSimInputBuffer < 99){
							inChar = SIM900.read();
							simInputBuffer[iForSimInputBuffer++] = inChar;
						}else{
							break;
						}	
						delay(1);
					}	
					
					Serial.print("\nLENGTH:");
					Serial.println(iForSimInputBuffer);
					
					simInputBuffer[iForSimInputBuffer] = '\0';
			}}}}
		}
		Serial.println(" ---END LOOP ---");
	}
	
	//everything to be done after the SMS is received
	//sms message evaluation
	if(postSim){
		//DEBUG OUTPUT DISPLAY
		Serial.println("--- MAY NARECEIVE AKO BUI -------- " );
		Serial.print("ENTIRE SMS: " );
		Serial.println(simInputBuffer);
		
		//extract number from sms
		strncpy(smsRecFromNumber, simInputBuffer + 3, 13);
		Serial.print("NUMBER: ");
		Serial.println(smsRecFromNumber);
		
		//check if valid contact
		if(strstr(simInputBuffer, TRUSTKEY) != NULL){
			smsRecTrustedPerson = true;
			Serial.println("TRUSTED: YES");
		}else{
			smsRecTrustedPerson = false;
			Serial.println("TRUSTED: NO");
		}
		
		//extract date/time
		char * tmp_p = strstr(simInputBuffer, "\n");
		if(tmp_p != NULL){
			strncpy(smsRecWhen, tmp_p-22, 20);
			
			Serial.print("DATE/TIME: ");
			Serial.println(smsRecWhen);
		}
		
		//extract message
		if(tmp_p != NULL){
			strncpy(smsRecMessage, tmp_p + 1, 49);
			Serial.print("MESSAGE: ");
			//delete new line after SIM900 echo.
			smsRecMessage[strstr(smsRecMessage, "\n") - smsRecMessage] = '\0';
			Serial.println(smsRecMessage);
		}
		
		//SMS COMMANDS EVALUTAION STUFF STUFF
		if(smsRecTrustedPerson){
			//lock lcd
			menuState = 6;
			screenDisplay();
	
			//change message to lowercase
			cstringToLower(smsRecMessage,strlen(smsRecMessage));
	
			// open door
			if(strncmp(smsRecMessage, OPENDOOR, strlen(OPENDOOR)) == 0){
				changeDoorAndText(smsRecFromNumber, smsRecWhen, true);
			}
			//close door
			if(strncmp(smsRecMessage, CLOSEDOOR, strlen(CLOSEDOOR)) == 0){
				changeDoorAndText(smsRecFromNumber, smsRecWhen, false);
			}
			
			//stop the alarm! for fire and manual
			if(strncmp(smsRecMessage, STOPBUZZANDALARM, strlen(STOPBUZZANDALARM)) == 0){
				stopAlarm();
				stopFan();
				
				firing = false;
				manualAlarm = false;
				
				prevTDoorContact = curTDoorContact;
			}
			
			//manual start of alarm
			if(strncmp(smsRecMessage, MANUALALARM, strlen(MANUALALARM)) == 0){
				manualAlarm = true;
				
				startAlarm();
			}
			
			//unlock lcd
			callMainMenu();
		}
		
		Serial.println("---- DONE----");
		
		postSim = false;
	}
	
	//--------------------- OUTPUT LCD ---------------
	//refresh rate for lcd
	if((unsigned long)(mill - prevDisp) > displayRefresh){
		ticksForSensorReset++;
		
		//LCD
		screenDisplay(); 
		prevDisp = mill;
	}

}
