/***
	gps_update_test.c - Tests parsing and formatting ability of gps.c
**/

#include "lcd.h"
#include "gps.h"

#define LINE_CHANGE_INTERVAL 40 //2 lines are read per second, so toggles information every 20 seconds

void display_wait(void);

char last_line[MAX_SENTENCE_LEN];
uint8_t line_len = 0;
uint8_t wait_displaying = 0;

// Selects which of the 3 fields (altitude, speed, direction) to display
uint8_t line12_displayed = 0;
uint8_t line12_counter = 0;
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
	update_info();
}

void update_info(void) {
	if(gps_check()) {
		line_len = gps_readline(last_line);
		int8_t result = gps_parse(last_line, line_len);
		if(result == -1) {
			if(line12_displayed == 0) { // If showing location switch to elapsed time
				line12_displayed = 1;
				line12_counter = 0;
			}
			display_elapsed();
			display_wait();
		} else {
			if(result & VALID_LINE_0) {
				display_time();
			}

			if(line12_displayed == 0) {
				if((!(result & VALID_LINE_1) || !(result & VALID_LINE_2))) {
					line12_counter++;
				} else {
					display_location();
				}
			} else {
				display_elapsed();
			}
			if(line12_counter == LINE_CHANGE_INTERVAL) {
				line12_counter = 0;
				line12_displayed ^= 1;
			}

			if(result & VALID_LINE_3) {
				if((line3_displayed == 0 && !(result & VALID_ALT)) ||
					(line3_displayed == 1 && !(result & VALID_SPD))||
					(line3_displayed == 2 && !(result & VALID_DIR))) {
					line3_counter++;
				} else {
					display_misc();
				}
				if(line3_counter == LINE_CHANGE_INTERVAL) {
					line3_counter=0;
					line3_displayed++;
					if(line3_displayed == 3) line3_displayed = 0;
				}
			} else {
				display_wait();
			}
		}
	}
}

void display_wait(void) {
	if(!wait_displaying) {
		wait_displaying = 1;
		lcd_moveto(3,0);
		lcd_stringout(" Waiting for GPS... ");
	}
}

void display_time(void) {
	lcd_moveto(0,0);
	lcd_stringout(display_screen[0]);
}

void display_elapsed(void) {
	lcd_moveto(1,0);
	lcd_stringout(display_screen[6]);
	lcd_moveto(2,0);
	lcd_stringout(display_screen[7]);
	line12_counter++;
}

void display_location(void) {
	lcd_moveto(1,0);
	lcd_stringout(display_screen[1]);
	lcd_moveto(2,0);
	lcd_stringout(display_screen[2]);
	line12_counter++;
}

void display_misc(void) {
	lcd_moveto(3,0);
	lcd_stringout(display_screen[3+line3_displayed]);
	wait_displaying = 0
	line3_counter++;
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
