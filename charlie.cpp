/****************************************
   Adafruit USB Power Gauge firmware 
   http://www.adafruit.com/products/1549
   Charlie plex code for the LEDs!

   Adafruit invests time and resources providing this open source code,
   please support Adafruit and open-source hardware by purchasing
   products from Adafruit!

   Written by Limor Fried/Ladyada for Adafruit Industries.
   BSD license, check license.txt for more information
   All text above must be included in any redistribution
****************************************/


#include <Arduino.h>

void allInputs() {
 PORTB &= ~_BV(0) & ~_BV(2) & ~_BV(4);
/*
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);
  digitalWrite(4, LOW);
*/
  DDRB &= ~_BV(0) & ~_BV(2) & ~_BV(4);
/*
  pinMode(0, INPUT);   
  pinMode(2, INPUT);   
  pinMode(4, INPUT); 
*/
}

void LED(uint8_t i, boolean f) {
  
  if (!f) {
    allInputs();
    return;
  }
  if (i == 0) {
    DDRB |= _BV(2) | _BV(4);
    //pinMode(2, OUTPUT);
    //pinMode(4, OUTPUT);
    DDRB &= ~_BV(0);
    //pinMode(0, INPUT);
    PORTB |= _BV(4);
    PORTB &= ~_BV(2);
    //digitalWrite(4, HIGH);
    //digitalWrite(2, LOW);
  }
  if (i == 1) {
    DDRB |= _BV(2) | _BV(4);
    DDRB &= ~_BV(0);
    PORTB |= _BV(2);
    PORTB &= ~_BV(4);
  }
  if (i == 2) {
    DDRB |= _BV(0) | _BV(4);
    DDRB &= ~_BV(2);
    PORTB |= _BV(0);
    PORTB &= ~_BV(4);
  }
  if (i == 3) {
    DDRB |= _BV(0) | _BV(4);
    DDRB &= ~_BV(2);
    PORTB |= _BV(4);
    PORTB &= ~_BV(0);
  }
  if (i == 4) {
    DDRB |= _BV(0) | _BV(2);
    DDRB &= ~_BV(4);
    PORTB |= _BV(0);
    PORTB &= ~_BV(2);
  }
  if (i == 5) {
    DDRB |= _BV(0) | _BV(2);
    DDRB &= ~_BV(4);
    PORTB |= _BV(2);
    PORTB &= ~_BV(0);
  }
}

