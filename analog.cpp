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
****************************************/

#include <Arduino.h>
#include <SoftwareSerial.h>

extern SoftwareSerial ss;
extern uint16_t calibration;

int readVCC(void) {
  ADMUX = 0x0C; // read VBG
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
  int32_t reply = high;
  reply <<= 8;
  reply |= low;
  //ss.println(reply);
  uint32_t temp;
  if (calibration != 0xFFFF) {
    temp = calibration;  // the 'true' mV reading
  } else {
    temp = 1100;   // its about 1100mV
  }
  temp *= 1024;
  temp /= reply;
  reply = temp;

  return reply;
}

int readCurrent(void) {
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

   if (calibration != 0xFFFF) {
    reply *= calibration;  // the 'true' mV reading
  } else {
    reply *= 1100;  // its about 1100mV
  }
  reply /= 1024;

  return reply;
}

