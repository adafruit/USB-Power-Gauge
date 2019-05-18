#ifndef TIMERSERIAL_H
#define TIMERSERIAL_H

#include <inttypes.h>
#include <Stream.h> 

/* Baud Rate is declared in this file to allow all of the computations to setup the timer to be done
   at compile time, by the compiler, reducing the code size. */

#define TIMERSERIAL_BAUD_RATE    9600

/* TIMERSERIAL_BIT_SLICES must be 3 or greater.  If you are sharing the timer, other users
   may need a faster interrupt rate, so you can set BIT_SLICES to a value higher. */

#define TIMERSERIAL_BIT_SLICES     3

/* Define how often the timerHook will be called */

#define TIMERSERIAL_INTERVALS_PER_SEC (TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)

/* If we are only going to transmit -- save some memory by not building the receive buffers or code */

#define TIMERSERIAL_TX_ONLY

/* Define the size of the RX and TX buffers */

#ifndef TIMERSERIAL_TX_ONLY
  #define TIMERSERIAL_IN_BUF_SIZE   8
#endif
#define TIMERSERIAL_OUT_BUF_SIZE  60

#if defined (__AVR_ATtiny85__)
/* 
  When targeting an ATtiny85 Arduino (ADAFRUIT Trinket 16mhz) we
   must use Timer1 to avoid conflict with Aurduino 1.0.5 use of Timer0
 
  N.B. ADAFRUIT Trinket 8mhz: uses the PLL at full speed and divides the system
   clock by two (in the bootloader) to get 8mhz (so the peripheral clock is still 64mhz).

  If you are using the RC oscillator directly to get 8mhz (CLKSEL=0010 -> Low Fuse Bits = 0xE2),
   you can select LSM to run the peripheral clock at 32mhz, which may be necessary at lower voltages. */

//#define TIMERSERIAL_USE_LSM  1  

  #if defined(SOFTAUART_USE_LSM)
    #define F_PERIPHERAL ((8000000L / 2) * 8))    // Peripheral clock runs at 32mhz
    #define TIMERSERIAL_PLLCSR         (1 << LSM | 1 << PCKE | 1 << PLLE)
  #else
    #define F_PERIPHERAL (8000000L * 8)       // Peripheral clock runs at 64mhz
    #define TIMERSERIAL_PLLCSR         (1 << PCKE | 1 << PLLE)
  #endif

  #ifndef TIMERSERIAL_TX_ONLY
    #define TIMERSERIAL_RXPIN PINB
    #define TIMERSERIAL_RXDDR DDRB
    #define TIMERSERIAL_RXBIT PB5        // Receive on "Reset" pin
  #endif

  #define TIMERSERIAL_TXPORT  PORTB
  #define TIMERSERIAL_TXPIN PINB
  #define TIMERSERIAL_TXDDR DDRB
  #define TIMERSERIAL_TXBIT PB1

  // timer1 is an 8-bit timer
  #define TIMERSERIAL_TIMERMAX 0xff

  #define TIMERSERIAL_T_COMP_LABEL    TIMER1_COMPA_vect
  #define TIMERSERIAL_T_COMP_REG      OCR1C
  #define TIMERSERIAL_T_CONTR_REGA    TCCR1
  #define TIMERSERIAL_T_CNT_REG     TCNT1
  #define TIMERSERIAL_T_INTCTL_REG    TIMSK

  #define TIMERSERIAL_CMPINT_EN_MASK    (1 << OCIE1A)

  #define TIMERSERIAL_CTC_MASKA     (1 << CTC1)

  /* Figure out the minimum prescale value */

  #if (F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (1)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS10)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/2) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (2)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS11)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/4) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (4)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS11 | 1 << CS10)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/8) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (8)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS12)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/16) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (16)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS12 | 1 << CS10)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/32) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (32)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS12 | 1 << CS11)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/64) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (64)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS12 | 1 << CS11 | 1 << CS10)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/128) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (128)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS13)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/256) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (256)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS13 | 1 << CS10)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/512) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (512)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS13 | 1 << CS11)
  #elif ((F_PERIPHERAL/(TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES)/1024) < (TIMERSERIAL_TIMERMAX - 1))
    #define TIMERSERIAL_PRESCALE (1024)
    #define TIMERSERIAL_PRESC_MASKA     (1 << CS13 | 1 << CS11 | 1 << CS10)
  #else
    #error "prescale unsupported"
  #endif

  #define TIMERSERIAL_SEGCLOCKS    ((F_PERIPHERAL + ((TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES) / 2)) / (TIMERSERIAL_BAUD_RATE * TIMERSERIAL_BIT_SLICES))
  #define TIMERSERIAL_TIMERTOP     (((TIMERSERIAL_SEGCLOCKS + TIMERSERIAL_PRESCALE/2) / TIMERSERIAL_PRESCALE) - 1)

#elif defined (__AVR_ATtiny25__) || defined (__AVR_ATtiny45__)
  #ifndef TIMERSERIAL_TX_ONLY
    #define TIMERSERIAL_RXPIN PINB
    #define TIMERSERIAL_RXDDR DDRB
    #define TIMERSERIAL_RXBIT PB0
  #endif

  #define TIMERSERIAL_TXPORT  PORTB
  #define TIMERSERIAL_TXDDR DDRB
  #define TIMERSERIAL_TXBIT PB1

  // timer0 is an 8-bit timer
  #define TIMERSERIAL_TIMERMAX 0xff
  
  #define TIMERSERIAL_T_COMP_LABEL    TIM0_COMPA_vect
  #define TIMERSERIAL_T_COMP_REG      OCR0A
  #define TIMERSERIAL_T_CONTR_REGA    TCCR0A
  #define TIMERSERIAL_T_CONTR_REGB    TCCR0B
  #define TIMERSERIAL_T_CNT_REG     TCNT0
  #define TIMERSERIAL_T_INTCTL_REG    TIMSK

  #define TIMERSERIAL_CMPINT_EN_MASK    (1 << OCIE0A)

  #define TIMERSERIAL_CTC_MASKA     (1 << WGM01)
  #define TIMERSERIAL_CTC_MASKB     (0)

  /* "A timer interrupt must be set to interrupt at three times 
     the required baud rate." */
  #define TIMERSERIAL_PRESCALE (8)
  // #define TIMERSERIAL_PRESCALE (1)

  #if (TIMERSERIAL_PRESCALE == 8)
    #define TIMERSERIAL_PRESC_MASKA     (0)
    #define TIMERSERIAL_PRESC_MASKB     (1 << CS01)
  #elif (TIMERSERIAL_PRESCALE==1)
    #define TIMERSERIAL_PRESC_MASKA     (0)
    #define TIMERSERIAL_PRESC_MASKB     (1 << CS00)
  #else 
    #error "prescale unsupported"
  #endif

  #define TIMERSERIAL_TIMERTOP ( F_CPU/TIMERSERIAL_PRESCALE/TIMERSERIAL_BAUD_RATE/TIMERSERIAL_BIT_SLICES - 1)

#elif defined (__AVR_ATmega324P__) || defined (__AVR_ATmega324A__)  \
   || defined (__AVR_ATmega644P__) || defined (__AVR_ATmega644PA__) \
   || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328PA__) \
   || defined (__AVR_ATmega164P__) || defined (__AVR_ATmega164A__)

  #ifndef TIMERSERIAL_TX_ONLY
    #define TIMERSERIAL_RXPIN PINB
    #define TIMERSERIAL_RXDDR DDRB
    #define TIMERSERIAL_RXBIT PORTB0
  #endif

  #define TIMERSERIAL_TXPORT  PORTD
  #define TIMERSERIAL_TXDDR DDRD
  #define TIMERSERIAL_TXBIT PORTD7
  
  // timer1 is a 16-bit timer
  #define TIMERSERIAL_TIMERMAX 0xffff
  
  #define TIMERSERIAL_T_COMP_LABEL    TIMER1_COMPA_vect
  #define TIMERSERIAL_T_COMP_REG      OCR1A
  #define TIMERSERIAL_T_CNT_REG     TCNT1
  
  /* "A timer interrupt must be set to interrupt at three times 
     the required baud rate." */
  // #define TIMERSERIAL_PRESCALE (8)
  #define TIMERSERIAL_PRESCALE (1)

  // TCCR1A: COM1A1 COM1A0 COM1B1 COM1B0    -    -  WGM11  WGM10
  // TCCR1B:  ICNC1  ICES1    �  WGM13  WGM12 CS12   CS11   CS10
  
  // CTC:   WGM 0 1 0 0 
  // CLKio/8:  CS   0 1 0
  // CLKio/1:  CS   0 0 1
  
  #define TIMERSERIAL_T_CONTR_REGA    TCCR1A
  #define TIMERSERIAL_CTC_MASKA     (0)
  
  #if (TIMERSERIAL_PRESCALE == 8)
    #define TIMERSERIAL_PRESC_MASKA   (0)
  #elif (TIMERSERIAL_PRESCALE==1)
    #define TIMERSERIAL_PRESC_MASKA   (0)
  #else 
    #error "prescale unsupported"
  #endif

  #define TIMERSERIAL_T_CONTR_REGB    TCCR1B
  #define TIMERSERIAL_CTC_MASKB     ( _BV(WGM12) )
  
  #if (TIMERSERIAL_PRESCALE == 8)
    #define TIMERSERIAL_PRESC_MASKB   ( _BV(CS11) )
  #elif (TIMERSERIAL_PRESCALE==1)
    #define TIMERSERIAL_PRESC_MASKB   ( _BV(CS10) )
  #else 
    #error "prescale unsupported"
  #endif
  
  #define TIMERSERIAL_T_INTCTL_REG    TIMSK1
  #define TIMERSERIAL_CMPINT_EN_MASK    ( _BV(OCIE1A) )

  #define TIMERSERIAL_TIMERTOP ( F_CPU/TIMERSERIAL_PRESCALE/TIMERSERIAL_BAUD_RATE/TIMERSERIAL_BIT_SLICES - 1)

#else
  #error "no defintions available for this AVR"
#endif

#if (TIMERSERIAL_TIMERTOP > TIMERSERIAL_TIMERMAX)
  #warning "Check TIMERSERIAL_TIMERTOP: increase prescaler, lower F_CPU or use a 16 bit timer"
#endif

class TimerSerial : public Stream {
  public:
    void begin();
    void end();
  
    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual void flush(void);
    virtual size_t write(uint8_t);
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }

    void beginCheckTxPin(void);
    void endCheckTxPin(void);
    bool readTXpin(void);
    bool txBusy(void);
    void rxEnable(void);
    void rxDisable(void);

    static void (*timerHook)(void);
    static void (*idleHook)(void);

    using Print::write;
    operator bool();
};

#endif // TIMERSERIAL_H
