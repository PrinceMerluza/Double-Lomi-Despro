/*
DESPRO 3 - double lomi final season
notes:
Phonebook format for EEPROM.
[MAXCONTACTS][2][11]
[0] - name of contact. (max. of 10 characters)
[1] - number and privilege. ex. 262543854(9d-number part)1(prvilege)
25B every contact. using max nmber of 25
25 * 22 = 550
+ 25B for entry activity array.
575

EEPROM index.
1-12 (Panel_pin) max. of 11 digits.
100 - 675 (TESTING)
700 - 1275(ACTUAL THESIS)

TODO:
sms tsting
testing with many users.
simultaneous teting
*/
#include <Keypad.h>
#include <Wire.h>
#include <stdio.h>
#include "rgb_lcd.h"

#include "SIM900.h"
#include <SoftwareSerial.h>
#include "sms.h"

#include <EEPROM.h>

#define MAINMENUOPTIONS 5
#define MAXCONTACTS 25
#define INBOXSIZE 20
#define DISPLAYREFRESH 750
#define KEYPRESSTIMEOUT 6000
#define ROM_PW 1
#define ROM_PB 100 //note: change when going to prod.

//PIN CONFIGS
#define	PIN_GASSENSOR A0
#define	PIN_REEDSWITCH 47
#define	PIN_WENG 48
#define	PIN_FAN 50
#define	PIN_LOCK 52

//SENSEOR 
#define THRESH_DOOR 10000
#define THRESH_SENSOR 300
#define DECAY_TIME 180000 //3 minutes

//DEBUGGING
//#define DISABLE_SMS
//#define DISABLE_FIRE_SENSOR
//#define DISABLE_DOOR_SENSOR
//#define DISABLE_EEPROM

#define DEBUG_LOMI
#define DEBUG_BREADBOARD_SENSING

//COMMANDS FOR SMS
const char* COM_OPENDOOR = "open"; 
const char* COM_STOPALARM = "stopa";
const char* COM_ALARM = "alarm";

enum SENDTYPE:byte{
	/*
	Used as parameter for function sendMessage to identify
	what type of message is to be sent by the SIM900 and to whom
	*/
	SND_OPENDOOR, //sms command to open the door
	SND_ALARM,  //manual alarming
	SND_DOOROPENED, //manually opening the door
	SND_DOORCLOSED, //manually closin
	SND_STOPALARM, 
	SND_DOORLEFTOPEN, //door open too long
	SND_FIREALARM
};

enum PANEL_STATE:byte{
	STARTING_UP, //first screen while setup is being run
	PASSWORD_SCREEN, //INPUT_NUM for PW entering
	MENU_SCROLL, //SCROLL screen for menu options
	ADD_CONTACT_NUM, //INPUT_NUM 
	ADD_CONTACT_NAME, //INPUT_TEXT
	ADD_CONTACT_PRIV, //SCROLL 2 options
	EDIT_CONTACT, //SCROLL list of contacts
	EDIT_CONTACT_OPTS, //SCROLL selection which partto edit on contact
	EDIT_CONTACT_NUM,
	EDIT_CONTACT_NAME,
	EDIT_CONTACT_PRIV,
	DEL_CONTACT, //SCROLL contacts list
	DEL_CONTACT_CONFIRM, //unused pa
	LST_CONTACT, //SCROLL contacts list
	CHANGE_PW, //INPUT_NUM to put new pw
	ALERT, //Announcement 
	BUSY_SENDING, //unused pa
};
PANEL_STATE st_panel;
PANEL_STATE prev_st_panel;

enum INPUT_SET:byte{
	IN_MENU_SCROLL,
	INPUT_NUMBER,
	INPUT_TEXT,
	CONTACT_SCROLL, //unused
	IN_PRIV_SCROLL, //unused
	CONTINUE, //for ALERT state
	NONE //for the start up screen
} input_type;

enum SCROLL_TYPE:byte{
	//actually reduntjnt to PANEL STATE haha but kept for stability reasons.
	//only applies to IN_MENU_SCROLL states.
	SCR_MAIN_MENU,
	SCR_CONTACTS,
	SCR_DEL_CONTACT,
	SCR_EDIT_CONTACT_OPTS,
	SCR_EDIT_CONTACT,
	SCR_PRIV_OPTION
} scroll_type;

class Phonebook{
	public:
		Phonebook();
		void addContact(char* num, char* name, char* priv);
		void addContact(char* num, char* name);
		//edit function will use menu I so need to convert to real I
		void editContactName(byte index, char* name);
		void editContactNum(byte index, char* num);
		void editContactPriv(byte index, char* priv);
		void actContact(byte index);
		//delete contact using actual array index
		void delContact(byte index);
		//delete contact using index from the menuScrollSpace
		void delContactFromMenuI(byte index); 
		char getPriv(byte index);
		bool checkIfNumExist(char* num);
		bool checkIfNameExist(char* name);
		bool nextCurIndex(void);
		byte getCurIndex(void);
		byte setCurIndex(byte val);
		byte utilMenuItoI(byte index); //convert menu i to actual i
	//private:
		byte cur_index = 0;
		char entries[MAXCONTACTS][2][11];
		bool entries_occupied[MAXCONTACTS];
};
Phonebook::Phonebook(){
	cur_index = 0;
}
void Phonebook::addContact(char* num, char* name, char* priv){
	/*
	num - 11d number. 12 length cstring
	name - 11length cstring
	priv - 0 or 1. 0 - admin. 1 - user.
	*/
	while(entries_occupied[cur_index] == true) nextCurIndex();

	Serial.print("Adding to index ");
	Serial.println(cur_index);
	
	entries[cur_index][0][0] = '\0';
	strncat(entries[cur_index][0], name, 10);
	entries[cur_index][1][0] = '\0';
	strncat(entries[cur_index][1],num + 2,9); 
	strncat(entries[cur_index][1],priv,1);
	
	//ensure that the strings are terminated by char 0
	entries[cur_index][0][10] = '\0';
	entries[cur_index][1][10] = '\0';
	
	entries_occupied[cur_index] = true;
	nextCurIndex();
}
void Phonebook::addContact(char* num, char* name){
	/*
	used on loading from eeprom. lazy converts 9d to 11d phone num and extracts priv
	call original addContact with modified parameters..
	*/
	char t_priv[2] = "";
	t_priv[0] = num[9];
	t_priv[1] = '\0';
	char t_num[12] = "";
	strncat(t_num, "xx", 2);
	strncat(t_num, num, 9);
	addContact(t_num, name, t_priv);
}
void Phonebook::editContactName(byte index, char* name){
	index = utilMenuItoI(index);
	entries[index][0][0] = '\0';
	strncat(entries[index][0], name, 10);
}
void Phonebook::editContactNum(byte index, char* num){
	index = utilMenuItoI(index);
	char orig_priv[1] = {entries[index][1][9]};
	entries[index][1][0] = '\0';
	strncat(entries[index][1], num, 9);
	strncat(entries[index][1], orig_priv, 1);
}
void Phonebook::editContactPriv(byte index, char* priv){
	index = utilMenuItoI(index);
	entries[index][1][9] = priv[0];
}
void Phonebook::actContact(byte index){
	if(index < MAXCONTACTS){
		entries_occupied[index] = true;
	}
}
void Phonebook::delContact(byte index){
	if(index == -1){
		return;
	}
	if(index < MAXCONTACTS){
		Serial.print("Deleting ");
		Serial.println(index);
		entries_occupied[index] = false;
	}
}
void Phonebook::delContactFromMenuI(byte index){
	delContact(utilMenuItoI(index));
}
char Phonebook::getPriv(byte index){
	return entries[index][1][9];
}
bool Phonebook::nextCurIndex(void){
	//-1 if pb is full
	//0 if succesffully go to next cur
	byte orig_cur = cur_index;
	while(true){
		if(entries_occupied[cur_index] == false) return 0;
		
		cur_index++;
		if(cur_index > (MAXCONTACTS - 1)){
			cur_index = 0;
		}
		if(cur_index == orig_cur) return -1;
	}
}
byte Phonebook::getCurIndex(void){
	return cur_index;
}
byte Phonebook::setCurIndex(byte val){
	cur_index = val;
}
byte Phonebook::utilMenuItoI(byte index){
	Serial.print("index of menu: ");
	Serial.println(index);
	byte c = 0;
	for(byte i = 0; i < MAXCONTACTS; i++){
		if(entries_occupied[i]){
			if(c == index){
				return i;
				break;
			}else{
				c++;
			}
		}
	}
	return -1;
}

//Main menu options
const char lcdMainMenu[MAINMENUOPTIONS][17] = {
	"1.Add Contact",
	"2.Edit Contact",
	"3.Del Contacts",
	"4.Lst Contacts",
	"5.Change PIN"
}; 

char menuScrollSpace[MAXCONTACTS][17];

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

//timing and display
unsigned long mill = 0;
unsigned long prevDisp = 0;
byte screenColor[] = {0, 255, 255};
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

//keypad inputs
char kp_input_buffer[12];
byte kp_input_i = 0; //index of char in inputting data
char kp_tmp_letter = 32; //temporary letter while typing text
char kp_prev_k = -1;
unsigned long t_lastPress = 0;

//PIN and PHONEBOOK
char panel_pin[12] = "1234";
char tmp_contact_num[11];
char tmp_contact_name[11];
char tmp_contact_priv[2];
Phonebook pb_users;

//variable for menu display and position
byte maxPage = 0;
byte curPage = 0;
byte curPos = -1;
byte curOption = 0;
byte numOptions = 0;
char alertMsg[16]; //message for alert menu state
PANEL_STATE st_panel_stack[15];
byte st_panel_stack_i = 0;

//SMS related variables
SMSGSM sms;
char smsbuffer[160];
char sms_n[20];
char sms_datetime[25];
byte sms_unreadPos = -1;
char sms_name[12];
unsigned long t_prevSMScheck = 0;
char message_buffer[150] = "";

//sensor
unsigned long t_doorOpen = 0;
bool isDoorOpen = false;
bool doorOpenMuch = false;
//set a time beyond mills before reactivating sensor read. DECAY_TIME
unsigned long t_gasSensorDecay = 0; 
/*  
*  ======================================
*			SCROLL STATE FUNCTIONS
*  =====================================
*/
void updateMenuState(SCROLL_TYPE s){
	byte entry_count = 0; 
	emptyMenuSpace();
	switch(s){
	case SCR_MAIN_MENU:
		for(byte i = 0; i < MAINMENUOPTIONS; i++){
			strcat(menuScrollSpace[i], lcdMainMenu[i]);
		}
		numOptions = MAINMENUOPTIONS;
		maxPage = (MAINMENUOPTIONS / 2) + 1;
		break;
	case SCR_PRIV_OPTION:
		strcat(menuScrollSpace[0], "Admin");
		strcat(menuScrollSpace[1], "User");
		numOptions = 2;
		maxPage = 2;
		break;
	case SCR_DEL_CONTACT: case SCR_EDIT_CONTACT: case SCR_CONTACTS:
		for(byte i = 0; i < MAXCONTACTS; i++){
			if(pb_users.entries_occupied[i] == true){
				Serial.println(pb_users.entries[i][1][9]);
				
				if(pb_users.entries[i][1][9] == '0')
					strcat(menuScrollSpace[entry_count], "[A]");
				strcat(menuScrollSpace[entry_count], pb_users.entries[i][0]);
				entry_count++;
			}
		}
		numOptions = entry_count;
		maxPage = (numOptions / 2) + 1;
		break;
	case SCR_EDIT_CONTACT_OPTS:
		strcat(menuScrollSpace[0], "Name");
		strcat(menuScrollSpace[1], "Number");
		strcat(menuScrollSpace[2], "Privilege");
		numOptions = 3;
		maxPage = (numOptions / 2) + 1;
		break;
	}
}
//remove all entries from menu
void emptyMenuSpace(){
	//maximum contacts is also maximum row for menu options.
	for(byte i = 0; i < MAXCONTACTS; i++){
		 menuScrollSpace[i][0] = '\0';
	}
}

/*  
*  ============================
*			STATES
*  ============================
*/
byte editContactI = -1;
void resetAddContactVars(){
	tmp_contact_name[0] = '\0';
	tmp_contact_num[0] = '\0';
	tmp_contact_priv[0] = '\0';
}
void resetInputVars(){
	kp_input_buffer[0] = '\0';
	kp_input_i = 0;
	kp_prev_k = -1;
	kp_tmp_letter = 32;
}
void resetCurPos(){
	curPos = 0;
	curOption = 0;
	curPage = 0;
}
//set the state to a new one
void setState(PANEL_STATE new_st, char* msg = NULL){
	
	st_panel = new_st;
	if(st_panel == MENU_SCROLL){
		st_panel_stack_i = 3;
		
		debugPrintPB();
	}else if(st_panel == PASSWORD_SCREEN){
		st_panel_stack_i = 2;
	}else{
		st_panel_stack_i++;
	}
	st_panel_stack[st_panel_stack_i] = new_st;

	resetInputVars();
	resetCurPos();
	alertMsg[0] = '\0';
	if(msg)
		strcat(alertMsg, msg);
	else
		strcat(alertMsg, "NaN");	
	setInputFromState();
}
//utility function to set input set after changing state
void setInputFromState(){
	switch(st_panel){
	case STARTING_UP: 
		input_type = NONE; 
		break;
	case PASSWORD_SCREEN: case ADD_CONTACT_NUM: case CHANGE_PW: case EDIT_CONTACT_NUM:
		input_type = INPUT_NUMBER; 
		break;
	case ADD_CONTACT_NAME: case EDIT_CONTACT_NAME:
		kp_prev_k = -1;
		kp_tmp_letter = 32;
		input_type = INPUT_TEXT;
		break;
	case ADD_CONTACT_PRIV: case EDIT_CONTACT_PRIV:
		scroll_type = SCR_PRIV_OPTION;
		updateMenuState(scroll_type);
		input_type = IN_MENU_SCROLL;
		break;
	case MENU_SCROLL: 
		scroll_type = SCR_MAIN_MENU;
		updateMenuState(scroll_type);
		input_type = IN_MENU_SCROLL; 
		//debugPrintPB();
		break;
	case LST_CONTACT:
		scroll_type = SCR_CONTACTS;
		updateMenuState(scroll_type);
		input_type = IN_MENU_SCROLL;
		break;
	case DEL_CONTACT:
		scroll_type = SCR_DEL_CONTACT;
		updateMenuState(scroll_type);
		input_type = IN_MENU_SCROLL;
		break;
	case EDIT_CONTACT:
		scroll_type = SCR_EDIT_CONTACT;
		updateMenuState(scroll_type);
		input_type = IN_MENU_SCROLL;
		break;
	case EDIT_CONTACT_OPTS:
		scroll_type = SCR_EDIT_CONTACT_OPTS;
		updateMenuState(scroll_type);
		input_type = IN_MENU_SCROLL;
		break;
	case ALERT: 
		input_type = CONTINUE; 
		break;
	case BUSY_SENDING:
		input_type = NONE; 
		break;
	}
	delay(10);
	
	checkUnreadSMS();
	displayLCD();
	//showStateStack();
}
void popState(){
	if(st_panel_stack_i > 2){
		//setState(st_panel_stack[--st_panel_stack_i]);
		st_panel_stack_i--;
		st_panel = st_panel_stack[st_panel_stack_i];
		resetInputVars();
		resetCurPos();
		setInputFromState();
	}
}
/*  
*  ============================
*			DISPLAY
*  ============================
*/
void setupLCD(){
	lcd.begin(16, 2);
	lcd.setRGB(screenColor[0], screenColor[1], screenColor[2]);
	lcd.createChar(1, cursor);
	displayLCD();
	delay(100);
}
//called from displayLCD to display input on the 2nd row.
void util_inputDisplayRow(){
	lcd.setCursor(0,1);
	lcd.print(kp_input_buffer);
	lcd.setCursor(kp_input_i,1);
	lcd.print("_");
}
void displayLCD(){
	lcd.clear();
	switch(st_panel){
	case STARTING_UP:
		lcd.setCursor(0,0);
		lcd.print("Starting up...");
		break;
	case PASSWORD_SCREEN:
		lcd.setCursor(0,0);
		lcd.print("Enter Password: ");
		util_inputDisplayRow();
		break;
	case CHANGE_PW:
		lcd.setCursor(0,0);
		lcd.print("Enter New Pass: ");
		util_inputDisplayRow();
		break;
	case ADD_CONTACT_NUM:
		lcd.setCursor(0,0);
		lcd.print("Enter Number: ");
		util_inputDisplayRow();
		break;
	case ADD_CONTACT_NAME:
		lcd.setCursor(0,0);
		lcd.print("Enter Name: ");
		util_inputDisplayRow();
		lcd.setCursor(kp_input_i,1);
		lcd.print(kp_tmp_letter);
		break;
	case EDIT_CONTACT_NUM:
		lcd.setCursor(0,0);
		lcd.print("New Number: ");
		util_inputDisplayRow();
		break;
	case EDIT_CONTACT_NAME:
		lcd.setCursor(0,0);
		lcd.print("New Name: ");
		util_inputDisplayRow();
		lcd.setCursor(kp_input_i,1);
		lcd.print(kp_tmp_letter);
		break;
	case ADD_CONTACT_PRIV:case LST_CONTACT:case EDIT_CONTACT:case DEL_CONTACT:
	case EDIT_CONTACT_PRIV:case MENU_SCROLL:case EDIT_CONTACT_OPTS:
		lcd.setCursor(0, curPos);
		lcd.write(1);
	
		//main menu contents
		lcd.setCursor(1, 0);
		lcd.print(menuScrollSpace[0 + (curPage * 2)]);
		if(curPage != maxPage - 1){
			lcd.setCursor(1, 1);
			lcd.print(menuScrollSpace[1 + (curPage * 2)]);
		}
		break;
	case ALERT:
		lcd.setCursor(0,0);
		lcd.print(alertMsg);
		lcd.setCursor(0,1);
		lcd.print("Press [C]ontinue.");
		break;
	case BUSY_SENDING:
		lcd.setCursor(0,0);
		lcd.print("Busy sending");
		lcd.setCursor(0,1);
		lcd.print("Please wait...");
		break;
	}
}

/*  
*  ============================
*			INPUT
*  ============================
*/
void keyPressed(char k){
	#ifdef DEBUG_LOMI //show stack
	/*Serial.print("Key Input: ");
	Serial.println(k);
	for(byte i = 0; i < 10; i++){
		Serial.print("Stack ");
		Serial.print(i);
		Serial.print(" : ");
		Serial.println(st_panel_stack[i]);
	}
	Serial.print("Current panel: ");
	Serial.println(st_panel);
	*/
	#endif
	
	switch(input_type){
	case INPUT_NUMBER:
		if((kp_input_i < 11) && ((k >= 48) && (k <= 57))){ //if key pressed is numbers 0-9
			kp_input_buffer[kp_input_i] = k;
			kp_input_buffer[++kp_input_i] = '\0';
		}
		if(k == 'B'){
			if(kp_input_i > 0){
				kp_input_buffer[--kp_input_i] = '\0'; 
			}else{
				popState();
			}
		}
		if(k == 'D') kp_input_buffer[0] = '\0';
		if(k == 'A') evaluateInput();
		break;
	case INPUT_TEXT:
		if((kp_input_i < 10) && ((k >= 48) && (k <= 57))){ //if key pressed is numbers 0-9
			if(kp_prev_k != k){
				if(kp_tmp_letter != 32){
					kp_input_buffer[kp_input_i] = kp_tmp_letter;
					kp_input_buffer[++kp_input_i] = '\0';
					kp_tmp_letter = 32;
				}
				kp_prev_k = k;
			}
			//typing the letters form the keypad
			if(k == '1'){ kp_tmp_letter = '1';
			}else if(k == '2'){
				if((kp_tmp_letter == 32) || (kp_tmp_letter == 'C')) kp_tmp_letter = 'A';
				else if(kp_tmp_letter == 'A') kp_tmp_letter = 'B';
				else if(kp_tmp_letter == 'B') kp_tmp_letter = 'C';
			}else if(k == '3'){
				if((kp_tmp_letter == 32) || (kp_tmp_letter == 'F')) kp_tmp_letter = 'D';
				else if(kp_tmp_letter == 'D') kp_tmp_letter = 'E';
				else if(kp_tmp_letter == 'E') kp_tmp_letter = 'F';
			}else if(k == '4'){
				if((kp_tmp_letter == 32) || (kp_tmp_letter == 'I')) kp_tmp_letter = 'G';
				else if(kp_tmp_letter == 'G') kp_tmp_letter = 'H';
				else if(kp_tmp_letter == 'H') kp_tmp_letter = 'I';
			}else if(k == '5'){
				if((kp_tmp_letter == 32) || (kp_tmp_letter == 'L')) kp_tmp_letter = 'J';
				else if(kp_tmp_letter == 'J') kp_tmp_letter = 'K';
				else if(kp_tmp_letter == 'K') kp_tmp_letter = 'L';
			}else if(k == '6'){
				if((kp_tmp_letter == 32) || (kp_tmp_letter == 'O')) kp_tmp_letter = 'M';
				else if(kp_tmp_letter == 'M') kp_tmp_letter = 'N';
				else if(kp_tmp_letter == 'N') kp_tmp_letter = 'O';
			}else if(k == '7'){
				if((kp_tmp_letter == 32) || (kp_tmp_letter == 'S')) kp_tmp_letter = 'P';
				else if(kp_tmp_letter == 'P') kp_tmp_letter = 'Q';
				else if(kp_tmp_letter == 'Q') kp_tmp_letter = 'R';
				else if(kp_tmp_letter == 'R') kp_tmp_letter = 'S';
			}else if(k == '8'){
				if((kp_tmp_letter == 32) || (kp_tmp_letter == 'V')) kp_tmp_letter = 'T';
				else if(kp_tmp_letter == 'T') kp_tmp_letter = 'U';
				else if(kp_tmp_letter == 'U') kp_tmp_letter = 'V';
			}else if(k == '9'){
				if((kp_tmp_letter == 32) || (kp_tmp_letter == 'Z')) kp_tmp_letter = 'W';
				else if(kp_tmp_letter == 'W') kp_tmp_letter = 'X';
				else if(kp_tmp_letter == 'X') kp_tmp_letter = 'Y';
				else if(kp_tmp_letter == 'Y') kp_tmp_letter = 'Z';
			}
		}
		if(k == '0'){ //continue to next letter
			if(kp_tmp_letter != 32){
				kp_input_buffer[kp_input_i] = kp_tmp_letter;
				kp_input_buffer[++kp_input_i] = '\0';
				kp_tmp_letter = 32;
			}
		}
		if(k == 'B'){
			if(kp_input_i > 0){
				kp_tmp_letter = kp_input_buffer[--kp_input_i];
				kp_input_buffer[kp_input_i] = '\0'; 
				kp_prev_k = -1;
			}else{
				popState();
			}
		}
		if(k == 'D'){
			resetInputVars();
		}
		if(k == 'A') {
			kp_input_buffer[kp_input_i] = kp_tmp_letter;
			kp_input_buffer[++kp_input_i] = '\0';
			evaluateInput();
		}
		
		#ifdef DEBUG_LOMI //show kp_input_buffer
		//Serial.println(kp_input_buffer);
		#endif
		break;
	case IN_MENU_SCROLL:
		if(k == 'D'){ //DOWN button
			curOption++;
			if(curOption >= numOptions) curOption = 0;
		}
		if(k == 'C'){ // UP button
			curOption--;
			if(curOption == 255) curOption = numOptions - 1;
		}
		curPage = curOption / 2;
		curPos = curOption % 2;
		
		if(k == 'B') popState();
		if(k == 'A'){
			if(scroll_type == SCR_MAIN_MENU){
				switch(curOption){
				case 0: setState(ADD_CONTACT_NUM); break;
				case 1: setState(EDIT_CONTACT); break;
				case 2: setState(DEL_CONTACT); break;
				case 3: setState(LST_CONTACT); break;
				case 4: setState(CHANGE_PW); break;
				}
			}else if(scroll_type == SCR_PRIV_OPTION){
				tmp_contact_priv[0] = curOption + 48;
				if(st_panel == ADD_CONTACT_PRIV){
					pb_users.addContact(tmp_contact_num, tmp_contact_name, tmp_contact_priv);
					
					//EEPROM SAVE
					savePhonebook();
					
					setState(MENU_SCROLL);
					setState(ALERT, "Contact Added");
				}else if(st_panel == EDIT_CONTACT_PRIV){
					pb_users.editContactPriv(editContactI,tmp_contact_priv);
					
					//EEPROM SAVE
					savePhonebook();
					
					setState(MENU_SCROLL);
					setState(ALERT, "Contact Updated");
				}
				resetAddContactVars();
				//debugPrintPB();
			}else if(scroll_type == SCR_DEL_CONTACT){
				pb_users.delContactFromMenuI(curOption);
				
				//EEPROM SAVE
				savePhonebook();
				
				setState(MENU_SCROLL);
				setState(ALERT, "Contact Deleted");
			}else if(scroll_type == SCR_EDIT_CONTACT){
				editContactI = curOption;
				setState(EDIT_CONTACT_OPTS);
			}else if(scroll_type == SCR_EDIT_CONTACT_OPTS){
				switch(curOption){
				case 0: setState(EDIT_CONTACT_NAME); break;
				case 1: setState(EDIT_CONTACT_NUM); break;
				case 2: setState(EDIT_CONTACT_PRIV); break;
				}
			}else if(scroll_type == SCR_CONTACTS){
				char t_num_show[16] = "";
				strncat(t_num_show, "09", 2);
				strncat(t_num_show, pb_users.entries[pb_users.utilMenuItoI(curOption)][1], 9);
				setState(ALERT, t_num_show);
			}
		}
		break;
	case CONTINUE:
		if(k == 'C') popState();
		break;
	}
}
/*Validation return values
*0 - no error
*1 - invalid form
*2 - exists
*/
byte validateNum(){
	if((strlen(kp_input_buffer) != 11) || 
		(kp_input_buffer[0] != '0') ||
		(kp_input_buffer[1] != '9')){
		return 1;
	}
	for(byte i = 0; i < MAXCONTACTS; i++){
		if(pb_users.entries_occupied[i]){
			if(strncmp(pb_users.entries[i][1], to9CPNum(kp_input_buffer), 9) == 0)
				return 2;
		}
	}
	return 0;
}
byte validateName(){
	if(strlen(kp_input_buffer) < 3) return 1;
	for(byte i = 0; i < MAXCONTACTS; i++){
		if(pb_users.entries_occupied[i]){
			if(strncmp(pb_users.entries[i][0], kp_input_buffer, strlen(kp_input_buffer)) == 0)
				return 2;
		}
	}
	return 0;
}
void evaluateInput(){
	switch(st_panel){
	case PASSWORD_SCREEN:
		if(strcmp(kp_input_buffer, panel_pin) == 0){
			setState(MENU_SCROLL);
		}else{
			setState(ALERT, "Wrong PIN");
		}
		break;
	case ADD_CONTACT_NUM:	
		if(validateNum() == 0){
			tmp_contact_num[0] = '\0';
			strcat(tmp_contact_num, kp_input_buffer);
			setState(ADD_CONTACT_NAME);
		}else if(validateNum() == 1){
			setState(ALERT,"Invalid number");
		}else if(validateNum() == 2){
			setState(ALERT,"Number Exists");
		}
		break;
	case ADD_CONTACT_NAME:
		if(validateName() == 0){
			tmp_contact_name[0] = '\0';
			strcat(tmp_contact_name, kp_input_buffer);
			setState(ADD_CONTACT_PRIV);
		}else if(validateName() == 1){
			setState(ALERT, "Name too short");
		}else if(validateName() == 2){
			setState(ALERT, "Name Exists");
		}
		break;
	case EDIT_CONTACT_NUM:	
		if(validateNum() == 0){
			tmp_contact_num[0] = '\0';
			strcat(tmp_contact_num, kp_input_buffer);
			pb_users.editContactNum(editContactI, tmp_contact_num);
			
			//EEPROM SAVE
			savePhonebook();
			
			setState(MENU_SCROLL);
			setState(ALERT,"Updated Contact");
		}else if(validateNum() == 1){
			setState(ALERT,"Invalid number");
		}else if(validateNum() == 2){
			setState(ALERT,"Number Exists");
		}
		break;
		break;
	case EDIT_CONTACT_NAME:
		if(validateName() == 0){
			tmp_contact_name[0] = '\0';
			strcat(tmp_contact_name, kp_input_buffer);
			pb_users.editContactName(editContactI, tmp_contact_name);
			
			//EEPROM SAVE
			savePhonebook();
			
			setState(MENU_SCROLL);
			setState(ALERT,"Updated Contact");
		}else if(validateName() == 1){
			setState(ALERT, "Name too short");
		}else if(validateName() == 2){
			setState(ALERT, "Name Exists");
		}
		break;
		break;
	case CHANGE_PW:
		if(strlen(kp_input_buffer) >= 4){
			panel_pin[0] = '\0';
			strcat(panel_pin, kp_input_buffer);
			setState(PASSWORD_SCREEN);
			
			//EEPROM SAVE
			savePW();
			
			setState(ALERT, "Pin Changed");
		}else{
			setState(ALERT, "PIN too short.");
		}
		break;
	}
}

/*  
*  ============================
*			SMS STUFF
*  ============================
*/
/* DETAILS
	checkUnreadSMS -> verifySMSSender -> sendMessage -> sendtommebver
*/
char * to13CPNum(char *num){
	static char t_num[14] = "";
	t_num[0] = 0;
	strcat(t_num,"+63");
	
	if(strlen(num) == 11){
		strcat(t_num, num+1);
	}else if(strlen(num) == 9){
		strcat(t_num,"9");
		strcat(t_num, num);
	}
	return t_num;
}
char * to11CPNum(char *num){
	static char t_num[12] = "";
	t_num[0] = 0;
	strcat(t_num,"0");
	
	if(strlen(num) == 13){
		strcat(t_num, num+3);
	}else if(strlen(num) == 9){
		strcat(t_num,"9");
		strcat(t_num, num);
	}
	return t_num;
}
char * to9CPNum(char *num){
	static char t_num[14] = "";
	t_num[0] = 0;
	
	if(strlen(num) == 13){
		strcat(t_num, num+4);
	}else if(strlen(num) == 11){
		strcat(t_num, num+2);
	}
	return t_num;
}
void clearInbox(){
	#ifdef DISABLE_SMS
	return;
	#endif
	
	for(byte i = 0; i < INBOXSIZE; i++){
		sms.DeleteSMS(i);
	}
}
bool verifySMSSender(char *num){
	#ifdef DISABLE_SMS
	return true;
	#endif
	
	for(byte i = 0; i < MAXCONTACTS; i++){
		if(pb_users.entries_occupied[i]){
			if(strncmp(pb_users.entries[i][1], to9CPNum(num), 9) == 0){
				sms_name[0] = '\0';
				strcat(sms_name, pb_users.entries[i][0]);
				return true;
			}
		}
	}
	return false;
}
void checkUnreadSMS(){
	#ifdef DISABLE_SMS
	return;
	#endif
	//sms_n[0] = '\0';
	//sms_datetime[0] = '\0';
	//smsbuffer[0] = '\0';
	/*
	sms_unreadPos = sms.IsSMSPresent(SMS_ALL);
	if(sms_unreadPos){
		Serial.println("received text");
		byte r = sms.GetSMS(sms_unreadPos, sms_n, 20, sms_datetime, smsbuffer, 160);
		delay(100);
		Serial.println(sms_n);
		Serial.println(sms_datetime);
		Serial.println(smsbuffer);
		if(verifySMSSender(sms_n))executeSMS();
		sms.DeleteSMS(sms_unreadPos);
		delay(100);
		sms_unreadPos = -1;
	}else{
		Serial.println("no text");
	}*/
	byte rr = sms.GetSMS(1, sms_n, 20, sms_datetime, smsbuffer, 160);
	toLowerCase(smsbuffer);
	
	Serial.print("RETURN: ");
	Serial.println(rr);
	
	if(rr == 2){
		Serial.println("received text");
		//sms.GetSMS(1, n, 20, datetime, smsbuffer, 160);
		Serial.println(sms_n);
		Serial.println(sms_datetime);
		Serial.println(smsbuffer);
		
		Serial.print("DELETEION: ");
		Serial.println(sms.DeleteSMS(1));
		
		if(verifySMSSender(sms_n))executeSMS();
	}else if(rr == 1){
		Serial.print("DELETEION: ");
		Serial.println(sms.DeleteSMS(1));
	}else{
		Serial.println("no text");
	}
	
}
void executeSMS(){
	Serial.print("SMSN VALUE: ");
	Serial.println(sms_n);
	
	#ifdef DISABLE_SMS
	return;
	#endif
	#ifdef DEBUG_LOMI
	Serial.print("COMMAND: ");
	Serial.println(smsbuffer);
	#endif
	
	//OPEN DOOR COMMAND
	if(strncmp(smsbuffer, COM_OPENDOOR, strlen(COM_OPENDOOR)) == 0){
		digitalWrite(PIN_LOCK, HIGH);
		delay(200);
		digitalWrite(PIN_LOCK, LOW);
		Serial.println("BUKAS@!!!!!!!");
		
		sendMessage(SND_OPENDOOR);
	//STOP THE ALARM COMMAND
	}else if(strncmp(smsbuffer, COM_STOPALARM, strlen(COM_STOPALARM)) == 0){
		off_Weng();
		off_Fan();
		sendMessage(SND_STOPALARM);
	//MANUAL ALARM
	}else if(strncmp(smsbuffer, COM_ALARM, strlen(COM_ALARM)) == 0){
		on_Weng();
		on_Fan();
		sendMessage(SND_ALARM);
	}
}
//recepient. 0 - for admins. 1 - for all.
void sendMessage(SENDTYPE s){	
	Serial.print("ASDASD");
	Serial.println(s);
	#ifdef DISABLE_SMS
	return;
	#endif

	setState(BUSY_SENDING);
	
	message_buffer[0] ='\0';
	switch(s){
	case SND_OPENDOOR:		
		strcat(message_buffer, sms_name);
		//strcat(message_buffer, "(");
		//strcat(message_buffer, sms_n);
		strcat(message_buffer, " texted to open the door on ");
		strcat(message_buffer, sms_datetime);
	
		sendToMember(0, message_buffer);
		break;
	case SND_DOOROPENED:
		
		strcat(message_buffer, "Door was manually opened.");
		sendToMember(0, message_buffer);
		break;
	case SND_DOORCLOSED:
		
		strcat(message_buffer, "Door was manually closed.");
		sendToMember(0, message_buffer);
		break;
	case SND_DOORLEFTOPEN:
		
		strcat(message_buffer, "Door is still open. Please close.");
		sendToMember(1, message_buffer);
		break;
	case SND_FIREALARM:
		
		strcat(message_buffer, "Gas sensor detected leakage! Possible fire.");
		sendToMember(1, message_buffer);
		break;
	case SND_ALARM:
		strcat(message_buffer, sms_name);
		//strcat(message_buffer, "(");
		//strcat(message_buffer, sms_n);
		strcat(message_buffer, " set a manual alarm. Please follow up.");
		sendToMember(1, message_buffer);
		break;
	case SND_STOPALARM:
		strcat(message_buffer, sms_name);
		//strcat(message_buffer, "(");
		//strcat(message_buffer, t_sms_n);
		strcat(message_buffer, " stopped the alarm.");
		sendToMember(1, message_buffer);
		break;
	default:
		break;
	}
	
	popState();
}
void sendToMember(byte p, char * message){
	char t_num[12] = "";
	
	for(byte i = 0; i < MAXCONTACTS; i++){
		if((p == 1) || ((p== 0) && (pb_users.getPriv(i) == '0'))){
			if(pb_users.entries_occupied[i]){
				t_num[0] = '\0';
				strcat(t_num, "09");
				strncat(t_num, pb_users.entries[i][1], 9);
				
				Serial.println((byte)sms.SendSMS(t_num, message));
				Serial.print("SENT TO ");
				Serial.println(t_num);
				
			}
		}
	}
}
void toLowerCase(char * s){
	while(s[0] != '\0'){
		s[0] = tolower(s[0]);
		s++;
	}
}
/*  
*  ============================
*			SENSOR TEST
*  ============================
*/
void off_Weng(){
	digitalWrite(PIN_WENG, LOW);
}
void off_Fan(){
	digitalWrite(PIN_FAN, LOW);
}
void on_Weng(){
	digitalWrite(PIN_WENG, HIGH);
}
void on_Fan(){
	digitalWrite(PIN_FAN, HIGH);
}
void evt_Fire(){
	on_Fan();
	on_Weng();
	
	sendMessage(SND_FIREALARM);
}
void evt_DoorOpenTooLong(){
	doorOpenMuch = true;
	sendMessage(SND_DOORLEFTOPEN);
	on_Weng();
}
void evt_DoorOpened(){
	sendMessage(SND_DOOROPENED);
}
void evt_DoorClosed(){
	off_Weng();
	doorOpenMuch = false;
	sendMessage(SND_DOORCLOSED);
}
void checkDoorOpenTime(){
	#ifdef DISABLE_DOOR_SENSOR
	return;
	#endif
	
	if(digitalRead(PIN_REEDSWITCH) == HIGH){
		if(isDoorOpen) evt_DoorClosed();
		isDoorOpen = false;
	}
	
	if((!isDoorOpen) && (digitalRead(PIN_REEDSWITCH) == LOW)){
		evt_DoorOpened();
		isDoorOpen = true;
		t_doorOpen = millis();
	}
	
	if((!doorOpenMuch) && (isDoorOpen) && ((unsigned long)(t_doorOpen - millis()) > THRESH_DOOR)){
		evt_DoorOpenTooLong();
	}
}
void checkFireSensor(){
	#ifdef DISABLE_FIRE_SENSOR
	return;
	#endif
	
	if(t_gasSensorDecay < millis()){
		if(analogRead(PIN_GASSENSOR) > THRESH_SENSOR){
			evt_Fire();
			t_gasSensorDecay = (unsigned long)(millis() + DECAY_TIME);
		}
	}
}

/*  
*  ============================
*			EEPROM SAVING
*  ============================
*/
void savePhonebook(){
	#ifdef DISABLE_EEPROM
	return;
	#endif
	//debugPrintPB();
	
	Serial.println("WRITING TO EEPROM");
	int addr = ROM_PB;
	
	//saving the phonebook contents
	for(byte i = 0; i < MAXCONTACTS; i++){
		EEPROM.put(addr, pb_users.entries[i][0]);
		addr += 11;
		EEPROM.put(addr, pb_users.entries[i][1]);
		addr += 11;
	}
	
	//saving the entry occupied array
	addr = ROM_PB + (MAXCONTACTS * 11 * 2);
	for(byte i = 0; i < MAXCONTACTS; i++){
		EEPROM.put(addr, pb_users.entries_occupied[i]);
		addr++;
	}
}
void loadPhonebook(){
	#ifdef DISABLE_EEPROM
	return;
	#endif
	int addr = ROM_PB;
	char tmp_num[11] = "";
	char tmp_name[11] = "";
	
	//load phonebook
	for(short i = 0; i < MAXCONTACTS; i++){
		for(short j = 0; j < 11; j++){
			tmp_name[j] = (char)EEPROM.read(addr + (i * 22) + (j));
			tmp_num[j] = (char)EEPROM.read(addr + (i * 22) + (j + 11));
		}
		pb_users.addContact(tmp_num, tmp_name);
		tmp_num[0] = '\0';
		tmp_name[0] = '\0';
	}
	
	addr = ROM_PB + (MAXCONTACTS * 11 * 2);
	//load entry activity array
	for(byte i = 0; i < MAXCONTACTS; i++){
		if((bool)EEPROM.read(addr + i)) pb_users.actContact(i);
		else pb_users.delContact(i);
	}
	
	pb_users.setCurIndex(0);
	
	/* direct connect to entries array. doesnt seemt o work somehow lol
	//loading phonebook entries
	for(short i = 0; i < (11 * 2 * MAXCONTACTS); i++){
		pb_users.entries[i / (11 * 2)]
		[(i % (11 * 2)) / 11][i / 11] = (char)EEPROM.read(addr + i);
	}
	
	addr = ROM_PB + (11 * 2 * MAXCONTACTS); //go to end of pb entry in eeprom
	//load entry activity array
	for(byte i = 0; i < MAXCONTACTS; i++){
		pb_users.entries_occupied[i] = (bool)EEPROM.read(addr);
		addr++;
	}
	*/
}
void savePW(){
	#ifdef DISABLE_EEPROM
	return;
	#endif
	Serial.println("WRITING TO EEPROM");
	EEPROM.put(ROM_PW, panel_pin);
}
void loadPW(){
	#ifdef DISABLE_EEPROM
	return;
	#endif
	
	char x[12] = "";
	
	for(byte i = 0; i < sizeof(panel_pin); i++){
		panel_pin[i] = (char)EEPROM.read(ROM_PW + i);
	}
}
/*  
*  ============================
*			DEBUG
*  ============================
*/
void debugPrintPB(){
	Serial.println("--------");
	for(byte i = 0; i < 25; i++){
		Serial.print("PB Entry: ");
		Serial.print(i);
		if(pb_users.entries_occupied[i])
			Serial.print("  ACTIVE  ");
		else
			Serial.print("  !DEAD!  ");
		Serial.print("\tName: ");
		Serial.print(pb_users.entries[i][0]);
		Serial.print("\tNumber: ");
		Serial.println(pb_users.entries[i][1]);
	}
	Serial.println("--------");
	Serial.print("Cursor: "); Serial.println(pb_users.getCurIndex());
	Serial.println("--------");
}
void debugPrintMenu(){
	Serial.println("--DEBUG PRINT MENU --");
	for(byte i = 0; i < MAXCONTACTS; i++){
		Serial.print(i);
		Serial.print(" ");
		Serial.println(menuScrollSpace[i]);
	}
}
void debugAddContact(){
	pb_users.addContact("","PRINCE","0");
	pb_users.addContact("","2Karla","0");
	pb_users.addContact("09123456789","3Test","1");
	pb_users.addContact("09000000000","4Shit","1");
	pb_users.addContact("09444444444","5NiceIe","1");
	//pb_users.delContact(2);
	pb_users.addContact("09444444444","6Lastone","1");
	pb_users.addContact("09000000000","7 asd","1");
	pb_users.addContact("09000000000","8 zz","1");
	pb_users.addContact("09000000000","9 LLL","1");
}
void showInbox(){
	for(byte i = 0; i < 40; i++){
		smsbuffer[0] = '\0';
		sms_datetime[0] = '\0';
		sms.GetSMS(i,sms_n,20,sms_datetime,smsbuffer,160);
		
		Serial.println("--------");
		Serial.print("INDEX: ");
		Serial.println(i);
		Serial.print("NUMBER: ");
		Serial.println(sms_n);
		Serial.print("DATE: ");
		Serial.println(sms_datetime);
		Serial.print("MESSAGE: ");
		Serial.println(smsbuffer);
	}
	Serial.println("--------");
}
void showEEPROMVals(){  
	int addr = ROM_PB;
	for(short i = 0; i < (11 * 2 * MAXCONTACTS); i++){
		Serial.print((char)EEPROM.read(addr + i));
	}
}
void showStateStack(){
	Serial.println("---- STATE STACK -------");
	for(byte i = 0; i < sizeof(st_panel_stack); i++){
		Serial.print(i);
		Serial.print(".) ");
		Serial.print(st_panel_stack[i]);
		if(st_panel_stack_i == i) Serial.print(" <---");
		Serial.println();
	}
	Serial.println("----------------------------------");
}
void erasePB(){
	for(byte i = 0; i < MAXCONTACTS; i++){
		pb_users.delContact(i);
	}
	savePhonebook();
}
/*  
*  ============================
*			SETUP AND LOOP
*  ============================
*/
void setupSMS(){
	#ifdef DISABLE_SMS
	return;
	#endif
	
	if (gsm.begin(2400)) {
		Serial.println("\nstatus=SMS READY");
	} else {
		Serial.println("\nstatus=SMS IDLE");
	}
	delay(1000);
}
void configurePins(){
	pinMode(PIN_REEDSWITCH, INPUT); 
	pinMode(PIN_GASSENSOR, INPUT); 
	pinMode(PIN_FAN, OUTPUT); 
	pinMode(PIN_WENG, OUTPUT); 
	pinMode(PIN_LOCK, OUTPUT); 
	
	#ifdef DEBUG_BREADBOARD_SENSING
	pinMode(40, OUTPUT);
	#endif
}
void setup(){
	Serial.begin(9600);
	delay(100);
	setupLCD();
	delay(100);
	setState(STARTING_UP);
	
	configurePins();
	setupSMS(); delay(3000);
	clearInbox();
	//eeprom load
	loadPW(); delay(100);
	loadPhonebook(); delay(100);
	
	//erasePB();
	//------startup debug
	//debugAddContact();
	//savePhonebook();
	//showEEPROMVals();
	//erasePB();
	debugPrintPB();
	//-------end startup
	
	setState(PASSWORD_SCREEN);
	
}

bool keypadIsFree = false;
void loop(){
	#ifdef DEBUG_BREADBOARD_SENSING
	if(digitalRead(PIN_REEDSWITCH))
		digitalWrite(40, HIGH);
	else
		digitalWrite(40, LOW);
	#endif
	
	mill = millis();
	//input
	char key = keypad.getKey();
	if (key){
		t_lastPress = mill;
		keypadIsFree = false;
		
		keyPressed(key);
		
		displayLCD();
	}
	
	//reset to the password screen
	if((!keypadIsFree) && 
	  ((unsigned long)(mill - t_lastPress) > KEYPRESSTIMEOUT)){
		setState(PASSWORD_SCREEN);
		keypadIsFree = true;
	}
	
	//sensor test and sms check. no longer for displaying
	if((unsigned long)(mill - prevDisp) > DISPLAYREFRESH){
		//displayLCD();
		
		//sensors
		checkDoorOpenTime();
		checkFireSensor();
		
		prevDisp = mill;
	}
	
	//mill = millis();
	if(keypadIsFree && ((unsigned long)(mill - t_prevSMScheck) > 1800)){
		checkUnreadSMS();
		
		t_prevSMScheck = mill;
	}
}
