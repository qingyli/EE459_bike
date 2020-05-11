#include <avr/io.h>

#include "adc.h"


void adc_init() {
	// Initialize the ADC
	ADMUX &= ~(1<<REFS1);	//Set REF bits to AVCC (REFS1=0 and REFS0=1)
	ADMUX |= (1<<REFS0);

	ADMUX &= ~(1<<ADLAR);   //Clear ADLAR to enable 10-bit ADC
	
	ADCSRA |= (1<<ADPS2)|(1<<ADPS1);    //Set prescalar bits to 64
	ADCSRA &= ~(1<<ADPS0);              // 7.37MHz clock turns to 115kHz which is within 50kHz-200kHz clock for ADC

	ADCSRA |= (1<<ADEN);    //Enable ADC
}

uint16_t adc_sample(uint8_t channel) {
	ADMUX &= 0xF0; //Clear mux bits
	ADMUX |= channel&0x0F; //Set channel
	ADCSRA |= (1<<ADSC); //Start conversion
	while(ADCSRA&(1<<ADSC));
	uint16_t result=ADC;
	return result;
}

uint16_t light_reading() {
	return adc_sample(LIGHT_CHAN);
}

uint16_t sonar_reading() {
	return adc_sample(SONAR_CHAN)/2;
}