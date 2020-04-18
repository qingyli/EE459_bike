#ifndef GPS_H
#define GPS_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define FREQ 7372800
#define BAUD 9600
#define UBRR FREQ/16/BAUD-1
#define MAX_SENTENCE_LEN 80

#define VALID_DATE (1 << 0)
#define VALID_TIME (1 << 1)
#define VALID_LOC  (1 << 2)
#define VALID_ALT  (1 << 3)
#define VALID_SPD  (1 << 4)
#define VALID_DIR  (1 << 5)

#define VALID_LINE_0 (VALID_DATE | VALID_TIME)
#define VALID_LINE_1 (VALID_LOC)
#define VALID_LINE_2 (VALID_LOC)
#define VALID_LINE_3 (VALID_ALT | VALID_SPD | VALID_DIR)

void gps_init(void);
uint8_t gps_check();
uint8_t gps_readline(char* str);
int8_t gps_parse(char* str, uint8_t length);

extern char display_screen[6][21];

#endif