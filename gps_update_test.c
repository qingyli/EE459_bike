/***
	gps_update_test.c - Tests parsing and formatting ability of gps.c
**/

#include "lcd.h"
#include "gps.h"

#define LINE3_CHANGE_INTERVAL 50

void display_wait(void);

char last_line[MAX_SENTENCE_LEN];
uint8_t line_len = 0;
uint8_t wait_displaying = 0;

// Selects which of the 3 fields (altitude, speed, direction) to display
uint8_t line3_displayed = 0;
uint8_t line3_counter = 0;

void init(void) {
	lcd_init();
	gps_init();
	sei();
}

void run_once(void) {
	display_wait();
}

void loop(void) {
	if(gps_check()) {
		line_len = gps_readline(last_line);
		int8_t result = gps_parse(last_line, line_len);
		if(result == -1) {
			display_wait();
		} else {
			if(result & VALID_LINE_0) {
				lcd_moveto(0,0);
				lcd_stringout(display_screen[0]);
			}
			if(result & VALID_LINE_1) {
				lcd_moveto(1,0);
				lcd_stringout(display_screen[1]);
			}
			if(result & VALID_LINE_2) {
				wait_displaying = 0;
				lcd_moveto(2,0);
				lcd_stringout(display_screen[2]);
			}

			lcd_moveto(3,0);
			if(result & VALID_LINE_3) {
				if(line3_displayed == 0 && !(result & VALID_ALT)) {
					line3_displayed = (result & VALID_SPD)?1:2;
					line3_counter = 0;
				}
				else if(line3_displayed == 1 && !(result & VALID_SPD)) {
					line3_displayed = (result & VALID_DIR)?2:0;
					line3_counter = 0;
				}
				else if(line3_displayed == 2 && !(result & VALID_DIR)) {
					line3_displayed = (result & VALID_ALT)?0:1;
					line3_counter = 0;
				}
				
				lcd_stringout(display_screen[3+line3_displayed]);
				line3_counter++;
				if(line3_counter == LINE3_CHANGE_INTERVAL) {
					line3_counter=0;
					line3_displayed++;
					if(line3_displayed == 3) line3_displayed = 0;
				}
			} else {
				lcd_stringout("                    ");
			}
		}
	}
}

void display_wait(void) {
	if(!wait_displaying) {
		wait_displaying = 1;
		lcd_moveto(1,0);
		lcd_stringout("                    ");
		lcd_moveto(2,0);
		lcd_stringout(" Waiting for GPS... ");
		lcd_moveto(3,0);
		lcd_stringout("                    ");
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
