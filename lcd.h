#ifndef LCD_H
#define LCD_H

#include <avr/io.h>
#include <util/delay.h>

#define LCD_DATA_BITS ((1 << PB5)|(1 << PB4)|(1 << PB3)|(1 << PB2))
#define LCD_DATA_OFFSET PB2
#define LCD_RS_BIT (1 << PB0)
#define LCD_EN_BIT (1 << PD4)

#define LCD_R_BIT (1 << PD3)
#define LCD_G_BIT (1 << PD5)
#define LCD_B_BIT (1 << PD6)

// Setup macros for common colors to allow modification in a single place. Order: R, G, B
#define LCD_COLOR_WHITE  255, 255, 255
#define LCD_COLOR_YELLOW 255, 255, 100
#define LCD_COLOR_RED    255, 0,   0

void lcd_init(void);
void lcd_clear(void);
void lcd_home(void);
void lcd_moveto(uint8_t r, uint8_t c);
void lcd_stringout(const char *str);
void lcd_charout(char ch);
void lcd_stringnout(const char *str, uint8_t max);
void lcd_set_rgb(uint8_t r, uint8_t g, uint8_t b);

#endif