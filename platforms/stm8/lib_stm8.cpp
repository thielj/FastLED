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

#define _STM_INTERNAL
#include "led_sysdefs_stm8.h"
#include "lib_stm8.h"

#define _STM8_PRESERVE_X 1
#define _STM8_PRESERVE_Y 1

_EXTERN_C

#ifndef _STM8_PREFER_C_INLINE

// i is actually uint8_t, except that we prefer to get it in the X register
NO_INLINE
uint8_t _stm8_scale8_alt( uint16_t i, fract8 scale)
{
  // return SCALE8( i, scale, i)

  // arguments passed:
  //      X       uint16_t i  -- we must preserve this
  //      A       scale
  asm("PUSHW  X                 \n" // backup as 16bit for later
      "MUL    X, A              \n"
      "ADDW   X, (1,SP)         \n" // add another i, ADDW doesnt support shortmem
      "LD     A, XH             \n" // return high 8 bits
      "__exit:                  \n"
      "POPW   X                 \n");
}

NO_INLINE
uint8_t _STM8_F(scale8_orig)( uint8_t i, fract8 scale)
{
  // arguments passed:
  //      A       uint8_t i
  //      ?b0     scale
  asm("PUSHW  X                 \n" // preserve register

      "CLR    s:VR+0            \n" // we're going to add the word
      "LD     s:VR+1, A         \n"
      "EXG    A, XL             \n"
      "LD     A, S:?b0          \n" // load scale
      "MUL    X, A              \n"
      "ADDW   X, VR+0           \n" // add another i, ADDW doesnt support shortmem
      "LD     A, XH             \n" // return high 8 bits

      "POPW   X                 \n");
}

// as above, but modifies the argument in place
NO_INLINE
void _STM8_F(nscale8)( uint8_t& i, fract8 scale)
{
  // arguments passed:
  //      X       uint8_t& i    - must be preserved !!!
  //      A       scale
  asm("PUSHW  Y                 \n" // preserve register

      "EXG    A, YL             \n" // move scale from A to Y
      "LD     A, (X)            \n"
      "MUL    Y, A              \n"
      "EXG    A, YL             \n"
      "ADD    A, (X)            \n" // add i to product of i * scale
      "LD     A, YH             \n"
      "ADC    A, #00            \n" // add carry, if there was one
      "LD     (X), A            \n"

      "POPW   Y                 \n");
}

// i is actually uint8_t, except that we prefer to get it in the X register
NO_INLINE
uint8_t _stm8_scale8_video_alt( uint16_t i, fract8 scale)
{
  // return SCALE8( i, scale, 0xFF )

  // arguments passed:
  //      X     uint8_t i
  //      A     fract8 scale
  asm("PUSHW  X                 \n" // preserve register

      "MUL    X, A              \n"
      "ADDW   X, #$FF           \n" // roundup, unless it's really 0xXH00
      "LD     A, XH             \n" // return high 8 bits

      "POPW   X                 \n");
}

NO_INLINE
uint8_t _STM8_F(scale8_video_orig)( uint8_t i, fract8 scale)
{
  // return SCALE8( i, scale, 0xFF )

  // arguments passed:
  //      A       uint8_t i
  //      ?b0     scale
  asm("PUSHW  X                 \n" // preserve register

      "LD     XL, A             \n" // move i to X, XH is ignored by MUL
      "LD     A, s:?b0          \n" // load scale into A
      "MUL    X, A              \n"
      "ADDW   X, #$FF           \n" // roundup, unless it's really 0xXH00
      "LD     A, XH             \n" // return high 8 bits

      "POPW   X                 \n");
}

// as above, but modifies the argument in place
NO_INLINE
void _STM8_F(nscale8_video)( uint8_t & i, fract8 scale)
{
  // i = SCALE8( i, scale, 0xFF )

  // arguments passed:
  //      X       uint8_t& i
  //      A       scale
  asm("PUSHW  Y                 \n" // preserve register

      "EXG    A, YL             \n" // move scale from A to Y
      "LD     A, (X)            \n"
      "MUL    Y, A              \n"
      "ADDW   Y, #$FF           \n"
      "LD     A, YH             \n"
      "LD     (X), A            \n"

      "POPW   Y                 \n");
}

#endif // #ifndef _STM8_PREFER_C_INLINE

// scale 3 consecutive bytes in place
NO_INLINE
void _STM8_F(nscale8x3)( uint8_t * data, fract8 scale)
{
  // arguments passed:
  //      X     uint8_t* data  -- must be preserved
  //      A     fract8 scale
  asm("PUSHW  Y                 \n" // preserve register

      "EXG    A, YL             \n" // backup scale

    //"__loop:                  \n"

      "LD     A, (X)            \n" // load data byte into A
      "EXG    A, YL             \n" // move value into Y and restore scale
      "MUL    Y, A              \n" //
      "RRWA   Y, A              \n" // backup scale, pop out YL
      "ADD    A, (X)            \n" // add i to product of i * scale
      "RRWA   Y, A              \n" // pop out YH, move scale to YL
      "ADC    A, #00            \n" // add carry, if there was one
      "LD     (X), A            \n" // store value

      "LD     A, (s:1,X)        \n" // load data byte into A
      "EXG    A, YL             \n" // move value into Y and restore scale
      "MUL    Y, A              \n" //
      "RRWA   Y, A              \n" // backup scale, pop out YL
      "ADD    A, (s:1,X)        \n" // add i to product of i * scale
      "RRWA   Y, A              \n" // pop out YH, move scale to YL
      "ADC    A, #00            \n" // add carry, if there was one
      "LD     (s:1,X), A        \n" // store value

      "LD     A, (s:2,X)        \n" // load data byte into A
      "EXG    A, YL             \n" // move value into Y and restore scale
      "MUL    Y, A              \n" //
      "RRWA   Y, A              \n" // backup scale, pop out YL
      "ADD    A, (s:2,X)        \n" // add i to product of i * scale
      "RRWA   Y, A              \n" // pop out YH, move scale to YL
      "ADC    A, #00            \n" // add carry, if there was one
      "LD     (s:2,X), A        \n" // store value

      "EXG    A, YL             \n" // re-store A, just in case & it's cheap

    //"__exit:                  \n"
      "POPW   Y                 \n");
}

// scale consecutive bytes in place, len is currently 0..INT16_MAX
NO_INLINE
void _STM8_F(nscale8x)( uint8_t * data, int16_t len, fract8 scale)
{
  // arguments passed:
  //      X     uint8_t* data  -- must be preserved
  //      Y     int16_t len    -- must be preserved
  //      A     fract8 scale
  asm("LDW    s:VR+0, Y         \n" // store away len
      "JREQ   __exit            \n" // early termination if len==0

      "PUSHW  X                 \n" // preserve registers
      "PUSHW  Y                 \n" //
      "EXG    A, YL             \n" // backup scale

      "__loop:                  \n"

      "LD     A, (X)            \n" // load data byte into A
    //"JREQ   __write           \n" // if( db == 0) store 0; not worth checking!
      "EXG    A, YL             \n" // move value into Y and restore scale
      "MUL    Y, A              \n" //
      "RRWA   Y, A              \n" // backup scale, pop out YL
      "ADD    A, (X)            \n" // add i to product of i * scale
      "RRWA   Y, A              \n" // pop out YH, move scale to YL
      "ADC    A, #00            \n" // add carry, if there was one
    //"__write:                 \n"
      "LD     (X), A            \n" // store first value

      "INCW   X                 \n" //

      "DEC    s:VR+1            \n" // decrement len in two steps
      "JRNE   __loop            \n" //
      "DEC    s:VR+0            \n" // This works only if len < INT16_MAX
      "JRPL   __loop            \n" //

      "EXG     A, XL            \n" // restore scale, it's cheap
      "POPW    Y                \n" //
      "POPW    X                \n" //

      "__exit:                  \n"
      );
}
// scales 3 consecutive bytes in place
NO_INLINE
void _STM8_F(nscale8x3_video)( uint8_t * data, fract8 scale)
{
  // arguments passed:
  //      X       uint8_t * data  -- must be preserved
  //      A       scale
  asm("PUSHW  Y                 \n"  // preserve register

      "EXG    A, YL             \n"  // backup scale

      "LD     A, (X)            \n"
      "EXG    A, YL             \n"  // move value into X and restore scale
      "MUL    Y, A              \n"
      "ADDW   Y, #$FF           \n"
      "RLWA   Y, A              \n"  // backup scale, move XH to A
      "LD     (X), A            \n"

      "LD     A, (S:1,X)        \n"
      "EXG    A, YL             \n"  // move value into X and restore scale
      "MUL    Y, A              \n"
      "ADDW   Y, #$FF           \n"
      "RLWA   Y, A              \n"  // backup scale
      "LD     (s:1,X), A        \n"

      "LD     A, (S:2,X)        \n"
      "EXG    A, YL             \n"  // move value into X and restore scale
      "MUL    Y, A              \n"
      "ADDW   Y, #$FF           \n"
      "RLWA   Y, A              \n"
      "LD     (s:2,X), A        \n"

      "POPW   Y                 \n");

}

// scales consecutive bytes in place, len is currently 0..INT16_MAX
NO_INLINE
void _stm8_nscale8x_video( uint8_t * data, int16_t len, fract8 scale)
{
  // arguments passed:
  //      X     uint8_t* data  -- must be preserved
  //      Y     int16_t len    -- must be preserved
  //      A     fract8 scale      is preserved
  asm(
      "LDW    s:VR+0, Y         \n" // store away len to VR.b0..b1
      "JREQ   __exit            \n" // early termination if len==0

      "PUSHW  X                 \n" // preserve registers
      "PUSHW  Y                 \n"
      "EXG    A, YL             \n"  // backup scale

      "__loop:                  \n"

      "LD     A, (X)            \n" // load data byte into A
      "EXG    A, YL             \n" // move value into Y and restore scale
      "MUL    Y, A              \n"
      "ADDW   Y, #$FF           \n"
      "RLWA   Y, A              \n" // backup scale, move YH to A
      "LD     (X), A            \n" // store first value

      "INCW   X                 \n"

      "DEC    s:VR+1            \n" // decrement len in two steps, first b1
      "JRNE   __loop            \n"
      "DEC    s:VR+0            \n" // ...and then carry over to b0 if necessary
      "JRPL   __loop            \n"

      "EXG     A, YL            \n" // restore scale, it's cheap
      "POPW    Y                \n"
      "POPW    X                \n"

      "__exit:                  \n");
}

#if 0

// memset ???
NO_INLINE
__root
void _stm8_fill( uint8_t * data, size_t len, uint8_t value)
{
  // passed in X, Y, A;
  asm("PUSHW  X                 \n"
      "PUSHW  Y                 \n"
      "TNZW   Y                 \n"
      "JREQ   __exit            \n"
      "__loop:                  \n"
      "LD     (X), A            \n"
      "INCW   X                 \n"
      "DECW   Y                 \n"
      "__test:                  \n"
      "JRNE   __loop            \n"
      "__exit:                  \n"
      "POPW   Y                 \n"
      "POPW   X                 \n"
   );
}

#endif

_END_EXTERN_C

