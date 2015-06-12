/* Name: USART_driver.h
 * 
 * Description: USART Function Prototypes
 * Modified by: Tim Ambrose and Mackenzie Keane
 * Copyright: 2014
 * Project: Project4 - USART communication
 */

#ifndef Project4_USART_driver_h
#define Project4_USART_driver_h

#define BAUD_PRESCALE 103
#define SCREEN_COLUMNS 95
#define SCREEN_ROWS 35

void usart_init(uint16_t baudin, uint32_t clk_speedin);

/*the send function will put 8bits on the trans line. */
void usart_send( uint8_t data );

/* the receive data function. Note that this a blocking call
 Therefore you may not get control back after this is called 
 until a much later time. It may be helpfull to use the 
 istheredata() function to check before calling this function
 @return 8bit data packet from sender
 */
uint8_t  usart_recv(void);

/* function check to see if there is data to be received
 @return true is there is data ready to be read */
uint8_t  usart_isThereData(void);

/*
 * Transmit string to USART
 */
void usart_string(char *str);

/*
 * Transmit multi-digit integer to USART
 */
void usart_integer(long);

/*
 * Transmit multi-digit floating point number to USART
 */
void usart_float(double, int);

/*
 * Flush USART receive buffer
 */
void usart_flush();

/*
 * Transmit special character sequences like 'up arrow'
 */
void usart_special(char *special);

/*
 * Print spaces to 'lines' # of lines above current line
 */
void usartClearLinesAbove(int lines);

/*
 * Clears terminal screen 
 */
void usart_clear();

/*
 * processes and stores characters form the USART keyboard into charArray
 * until stopSeq is encountered
 */
void usart_processWords(int max, char *charArray, char *stopSeq);

#endif
