#include "SIM900.h"
#include <SoftwareSerial.h>
#include "sms.h"
SMSGSM sms;

int numdata;
boolean started=false;
char smsbuffer[160]; //sms received will be stored
char n[20]; //number of sender of received sms
char datetime[25];

void setup()
{
     Serial.begin(9600);
     Serial.println("GSM Shield testing.");
     if (gsm.begin(2400)) {
          Serial.println("\nstatus=READY");
          started=true;
     } else Serial.println("\nstatus=IDLE");

     if(started) {
          //Enable this two lines if you want to send an SMS.
          //if (sms.SendSMS("8888", "extend"))
         // Serial.println("\nSMS sent OK");
     }

};
byte sms_unreadPos = -1;
void loop()
{
     if(started) {
		 //sms_unreadPos = sms.IsSMSPresent(SMS_UNREAD);
		 Serial.println(sms_unreadPos);
		 byte rr = sms.GetSMS(1, n, 20, datetime, smsbuffer, 160);
		Serial.print("RETURN: ");
		Serial.println(rr);
		if(rr == 2){
			Serial.println("received text");
			//sms.GetSMS(1, n, 20, datetime, smsbuffer, 160);
			Serial.println(n);
			Serial.println(datetime);
			Serial.println(smsbuffer);
			
			Serial.print("DELETEION: ");
            Serial.println(sms.DeleteSMS(1));
			//sms.DeleteSMS(sms_unreadPos);
			//sms_unreadPos = -1;
		}else if(rr == 1){
			Serial.print("DELETEION: ");
            Serial.println(sms.DeleteSMS(1));
		}else{
			Serial.println("no text");
		}
		
          //Serial.println(smsbuffer);
		  
          delay(2000);
     }
};
