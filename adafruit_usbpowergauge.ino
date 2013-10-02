/****************************************
   Adafruit USB Power Gauge firmware 

   http://www.adafruit.com/products/1549

   Adafruit invests time and resources providing this open source code,
   please support Adafruit and open-source hardware by purchasing
   products from Adafruit!

   Written by Limor Fried/Ladyada for Adafruit Industries.
   BSD license, check license.txt for more information
   All text above must be included in any redistribution
****************************************/

// Select Trinket 16MHz, 'burn bootloader' to set fuses
// then upload via an *external usbtiny* (not the bootloader!)

#include <SoftwareSerial.h>
#include <EEPROM.h>

SoftwareSerial ss(-1, 1);

uint16_t calibration;

void allInputs(void);
void LED(uint8_t i, boolean f);
int readVCC(void);
int readCurrent(void);

volatile uint8_t x = 0;
volatile uint8_t Lidx = 0, litLEDs = 0;
volatile uint8_t Ldim[6] = {0,0,0,0,0,0};


volatile uint8_t pwm=0;
#define MAXPWM 32
uint8_t gamma[] = {0x0, 0x01, 0x03, 0x06, 0x0A, 0x10, 0x17, 0x20};

SIGNAL(TIMER1_COMPA_vect) {
  // turn all LEDs off
  allInputs();
  // find the next LED and turn one LED on
  while (Lidx < 6) {
    if (litLEDs & _BV(Lidx)) {
      // this LED is on!
      if (Ldim[Lidx] >= pwm) {
        LED(Lidx, true);
      }
      Lidx++;
      break;
    }
    Lidx++;
  }
  if (Lidx >= 6) {
    Lidx = 0;
    pwm++;
    if (pwm > MAXPWM) 
      pwm = 0;
  }
}


// the setup routine runs once when you press reset:
void setup() {                
  allInputs();
  
  ss.begin(9600);

  TCCR1 = _BV(CS10);
  OCR1A = 0xAF;
  TIMSK |= _BV(OCIE1A);
  
  // for just a tiny bit, turn all the LEDs on
  litLEDs = 0xFF;
  for (uint8_t i=0; i<6; i++) Ldim[i] = MAXPWM;
  delay(20);
  litLEDs = 0;
  for (uint8_t i=0; i<6; i++) {
    litLEDs |= _BV(i);
    for (uint8_t p=0; p<MAXPWM; p++) {
      Ldim[i]=p;
      delay(2);
    }
  }
  litLEDs = 0;

  ss.println(F("Adafruit USB Power Meter"));
  ss.println(__DATE__);
  
  // read calibration
  calibration = EEPROM.read(0);
  calibration <<= 8;
  calibration |= EEPROM.read(1);
  if ((calibration == 0xFFFF) || (calibration < 800) || (calibration > 1300)) {
    calibration = 0xFFFF;
    ss.println(F("Not calibrated"));
  } else {
    ss.print(F("Calibration: "));
    ss.println(calibration);
  }
}


uint32_t vcc, icc, watt;

void printStringDelay(char *str) {
  while (str[0]) {
    while (Lidx != 0) {}
    ss.write(str[0]);
    str++; 
  }
}

uint8_t printCounter = 0;

void printDotDecimal(uint16_t x, uint8_t d) {
  ss.print(x/1000);
  if (d > 0) {
    ss.print('.');
    x %= 1000;
    ss.print(x / 100);
  }
  if (d > 1) {
    x %= 100;
    ss.print(x / 10);
  }
}

// the loop routine runs over and over again forever:
void loop() {
  vcc = readVCC();
  icc = readCurrent();
  watt = vcc * icc;
  watt /= 1000;
  
  if (printCounter == 0) {
    while (Lidx != 0) {}
  //  TIMSK &= ~_BV(OCIE1A);
    printStringDelay("\n\rV: "); //ss.println(vcc);  
    vcc += 50; // this is essentially a way to 'round up'
    printDotDecimal(vcc, 1);
    delay(50);
    printStringDelay(" I: "); 
    if (icc < 100) ss.print(' ');
    if (icc < 10) ss.print(' ');
    ss.print(icc); 
    printStringDelay(" mA ");
    delay(50);
    printStringDelay("Watts: "); 
    watt += 50; // this is essentially a way to 'round up'
    printDotDecimal(watt, 1);
    delay(50);
  //  TIMSK |= _BV(OCIE1A);
  }
  printCounter++;
  printCounter %= 10;
  
  // for OK voltages, turn on the green 'OK' LED
  if (vcc >= 4500) {
    litLEDs |= 0x1;
    Ldim[0] = MAXPWM;
  } else {
    litLEDs &= ~0x1;
    Ldim[0] = 0;
  }
  
  for (uint16_t w=1; w<6; w++) {
    if (watt > (w*1000)) {
      // on all the way
      litLEDs |= _BV(w);
      Ldim[w] = MAXPWM;
    } else if (watt > (w-1)*1000) {
      // on partially
      litLEDs |= _BV(w);
      Ldim[w] = gamma[(watt % 1000) / 125];
    } else {
      litLEDs &= ~_BV(w);
      Ldim[w] = 0;
    }
  }
  delay(100);
}
