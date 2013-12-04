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

extern uint16_t calVbg11;
extern uint16_t calVbg256;

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

// Read the VBG voltage based on the reference supplied by the caller

uint16_t readVBGint(uint8_t refBits) {
  /* The Band Gap Reference Voltage is supposed to be 1.1 volts
      but according to the specifications it can range from 1.0 to 1.2 volts */
  
  ADMUX = 0x0C | refBits;     // read VBG
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

// Read VBG based on Vcc (5v)

uint16_t readVBG(void) {
  return readVBGint(0);
}

// Compute the internal transfer function from VBG (1.1v) to 2.56v
//  by reading VBG against the 2.56v source (which is VBG itself through an OPAmp)

uint16_t readXfer256(void) {
  return readVBGint(_BV(REFS2) | _BV(REFS1));
}

// Return the computed VCC based on the current VBG vs the calibrated value

uint16_t readVCC(void) {
  return getVCC(readVBGint(0), calVbg11);
}

// Read the current (1V/1A) based on the reference (1.1v or 2.56v) supplied by the caller

uint16_t readCurrentInt(uint8_t refBits) {
  ADMUX = 3 | refBits; // read PB3 (output from sensor)
  ADCSRA = _BV(ADEN) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);

  delay(10);
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));
  uint8_t low = ADCL;
  uint8_t high = ADCH;
  delay(10);
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

// Read the current (1V/1A) choosing the reference based on the value read

uint16_t readCurrent(void) {
  uint16_t curAdc = readCurrentInt(_BV(REFS1));    // Read current against 1.1v
  uint16_t calValue = calVbg11;

  if (curAdc > 1010) {           // If we are near the limit
    curAdc = readCurrentInt(_BV(REFS2) | _BV(REFS1));   // Read the current against 2.56v
    calValue = calVbg256;
  }
  
  uint32_t reply = curAdc;
  
  reply *= calValue;
  reply /= 1024;
  
  return reply;
}      
