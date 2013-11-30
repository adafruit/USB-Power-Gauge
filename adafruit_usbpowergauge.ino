/****************************************
   Adafruit USB Power Gauge firmware 

   http://www.adafruit.com/products/1549

   Adafruit invests time and resources providing this open source code,
   please support Adafruit and open-source hardware by purchasing
   products from Adafruit!

   Written by Limor Fried/Ladyada for Adafruit Industries.
   BSD license, check license.txt for more information
   All text above must be included in any redistribution
   
   Updated by Eugene Skopal on 29-Nov-2013:
   1) Use TimerSerial a timer based UART simulation so that the
      charlieplexed lights don't flicker when outputting text.
      (The SoftwareSerial code disables interrupts when outputting characters
       causing the charlieplexing routine to stall long enough for it to
       be noticable.)
   2) Allow the user to plug the Power Gauge into a known accurate
      5.0v USB port and reset the calibration value so that the 
      reported voltage levels are more accurate.
      When programming the first time, simply plug the USB Power Gauge
      into a known accurate 5.0v source immediately after programming.
      After that, you can force a recalibration by grouding the TX pin
      through a 1K resistor for 3 seconds.
   3) Fix the LED display routines so that the LSB blue light doesn't flash
      randomly when very low current levels are seen.
   4) Average the ADC readings for one second to eliminate noise.
   5) Display the peak current reading each second.
****************************************/

// Select Trinket 16MHz, 'burn bootloader' to set fuses
// then upload via an *external usbtiny* (not the bootloader!)

#include <EEPROM.h>
#include "TimerSerial.h"
#include "analog.h"

// Use the correct syntax for storing strings in FLASH to make avr-gcc happy
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))

TimerSerial ts;           // Our Timer Based Serial Port

uint16_t calibration;
uint8_t     calCount;

void allInputs(void);
void LED(uint8_t i, boolean f);
void updateCalibration(void);
void showLEDS(uint8_t reverse);

uint32_t  vcc;
uint32_t  icc;
uint32_t  iccPeak;
uint32_t  watt;
uint8_t   count = 0;
volatile	bool       doReport = false;
volatile	uint16_t   intervals = 0;

volatile uint8_t x = 0;
volatile uint8_t Lidx = 0, litLEDs = 0;
volatile uint8_t Ldim[6] = {0,0,0,0,0,0};

volatile uint8_t pwm=0;
#define MAXPWM 32
uint8_t gamma[] = {0x0, 0x01, 0x03, 0x06, 0x0A, 0x10, 0x17, 0x20};

uint8_t     txPinState = 0;
uint8_t     txPinLowCount = 0;
bool        updateCal = false;

void TIMER1_Handler() {
  // turn all LEDs off
  allInputs();
  // find the next LED and turn one LED on
  while (Lidx < 6) {
    if (litLEDs & _BV(Lidx)) {
      // this LED is on!
      if (Ldim[Lidx] > pwm) {
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
    if (pwm >= MAXPWM) 
      pwm = 0;
  }

  // Print a report once per second

  intervals++;
  if (intervals > TIMERSERIAL_INTERVALS_PER_SEC) {
    doReport = true;
    intervals = 0;
  }
}

void flashLEDS(uint8_t count) {
	for (uint8_t i=0; i<6; i++) Ldim[i] = MAXPWM;
	for (uint8_t i=0; i < count ; i++) {
		litLEDs = 0xFF;
		delay(250);
		litLEDs = 0;
		delay(250);
	}
}

void showLEDS(void) {
	flashLEDS(1);
	for (uint8_t i=0; i<6; i++) {
		litLEDs |= _BV(i);
		for (uint8_t p=0; p<MAXPWM; p++) {
			Ldim[i]=p;
			delay(2);
		}
	}
	litLEDs = 0;
}

void showCal(void) {
  ts.print(F("cal: "));
  ts.print(calibration);
  ts.print(" write count: ");
  ts.print(calCount);
  ts.print(F(" VBG adc: "));
  uint16_t vbg = readVBG();
  ts.print(vbg);
  ts.print(F(" vcc: "));
  ts.print(getVCC(vbg, calibration));
  ts.print(" 5v cal: ");
  ts.println(getCal5v(vbg));
}

// Update the calibration constant assuming we are plugged into a 5.0v source

void updateCalibration(void) {
	uint32_t vbg = 0;
	uint8_t  cnt = 0;

	flashLEDS(2);
	
	for (uint8_t i=0; i<8; i++) {
		delay(1000/8);              // Take readings for one second
		vbg += readVBG();
		cnt++;
	}
	vbg /= cnt;               // Get the average value
	calibration = getCal5v(vbg);        // Get the new calibration value
	EEPROM.write(0, (calibration >> 8) & 0xFF);
	EEPROM.write(1, calibration & 0xFF);
	EEPROM.write(2, ++calCount);

	ts.print(F("** wrote "));
	showCal();
	
	flashLEDS(3);       // Signal that we have updated the calibration value
  doReport = false;
  intervals = 0;      // Wait for one seconds before reporting
}

// the setup routine runs once when you press reset:
void setup() {                
  allInputs();

  ts.begin();           // UART emulation uses TIMER1

  ts.timerHook = &TIMER1_Handler;            // Hook into the timer

  // for just a tiny bit, turn all the LEDs on
  showLEDS();

  ts.println(F("\r\nAdafruit USB Power Meter" " -- " __DATE__ " " __TIME__ " /ES"));
	ts.println(F("[To calibrate voltage, ground TX via 1K resistor for 3 seconds with 5.0 volt supply]"));
  
  // read calibration
  calibration = EEPROM.read(0);
  calibration <<= 8;
  calibration |= EEPROM.read(1);
  calCount = EEPROM.read(2);        // How many times has it been written
  if ((calibration < 800) || (calibration > 1300)) {
    calibration = 0xFFFF;       // calibration value is suspect
    calCount = 0xFF;            // so is the count
  }
  if (calCount == 0xFF) {
    calCount = 0;
  }
	
	showCal();
  
  // The first two times we startup -- write the calibration value
  //   The first time we startup, we have just been programmed, so the quality of Vcc is unknown
  //   The second time we startup, we should be plugged into a known high-quality 5.0v source

  if (calCount < 2) {
    updateCalibration();
  }
  
  ts.flush();           // Let our announcement finish displaying
  doReport = false;
  intervals = 0;        // wait one second before first display
}

void printDotDecimal(uint16_t x, uint8_t d) {
  ts.print(x/1000);
  if (d > 0) {
    ts.print('.');
    x %= 1000;
    ts.print(x / 100);
  }
  if (d > 1) {
    x %= 100;
    ts.print(x / 10);
  }
}

// the loop routine runs over and over again forever:
void loop() {
  vcc += readVCC();
  uint16_t  iccNow = readCurrent();
  if (iccNow > iccPeak) {
    iccPeak = iccNow;
  }    
  icc += iccNow;
  
  count++;
  if (ts.txBusy() == false) {
    ts.beginCheckTxPin();      // Prepare to read the state of the TX pin
  }

  while (doReport) {
    doReport = false;
    litLEDs = 0;        // Clear the LEDS
    for (uint8_t i=0; i<6; i++) Ldim[i] = MAXPWM;   // If we turn one on, make it bright
    
    txPinState = ts.readTXpin(); // Read the TX pin
    ts.endCheckTxPin();         // Back to UART mode

    if (txPinState == 0) {
      // The pin is low (someone is signaling us)
      txPinLowCount++;
      if (txPinLowCount == 0) {
        txPinLowCount = 0xFF;     // Handle overflow
      }
      if (txPinLowCount > 2) {
        updateCal = true;            // We want to update the Calibration value
        litLEDs = _BV(5) | _BV(4) | _BV(3);
      }
      litLEDs |= _BV(5);
      if (txPinLowCount > 1) {
          litLEDs |= _BV(4);
      }
    } else {
      // The pin is high (this is normal)
      txPinLowCount = 0;
      if (updateCal) {
        updateCalibration();        // Update the calibration value

        updateCal = false;

        vcc = 0;                    // Toss old values
        icc = 0;
        iccPeak = 0;
        count = 0;
        break;                  // Skip this reporting cycle
      }
    }
    
    vcc = vcc + count / 2;
    vcc = vcc/count;

    icc = icc + count / 2;
    icc = icc/count;

    watt = vcc * icc;
    watt /= 1000;
    ts.print(F("V: "));
    vcc += 50; // this is essentially a way to 'round up'
    printDotDecimal(vcc, 1);
    ts.print(F(" I: ")); 
    if (icc < 100) ts.print(' ');
    if (icc < 10) ts.print(' ');
    ts.print(icc);
    ts.print(F(" mA Ipk: "));
    if (iccPeak < 100) ts.print(' ');
    if (iccPeak < 10) ts.print(' ');
    ts.print(iccPeak);
    ts.print(F(" mA Watts: ")); 
    watt += 50; // this is essentially a way to 'round up'
    printDotDecimal(watt, 1);
    ts.println();
    iccPeak = 0;
    count = 1;
  }

  if (litLEDs == 0) {
    // Show normal LED information
    
    // for OK voltages, turn on the green 'OK' LED
    if (vcc >= 4500) {
      litLEDs |= 0x1;
    } else {
      litLEDs &= ~0x1;
    }

    for (uint16_t w=1; w<6; w++) {
      if (watt > (w*1000)) {
        // on all the way
        litLEDs |= _BV(w);
      } else if (watt > (w-1)*1000) {
        // on partially
        litLEDs |= _BV(w);
        Ldim[w] = gamma[(watt % 1000) / 125];
      } else {
        litLEDs &= ~_BV(w);
      }
    }
  } else {
    // We are showing a special state
    if (updateCal) {
      // Blink the Green LED if we are going to update the calibration value
			uint8_t t = intervals / (TIMERSERIAL_INTERVALS_PER_SEC / 8);	// blink 4 times per second
      if ((t & 1) == 0) {
        litLEDs |= 1;
      } else {
				litLEDs &= ~1;
			}
    }
  }
}
