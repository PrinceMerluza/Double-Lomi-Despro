/* 
LC DCOntrol with Keypad + LCD
*/
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include "rgb_lcd.h"
#define MAINMENUOPTIONS 3

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

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//MAIN MENU SETUP
const char lcdMainMenu[MAINMENUOPTIONS][17] = {
	"1.Add Contact",
	"2.Del Contact",
	"3.Lst Contacts"
}; 
byte menuState = 0;
//states 0 -main menu.
//1: adding a contact number
//2: delete a number
//3: List Contacts
//4: Messgae

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
	Serial.begin(19200);
	while(!Serial);
	
	//lcd setup
	lcd.begin(16, 2);
	lcd.setRGB(0,255,255);
	lcd.createChar(1, cursor);
	
	callMainMenu();
	delay(1000); 
}

//switch menu to the main menu
void callMainMenu(){
	curPos = 0;
	curOption = 0;
	curPage = 0;
	maxPage = (MAINMENUOPTIONS / 2) + 1;
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

void showMessage(char* msg){
	messageForDisplay[0] = '\0';
	strcat(messageForDisplay, msg);
	prevState = menuState;
	menuState = 4;
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
			case 0: menuState = 1; break;
			case 1: menuState = 2; break;
			case 2: menuState = 3; break;
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

				//
				// CODE FOR ADDING NUMBER TO PHONEBOOK
				//
				
				//cleanup
				inputPhoneNum[0] = '\0';
				menuState = 0;
				showMessage("Number added");
			}
		}
	case 4:
		if(k == 'C')
			menuState = prevState;
		break;
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
	case 4:
		lcd.setCursor(0,0);
		lcd.print(messageForDisplay);
		lcd.setCursor(0, 1);
		lcd.print("press [C]ontinue");
	}
}

void loop(){
	char key = keypad.getKey();
	mill = millis();

	//check for keypad input
	if (key){
		keyPressed(key);
	}
	
	//refresh rate for lcd
	if((unsigned long)(mill - prevDisp) > displayRefresh){
		screenDisplay();
		prevDisp = mill;
	}
}