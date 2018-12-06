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
#ifndef __INC_TIMER_STM8_H
#define __INC_TIMER_STM8_H

#define FASTLED_HAS_MILLIS 1

////////////////////////////////////////////////////////////////////////////////
//
// microseconds should correspond to real time (i.e. 1000us per 1ms)
// but each millisecond is 2.4% longer, i.e. has 1024 real microseconds:
//  realSecondsElapsed = ourMillisElapsed >> 10;              // fast
//  realMillisElapsed = ( ourMillisElapsed * 1000 ) >> 10;    // expensive multiplication

extern TINY volatile _stm8_reg32 _stm8_time_elapsed;

// Fast version, ignoring pending timer interrupt
OPTIMIZE_SPEED
ALWAYS_INLINE
PURE
inline uint16_t millis16()
{
  return _stm8_time_elapsed.w[1];       // should be atomic
}

////////////////////////////////////////////////////////////////////////////////

_EXTERN_C

void stm8_enable_timer();

PURE NO_INTERRUPTS uint32_t millis32();
#define millis() millis32()

PURE NO_INTERRUPTS uint16_t micros16();
#define micros() micros16()

NO_INLINE
void delay_ms16(uint16_t millis);
#define delay(millis)    delay_ms16(millis)
#define delay_ms(millis) delay_ms16(millis)

NO_INLINE
void delay_us16(uint16_t wait_us);

// 16MHz, 16 cycles ~ 1us
NO_INLINE
void delay_us_cycles8(uint8_t wait_us);
NO_INLINE
void delay_us_cycles16(uint16_t wait_us);

#ifdef _STM8_SIMULATOR_
#define delay_us(wait_us) delay_us_cycles16(wait_us)
#else
#define delay_us(wait_us) delay_us16(wait_us)
#endif
#define delayMicroseconds(wait_us) delay_us(wait_us)

_END_EXTERN_C

#endif