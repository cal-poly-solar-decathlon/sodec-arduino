//
//  miniMain.c
//  TempHygroPOC
//
//  Created by Timothy Ambrose (And sorta Andrew Elliott) on 2/10/15.
//  Copyright (c) 2015 Luminoscity®. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#define F_CPU 16000000UL

#include <math.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define MAIN_DELAY_MS 500
#define RHT03_DATA_KIT 8
#define RHT03_DATA_OUT 9
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
#define RHT03_Read(__pin_x) {\
   digitalWrite((__pin_x),HIGH);\
   digitalWrite((__pin_x),LOW);\
   _delay_ms(2);\
   digitalWrite((__pin_x),HIGH);\
   _delay_us(30);\
   digitalWrite((__pin_x),LOW);\
}

// global variables
volatile long overflows;
volatile char pulseHigh;
volatile int time1;
volatile long pulseWidth;
volatile int pulseCount;
volatile int pulses[MAX_PULSES];

unsigned int digit;
uint16_t temperature;
uint16_t humidity;
uint8_t checksum;
uint8_t outOfRange = 0;

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

void RHT03_KIT_init() {
   PCICR = 1<<PCIE0;
   PCMSK0 = 1<<PCINT0;
}

void RHT03_OUT_init() {
   PCICR = 1<<PCIE0;
   PCMSK0 = 1<<PCINT1;
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



void setup () {
   DDRB = 0xFF;
   digitalWrite(RHT03_DATA_KIT,HIGH);
   digitalWrite(RHT03_DATA_OUT,HIGH);
   //   DDRD = 0xFF;
   
   initTimer1();                    //initialize timer/counter
   RHT03_KIT_init();      //enable interrupts for kitchen sensor
   RHT03_OUT_init();      //enable interrupts for outside sensor
   Serial.begin(9600);    //initialize USART communication
   
   pulseHigh = pulseWidth = overflows = pulseCount = 0;
   
   sei();
   delay(950);
}

void loop () {
   int i;
   //----------------Read from Kitchen sensor-----------------
   pinMode(RHT03_DATA_KIT, OUTPUT);
   RHT03_Read(RHT03_DATA_KIT)
   pinMode(RHT03_DATA_KIT, INPUT);
   
   while (pulseCount < TRIG_PULSE);
   
   checksum = temperature = humidity = outOfRange = 0;
   
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
         
   if (!outOfRange) {
      if (checksumCheck(temperature, humidity, checksum)) {
         Serial.write('T');
         Serial.write('K');
         Serial.write((uint8_t)(temperature & 0x00FF));
         Serial.write((uint8_t)((temperature & 0xFF00) >> BYTE_SIZE));
         Serial.write('H');
         Serial.write('K');
         Serial.write((uint8_t)(humidity & 0x00FF));
         Serial.write((uint8_t)((humidity & 0xFF00) >> BYTE_SIZE));
         Serial.write('C');
         Serial.write('K');
         Serial.write(checksum);
      }
   }

   pulseCount = 0;
   
   //----------------Read from Outside sensor-----------------
   pinMode(RHT03_DATA_OUT, OUTPUT);
   RHT03_Read(RHT03_DATA_OUT)
   pinMode(RHT03_DATA_OUT, INPUT);
   
   while (pulseCount < TRIG_PULSE);
   
   checksum = temperature = humidity = outOfRange = 0;
   
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
         
   if (!outOfRange) {
      if (checksumCheck(temperature, humidity, checksum)) {
         Serial.write('T');
         Serial.write('O');
         Serial.write((uint8_t)(temperature & 0x00FF));
         Serial.write((uint8_t)((temperature & 0xFF00) >> BYTE_SIZE));
         Serial.write('H');
         Serial.write('O');
         Serial.write((uint8_t)(humidity & 0x00FF));
         Serial.write((uint8_t)((humidity & 0xFF00) >> BYTE_SIZE));
         Serial.write('C');
         Serial.write('O');
         Serial.write(checksum);
      }
   }
   pulseCount = 0;
   _delay_ms(MAIN_DELAY_MS);
}
