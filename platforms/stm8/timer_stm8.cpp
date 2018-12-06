/*******************************************************************************

Copyright (c) 2017-present J Thiel

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/
#error obsolete
#define _STM8_INTERNAL

#include "platform_stm8.h"
#include "timer_stm8.h"
#include "lib_stm8.h"

//uncomment to toggle pin on every timer interrupt
//#define _STM8_TIMER_DEBUG_PIN PB4

////////////////////////////////////////////////////////////////////////////////
// Our timer is based on either TIM4 or TIM6, which for our purposes are
// functionally identical. Hardware implements either one or the other.
//
#if defined(__ICCSTM8__)

# if defined(TIM4_OVR_UIF_vector)
#  define _STM8_HAVE_TIM4
#  define _STM8_TIMER TIM4
# elif defined(TIM6_OVR_UIF_vector)
#  define _STM8_HAVE_TIM6
#  define _STM8_TIMER TIM6
# endif
  // This should be 25 for either TIM4 or TIM6,
  // corresponsing to IRQ No. 23 in the STM8 Manual
  //  #define TIM4_OVR_UIF_vector   25
  //  #define TIM6_OVR_UIF_vector   25
# define _STM8_TIMER_IRQ_VECTOR  _GLUE( _STM8_TIMER, _OVR_UIF_vector )
  // these can have slightly different addresses
# define _STM8_TIMER_COUNTER     _GLUE( _STM8_TIMER, _CNTR )
# define _STM8_TIMER_PRESCALER   _GLUE( _STM8_TIMER, _PSCR)  // PSC 0x07
# define _STM8_TIMER_ARR         _GLUE( _STM8_TIMER, _ARR )
# define _STM8_TIMER_IER         _GLUE( _STM8_TIMER, _IER )  // UIE 0x01
# define _STM8_TIMER_SR1         _GLUE( _STM8_TIMER, _SR )   // UIF 0x01
# define _STM8_TIMER_CR1         _GLUE( _STM8_TIMER, _CR1 )  // CEN 0x01

  // TODO: These are highly variable between the different lines
  // however, TIM4/6 always seem to use the same values
# define _STM8_TIMER_CGR         CLK_PCKENR1    // clock gating register
# define _STM8_TIMER_CGR_MASK    (1<<4)

#else // check for STM's system header defines

# if defined(TIM4)
#  define _STM8_HAVE_TIM4
#  define _STM8_TIMER TIM4
# elif defined(TIM6)
#  define _STM8_HAVE_TIM6
#  define _STM8_TIMER TIM6
# endif

# define _STM8_TIMER_IRQ_VECTOR _Pragma("error Unsupported compiler")

  // these can have slightly different addresses
# define _STM8_TIMER_COUNTER     _GLUE( _STM8_TIMER, ->CNTR )
# define _STM8_TIMER_PRESCALER   _GLUE( _STM8_TIMER, ->PSCR) // PSC 0x07
# define _STM8_TIMER_ARR         _GLUE( _STM8_TIMER, ->ARR )
# define _STM8_TIMER_IER         _GLUE( _STM8_TIMER, ->IER)  // UIE 0x01
# define _STM8_TIMER_SR1         _GLUE( _STM8_TIMER, ->SR1 ) // UIF 0x01
# define _STM8_TIMER_CR1         _GLUE( _STM8_TIMER, ->CR1 ) // CEN 0x01

# define _STM8_TIMER_CGR         CLK_PCKENR1

#endif


////////////////////////////////////////////////////////////////////////////////
//

// A pin that is toggled every millisecond
#ifdef _STM8_TIMER_DEBUG_PIN
FastPin< _STM8_TIMER_DEBUG_PIN > _stm8_timer_debug_pin;
#endif

// millisecond counter
TINY volatile _stm8_reg32 _stm8_time_elapsed = { 0ul };

_EXTERN_C

// Interrupt handler for the TIM4 or TIM6 update interrupt
OPTIMIZE_SPEED
INTERRUPT( _STM8_TIMER_IRQ_VECTOR )
void _stm8_timer_update(void)
{
#ifdef _STM8_TIMER_DEBUG_PIN
  _stm8_timer_debug_pin.toggle();
#endif

  // when the interrupt is raised (and UIF set),
  // the counter value is already at 0

#ifdef _STM8_SIMULATOR_
  // The simulator doesn't simulate any timer activity, only interrupts
  _STM8_TIMER_COUNTER = 0;
#endif

  //  ++millis_elapsed.value;
  // _stm8_inc32 is a total fail in this case!
  //_stm8_inc32(_stm8_time_elapsed);
  asm("INC   s:_stm8_time_elapsed+3   \n"
      "JRNE  done                     \n"
      "INC   s:_stm8_time_elapsed+2   \n"
      "JRNE  done                     \n"
      "INC   s:_stm8_time_elapsed+1   \n"
      "JRNE  done                     \n"
      "INC   s:_stm8_time_elapsed     \n"
      "done:                          \n");

  _STM8_TIMER_SR1 &= ~0x01;             // UIF=0x01, clear flag
}

////////////////////////////////////////////////////////////////////////////////

#if 0
#define TIM_PRESCALER_1         0x00
#define TIM_PRESCALER_2         0x01
#define TIM_PRESCALER_4         0x02
#define TIM_PRESCALER_8         0x03
#define TIM_PRESCALER_16        0x04
#define TIM_PRESCALER_32        0x05
#define TIM_PRESCALER_64        0x06
#define TIM_PRESCALER_128       0x07    // max for TIM4/6
#endif

//__attribute__((constructor))
OPTIMIZE_SIZE
void stm8_enable_timer()
{
  // Peripheral clock gating register
  _STM8_TIMER_CGR |= _STM8_TIMER_CGR_MASK;

  // TODO: Use F_CPU to maximize RELOAD/PERIOD
  _STM8_TIMER_PRESCALER = 6;  // 1<<6 == 64, e.g.  16,000,000 / 64  = 250,000
  _STM8_TIMER_ARR = 249;   // period (250-1), e.g. 250,000 / 250 = 1,000

  _STM8_TIMER_IER |= 0x01;     // UIE=0x01, Enable Update Interrupt
  //_STM8_TIMER_SR1 &= ~0x01;    // UIF=0x01, clear any pending updates
  _STM8_TIMER_CR1 |= 0x01;     // CEN=0x01, Enable Timer

#ifdef _STM8_TIMER_DEBUG_PIN
  _stm8_timer_debug_pin.setOutput(PushPull);
  _stm8_timer_debug_pin.hi();
#endif

  enableInterrupts();
}

////////////////////////////////////////////////////////////////////////////////

// TODO as our interrupt ticks every millisecond, it wouldnt
// make much sense to use millisXX without interrupts enabled

OPTIMIZE_SIZE
NO_INTERRUPTS   // interrupts are disabled during execution
uint32_t millis32()
{
  uint32_t m = _stm8_time_elapsed.value;
  if( _STM8_TIMER_SR1 & 0x01 ) _stm8_inc32(m);
  return m;
}

OPTIMIZE_SPEED
NO_INTERRUPTS   // interrupts are disabled during execution
uint16_t micros16()
{
  register uint16_t m = _stm8_time_elapsed.w[1];
  if( _STM8_TIMER_SR1 & 0x01 ) ++m;

#ifdef _STM8_SIMULATOR_
  // The simulator doesn't simulate the timer, only the interrupt
  // so make sure we have some progress here
  ++_STM8_TIMER_COUNTER;
#endif

  uint8_t ctr = _STM8_TIMER_COUNTER;
  // m has now millis, and we add a 10bit microsecond value
  // by multiplying the counter value (0..255) by 4.
  return ( ((uint16_t)( m << 8 )) | ctr ) << 2 | ( ctr >> 6 );
}

////////////////////////////////////////////////////////////////////////////////

OPTIMIZE_SIZE
NO_INLINE
void delay_ms16(uint16_t wait_ms)
{
  // TODO: feed the dog? wdg();
  // TODO: define a weak void yield();  ???
  const uint16_t start = millis16();
  do { /* nothing */ } while( ((uint16_t)( millis16() - start)) < wait_ms);
}

OPTIMIZE_SIZE
NO_INLINE
void delay_us16(uint16_t wait_us)
{
  // TODO: feed the dog? wdg();
  const uint16_t start = micros16();
  do { /* nothing */ } while( ((uint16_t)( micros16() - start)) < wait_us);
}

////////////////////////////////////////////////////////////////////////////////

OPTIMIZE_SIZE
NO_INLINE
void delay_us_cycles_8(volatile uint8_t wait_us)
{
  // allow for call overhead on first execution
  // ldw x, #wait_us; call func; jra; loop; nop; skip; ...; decw x; jrne loop; ret
  do {
    nop();
    if( --wait_us == 0 ) return;
    nop(); nop(); nop(); nop();
    nop(); nop(); nop(); nop();
    nop(); nop(); nop();
  } while( 1);
}

OPTIMIZE_SIZE
NO_INLINE
CONST
void delay_us_cycles16(uint16_t wait_us)
{
  delay_us_cycles8((uint8_t)wait_us);
  for(uint8_t w = wait_us >>= 8; w; --w)
    delay_us_cycles8(255);
}

////////////////////////////////////////////////////////////////////////////////

_END_EXTERN_C

