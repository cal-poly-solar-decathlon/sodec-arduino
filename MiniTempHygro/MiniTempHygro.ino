//
//  miniMain.c
//  TempHygroPOC
//
//  Created by Timothy Ambrose (And sorta Andrew Elliott) on 2/10/15.
//  Copyright (c) 2015 Luminoscity®. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>

#include <math.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define F_CPU 16000000UL
#define MAIN_DELAY_MS 5000  //5 seconds
#define DELAY_MULTIPLIER 9  //45 total seconds
#define RHT03_DATA 8
#define TIMER0_OVER 256
#define TIMER1_OVER 65536
#define TEMP_MASK 0xFFFF
#define CHECKSUM_MASK 0xFF
#define HUMID_OFFSET 16
#define BYTE_SIZE 8
#define MAX_PULSES 45
#define TRIG_PULSE 43
#define UNUSED_PULSES 4
#define LOW_ONE 68
#define HIGH_ONE 80
#define LOW_ZERO 20
#define HIGH_ZERO 32
#define MAX_PULSE_WAIT 10
#define RHT03_Read {\
digitalWrite(RHT03_DATA,HIGH);\
digitalWrite(RHT03_DATA,LOW);\
_delay_ms(2);\
digitalWrite(RHT03_DATA,HIGH);\
_delay_us(30);\
digitalWrite(RHT03_DATA,LOW);\
}

// global variables
volatile long overflows;
volatile char pulseHigh;
volatile int time1;
volatile long pulseWidth;
volatile int pulseCount;
volatile int pulses[MAX_PULSES];

//ISR
//Function Output Interrupt
ISR(TIMER1_OVF_vect) {
   ++overflows;
}

ISR(PCINT0_vect) {
   if (PINB & 0x01) {
      time1 = TCNT1;
      overflows = 0;
   }
   else if (pulseCount < MAX_PULSES) {
      pulses[pulseCount++] = TCNT1 - time1;// + overflows * TIMER1_OVER;
   }
}

//Functions
void initTimer0(void) {
   TCCR0A = 0x00;                //Normal mode
   TCCR0B = 0x02;                //prescaler = 8
   TIFR0 = 0x01;              
   TIMSK0 = 0x01;                //enable overflow interrupt
}

void initTimer1(void) {
   TCCR1A = 0x00;                //Normal mode
   TCCR1B = 0x02;                //prescaler = 8
   TIFR1 = 0x01;              
   TIMSK1 = 0x01;                //enable overflow interrupt
}

void RHT03_init() {
   PCICR |= 1<<PCIE0;
   PCMSK0 |= 1<<PCINT0;
}

int checksumCheck(uint16_t temp, uint16_t humid, char checksum) {
   char sum = 0;
   int i;
   
   for (i = 0; i < 2; ++i)
      sum += (temp >> i * 8) & 0xFF;
   for (i = 0; i < 2; ++i)
      sum += (humid >> i * 8) & 0xFF;
   
   return checksum == sum;
}

unsigned int digit;
uint16_t temperature;
uint16_t humidity;
uint8_t checksum;
uint8_t outOfRange = 0;
uint32_t delays;
char fromSerial;

void setup () {
   DDRB = 0xFF;
   digitalWrite(RHT03_DATA,HIGH);
   wdt_enable(WDTO_8S);
   
   initTimer1();                    //initialize timer/counter
   RHT03_init();
   Serial.begin(9600);    //initialize USART communication
   
   pulseHigh = pulseWidth = overflows = pulseCount = 0;
   
   sei();
   delay(950);
}

void loop () {
   int i;
   int waitCount;

   while (Serial.available())
      Serial.readBytes(&fromSerial, 1);
      
   pinMode(RHT03_DATA, OUTPUT);
   RHT03_Read
   pinMode(RHT03_DATA, INPUT);
   
   delays = 0;
   while (pulseCount < TRIG_PULSE && delays < MAX_PULSE_WAIT) {
      ++delays;
      _delay_ms(2);
   }
   wdt_reset();
   checksum = temperature = humidity = outOfRange = 0;
   
   if (delays < MAX_PULSE_WAIT) {
      for (i = UNUSED_PULSES; i < pulseCount; ++i) {
         digit = pulses[i]/2;
         
         if (digit < HIGH_ZERO && digit > LOW_ZERO){         //digital 0
            ;
         }
         else if (digit < HIGH_ONE && digit > LOW_ONE) {   //digital 1
            if (i >= pulseCount - BYTE_SIZE)
               checksum |= 1 << (TRIG_PULSE - i);
            else if (i > TRIG_PULSE - 3 * BYTE_SIZE)
               temperature |= 1 << (TRIG_PULSE - BYTE_SIZE - i);
            else
               humidity |= 1 << (TRIG_PULSE - 3 * BYTE_SIZE - i);
         }
         else {
            outOfRange = 1;
         }
      }
           
      if (!outOfRange && delays < MAX_PULSE_WAIT) {
         if (checksumCheck(temperature, humidity, checksum)) {
            Serial.write('T');
            Serial.write((uint8_t)(temperature & 0x00FF));
            Serial.write((uint8_t)((temperature & 0xFF00) >> BYTE_SIZE));
            Serial.write('H');
            Serial.write((uint8_t)(humidity & 0x00FF));
            Serial.write((uint8_t)((humidity & 0xFF00) >> BYTE_SIZE));
            Serial.write('C');
            Serial.write(checksum);
         }
//      else {
//         Serial.print("BAD!\n\r");
//         for (i = 0; i < pulseCount; ++i) {
//            Serial.print(pulses[i]/2);
//            Serial.print(' ');
//         }
//         Serial.print("\n\r");
//         Serial.print(humidity, BIN);
//         Serial.print(' ');
//         Serial.print(temperature, BIN);
//         Serial.print(' ');
//         Serial.print(checksum, BIN);
//         Serial.print("\n\r");
//      }
      }
//   else
//   {
//      Serial.print("outOfRange\n\r");
//   }  
   }

   pulseCount = 0;
   waitCount = DELAY_MULTIPLIER;
   while (waitCount--) {
      wdt_reset();
      _delay_ms(MAIN_DELAY_MS);
   }
}
