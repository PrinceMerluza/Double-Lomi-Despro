/*PHONEBOK 3
Using actual phonebok entries as entries for usrs
v3 - using char arrays instead of Striong objects
*/

#include <SoftwareSerial.h>
#include <stdio.h>
#define MAXCONTACTS 15
#define TEXTMESSAGEMAX 30

SoftwareSerial SIM900(10, 11);

byte index = 0; //index for filling serial input array
byte iForPBList = 1; //index for phonebook list
char inChar = -1; //temp char for incoming char for serial input
byte state = 0;
const char* TRUSTKEY = "DoMoLuMe"; //contact name for trusted peeps
const char* OPENDOOR = "bukas"; 
const char* CLOSEDOOR = "sara";

byte numContacts = 0;

char input[20] = ""; //serial monitor input

char ATcommand[50] = ""; //commands to be sent to SIM900
char tempNum[12] = ""; //phone number parameter

//first 11 digits of cstring is celphone number. 12th is a letter [a-z] for indexing and ID.
//this makes the technical limit of cp numbers up to 26.
char pbNumberList[MAXCONTACTS][13];

// ---- for receiving text messages. ----------------
//char textMsgBuffer[100]; //47 is minimum for AT message return. technical limitation of 255 dut to iForReceiingMsg 
char textMsg[TEXTMESSAGEMAX]; //actual message content
//boolean receivingMessage = false;
//byte iForReceivingMsg = 0;
char simInputBuffer[100]; //sim900 output for receiving sms
char smsRecFromNumber[14]; // sms received from number
char smsRecMessage[50]; //the actual sms message
char smsRecWhen[21]; //date time of sms received
char messageToSend[100]; //array for the message to send from SIM900
boolean smsRecTrustedPerson = false;
byte iForSimInputBuffer = 0;
boolean postSim = false;

void setup(){
	Serial.begin(19200); // for serial monitor
	while(!Serial);
	SIM900.begin(19200); // for GSM shield
	while(!SIM900);
	delay(100);
	
	countContacts();
	showMenu();

}

void countContacts(){
	numContacts = 0;
	clearNumberList();
	for(byte i = 1; i < MAXCONTACTS; i++){
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

//Display menu to Serial Monitr
void showMenu(){
	Serial.println("1. Add contact");
	Serial.println("2. Remove Contact");
	Serial.println("3. Find Contact");
	Serial.println("4. List Contacts");
	Serial.println("5. List Numbers");
	Serial.println("6. Direct Command");
	Serial.println("7. Send Me Shit");
	Serial.println("0. Factory Reset");
}

//concatenate a single char to a cstring
void addCharToString(char* srcStr, char a){
    char* tmpstr;
    tmpstr = (char *) malloc(2);
    tmpstr[0] = a;
    tmpstr[1] = '\0';
 
    strcat(srcStr, tmpstr);
    free(tmpstr);
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

//reset vlaues after every state change
void resetStuff(){
	ATcommand[0] = '\0';
	iForPBList = 1;
	state = 0;
	showMenu();
}

//sending message
//@number - pointer to cstring for sms number
//@message - pointer to cstring of mesage to be sent
void sendMessage(char * number, char * message){
	ATcommand[0] = '\0';
			
	SIM900.println("AT+CMGF=1");
	delay(100);
	SIM900.print("AT+CMGS=\"");
	SIM900.print(number);
	SIM900.println("\"\r");
	delay(100);
	if(message> 0)
		SIM900.println(message);
	else
		SIM900.println("BLANK SHIT");
	delay(100);
	SIM900.println((char)26);
	delay(100);
	
	//DEBUG
	Serial.print("SENT THIS SMS: ");
	Serial.println(message);
}


void loop(){
	//Serial input
	
	switch(state){		
	//SERIAL INPUT	
	case 0:
		index = 0;
		while(Serial.available() > 0){
			if(index < 19){
				inChar = Serial.read();
				input[index] = inChar;	
				index++;
				input[index] = '\0';
			}
			delay(50); //to capture next characters instead of escaping the while loop
		}
		break;
	
	//LIST CONTACTS	
	case 4:	
		ATcommand[0] = '\0';
		if(iForPBList <= MAXCONTACTS){
			strcpy(ATcommand, "AT+CPBR=");
			addBytToString(ATcommand, iForPBList);
			strcat(ATcommand, "\r");
			
			SIM900.println(ATcommand);
			delay(100);
			iForPBList++;
		}else{
			resetStuff();
		}
		break;
		
	//DELETING ALL CONTACTS
	//NOTE: PROBLEM WITH NOT DELETING ALL CONTACTS!!!
	case 9:
		ATcommand[0] = '\0';
		if(iForPBList <= MAXCONTACTS){
			strcpy(ATcommand, "AT+CPBW=");
			addBytToString(ATcommand, iForPBList);
			strcat(ATcommand, "\r");
			
			SIM900.println(ATcommand);
			delay(100);
			iForPBList++;
		}else{
			resetStuff();
		}
		break;
	default: break;
	}	

	//INPUT TEST
	if(strlen(input) != 0){
		switch(input[0]){
		case '0':
			state = 9;
			break;
		case '1':
			//ADDING CONTACTS
			strcpy(ATcommand, "AT+CPBW=,\"");
			strncpy(tempNum, input + 2, 11);
			strcat(ATcommand, tempNum);
			strcat(ATcommand, "\",129,\"");
			strcat(ATcommand, TRUSTKEY);
			strcat(ATcommand, "\"\r");

			SIM900.print(ATcommand);
			delay(100);
			break;
		case '2':
			//REMOVING CONTACTS
			strcpy(ATcommand, "AT+CPBW=");
			addBytToString(ATcommand, (byte)(input[2] - 96));
			strcat(ATcommand, "\r");

			SIM900.print(ATcommand);
			delay(100);
			break;
		case '3':
		
			break;
		case '4':
			state = 4;
			break;
		case '5':
			//create the list of contact numbers. and display them
			ATcommand[0] = '\0';
			countContacts();
			for(byte i = 0; i < MAXCONTACTS; i++){
				Serial.print(i + 1);
				Serial.print(". ");
				Serial.println(pbNumberList[i]);
			}
			break;
		case '7':
			if(strlen(input) >= 3){
				strncpy(messageToSend, input + 2, 15);
			}else{
				messageToSend[0] = '\0';
			}
			
			sendMessage("+639262543854", messageToSend);
			
			break;
		default: 
			strcpy(ATcommand, input);
			SIM900.println(ATcommand);
			delay(100);
			break;
		}
		input[0] = '\0';
	}

	/*TO ADD: 
		Add another coindition for looping. Receiving text while other
		stuff being done (listing contacts etc.) may fuck up SIM900 prints.
		variable when those things are being done to not go through this loop for
		checking received message.
	*/
	
	if(SIM900.available()){
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
			
					
			//buffer the sim prints
			//if(iForSimInputBuffer < 99) simInputBuffer[iForSimInputBuffer++] = inChar;
			
		}
		//if(iForSimInputBuffer > 30) postSim = true;
	}
	
	//everything to be done after the SMS is received
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
		
		messageToSend[0] = '\0';
		
		if(strncmp(smsRecMessage, OPENDOOR, strlen(OPENDOOR)) == 0){
			Serial.println("DOOR BUKAS!");		
			strcat(messageToSend, smsRecFromNumber);
			strcat(messageToSend, " just opened the door at ");
			strcat(messageToSend, smsRecWhen);
			sendMessage("+639262543854", messageToSend);
		}
		if(strncmp(smsRecMessage, CLOSEDOOR, strlen(CLOSEDOOR)) == 0){
			Serial.println("SUMARA KA BUI!");
			strcat(messageToSend, smsRecFromNumber);
			strcat(messageToSend, " just closed the door at ");
			strcat(messageToSend, smsRecWhen);
			sendMessage("+639262543854", messageToSend);
		}
		
		Serial.println("---- DONE----");
		
		postSim = false;
	}
	
	
	
	/*
	if(strstr(simInputBuffer, "+CMT: ") != NULL){
		if(strstr(simInputBuffer, TRUSTKEY) != NULL){
			Serial.println("\n VALID ");
		}else{
			Serial.println("\n INVALID ");
		}
		
		strncpy(textMsg, strstr(simInputBuffer, "\n") + 1, TEXTMESSAGEMAX - 1);
		Serial.print("\n MESSAGE");
		Serial.println(textMsg);
	}
	*/
	/*
	if(postSim){	
		simInputBuffer[iForSimInputBuffer] = '\0';
		iForSimInputBuffer = 0;
		
		Serial.print("\n P: ");
		Serial.println(simInputBuffer);
		
		//check if sim received a SMS message
		if(strstr(simInputBuffer, "+CMT: ") != NULL){
			if(strstr(simInputBuffer, TRUSTKEY) != NULL){
				Serial.println("\n VALID ");
			}else{
				Serial.println("\n INVALID ");
			}
			
			strncpy(textMsg, strstr(simInputBuffer, "\n") + 1, TEXTMESSAGEMAX - 1);
			Serial.print("\n MESSAGE");
			Serial.println(textMsg);
		}
		
		postSim = false;
	}
	*/

}