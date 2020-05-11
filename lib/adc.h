#ifndef ADC_H
#define ADC_H

#define LIGHT_CHAN 2 //ADC channel for light sensor
#define SONAR_CHAN 3 //ADC channel for sonar sensor

void adc_init();
uint16_t light_reading();
uint16_t sonar_reading();
#endif