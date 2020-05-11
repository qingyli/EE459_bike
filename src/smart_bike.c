/***
    gps_update_test.c - Tests parsing and formatting ability of gps.c
**/

#include <stdio.h>
#include "lcd.h"
#include "gps.h"
#include "adc.h"

#define LINE_CHANGE_INTERVAL 40 //2 lines are read per second, so toggles information every 20 seconds
#define DARK_THRESH  400 // When lights are off, turn them on when below this threshold
#define LIGHT_THRESH 600 // When lights are on, turn them off when above this threshold
#define LED_BIT (1 << PD7)

#define RED_THRESH      10*12 //Set Red warning at 10 feet
#define YELLOW_THRESH   20*12 //Set Yellow warning at 20 feet
#define BUZZER_BIT (1 << PB1)

void light_update(void);
void sonar_update(void);
void lcd_update(void);
void display_wait(void);
void display_time(void);
void display_elapsed(void);
void display_location(void);
void display_misc(void);

char last_line[MAX_SENTENCE_LEN];
uint8_t line_len = 0;
uint8_t wait_displaying = 0;

// Selects which of the 3 fields (altitude, speed, direction) to display
uint8_t line12_displayed = 0;
uint8_t line12_counter = 0;
uint8_t line3_displayed = 0;
uint8_t line3_counter = 0;

uint8_t lcd_color = 0; // 0 - white, 1 - yellow, 2 - red

void init(void) {
    lcd_init();
    gps_init();
    adc_init();
    PORTD &= ~LED_BIT;
    PORTB &= ~BUZZER_BIT;
    sei(); // Enable interrupts
}

void splash(void) {
    lcd_moveto(0,0);
    lcd_stringout("EE459 Project");
    lcd_moveto(1,0);
    lcd_stringout("Smart Bike Accessory");
    _delay_ms(3000); // Sleep 3 seconds
    display_wait();
}

void loop(void) {
    light_update();
    sonar_update();
    if(gps_check()) {
        lcd_update();
    }
    _delay_ms(10);
}

void light_update() {
    uint16_t light_lvl = light_reading();
    //Two separate thresholds are used to avoid LED from flickering if light level fluctuates around single threshold
    if((PORTD & LED_BIT)==0 && light_lvl < DARK_THRESH) {
        //Turn on lights when ambient light is dark enough
        PORTD |= LED_BIT;
    } else if((PORTD & LED_BIT) && light_lvl > LIGHT_THRESH) {
        //Turn off lights when ambient light is bright enough
        PORTD &= ~LED_BIT;
    }
}

void sonar_update() {
    uint16_t distance = sonar_reading();
    if(distance < RED_THRESH) {
        if(lcd_color != 2) { // Only change if not already red
            lcd_set_rgb(LCD_COLOR_RED);
            lcd_color = 2;
        }
        PORTB |= BUZZER_BIT;
    } else if(distance < YELLOW_THRESH) {
        if(lcd_color != 1) { // Only change if not already yellow
            lcd_set_rgb(LCD_COLOR_YELLOW);
            lcd_color = 1;
        }
        PORTB &= ~BUZZER_BIT;
    } else {
        if(lcd_color != 0) { // Only change if not already white
            lcd_set_rgb(LCD_COLOR_WHITE);
            lcd_color = 0;
        }
        PORTB &= ~BUZZER_BIT;
    }
}

void lcd_update() {
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
            if((!(result & VALID_LINE_1) || !(result & VALID_LINE_2))) { // Don't update LCD if error occured when parsing the last message
                line12_counter++;
            } else {
                display_location();
            }
        } else {
            display_elapsed();
        }
        if(line12_counter == LINE_CHANGE_INTERVAL) { // Toggle between whether location or elapsed time is displayed
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
            if(line3_counter == LINE_CHANGE_INTERVAL) { // Toggle between whether altitude, speed, or direction are displayed
                line3_counter=0;
                line3_displayed++;
                if(line3_displayed == 3) line3_displayed = 0;
            }
        } else {
            display_wait();
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
    if (line12_counter == 0) { //Only need to print when LCD switches from displaying location to elapsed
        lcd_moveto(1,0);
        lcd_stringout("Elapsed Time:       ");
    }
    lcd_moveto(2,0);
    char timeStr[21];
    snprintf(timeStr, 21, "      %02d:%02d:%02d      ",elapsedTime/3600,(elapsedTime/60)%60,elapsedTime%60);
    lcd_stringout(timeStr);
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
    wait_displaying = 0;
    line3_counter++;
}

int main(void)
{
    init();
    splash();
    while(1) {
        loop();
    }
    return 0;   /* never reached */
}
