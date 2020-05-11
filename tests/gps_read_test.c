/***
	gps_read_test.c - Tests simple functionality of the GPS module by displaying received sentences to LCD
**/

#include "lcd.h"
#include "gps.h"

char last_line[MAX_SENTENCE_LEN];
uint8_t line_len = 0;
uint8_t splash_on = 0;

void init(void) {
	lcd_init();
	gps_init();
	sei();
}

void run_once(void) {
	lcd_moveto(2,0);
	lcd_stringout(" Waiting for GPS... ");
	splash_on = 1;
}

void loop(void) {
	if(gps_check()) {
		line_len = gps_readline(last_line);
		if(splash_on) {
			lcd_clear();
			splash_on = 0;
		}
		uint8_t i=0;
		while(i < line_len && i < 80) {
			if(i==0) lcd_moveto(0,0);
			if(i==20) lcd_moveto(1,0);
			if(i==40) lcd_moveto(2,0);
			if(i==60) lcd_moveto(3,0);
			lcd_charout(last_line[i++]);
		}
	}
}

int main(void)
{
	init();
	run_once();
    while(1) {
		loop();
    }
    return 0;   /* never reached */
}
