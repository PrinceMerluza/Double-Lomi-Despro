/*PHONEBOK 2
Using actual phonebok entries as entries for usrs
*/

#include <SoftwareSerial.h>
#include <stdio.h>
#define MAXCONTACTS 20

SoftwareSerial SIM900(10, 11);

char incoming_char = 0;
byte state = 0;
byte iForPBList = 1;
byte numContacts = 0;
String st = ""; //input for serial monitor from sim900
String tmp = "";
String param = ""; //parameters for input on serial monitor
boolean command = false; //if serial took a command from input

void setup(){
	Serial.begin(19200); // for serial monitor
	while(!Serial);
	SIM900.begin(19200); // for GSM shield
	while(!SIM900);
	delay(100);
	
	SIM900.println("AT+CPBR=1,99");
	delay(100);
	if(SIM900.available() > 0){
		st = SIM900.readString();
		Serial.println(st.length());
		for(short i = 0; i < st.length(); i++){
			if(st.charAt(i) == ':')
				numContacts++;
		}
		
		//delay(50);
	}
	
	showMenu();
	Serial.print("Number of Contacts: ");
	Serial.println(numContacts);
}

void showMenu(){
	Serial.println("1. Add contact");
	Serial.println("2. Remove Contact");
	Serial.println("3. Find Contact");
	Serial.println("4. List Contacts");
	Serial.println("0. Factory Reset");
	Serial.flush();
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
	
	if(st.length() > 2){
		param = st.substring(2);
		//Serial.println("With param: " + param);
		//Serial.flush();
	}

	if(command){
		tmp = ""; 
		switch(st.charAt(0)){
		case '1':
			//SIM900.print("AT+CPBW=,\"\",129,\"Last Test\"\r"); //thgis works
			//below does not work
			tmp = "AT+CPBW=";
			tmp.concat(",\"");
			tmp.concat(param);
			tmp.concat("\",129,\"DUDE\"\r");
			SIM900.print(tmp); 

			
			SIM900.print(tmp);
			delay(100);
			break;
		case '2':
			
			break;
		case '3':
			
			break;
		case '4':
			/*tmp = "AT+CPBR=1,99";
			SIM900.println(tmp);*/
			state = 4;
			break;
		case '0':
			state = 5;
			break;
		default:
			Serial.println("Invalid Command.");
			break;
		}
		command = false;
	}
	
	
	//Display to Hardware Serial
	//boolean simDisp = true;
	while(SIM900.available() > 0)
	{
		/*if(simDisp){
			Serial.print("SIM900: ");
			simDisp = false;
		}*/
		incoming_char = SIM900.read(); 
		Serial.print(incoming_char); 
		//Serial.flush();
	}
	
	//Loop the phonebook directory
	if(state == 4){
		if(iForPBList <= MAXCONTACTS){
			tmp = "AT+CPBR=";
			tmp.concat(iForPBList);
			tmp.concat("\r");
			SIM900.println(tmp);
			delay(100);
			iForPBList++;
		}else{
			iForPBList = 1;
			state = 0;
		}
	}
	
	//deleting all contacts
	if(state == 5){
		if(iForPBList <= MAXCONTACTS){
			tmp = "AT+CPBW=";
			tmp.concat(iForPBList);
			SIM900.println(tmp);
			iForPBList++;
		}else{
			iForPBList = 1;
			state = 0;
		}
	}

}
