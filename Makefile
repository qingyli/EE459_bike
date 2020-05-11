LIB = ./lib
SRC = ./src
TESTS = ./tests
BIN = ./bin

DEVICE     = atmega328p
CLOCK      = 7372800
PROGRAMMER = -c usbtiny -P usb
OBJECTS    = $(BIN)/smart_bike.o $(BIN)/lcd.o $(BIN)/gps.o $(BIN)/adc.o
FUSES      = -U hfuse:w:0xd9:m -U lfuse:w:0xe0:m

# Fuse Low Byte = 0xe0   Fuse High Byte = 0xd9   Fuse Extended Byte = 0xff
# Bit 7: CKDIV8  = 1     Bit 7: RSTDISBL  = 1    Bit 7:
#     6: CKOUT   = 1         6: DWEN      = 1        6:
#     5: SUT1    = 1         5: SPIEN     = 0        5:
#     4: SUT0    = 0         4: WDTON     = 1        4:
#     3: CKSEL3  = 0         3: EESAVE    = 1        3:
#     2: CKSEL2  = 0         2: BOOTSIZ1  = 0        2: BODLEVEL2 = 1
#     1: CKSEL1  = 0         1: BOOTSIZ0  = 0        1: BODLEVEL1 = 1
#     0: CKSEL0  = 0         0: BOOTRST   = 1        0: BODLEVEL0 = 1
# External clock source, start-up time = 14 clks + 65ms
# Don't output clock on PORTB0, don't divide clock by 8,
# Boot reset vector disabled, boot flash size 2048 bytes,
# Preserve EEPROM disabled, watch-dog timer off
# Serial program downloading enabled, debug wire disabled,
# Reset enabled, brown-out detection disabled

# Tune the lines below only if you know what you are doing:

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE = avr-gcc -g -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) -I$(LIB) -I$(SRC)

# symbolic targets:
all: main.hex
.PHONY: all clean flash

smart_bike: all
gps_update_test: OBJECTS = $(BIN)/gps_update_test.o $(BIN)/gps.o $(BIN)/lcd.o
gps_update_test: $(BIN)/gps_update_test.o all
gps_read_test: OBJECTS = $(BIN)/gps_read_test.o $(BIN)/gps.o $(BIN)/lcd.o
gps_read_test: $(BIN)/gps_read_test.o all
lcd_test: OBJECTS = $(BIN)/lcd_test.o $(BIN)/lcd.o
lcd_test: $(BIN)/lcd_test.o all

$(BIN)/smart_bike.o: $(SRC)/smart_bike.c $(LIB)/gps.h $(LIB)/lcd.h $(LIB)/adc.h
$(BIN)/gps_update_test.o: $(TESTS)/gps_update_test.c $(LIB)/gps.h $(LIB)/lcd.h
$(BIN)/gps_read_test.o: $(TESTS)/gps_read_test.c $(LIB)/gps.h $(LIB)/lcd.h
$(BIN)/lcd_test.o: $(TESTS)/lcd_test.c $(LIB)/lcd.h
$(BIN)/lcd.o: $(SRC)/lcd.c $(LIB)/lcd.h
$(BIN)/gps.o: $(SRC)/gps.c $(LIB)/gps.h
$(BIN)/adc.o: $(SRC)/adc.c $(LIB)/adc.h

$(BIN)/%.o: $(SRC)/%.c
	$(COMPILE) -c $< -o $@

$(BIN)/%.o: $(TESTS)/%.c
	$(COMPILE) -c $< -o $@

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:main.hex:i

fuse:
	$(AVRDUDE) $(FUSES)

# Xcode uses the Makefile targets "", "clean" and "install"
install: flash fuse

# if you use a bootloader, change the command below appropriately:
load: all
	bootloadHID main.hex

clean:
	rm -f main.hex main.elf $(wildcard $(BIN)/*)

# file targets:
main.elf: .FORCE
	$(MAKE) $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size --format=avr --mcu=$(DEVICE) main.elf
# If you have an EEPROM section, you must also create a hex file for the
# EEPROM and add it to the "flash" target.

# Targets for code debugging and analysis:
disasm:	main.elf
	avr-objdump -d main.elf

cpp:
	$(COMPILE) -E main.c

.FORCE:
