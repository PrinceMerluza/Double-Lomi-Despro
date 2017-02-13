/*
	ADDITION
	3 digits
	

	SUBTRACTIOn 
	3 digits. 
	No negative result.

	MULTIPLICATION
	1-12 both digits.

	DIVISION
	2 digits / 1 digits
*/

#include "rgb_lcd.h"

rgb_lcd lcd;

byte score = 0;
byte operationType = 0;
/*
0 - addition
1 - subtraction
2 - multiplication
3 - division
*/
boolean nextQuestion = true;
const char operators[] = {'+', '-', '*', '/'};

short min_3d = 50;
short max_3d = 1000;
short min_2d = 1;
short max_2d = 13;
short operand_1 = 0;
short operand_2 = 0;
short theAnswer = 0;
short wrongAnswer = 0;
byte posOfCorrect = 0; //0 or 1
boolean stopPlay = false;

int cdTimer = 0; //in seconds
const int MAXIMUM_TIME = 30; //in seconds
unsigned long curT = 0;
unsigned long prevT = 0;

int but1State;
int prevBut1State = LOW;
int but2State;
int prevBut2State = LOW;
unsigned long lastDebounceTime = 0;  
unsigned long debounceDelay = 50;

//PIN CONFIGURATION
byte pinBut_1 = 2;
byte pinBut_2 = 3;
byte pinAlarm = 4;

void generateQuestion(){
	Serial.println("GENERATING QUESTION");
	
	operationType = random(4);
	
	switch(operationType){
	case 0: //addition
		operand_1 = random(min_3d, max_3d);
		operand_2 = random(min_3d, max_3d);
		theAnswer = operand_1 + operand_2;
		
		wrongAnswer = theAnswer + random(-25,25); 
		break;
	case 1: //subtraction
		operand_1 = random(min_3d, max_3d);
		operand_2 = random(min_3d - 20, operand_1 - 15);
		theAnswer = operand_1 - operand_2;
		
		wrongAnswer = theAnswer + random(-25,25); 
		break;
	case 2: //multiplication
		operand_1 = random(min_2d, max_2d);
		operand_2 = random(min_2d, max_2d);
		theAnswer = operand_1 * operand_2;
		
		wrongAnswer = theAnswer + random(-25,25); 
		break;
	case 3: //division
		operand_2 = random(min_2d, max_2d);
		theAnswer = random(min_2d, max_2d);
		operand_1 = operand_2 * theAnswer;
		
		wrongAnswer = random(min_2d, max_2d);
		break;
	default: 
		break;
	}
	
	if(theAnswer == wrongAnswer) wrongAnswer++;
	posOfCorrect = random(0,2);

	Serial.println("DONE Generating");
}

void showQuestion(){
	/*lcd.clear();
	
	
	//show problem
	lcd.setCursor(0, 0);
	lcd.print(operand_1);
	lcd.setCursor(4, 0);
	lcd.print(operators[operationType]);
	lcd.setCursor(6, 0);
	lcd.print(operand_2);
	
	//show options
	lcd.setCursor(1, 0);
	lcd.print("a.");
	lcd.setCursor(1, 8);
	lcd.print("b.");
	lcd.setCursor(1, 2 + (posOfCorrect * 8));
	lcd.print(theAnswer);
	lcd.setCursor(1, 8 - (posOfCorrect * 6));
	lcd.print(wrongAnswer);
	*/
	
	//SERIAL MONITOR
	Serial.print(operand_1);
	Serial.print(" ");
	Serial.print(operators[operationType]);
	Serial.print(" ");
	Serial.println(operand_2);
	
	if(posOfCorrect == 0){
		Serial.print("a. ");
		Serial.println(theAnswer);
		Serial.print("b. ");
		Serial.println(wrongAnswer);
	}else{
		Serial.print("a. ");
		Serial.println(wrongAnswer);
		Serial.print("b. ");
		Serial.println(theAnswer);
	}
}

void button_press(byte button){
	if(stopPlay){
		restart();
		return;
	}
	
	if(button == posOfCorrect){
		nextQuestion = true;
		score++;
		if(score >= 10) gameOver();
	}else{
		gameOver();
	}	
}

void gameOver(){
	stopPlay = true;
	
	/*
	lcd.clear();
	
	lcd.setCursor(0,0);
	lcd.print("Score: ");
	lcd.setCursor(7,0);
	lcd.print(score);
	
	lcd.setCursor(0,1);
	if(score < 10){
		lcd.print("Ew, you lose!");
		Serial.print("Ew, you lose!");
	}else{
		lcd.print("   YOU WIN!");
		Serial.print("   YOU WIN!");
	}
	*/
	Serial.print("Score :");
	if(score < 10){
		Serial.println("Ew, you lose!");
		digitalWrite(pinAlarm, HIGH);
	}else{
		Serial.println("   YOU WIN!");
	}
}

void setup() {
	Serial.begin(19200);
	
	pinMode(pinBut_1, INPUT);
	pinMode(pinBut_2, INPUT);
	pinMode(pinAlarm, OUTPUT);
	//lcd.begin(16, 2);
	//lcd.setRGB(200, 50, 255);
	
	delay(100);
	randomSeed(analogRead(A0));
}

void restart(){
	score = 0;
	cdTimer = 0;
	digitalWrite(pinAlarm, LOW);
	stopPlay = false;
}

void loop() {
	curT = millis();
	int reading1 = digitalRead(pinBut_1);
	int reading2 = digitalRead(pinBut_2);
	//every new question
	if(nextQuestion && !stopPlay){
		generateQuestion();
		showQuestion();
		nextQuestion = false;
	}
	
	//debounce
	if((reading1 != prevBut1State) || (reading2 != prevBut2State)){
		lastDebounceTime = millis();
	}
	
	//Key press
	if ((millis() - lastDebounceTime) > debounceDelay) {
		if (reading1 != but1State) {
			but1State = reading1;
			if (but1State == HIGH) {
				button_press(0);
			}
		}
		if (reading2 != but2State) {
			but2State = reading2;
			if (but2State == HIGH) {
				button_press(1);
			}
		}
	}
	
	//timer of 1 sec
	if((unsigned long)(curT - prevT) >= 1000){
		cdTimer++;
		prevT = curT;
	}
	
	//time's up
	if(cdTimer >= MAXIMUM_TIME){
		gameOver();
	}
	
	prevBut1State = reading1;
	prevBut2State = reading2;
}
