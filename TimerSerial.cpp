// TimerSerial.cpp
// Arduino port of the generic software uart written in C
//
// Generic code from
// Colin Gittins, Software Engineer, Halliburton Energy Services
// (has been available from iar.com web-site -> application notes)
//
// Adapted to AVR using avr-gcc and avr-libc
// by Martin Thomas, Kaiserslautern, Germany
// <eversmith@heizung-thomas.de>
// http://www.siwawi.arubi.uni-kl.de/avr_projects
//
// Adapted to the C++ Arduino environment,
//  renamed to TimerSerial,
//  added buffered output, 
//  sharing the Timer Resource, 
//  allowing the TX pin to be read, 
//  support for TX Only (to save code space),
//  more extensive support for ATtiny85 (Adafruit Trinket),
//  minor bug fixes
//  minor code cleanup
//  update to follow Arduino style guide
// by Eugene Skopal, AE2F Designs LLC, 30-Nov-2013
//

/* Copyright (c) 2003, Colin Gittins
   Copyright (c) 2005, 2007, 2010, Martin Thomas
   Copyright (c) 2013 Eugene Skopal
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. 
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "TimerSerial.h"

#ifndef TIMERSERIAL_TX_ONLY
  #define RX_NUM_OF_BITS (8)        // startbit and stopbit parsed internally (see ISR)
  volatile static char      inbuf[TIMERSERIAL_IN_BUF_SIZE];
  volatile static uint8_t   rx_qin;
           static uint8_t   rx_qout;
  volatile static bool      flag_rxDisable;
  volatile static bool      flag_rx_busy;
#endif

#define TX_NUM_OF_BITS (10)         // 1 Startbit, 8 Databits, 1 Stopbit = 10 Bits/Frame
volatile static char      outbuf[TIMERSERIAL_OUT_BUF_SIZE];
         static uint8_t   tx_qin;
volatile static uint8_t   tx_qout;
volatile static bool      flag_txBusy;

#define set_tx_pin_high()      ( TIMERSERIAL_TXPORT |=  ( 1 << TIMERSERIAL_TXBIT ) )
#define set_tx_pin_low()       ( TIMERSERIAL_TXPORT &= ~( 1 << TIMERSERIAL_TXBIT ) )
#define get_tx_pin_status()    ( TIMERSERIAL_TXPIN  &   ( 1 << TIMERSERIAL_TXBIT ) )
#define get_rx_pin_status()    ( TIMERSERIAL_RXPIN  &   ( 1 << TIMERSERIAL_RXBIT ) )

void TIMER_NULL(void)
{
}

// Allow other users to hook into the Timer Interrupt

void (*TimerSerial::timerHook)(void) = &TIMER_NULL;

void idle(void) 
{
  // timeout handling goes here
  // add watchdog-reset here if needed
}

// Allow other users to hook into the Idle routine
//  the idle routine is called when: 
//    we are looping waiting for a character to be received 
//    we are looping waiting for room in the transmit buffer

void (*TimerSerial::idleHook)(void) = &idle;

static void io_init(void) 
{
  // TX-Pin as output
  TIMERSERIAL_TXPORT |=  ( 1 << TIMERSERIAL_TXBIT );       // Set TX pin high (activates pullup on input as well)
  TIMERSERIAL_TXDDR |=  ( 1 << TIMERSERIAL_TXBIT );
#ifndef TIMERSERIAL_TX_ONLY
  // RX-Pin as input
  TIMERSERIAL_RXDDR &= ~( 1 << TIMERSERIAL_RXBIT );
#endif
}

static void timer_init(void)
{
  uint8_t sreg_tmp;
  
  sreg_tmp = SREG;
  cli();
  
  #if defined(TIMERSERIAL_PLLCSR)
    while (!(PLLCSR & (1<<PLOCK))) {};         // wait for pll to lock
    PLLCSR = TIMERSERIAL_PLLCSR;                   // Turn on PCKE (64mhz clock for timer1)
  #endif

  TIMERSERIAL_T_CONTR_REGA = TIMERSERIAL_CTC_MASKA | TIMERSERIAL_PRESC_MASKA;

  #if defined(TIMERSERIAL_T_CONTR_REGB)
    TIMERSERIAL_T_CONTR_REGB = TIMERSERIAL_CTC_MASKB | TIMERSERIAL_PRESC_MASKB;
  #endif

  TIMERSERIAL_T_COMP_REG = TIMERSERIAL_TIMERTOP;     /* set top */
  
  TIMERSERIAL_T_INTCTL_REG |= TIMERSERIAL_CMPINT_EN_MASK;

  TIMERSERIAL_T_CNT_REG = 0; /* reset counter */
  
  SREG = sreg_tmp;
}

// Setup to use a TIMER to emulate a UART
void TimerSerial::begin(void) 
{
  flag_txBusy  = false;
  #ifndef TIMERSERIAL_TX_ONLY
    flag_rx_busy  = false;
    flag_rxDisable   = false;
  #endif
  io_init();
  timer_init();
}

#ifndef TIMERSERIAL_TX_ONLY
  // Enable receiving characters
  void TimerSerial::rxEnable(void)
  {
    flag_rxDisable = false;
  }
  
  // Disable receiving characters
  void TimerSerial::rxDisable(void)
  {
    while (flag_rx_busy) ;      // Wait until we aren't busy receiving
    flag_rxDisable = true;
    rx_qin = 0;
    rx_qout = 0;
  }

  // Return the number of characters available to read
  int TimerSerial::available(void)
  {
    int8_t avail = rx_qin - rx_qout;
    if (avail < 0) avail += TIMERSERIAL_IN_BUF_SIZE;
    return (avail & 0x7F);
  }

  // Read the next character from the receive buffer (return -1 if there are none)
  int TimerSerial::read(void)
  {
    if ( rx_qin == rx_qout ) {
      return -1;
    } else {
      char c = inbuf[rx_qout];
      if ( ++rx_qout >= TIMERSERIAL_IN_BUF_SIZE ) {
        rx_qout = 0;
      }
      return c;
    }
  }

  // Peek at the next character in the receive buffer (return -1 if there are none)
  int TimerSerial::peek(void)
  {
    if ( rx_qin == rx_qout ) {
      return -1;
    } else {
      return inbuf[rx_qout];
    }
  }

#else 
  // Include dummy routines if we are only transmitting
  
  void TimerSerial::rxEnable(void)
  {
  }

  void TimerSerial::rxDisable(void)
  {
  }

  int TimerSerial::available(void)
  {
    return 0;
  }

  int TimerSerial::read(void)
  {
    return -1;
  }

  int TimerSerial::peek(void)
  {
    return -1;
  }

#endif

// Return true if we are currently transmitting data
bool TimerSerial::txBusy(void)
{
  return (flag_txBusy);
}

// Wait for all characters to be completely transmitted
void TimerSerial::flush(void)
{
  while (txBusy()) {      // Wait for all characters to be sent
    TimerSerial::idleHook();
  }    
}

// Add a character to the output buffer
size_t TimerSerial::write(const uint8_t ch) 
{
  uint8_t  tx_qnext = tx_qin;

  if (++tx_qnext >= TIMERSERIAL_OUT_BUF_SIZE) {
    tx_qnext = 0;
  }

  while (tx_qnext == tx_qout) {
    TimerSerial::idleHook();
  };       // Wait for room in the buffer

  outbuf[tx_qin] = ch;
  tx_qin = tx_qnext;
  flag_txBusy = true;
  return 1;
}

// Allow the user to signal us by setting the tx pin low
void TimerSerial::beginCheckTxPin(void)
{
  flush();                                      // insure we aren't transmitting
  TIMERSERIAL_TXPORT |=  ( 1 << TIMERSERIAL_TXBIT);   // Enable the pullup
  TIMERSERIAL_TXDDR  &= ~( 1 << TIMERSERIAL_TXBIT);   // Make it an input
}

// Back to our controlling the tx pin to send data
void TimerSerial::endCheckTxPin(void)
{
  TIMERSERIAL_TXPORT |=  ( 1 << TIMERSERIAL_TXBIT);         // Make sure it's high
  TIMERSERIAL_TXDDR |=  ( 1 << TIMERSERIAL_TXBIT );         // Back to output
}

// Check the state of the TX pin
bool TimerSerial::readTXpin(void)
{
  return (get_tx_pin_status() != 0);
}

// Check if the Serial Port is ready to use
TimerSerial::operator bool() 
{
  return true;
}  

void TimerSerial::end(void)
{
  rxDisable();     // Stop receiving
  flush();      // flush the output
}

ISR(TIMERSERIAL_T_COMP_LABEL)
{
  static uint8_t  tx_bitslice_ctr;
  static uint16_t tx_char_buffer;

  #ifndef TIMERSERIAL_TX_ONLY
    static bool     flag_rx_waiting_for_stop_bit = false;
    static uint8_t  rx_bit_mask;
    
    static uint8_t  rx_bitslice_ctr;
    static uint8_t  rx_bits_left_ctr;
    static uint8_t  rx_char_buffer;
  #endif
  
  // Transmitter Section
  while ( flag_txBusy == true ) {
    if (tx_bitslice_ctr != 0) {            // Are we sending a bit
      if (--tx_bitslice_ctr != 0) {          // are we done yet?
        break;                              // nope.  we're done here
      }
    }
    if ( tx_char_buffer == 0 ) {    // Have we sent the entire character?
      if ( tx_qout != tx_qin ) {         // yes.  Are there more chars to send?
        tx_char_buffer = ( outbuf[tx_qout] << 1 ) | 0x200; // build the char to send
        if ( ++tx_qout >= TIMERSERIAL_OUT_BUF_SIZE ) {
          tx_qout = 0; // wrap
        }
      } else {
        flag_txBusy = false;
        break;
      }
    }
    
    // Send the next bit
    
    if ( tx_char_buffer & 0x01 ) {
      set_tx_pin_high();
    } else {
      set_tx_pin_low();
    }
    tx_char_buffer >>= 1;
    tx_bitslice_ctr = TIMERSERIAL_BIT_SLICES;
    break;
  }

  #ifndef TIMERSERIAL_TX_ONLY
    // Receiver Section
    if ( flag_rxDisable == false ) {
      if ( flag_rx_waiting_for_stop_bit ) {
        if ( --rx_bitslice_ctr == 0 ) {
          flag_rx_waiting_for_stop_bit = false;
          flag_rx_busy = false;
          inbuf[rx_qin] = rx_char_buffer;
          if ( ++rx_qin >= TIMERSERIAL_IN_BUF_SIZE ) {
            rx_qin = 0;    // wrap around
          }
        }
      } else {
        if ( flag_rx_busy == false ) {
          // We aren't receiving a char -- so check for the start bit
          if ( get_rx_pin_status() == 0 ) {
            flag_rx_busy       = true;
            rx_char_buffer     = 0;
            // Wait for the start bit + 1/2 of the next bit
            rx_bitslice_ctr    = TIMERSERIAL_BIT_SLICES + (TIMERSERIAL_BIT_SLICES / 2);
            rx_bits_left_ctr   = RX_NUM_OF_BITS;
            rx_bit_mask        = 1;
          }
        } else {
          // We are receiving a character
          if ( --rx_bitslice_ctr == 0 ) {
            // read the next bit
            rx_bitslice_ctr = TIMERSERIAL_BIT_SLICES;
            if ( get_rx_pin_status() ) {
              rx_char_buffer |= rx_bit_mask;
            }
            rx_bit_mask <<= 1;
            if ( --rx_bits_left_ctr == 0 ) {
              flag_rx_waiting_for_stop_bit = true;
            }
          }
        }
      }
    }
  #endif
  TimerSerial::timerHook();
}
