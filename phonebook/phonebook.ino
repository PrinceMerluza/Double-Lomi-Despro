/*PHONEBOOK VERSION WHICH USES THE FIRST INDEX OF PHONEBOOK AS MASTERLIST 
and counts the number of entries currently on the pb.
*/

#include <SoftwareSerial.h>
SoftwareSerial SIM900(7, 8);

byte contactNum = 0; //number of contacts  in the MASTERLIST
char incoming_char = 0;
char state = 0; //state for loops
//short iForPBList = 1; //index for phonebook contact print loop
String st = ""; //input for serial monitor
String param = ""; //parameters for input on serial monitor
boolean command = false; //if serial took a command from input

void setup(){
	delay(500);
	Serial.begin(19200); // for serial monitor
	while(!Serial);
	SIM900.begin(19200); // for GSM shield
	while(!SIM900);
	delay(3000);
	
	SIM900.println("AT+CPBR=1");
	while(SIM900.available() > 0){
		st += SIM900.read(); 
		delay(50);
	}
	
	showMenu();
}

void showMenu(){
	Serial.println("1. Add contact");
	Serial.println("2. Remove Contact");
	Serial.println("3. Find Contact");
	Serial.println("4. List Contacts");
	Serial.println("0. Factory Reset");
	Serial.flush();
}


void setContactNum(char num){
	contactNum = num;
	String tmp = "AT+CPBW=1,\"";
	tmp.concat(contactNum);
	tmp.concat("\",129,\"MASTERLIST\"");
	SIM900.println(tmp);
}

void loop(){
	
	//Send commands to HW Serial
	st = "";
	if(Serial.available() > 0){
		while(Serial.available() > 0){
			incoming_char = Serial.read();
			st += incoming_char;
	
			delay(50); //to capture next characters instead of escaping the while loop
		}
		command = true;	
		//Serial.print("LENGTH");
		//Serial.println(st.length());
		//Serial.flush();
	}
	
	/*if(st.length() > 2){
		param = st.substring(2);
		Serial.println("With param: " + param);
		Serial.flush();
	}*/

	if(command){
		String tmp = "";
		switch(st.charAt(0)){
		case '1':
			//update ContactNum to add another one
			setContactNum(contactNum + 1);
			//NOTE: Might cause serial issues because of nextAT Command
			
			tmp = "AT+CPBW=";
			tmp.concat(contactNum+1);
			tmp.concat(",\"");
			tmp.concat(param);
			tmp.concat("\",129,\"DUDE\"\r");
			SIM900.println(tmp);
			delay(100);
			break;
		case '2':
			
			break;
		case '3':
			
			break;
		case '4':
				//state = 4;
				tmp = "AT+CPBR=1,99";
				//tmp.concat(iForPBList+1);
				SIM900.println(tmp);
				delay(100);
			break;
		case '0':
			setContactNum(0);
			break;
		default:
			Serial.println("Invalid Command.");
			break;
		}
		command = false;
	}
	
	
	//Display to Hardware Serial
	boolean simDisp = true;
	while(SIM900.available() > 0)
	{
		if(simDisp){
			Serial.print("SIM900: ");
			simDisp = false;
		}
		incoming_char = SIM900.read(); 
		Serial.print(incoming_char); 
	}
	
	//Loop the phonebook directory
	/*
	if(state == 4){
		if(iForPBList <= contactNum){
			String tmp = "AT+CPBR=";
			tmp.concat(iForPBList+1);
			tmp.concat("\r");
			SIM900.println(tmp);
			delay(100);
			iForPBList++;
		}else{
			iForPBList = 1;
			state = 0;
		}
	}
	*/
	
}