/* Name: main.c
 * 
 * Description:
 * Authors: Tim Ambrose
 * Copyright: 2014
 * Project: 
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "USART_driver.h"

#define ZERO 48
#define MOMENTARY_DELAY 1000
#define RELAY1 PB4
#define RELAY2 PB3
#define RELAY3 PB2
#define RELAY4 PB1
#define RELAY5 PD7
#define RELAY6 PD6
#define RELAY7 PD5
#define RELAY8 PD4

int main(void)
{
   char received = 'm',
        latching = 0;
   int delay = MOMENTARY_DELAY;
   
   DDRB = 0x0F;
   DDRD = 0xF0;
   usart_init(9600, 16000000UL);    //initialize USART communication
   
   while(1) {
      switch (received) {           //switch relays
      case '1':                     //Relay #1
         if (latching)
            PORTB ^= (1<<RELAY1);
         else {
            PORTB |= (1<<RELAY1);
            _delay_ms(delay);
            PORTB &= ~(1<<RELAY1);
         }
         break;
         
      case '2':                     //Relay #2
         if (latching)
            PORTB ^= (1<<RELAY2);
         else {
            PORTB |= (1<<RELAY2);
            _delay_ms(delay);
            PORTB &= ~(1<<RELAY2);
         }
         break;
         
      case '3':                     //Relay #3
         if (latching)
            PORTB ^= (1<<RELAY3);
         else {
            PORTB |= (1<<RELAY3);
            _delay_ms(delay);
            PORTB &= ~(1<<RELAY3);
         }         break;
         
      case '4':                     //Relay #4
         if (latching)
            PORTB ^= (1<<RELAY4);
         else {
            PORTB |= (1<<RELAY4);
            _delay_ms(delay);
            PORTB &= ~(1<<RELAY4);
         }
         break;
         
      case '5':                     //Relay #5
         if (latching)
            PORTD ^= (1<<RELAY5);
         else {
            PORTD |= (1<<RELAY5);
            _delay_ms(delay);
            PORTD &= ~(1<<RELAY5);
         }
         break;
         
      case '6':                     //Relay #6
         if (latching)
            PORTD ^= (1<<RELAY6);
         else {
            PORTD |= (1<<RELAY6);
            _delay_ms(delay);
            PORTD &= ~(1<<RELAY6);
         }
         break;
         
      case '7':                     //Relay #7
         if (latching)
            PORTD ^= (1<<RELAY7);
         else {
            PORTD |= (1<<RELAY7);
            _delay_ms(delay);
            PORTD &= ~(1<<RELAY7);
         }
         break;
         
      case '8':                     //Relay #8
         if (latching)
            PORTD ^= (1<<RELAY8);
         else {
            PORTD |= (1<<RELAY8);
            _delay_ms(delay);
            PORTD &= ~(1<<RELAY8);
         }
         break;
         
      case 'l':                     //Toggle Latching
      case 'L':
         if(latching) {
            latching = 0;
            usart_string("Moment\r\n");
         }
         else{
            latching = 1;
            usart_string("Latch\r\n");
         }
         break;
            
      case '0':
         PORTB &= 0xF0;
         PORTD &= 0x0F;
         break;
            
      case '!':
         PORTB |= 0x0F;
         PORTD |= 0xF0;
         break;   
            
      case 'm':
      case 'M':
         usart_string("\e[2K\rMomentary Delay: ");
         received = (char)usart_recv();
         delay = received > ZERO ? (received - ZERO) * 100 : MOMENTARY_DELAY;
         
         usart_integer(delay);
         usart_string(" ms\r\n");
         break;
         
      default:
         usart_string("Invalid\r\n");
         break;
      }
      
      received = (char)usart_recv();
      usart_send(received);
      usart_send(' ');
   }
   
   return 1;                        //unreachable
}
