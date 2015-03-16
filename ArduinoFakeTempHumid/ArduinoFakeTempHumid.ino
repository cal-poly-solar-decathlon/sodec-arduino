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

#define BYTE_SIZE 8
//#define SEND_FREQ 0.2          //sends per second
#define TEMP_INCREMENT 4.0     //degrees per second
#define HUMID_INCREMENT 8.0    //percent increase per second

#define TEMP_LOWER_LIMIT -50   //degrees C
#define TEMP_LIMIT 200         //degrees C
#define HUMID_LIMIT 100        //percent humidity

char checksumCheck(int16_t temp, uint16_t humid) {
   char sum = 0;
   int i;
   
   for (i = 0; i < 2; ++i)
      sum += (temp >> i * 8) & 0xFF;
   for (i = 0; i < 2; ++i)
      sum += (humid >> i * 8) & 0xFF;
   
   return sum;
}

unsigned int digit;
int16_t temperature;
uint16_t humidity;
uint8_t checksum;
//int _global_delay;

void setup () {
   DDRB = 0xFF;

   Serial.begin(9600);    //initialize USART communication
   temperature = humidity = checksum = 0;
//   _global_delay = 1000 / SEND_FREQ;
}

void loop () {
   checksum = checksumCheck(temperature, humidity);
   Serial.write('T');
   Serial.write((uint8_t)(temperature & 0x00FF));
   Serial.write((uint8_t)((temperature & 0xFF00) >> BYTE_SIZE));
   Serial.write('H');
   Serial.write((uint8_t)(humidity & 0x00FF));
   Serial.write((uint8_t)((humidity & 0xFF00) >> BYTE_SIZE));
   Serial.write('C');
   Serial.write(checksum);
   
   temperature += (uint16_t)(TEMP_INCREMENT * 10);
   if (temperature > TEMP_LIMIT * 10)
      temperature = TEMP_LOWER_LIMIT;
   humidity = (humidity + (uint16_t)(HUMID_INCREMENT * 10)) % (HUMID_LIMIT * 10);

   _delay_ms(5000);
}
