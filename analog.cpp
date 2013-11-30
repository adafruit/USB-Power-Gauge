/****************************************
   Adafruit USB Power Gauge firmware 
   http://www.adafruit.com/products/1549
   Analog current/voltage reading support

   Adafruit invests time and resources providing this open source code,
   please support Adafruit and open-source hardware by purchasing
   products from Adafruit!

   Written by Limor Fried/Ladyada for Adafruit Industries.
   BSD license, check license.txt for more information
   All text above must be included in any redistribution
	 
	 Updated by Eugene Skopal to facilitate re-calibration.
****************************************/

#include <Arduino.h>
#include "analog.h"

extern uint16_t calibration;

// Compute the VCC given the current VBG (voltage-band-gap) ADC reading and the actual VBG voltage

uint16_t getVCC(uint16_t vbgAdc, uint16_t vbgActual) {
  uint32_t temp = vbgActual;
  temp *= 1024;
  temp /= vbgAdc;

  return (uint16_t)temp;
}

// Compute the actual VBG value given the VBG ADC reading
//  and assuming we have a 5.0v supply

uint16_t getCal5v(uint16_t vbgAdc) {
  uint32_t  temp;
  temp = 5000;        // Assume Vcc is 5v
  temp *= vbgAdc;
  temp /= 1024;
  return (uint16_t)temp;
}

uint16_t readVBG(void) {
  /* The Band Gap Reference Voltage is supposed to be 1.1 volts
      but according to the specifications it can range from 1.0 to 1.2 volts */
  
  ADMUX = 0x0C;     // read VBG
  ADCSRA = _BV(ADEN) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);

  delay(10);
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));
  delay(10);
  uint8_t low = ADCL;
  uint8_t high = ADCH;
  
  // do a second conversion
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));

  low = ADCL;
  high = ADCH;
  
  uint16_t reply = high;
  reply <<= 8;
  reply |= low;

  return reply;
}

uint16_t readVCC(void) {
  return getVCC(readVBG(), calibration);
}

uint16_t readCurrent(void) {
  ADMUX = 3 | _BV(REFS1); // read PB3 (output from sensor)
  ADCSRA = _BV(ADEN) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);

  delay(10);
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));
  delay(10);
  // do a second conversion
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));

  int32_t reply = ADC;

  reply *= calibration;  // the 'true' mV reading
  reply /= 1024;

  return reply;
}
