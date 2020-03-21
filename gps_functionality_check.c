/***
	gps_functionality_check.c - Tests the parsing capabilities of the parsing functions from gps.c
	Most of the code is copied directly from gps.h and gps.c, but with all microcontroller specifics removed (interrupts/init/etc)
	main() contains a few examples of sentences that the GPS could send and it prints changes to display_screen to the terminal

	IMPORTANT: This is not meant to be run on the microcontroller, compile and run on a computer using gcc NOT avr-gcc

	gcc -o gps_func_check gps_functionality_check.c
	./gps_func_check
**/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define IGNORE_CHECK 0
#define MAX_SENTENCE_LEN 80

#define VALID_DATE (1 << 0)
#define VALID_TIME (1 << 1)
#define VALID_LOC  (1 << 2)
#define VALID_ALT  (1 << 3)
#define VALID_SPD  (1 << 4)
#define VALID_DIR  (1 << 5)

#define VALID_LINE_0 VALID_DATE | VALID_TIME
#define VALID_LINE_1 VALID_LOC
#define VALID_LINE_2 VALID_LOC
#define VALID_LINE_3 VALID_ALT
#define VALID_LINE_4 VALID_SPD
#define VALID_LINE_5 VALID_DIR

uint8_t gps_check();
uint8_t gps_readline(char* str);
int8_t gps_parse(char* str, uint8_t length);
void gps_stringout(const char* str);
void gps_charout(char ch);
uint8_t gps_checksum_check(const char* str, uint8_t length);
int8_t hex_to_int(char c);

uint8_t gps_set_time(const char* str);
uint8_t gps_set_lat(const char* str);
uint8_t gps_set_long(const char* str);
uint8_t gps_set_alt(const char* str);
uint8_t gps_set_speed(const char* str);
uint8_t gps_set_direction(const char* str);
uint8_t gps_set_date(const char* str);

volatile uint8_t _new_msg = 0;
volatile char _line1[MAX_SENTENCE_LEN];
volatile char _line2[MAX_SENTENCE_LEN];
volatile char* _buff_line = _line1;
volatile char* _last_line = _line2;
volatile uint8_t _buff_idx = 0;
volatile uint8_t _start_flag = 0;
volatile uint8_t _line_len = 0;

uint8_t _valid_data = 0;
volatile uint8_t _msgs_elapsed = 0;

#define PARSE_ERROR_CODE (_msgs_elapsed >= 120)?-1:_valid_data

/*
Format of 20x4 LCD screen (Row 3 will be toggled between speed, altitude, and direction)

  01234567890123456789  
 |====================| 
0|MM/DD/YY   HH:MM GMT|0
1|    la* ti.tude' N  |1
2|   lon* gi.tude' E  |2
3|Altitude: alti.t m  |3
3|Speed:    spee.d mph|3
3|Direction:    NW    |3
 |====================| 
  01234567890123456789  

*/

// Note: 0xDF is the character code for displaying a degree symbol on the LCD
char display_screen[6][21] = {
	"--/--/--   --:-- GMT",
	"    --* --.----' -  ",
	"   ---* --.----' -  ",
	"Altitude: ----.- m  ",
	"Speed:    ----.- mph",
	"Direction:    --    "
};

/***
    PUBLIC FUNCTIONS
***/

/*
	Check if a new message is available
*/
uint8_t gps_check() {
	return _new_msg;
}

/*
	Copy the last transmitted line to str
	Return length of new line
*/
uint8_t gps_readline(char* str) {
	if(_new_msg) {
		strcpy(str, (char*)_last_line);
		_new_msg = 0;
		return _line_len;
	}
	return 0;
}

/*
	Parse the NMEA sentence and update display_screen
	Returns:	-1 if no valid location has been found in 120 sentences (=1 minute when receiving 2 sentences each second)
				_valid_data to indicate fields that are valid
*/
int8_t gps_parse(char* str, uint8_t length) {
	//Check if checksum is accurate
	if(!gps_checksum_check(str, length))
		return PARSE_ERROR_CODE;
	//ptr1 will be at the start of a field, ptr2 will be at the end
	char* ptr1 = str;
	char* ptr2 = str;
	ptr2 = strchr(ptr1, ',');

	if(strncmp(ptr1+2, "GGA", 3) == 0) { 		// Parse GGA sentence
		// Time field
		// Format: hhmmss.ss (variable decimal point precision)
		// hh - hours, mm - minutes, ss.ss - seconds
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2 || ptr2-ptr1 < 6)
			return PARSE_ERROR_CODE;
		if(gps_set_time(ptr1))
			_valid_data |= VALID_TIME;
		else
			_valid_data &= ~VALID_TIME;

		_valid_data &= ~VALID_LOC;
		// Latitude field
		// Format: ddmm.mmmm,h (variable decimal point precision)
		// dd - degrees, mm.mmmm - minutes, h - lat. hemisphere (N/S)
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2) return PARSE_ERROR_CODE;
		ptr2 = strchr(ptr2+1, ',');
		if(!ptr2 || ptr2-ptr1 < 6)
			return PARSE_ERROR_CODE;
		if(!gps_set_lat(ptr1))
			return PARSE_ERROR_CODE;

		// Longitude field
		// Format: dddmm.mmmm,h (variable decimal point precision)
		// ddd - degrees, mm.mmmm - minutes, h - long. hemisphere (E/W)
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2) return PARSE_ERROR_CODE;
		ptr2 = strchr(ptr2+1, ',');
		if(!ptr2 || ptr2-ptr1 < 7)
			return PARSE_ERROR_CODE;
		if(!gps_set_long(ptr1))
			return PARSE_ERROR_CODE;

		_valid_data |= VALID_LOC;
		
		// Fix quality indicator
		// Format: f
		// f - GPS Quality of Signal (0 - No fix, 1 - GPS fix, 2 - DGPS fix)
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2 || ptr1+1!=ptr2 || *ptr1 < '1' || *ptr1 > '9') {
			_valid_data &= ~VALID_LOC;
			return PARSE_ERROR_CODE;
		}
		_msgs_elapsed = 0;

		// Skip Number of Satellites and HDOP
		ptr2 = strchr(ptr2+1, ',');
		if(!ptr2) return PARSE_ERROR_CODE;
		ptr2 = strchr(ptr2+1, ',');
		if(!ptr2) return PARSE_ERROR_CODE;

		// Altitude
		// Format: aa.aa (variable number digits before and after decimal)
		ptr1 = ptr2+1;
		if(gps_set_alt(ptr1))
			_valid_data |= VALID_ALT;
		else
			_valid_data &= ~VALID_ALT;

		// Skip the rest of the sentence
		return _valid_data;
	} else if(strncmp(ptr1+2, "RMC", 3) == 0) {	// Parse RMC sentence
		// Time field
		// Format: hhmmss.ss (variable decimal point precision)
		// hh - hours, mm - minutes, ss.ss - seconds
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2 || ptr2-ptr1 < 6)
			return PARSE_ERROR_CODE;
		if(gps_set_time(ptr1))
			_valid_data |= VALID_TIME;
		else
			_valid_data &= ~VALID_TIME;

		// Validity indicator
		// Format: f
		// f - (A - OK, V - Warning)
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2 || ptr1+1!=ptr2 || *ptr1 != 'A')
			return PARSE_ERROR_CODE;

		_valid_data &= ~VALID_LOC;
		// Latitude field
		// Format: ddmm.mmmm,h (variable decimal point precision)
		// dd - degrees, mm.mmmm - minutes, h - lat. hemisphere (N/S)
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2) return PARSE_ERROR_CODE;
		ptr2 = strchr(ptr2+1, ',');
		if(!ptr2 || ptr2-ptr1 < 6)
			return PARSE_ERROR_CODE;
		if(!gps_set_lat(ptr1))
			return PARSE_ERROR_CODE;

		// Longitude field
		// Format: dddmm.mmmm,h (variable decimal point precision)
		// ddd - degrees, mm.mmmm - minutes, h - long. hemisphere (E/W)
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2) return PARSE_ERROR_CODE;
		ptr2 = strchr(ptr2+1, ',');
		if(!ptr2 || ptr2-ptr1 < 7)
			return PARSE_ERROR_CODE;
		if(!gps_set_long(ptr1))
			return PARSE_ERROR_CODE;

		_valid_data |= VALID_LOC;
		_msgs_elapsed = 0;

		// Speed field
		// Format: ss.ss (variable number digits before and after decimal)
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2 || ptr1==ptr2)
			return PARSE_ERROR_CODE;
		if(gps_set_speed(ptr1))
			_valid_data |= VALID_SPD;
		else
			_valid_data &= ~VALID_SPD;

		// Direction field
		// Format ddd.d (variable number after decimal)
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2 || ptr1==ptr2)
			return PARSE_ERROR_CODE;
		if(gps_set_direction(ptr1))
			_valid_data |= VALID_DIR;
		else
			_valid_data &= ~VALID_DIR;

		// Date field
		// Format ddmmyy
		// d - day, m - month, y - year
		ptr1 = ptr2+1;
		ptr2 = strchr(ptr1, ',');
		if(!ptr2 || ptr2-ptr1 < 6)
			return PARSE_ERROR_CODE;
		if(gps_set_date(ptr1))
			_valid_data |= VALID_DATE;
		else
			_valid_data &= ~VALID_DATE;

		// Skip the rest
		return PARSE_ERROR_CODE;
	}
	return PARSE_ERROR_CODE;
}

/***
    PRIVATE FUNCTIONS
***/

uint8_t gps_set_time(const char* str) {
	display_screen[0][11] = str[0];	// First digit of hour
	display_screen[0][12] = str[1];	// Second digit of hour
	display_screen[0][14] = str[2];	// First digit of minute
	display_screen[0][15] = str[3];	// Second digit of minute
	return 1;
}

uint8_t gps_set_lat(const char* str) {
	display_screen[1][4] = (str[0]=='0')?' ':str[0]; // First digit of degree
	display_screen[1][5] = str[1];			 		 // Second digit of degree
	display_screen[1][8] = (str[2]=='0')?' ':str[2]; // First digit of minute
	display_screen[1][9] = str[3];					 // Second digit of minute
	str = str+4;
	uint8_t i = (*str==',')?1:0;
	while(*str != ',' && i<5) {
		display_screen[1][10+i] = *str;
		str++;
		i++;
	}
	while(i<5) {
		display_screen[1][10+i] = '0';
		i++;
	}
	display_screen[1][17] = str[1];
	return (str[1] == 'N' || str[1] == 'S');
}

uint8_t gps_set_long(const char* str) {
	display_screen[2][3] = (str[0]=='0')?' ':str[0];				// First digit of degree
	display_screen[2][4] = (str[0]=='0'&&str[1]=='0')?' ':str[1];	// Second digit of degree
	display_screen[2][5] = str[2];									// Third digit of degree
	display_screen[2][8] = (str[3]=='0')?' ':str[3];				// First digit of minute
	display_screen[2][9] = str[4];									// Second digit of minute
	str = str+5;
	uint8_t i = (*str==',')?1:0;
	while(*str != ',' && i<5) {
		display_screen[2][10+i] = *str;
		str++;
		i++;
	}
	while(i<5) {
		display_screen[2][10+i] = '0';
		i++;
	}
	display_screen[2][17] = str[1];
	return (str[1] == 'E' || str[1] == 'W');
}

uint8_t gps_set_alt(const char* str) {
	char* ptr = strchr(str, '.');
	char* end = strchr(str, ',');
	if(!end || end==str) return 0;
	if(!ptr || end < ptr) //No decimal
		ptr = end;
	uint8_t i = 4;
	while(i > ptr-str) { //Replace unneeded digits with a space
		display_screen[3][14-i] = ' ';
		i--;
	}
	while(i > 0) {
		display_screen[3][14-i] = *str;
		str++;
		i--;
	}
	
	if(ptr != end) {
		ptr++;
		display_screen[3][15] = *ptr;
	} else {
		display_screen[3][15] = '0';
	}

	return 1;
}

uint8_t gps_set_speed(const char* str) {
	uint32_t speed = 0;
	char* ptr = strchr(str, '.');
	char* end = strchr(str, ',');
	if(!end || end==str) return 0;
	if(!ptr || end < ptr) //No decimal
		ptr = end;
	while(str < ptr) {
		speed *= 10;
		if(*str < '0' || *str > '9') return 0;
		speed += (*str-'0');
		str++;
	}
	speed *= 10;
	// Use decimal point by making speed 10x the received value
	if(ptr != end) {
	    speed += (*(ptr+1)-'0');
	}
	// Speed is given in knots, convert to mph by multiplying by 1.151
	speed *= 1151;
	speed /= 1000;

	// Set decimal point
	display_screen[4][15] = (char)(speed%10)+'0';
	speed /= 10;

	// Set integer part from least significant to most
	display_screen[4][13] = (char)(speed%10)+'0';
	speed /= 10;
	display_screen[4][12] = (speed == 0)?' ':((char)(speed%10)+'0');
	speed /= 10;
	display_screen[4][11] = (speed == 0)?' ':((char)(speed%10)+'0');
	speed /= 10;
	display_screen[4][10] = (speed == 0)?' ':((char)(speed%10)+'0');
	return (speed==0);
}

uint8_t gps_set_direction(const char* str) {
	uint16_t direction = 0;
	char* ptr = strchr(str, '.');
	char* end = strchr(str, ',');
	if(!end || end==str) return 0;
	if(!ptr || end < ptr) //No decimal
		ptr = end;
	while(str < ptr) {
		direction *= 10;
		if(*str < '0' || *str > '9') return 0;
		direction += (*str-'0');
		str++;
	}
	
	// Determine N/S
	if(direction <= 68 || direction >= 292) { // North
		display_screen[5][14] = 'N';
	} else if(direction >= 112 && direction <= 248) { // South
		display_screen[5][14] = 'S';
	} else { // Neither N/S
		display_screen[5][14] = ' ';
	}

	// Determine E/W
	if(direction >= 22 && direction <= 158) { // East
		display_screen[5][15] = 'E';
	} else if(direction >= 202 && direction <= 338) { // West
		display_screen[5][15] = 'W';
	} else { // Neither E/W
		display_screen[5][15] = ' ';
	}
	return 1;
}

uint8_t gps_set_date(const char* str) {
	display_screen[0][0] = str[2];	// First digit of month
	display_screen[0][1] = str[3];	// Second digit of month
	display_screen[0][3] = str[0];	// First digit of day
	display_screen[0][4] = str[1];	// Second digit of day
	display_screen[0][6] = str[4];	// First digit of year
	display_screen[0][7] = str[5];	// Second digit of year
	return 1;
}

/*
	Check for valid checksum
	Return 0 if invalid, 1 if valid
*/
uint8_t gps_checksum_check(const char* str, uint8_t length) {
    if(IGNORE_CHECK) {
        return (length >= 10);
    }
    if(length < 10 || str[length-3] != '*')
        return 0;
    int8_t cs1 = hex_to_int(str[length-2]);
    int8_t cs2 = hex_to_int(str[length-1]);
    if(cs1 < 0 || cs2 < 0)
        return 0;
    uint8_t checksum = (cs1<<4)|cs2;
    uint8_t i = 0;
    uint8_t parity = 0;
    while(i < length-3) {
        parity ^= str[i++];
    }
    return (checksum == parity);
}

/*
	Convert a hex digit 0-F to decimal 0-15
	Return -1 if hex digit is an error
*/
int8_t hex_to_int(char c) {
    if(c >= '0' && c <= '9')
        return c-'0';
    if(c >= 'A' && c <= 'F')
        return 10+(c-'A');
    if(c >= 'a' && c <= 'f')
        return 10+(c-'a');
    return -1;
}

int main()
{
    uint8_t num_lines = 4;
    char str[4][MAX_SENTENCE_LEN] = {
        "GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62",
        "GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
        "GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*68",
        "GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"
    };
    
    uint8_t i = 0;
    for(i=0; i<num_lines; i++){
        int8_t result = gps_parse(str[i], strlen(str[i]));
        printf("Test %d\n", i);
        printf("Result: %X\n", result);
        printf("%s\n", display_screen[0]);
        printf("%s\n", display_screen[1]);
        printf("%s\n", display_screen[2]);
        printf("%s\n", display_screen[3]);
        printf("%s\n", display_screen[4]);
        printf("%s\n", display_screen[5]);
        printf("\n");
    }

    return 0;
}
