/***
	lcd_test.c - Test's the functionality of the LCD by printing strings and changing the RGB backlight
**/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "lcd.h"

void init(void) {
	lcd_init();
}

void run_once(void) {
	lcd_moveto(0,3);
	lcd_stringout("test1");
	lcd_moveto(1,2);
	lcd_stringout("test2");
	lcd_moveto(2,1);
	lcd_stringout("test3");
	lcd_moveto(3,0);
	lcd_stringout("test4");
	_delay_ms(10);
	lcd_clear();
}

void loop(void) {
	uint8_t r, g, b;
	for(r=0; r < 255; r++) {
		for(g=0; g < 255; g++) {
			for(b=0; b < 255; b++) {
				lcd_set_rgb(r, g, b);
				lcd_home();
				char buf[20];
				snprintf(buf, 20, "RGB: %03d, %03d, %03d", r, g, b);
				lcd_stringout(buf);
			}
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
