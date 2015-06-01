#include <math.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <SPI.h>
#include <avr/wdt.h>
//#include "TimerOne.h"

#define MIN_TRANS_TEMP 20
#define MAX_TRANS_TEMP 30
#define FLUSH_OUTSIDE_TIME 20    //2:00 AM
#define ABSORB_INSIDE_TIME 140   //2:00 PM
#define FAN_RELAY 4
#define OUTSIDE_DAMPER_RELAY 5
#define INSIDE_DAMPER_RELAY 6
#define TRANSFORMER_RELAY 7
#define THERMOCOUPLE_PIN 14

uint8_t reportedTime;

bool flushComplete,
     absorbComplete;
     
uint16_t outTemperature;
uint16_t kitTemperature;
uint16_t pcmTemperature;

void FlushToOutsideAir() {
   static bool waitingOnPCM = false;
   
   if (!waitingOnPCM) {
      digitalWrite(TRANSFORMER_RELAY, HIGH);
      digitalWrite(OUTSIDE_DAMPER_RELAY, HIGH);      //Assuming outside dampers are normally closed and inside dampers are normally open
      digitalWrite(INSIDE_DAMPER_RELAY, HIGH);
      delay(5);
      digitalWrite(TRANSFORMER_RELAY, LOW);
      _delay_ms(500);
      digitalWrite(OUTSIDE_DAMPER_RELAY, LOW);
      digitalWrite(INSIDE_DAMPER_RELAY, LOW);
      waitingOnPCM = true;
   }
   if (pcmTemperature > outTemperature)
      digitalWrite(FAN_RELAY, HIGH);
   else {
      digitalWrite(FAN_RELAY, LOW);
      waitingOnPCM = false;
      flushComplete = true;
   }
}

void AbsorbInsideHeat() {
   static bool waitingOnPCM = false;
   
   if (!waitingOnPCM) {
      digitalWrite(TRANSFORMER_RELAY, HIGH);
      digitalWrite(OUTSIDE_DAMPER_RELAY, LOW);      //Assuming outside dampers are normally closed and inside dampers are normally open
      digitalWrite(INSIDE_DAMPER_RELAY, LOW);
      delay(5);
      digitalWrite(TRANSFORMER_RELAY, LOW);
//      _delay_ms(500);
//      digitalWrite(OUTSIDE_DAMPER_RELAY, LOW);
//      digitalWrite(INSIDE_DAMPER_RELAY, LOW);
      waitingOnPCM = true;
   }
   if (pcmTemperature < kitTemperature)
      digitalWrite(FAN_RELAY, HIGH);
   else { 
      digitalWrite(FAN_RELAY, LOW);
      waitingOnPCM = false;
      absorbComplete = true;
   }
}

void setup() {
   DDRD = 0xFF;
   DDRC = 0x00;
   Serial.begin(9600);    //initialize USART communication
   flushComplete = absorbComplete = false;
   outTemperature = 20;
   kitTemperature = 25;
   pcmTemperature = 21;
}

void loop() {
   if (Serial.available()) {
      Serial.readBytes(&reportedTime, 1);
      if (!flushComplete && reportedTime > FLUSH_OUTSIDE_TIME && reportedTime < ABSORB_INSIDE_TIME)
         FlushToOutsideAir();
      else if (!absorbComplete && reportedTime > ABSORB_INSIDE_TIME)
         AbsorbInsideHeat();
   }
}
