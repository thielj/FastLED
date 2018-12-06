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

#ifndef __INC_LIB_STM8_H
#define __INC_LIB_STM8_H

////////////////////////////////////////////////////////////////////////////////
//
// scale8 - a summary of different implementations found in FastLED
//
//
// the differences are in rounding only, i.e. all implementations can be
// expressed as:
//

//
// Depending on the value of add16, there are currently 5 different variants:
//
//  (0) add16 = 0x00, aka ROUND DOWN or !FASTLED_SCALE8_FIXED
//
//      * for inputs 1..255 and scale > 0, we get an output range of 0..254
//
//
//  (1) add16 = 0x80, aka ROUND
//
//      * this is the mathematically correct way to minimize rounding errors
//
//      * for inputs 1..255 and scale > 0, we get an output range of 0..254
//
//      * as a reminder, 255.0 * 255.0 / 256.0 == 254.00390625 !
//
//
//  (2) add16 = 0xFF, aka ROUND UP or SHOULD_BE_VIDEO
//
//      * for inputs 1..255 and scale > 0, we get an output range of 1..255
//
//      * a scale of 255 leaves the input unchanged
//
//
//  (3) add16 = i, aka FASTLED_SCALE8_FIXED
//
//       * this would be equivalent to SCALE8( i, 1+scale, 0), but is usually
//       implemented as ( i*j + i ) to avoid the possible uint8_t overflow.
//
//       * for inputs 1..255 and scale > 0, we get an output range of 0..255
//
//       * a scale of 0 will result in 0 for all inputs
//
//       * a scale of 255 leaves the input unchanged
//
//       * gives a 'smoother' appearance than the more correct 'ROUND' version??
//
//
//  (4) round = 0x00, add +1 for non-zero inputs; aka BUGGED_VIDEO
//
//      * in FastLED, this sometimes appears as:
//
//         ( (uint16_t)(i*j) >> 8 ) + ( (i&&j) ? 1 : 0 );
//
//      * Similar to SHOULD_BE_VIDEO for most inputs, except that whenever
//        the original result is already an integer, e.g. (4, 128):
//
//          4 * 128/256 = 2; no rounding necessary! This algorithm returns 3.
//
//      * for inputs 1..255 and scale > 0, we get an output range of 1..255
//
//      * a scale of 255 leaves the input unchanged
//
//
// The round parameter isn't limited to unisgned 8 bit, but can be used to
// implement a tight multiply, round and add operation.
//
////////////////////////////////////////////////////////////////////////////////

#ifdef _STM8_PREFER_C_INLINE

ALWAYS_INLINE
inline uint8_t _STM8_F(scale8)( uint8_t i, fract8 scale)
{
  return SCALE_ADD8( i, scale, i);
}
ALWAYS_INLINE
static inline uint8_t _STM8_F(scale8_video)( uint8_t i, fract8 scale)
{
  return SCALE_ADD8( i, scale, 0xFF);
}
ALWAYS_INLINE
static inline void _STM8_F(nscale8_video)( uint8_t & i, fract8 scale)
{
  i = SCALE_ADD8( i, scale, 0xFF);
}

#endif // _STM8_PREFER_C_INLINE

_EXTERN_C

#ifndef _STM8_PREFER_C_INLINE

extern uint8_t _STM8_F(scale8_orig)( uint8_t i, fract8 scale);
// i is actually uint8_t, except that we prefer to get it in the X register
extern uint8_t _STM8_F(scale8_alt)( uint16_t i, fract8 scale);
extern uint8_t _STM8_F(scale8_video_orig)( uint8_t i, fract8 scale);
// i is actually uint8_t, except that we prefer to get it in the X register
extern uint8_t _STM8_F(scale8_video_alt)( uint16_t i, fract8 scale);

// as above, but modifies the argument in place
extern void _STM8_F(nscale8)( uint8_t& i, fract8 scale);
// as above, but modifies the argument in place
extern void _STM8_F(nscale8_video)( uint8_t & i, fract8 scale);

#endif // _STM8_PREFER_C_INLINE

// scale n consecutive bytes in place
extern void _STM8_F(nscale8x)( uint8_t * data, int16_t len, fract8 scale);
// scale 3 consecutive bytes in place
extern void _STM8_F(nscale8x3)( uint8_t * data, fract8 scale);
// scale n consecutive bytes in place
extern void _STM8_F(nscale8x_video)( uint8_t * data, int16_t len, fract8 scale);
// scale 3 consecutive bytes in place
extern void _STM8_F(nscale8x3_video)( uint8_t * data, fract8 scale);

//extern void _stm8_fill( uint8_t * data, size_t len, uint8_t value);

_END_EXTERN_C

#endif // __INC_LIB_STM8_H