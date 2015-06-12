/* Name: USART_driver.c
 * 
 * Description: Function implementations for sending and receiving from USART
 * Modified by: Tim Ambrose and Mackenzie Keane
 * Copyright: 2014
 * Project: Project4 - USART Communication
 */

#include <stdlib.h>                    // Standard C library
#include <avr/io.h>                    // Input-output ports, special registers
#include "USART_driver.h"
#include <string.h>
#include <math.h>

void usart_init(uint16_t baudin, uint32_t clk_speedin)
{
   uint32_t ubrr = (clk_speedin/16UL)/baudin-1;
   UBRR0H = (unsigned char)(ubrr>>8);
   UBRR0L = (unsigned char)ubrr;
   /*UBRR0H = (BAUD_PRESCALE>>8);
    UBRR0L = BAUD_PRESCALE;*/
   /* Enable receiver and transmitter */
   UCSR0B = (1<<RXEN0)|(1<<TXEN0);//|(1<<RXCIE0);
   /* Set frame format: 8data, 1stop bit */
   UCSR0C = (0<<USBS0)|(3<<UCSZ00);
   UCSR0A &= ~(1<<U2X0);
}

/*the send function will put 8bits on the trans line. */
void usart_send( uint8_t data )
{
   /* Wait for empty transmit buffer */
   while ( !( UCSR0A & (1<<UDRE0)) );
   /* Put data into buffer, sends the data */
   UDR0 = data;
}

/* the receive data function. Note that this a blocking call
 Therefore you may not get control back after this is called 
 until a much later time. It may be helpfull to use the 
 istheredata() function to check before calling this function
 @return 8bit data packet from sender
 */
uint8_t  usart_recv()
{
   /* Wait for data to be received */
   while ( !(UCSR0A & (1<<RXC0)) )
      ;
   /* Get and return received data from buffer */
   return UDR0;
}

/* function check to see if there is data to be received
 @return true is there is data ready to be read */
uint8_t  usart_isThereData()
{
   return (UCSR0A & (1<<RXC0));
}

/*
 * Transmit string to USART
 */
void usart_string(char *str) {
   int i;
   
   for (i = 0; i < strlen(str); ++i)
      usart_send(str[i]);
}


/*
 * Transmit multi-digit integer to USART
 */
void usart_integer(long num) {
   long place = 10000000;                    //maximum place value to transmit
   char zeros = 1,
   digit;
   
   while (place) {   
      digit = num % (place * 10) / place;
      if (zeros && digit) {                  //if all zeros so far and currently
                                             //not a zero
         zeros = 0;
      }
      if (!zeros) {                          //ignore zeros in high place values
         usart_send(digit + 48);
      }
      place /= 10;                           //move on to next digit
   }
}

/*
 * Transmit multi-digit floating point number to USART
 */
void usart_float(double num, int decimalPlaces) {
   long place = 100000;                      //maximum place value to transmit
   double decPlace = 0.1;
   char zeros = 1,
   digit;
   
   while (place) {
      digit = (int)num % (place * 10) / place;
      if (zeros && digit) {                  //if all zeros so far and currently
                                             //not a zero
         zeros = 0;
      }
      if (!zeros) {                          //ignore zeros in high place values
         usart_send(digit + 48);
      }
      place /= 10;                           //move on to next digit
   }
   
   usart_send('.');
   
   while (decimalPlaces--) {
      digit = (int) (fmod(num, decPlace * 10.0) / decPlace);
      if (zeros && digit) {                  //if all zeros so far and currently
                                             //not a zero
         zeros = 0;
      }
      if (!zeros) {
         usart_send(digit + 48);             //ignore zeros in high place values
      }
      decPlace /= 10.0;                      //move on to next decimal place
   }
}

/*
 * Clear-out USART receive buffer
 */
void usart_flush() {
   char dump;
   while (UCSR0A & (1<<RXC0))
      dump = UDR0;
}

/*
 * Transmit special character sequences like 'move cursor up'
 */
void usart_special(char *special) {
   if (!strcmp(special, "up")) {
      usart_string("\e[A");
   }
   else if (!strcmp(special, "down")) {
      usart_string("\e[B");
   }
   else if (!strcmp(special, "left")) {
      usart_string("\e[D");
   }
   else if (!strcmp(special, "right")) {
      usart_string("\e[C");
   }
}

/*
 * Erases 'lines' # of lines above current line
 */
void usartClearLinesAbove(int lines) {
   while (lines--)
      usart_string("\e[A\e[2K");     //up, erase line
   usart_send('\r');
}

/*
 * Clears terminal screen 
 */
void usart_clear() {
   usart_string("\e[2J");
}

/*
 * Processes and stores characters from the USART keyboard into charArray
 * until stopSeq is encountered
 */
void usart_processWords(int max, char *charArray, char *stopSeq) {
   int charCount = 0;
   char compChar,
        done = 0;
   int pos;
   
   while (charCount < max && !done) {           //not reached char limit yet
                                                //read in next character
      charArray[charCount++] = compChar = (char)usart_recv();
      
      if (!strcmp(stopSeq, "\r")) {             //if stop sequence is [return]
         if (compChar == '\r') {                //if current char is [return]
            done = 1;                           //finished
            continue;
         }
      }
      else {                                    //all other stop sequences
         if (compChar == '\r') {                //if carriage return is read
            usart_send('\n');                   //send 'newline'
            charArray[charCount++] = '\n';      //append 'newline'
         }
         //if the stop sequence is read
         if (!strncmp(charArray + charCount - strlen(stopSeq), stopSeq,
                     strlen(stopSeq))) {
            done = 1;
            continue;
         }
      }
      
      if (compChar == 127) {                    //if backspace is read
         usart_send(8);                         //send backspace
         usart_send(' ');                       //clear out that position
         usart_send(8);                         //set cursor back one
         --charCount;                           //don't count backspace
         if (--charCount < 0)
            charCount = 0;
      }
      else
         usart_send(compChar);                  //display all characters typed
   }
   
   charArray[charCount - strlen(stopSeq)] = 0;  //append null terminator to
                                                //array when finished
}
