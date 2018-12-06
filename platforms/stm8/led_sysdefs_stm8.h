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

#ifndef __INC_LED_SYSDEFS_STM8_H
#define __INC_LED_SYSDEFS_STM8_H

#include "hal/stm8hal.h"
#include "hal/math.h"
#include "hal/timer.h"
#define FASTLED_HAS_MILLIS 1
#include "hal/delay.h"
#include "hal/crc.h"

#define FASTLED_STM8

//#define FASTLED_NAMESPACE_BEGIN namespace NSFastLED {
//#define FASTLED_NAMESPACE_END }
//#define FASTLED_USING_NAMESPACE using namespace NSFastLED;

#include "lib_stm8.h"

#ifdef _STM8_PREFER_PASS_X_REG
  #define _stm8_scale8(i,scale)       _STM8_F(scale8_alt)(i, scale)
  #define _stm8_scale8_video(i,scale) _STM8_F(scale8_video_alt)(i, scale)
#else
  #define _stm8_scale8(i,scale)       _STM8_F(scale8_orig)(i, scale)
  #define _stm8_scale8_video(i,scale) _STM8_F(scale8_video_orig)(i, scale)
#endif // #ifdef _STM8_PREFER_PASS_X_REG

//#define NO_CORRECTION = 1
//#define NO_DITHERING = 1

// Default to allowing interrupts
#ifndef FASTLED_ALLOW_INTERRUPTS
#define FASTLED_ALLOW_INTERRUPTS 1
#endif

// ??????????
//#if FASTLED_ALLOW_INTERRUPTS == 1
//#define FASTLED_ACCURATE_CLOCK
//#endif

// ??????????
#ifndef INTERRUPT_THRESHOLD
#define INTERRUPT_THRESHOLD 1
#endif



#if defined(FASTLED_FORCE_SOFTWARE_PINS)
#warning "Software pin support forced, pin access will be slightly slower."
#define NO_HARDWARE_PIN_SUPPORT
#undef HAS_HARDWARE_PIN_SUPPORT
#else
#include "hal/pin.h"
#define HAS_HARDWARE_PIN_SUPPORT
#endif

// Indicate that we do NOT have global digitalPinToBitMask(PIN), digitalPinToPort(PIN),
// portOutputRegister(PORT) and portInputRegister(PORT) functions.
// Used to initialize the FastPin class when FASTLED_FORCE_SOFTWARE_PINS is defined.
#define FASTLED_NO_PINMAP



// pgmspace definitions
#define PROGMEM
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#define pgm_read_dword_near(addr) pgm_read_dword(addr)

// Default to NOT using PROGMEM here
#ifndef FASTLED_USE_PROGMEM
#define FASTLED_USE_PROGMEM 0
#endif


#endif // __LED_SYSDEFS_STM8_H
