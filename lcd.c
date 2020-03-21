#include "lcd.h"

// Private functions for lcd.c
void lcd_writenibble(uint8_t bits);
void lcd_writecommand(uint8_t cmd);
void lcd_writedata(uint8_t dat);
uint8_t row_offset(uint8_t row);


/***
    PUBLIC FUNCTIONS
***/

/*
    Initializes LCD
*/
void lcd_init(void) {
    // Set output pins
    DDRB |= LCD_DATA_BITS;
    DDRD |= LCD_RS_BIT;
    DDRB |= LCD_EN_BIT;

    // From HD44780 datasheet figure 24 on page 46
    _delay_ms(50);          // Delay at least 40ms after 2.7V is reached
    PORTD &= ~LCD_RS_BIT;   // Set to command mode

    lcd_writenibble(0x03);    
    _delay_ms(5);           // Delay at least 4ms
    lcd_writenibble(0x03);
    _delay_ms(5);           // Delay at least 4ms
    lcd_writenibble(0x03);
    _delay_us(120);         // Delay at least 100us
    lcd_writenibble(0x03);

    lcd_writenibble(0x02);  // Use 4-bit interface
    _delay_ms(2);           // Delay at least 2ms
    
    lcd_writecommand(0x28); // Function Set: 4-bit interface, 2 lines, 5x8 chars

    uint8_t display_cmd = 0x08;

    // Comment any of the 3 lines below to set the feature off
    display_cmd |= 0x04; // Enable display on
    display_cmd |= 0x02; // Enable cursor on
    display_cmd |= 0x01; // Enable blinking on

    lcd_writecommand(display_cmd); // Set up display mode
    lcd_writecommand(0x06);        // Set Entry Mode

    // Setup RGB backlight
    DDRD |= (LCD_R_BIT | LCD_G_BIT | LCD_B_BIT);

    // Enable PWM for TIMER2 for R
    TCCR2A |= (1 << WGM01) | (1 << WGM00);
    TCCR2B |= (1 << CS22);   // Prescaler 64
    //TCCR2B |= (1 << CS21); // Prescaler 8
    TCCR2A |= (1 << COM2B1); // R

    // Enable PWM on TIMER0 for G and B
    TCCR0A |= (1 << WGM01) | (1 << WGM00);
    TCCR0B |= (1 << CS01) | (1 << CS00); // Prescaler 64
    //TCCR0B |= (1 << CS01); // Prescaler 8
    TCCR0A |= (1 << COM0B1); // G
    TCCR0A |= (1 << COM0A1); // B

    lcd_set_rgb(LCD_COLOR_WHITE);   //Set white backlight
    lcd_clear();                    // Clear the LCD screen
}

/*
    Clears the LCD screen
*/
void lcd_clear(void) {
    lcd_writecommand(0x01);
    _delay_ms(2);           // Delay 2ms
}

/*
    Sets the cursor to home
*/
void lcd_home(void) {
    lcd_writecommand(0x02);
    _delay_ms(2);           // Delay 2ms
}

/*
    Move the cursor to a specified location
    'row': [0, 3], 'col': [0, 19]
*/
void lcd_moveto(uint8_t row, uint8_t col) {
    uint8_t pos;
    pos = row_offset(row) | col;
    lcd_writecommand(0x80 | pos); // Send move command
}

/*
    Write a single character to LCD
*/
void lcd_charout(char ch) {
    lcd_writedata(ch);
}

/*
    Print up to 'max' amount of characters from 'str' at the current cursor location
    Will not autowrap on lcd screen
*/
void lcd_stringnout(const char *str, uint8_t max) {
    uint8_t i = 0;
    while (str[i] != '\0' && i < max) {  // Loop until next charater is NULL byte or i==max
        lcd_writedata(str[i]);  // Send the character
        i++;
    }
}

void lcd_stringout(const char *str) {
    lcd_stringnout(str, 20);
}

/*
    Set color of RGB backlight by changing the OCR values of the coresponding timers for each color
    The OCR values change the PWM duty cycle by setting the signal high until it reaches the OCR value,
    then keeping the signal low until reaching 255
*/
void lcd_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    OCR2B = r;
    OCR0B = g;
    OCR0A = b;
}





/***
    PRIVATE FUNCTIONS
***/

/*
    Write a byte to command register
*/
void lcd_writecommand(uint8_t cmd) {
    PORTB &= ~LCD_RS_BIT;       // Set to command mode
    lcd_writenibble(cmd >> 4);  // Send upper 4 bits
    lcd_writenibble(cmd);       // Send lower 4 bits
}

/*
    Write a byte to data register
*/
void lcd_writedata(uint8_t dat) {
    PORTB |= LCD_RS_BIT;        // Set to data mode
    lcd_writenibble(dat >> 4);  // Send upper 4 bits
    lcd_writenibble(dat);       // Send lower 4 bits
}

/*
    Send 4 bits to the LCD (lower 4 of lcdbits)
*/
void lcd_writenibble(uint8_t lcdbits) {
    lcdbits &= 0x0F;
    
    PORTB &= ~LCD_DATA_BITS;
    PORTB |= (lcdbits << LCD_DATA_OFFSET);
    
    //Send enable pulse
    PORTD |= LCD_EN_BIT;
    _delay_us(1);
    PORTD &= ~LCD_EN_BIT;
    _delay_us(50);
}

/*
    Returns the address offset that each row begins at
*/
uint8_t row_offset(uint8_t row) {
    switch(row) {
        case 0:
            return 0x00;
        case 1:
            return 0x40;
        case 2:
            return 0x14;
        case 3:
            return 0x54;
    }
    return 0x00;
}