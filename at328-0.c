/*************************************************************
*       at328-0.c - Demonstrate simple I/O functions of ATmega328
*
*       Program loops turning PC0 on and off as fast as possible.
*
* The program should generate code in the loop consisting of
*   LOOP:   SBI  PORTC,0        (2 cycles)
*           CBI  PORTC,0        (2 cycles)
*           RJMP LOOP           (2 cycles)
*
* PC0 will be low for 4 / XTAL freq
* PC0 will be high for 2 / XTAL freq
* A 9.8304MHz clock gives a loop period of about 600 nanoseconds.
*
* Revision History
* Date     Author      Description
* 09/14/12 A. Weber    Initial Release
* 11/18/13 A. Weber    Renamed for ATmega328P
*************************************************************/

#include <avr/io.h>

#define FREQ 7372800
#define UBRR FREQ/16/9600 - 1

void serial_init ( unsigned short ubrr ) {

	UBRR0 = ubrr ; // Set baud rate
	UCSR0B |= (1 << TXEN0 ); // Turn on transmitter
	UCSR0B |= (1 << RXEN0 ); // Turn on receiver
	UCSR0C = (3 << UCSZ00 ); // Set for async . operation , no parity ,
	// one stop bit , 8 data bits
	}
	
/*
	serial_out - Output a byte to the USART0 port
	*/
void serial_string_out(char* str) {
	while(*str) {
		serial_out(*str);
		str++;
	}
}
void serial_out ( char ch )
{
	while (( UCSR0A & (1 << UDRE0 )) == 0);
	UDR0 = ch ;
}

/*
serial_in - Read a byte from the USART0 and return it
*/
char serial_in ()
{
	while ( !( UCSR0A & (1 << RXC0 )) );
	return UDR0;
} 
int main(void)
{
    serial_init(UBRR);

    while (1) {
		if(UCSR0A & (1 << RXC0)) {
			char received = UDR0;
			if(received >= 'a' && received <= 'z') {
				if(received == 'a' || received == 'e' || received == 'i' || received == 'o' || received == 'u') {
					char str[30];
					sprintf(str, "You entered the vowel \"%c\"\r\n", received);
					serial_string_out(str);
				}
				else {
					char str[30];
					sprintf(str, "That was the consonant \"%c\"\r\n", received);
					serial_string_out(str);
				}
			}
		}
    }

    return 0;   /* never reached */
}