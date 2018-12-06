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

#include <stdint.h>

#define _STM_INTERNAL
#include "led_sysdefs_stm8.h"
#include "lib_stm8.h"

#define FASTLED_INTERNAL
#include "FastLED.h"

FASTLED_NAMESPACE_BEGIN

//template< bool YB = false>
//inline CRGB hue2rgb_rainbow( uint8_t hue )
//{
//  CHSV tmp( hue, 255, 255);
//  CRGB rgb;
//  hsv2rgb_rainbow<YB, 256>( tmp, rgb);
//  return rgb;
//}

////////////////////////////////////////////////////////////////////////////////
NO_INLINE
FLATTEN
template< uint8_t YB = 1, uint16_t GS = 256>
void hsv2rgb_rainbow( const CHSV& hsv, CRGB& rgb)
{
  static_assert( GS <= 256, "Invalid hsv2rgb_rainbow parameters" );

  asm("PUSHW  X             \n");

  //const uint8_t offset8 = hsv.hue << 3; | hsv.hue >> 2;
  asm("LD     A, (0,X)      \n"
      "LD     s:VR+1, A     \n"   //hue
      "SLA    A             \n"   // << 1
      "SLA    A             \n"   // << 1 << 1
      "LD     s:VR+4, A     \n"   //tmp
      "SWAP   A             \n"
      "SLA    s:VR+4        \n"   // << 1 << 1 << 1
      "AND    A, $07        \n"   // last 3 bits
      "OR     A, s:VR+4     \n"
      "LD     s:VR+4, A     \n"); //offset8 (0..255)

//      "LD     A, (1,X)      \n"
//      "LD     s:VR+2, A     \n"   //sat
//      "LD     A, (2,X)      \n"
//      "LD     s:VR+3, A     \n"); //val
  asm("LDW    X, (1,X)      \n"
      "LDW    s:VR+2, X     \n"); // sat / val

  //const uint8_t third = scale8( offset8, (256 / 3)); // max = 85
  //const uint8_t twothirds = offset8 - third;         // max = 170
  asm("LDW    X, #85        \n"
      "MUL    X, A          \n"
      "ADDW   X, #$80       \n"   // round mathematically
      "EXG    A, XL         \n"
      "LD     s:VR+5, A     \n"); // third

  asm("LDW    X, Y          \n");

  //////////////////////////////////////////////////

  //const uint8_t hue = VR[1];
  if(   (VR[1] & 0x80) ) goto H1XX;
  if( ! (VR[1] & 0x40) ) goto H00X;
  if( ! (VR[1] & 0x20) ) goto H010;

  // 011  case 3: // G -> A
  //return CRGB( 0, 255 - third, third);
  asm("CLR   (0,X)          \n"   // Red
      "LD    A, s:VR+5      \n"
      "LD    (2,X), A       \n"   // Blue
      "CPL   A              \n"   // ~third=255-third
      "LD    (1,X), A       \n"); // Green
  goto CONT;

H010:
  // 010  case 2: // Y -> G
if( YB == 1 ) { //return CRGB( 170 + third - offset8, 170 + third, 0);
  asm("LD    A, #170        \n"
      "ADD   A, s:VR+5      \n"); // +third
} else {   //return CRGB( 255         - offset8, 255,         0);
  asm("LD    A, #255        \n");
}
  asm("LD    (1,X), A       \n"   // Green
      "SUB   A, s:VR+4      \n"   // Red = Green - offset8
      "LD    (0,X), A       \n"
      "CLR   (2,X)          \n"); // Blue = 0
  goto CONT;

H00X:

  asm("CLR   (2,X)          \n"); // Blue
  if( (VR[1] & 0x20) ) goto H001;

  // 000  case 0: // R -> O
  //return CRGB( 255 - third, third, 0);
  asm("LD    A, s:VR+5      \n"    // third
      "LD    (1,X), A       \n"    // Green
      "CPL   A              \n"    // ~third =255-third
      "LD    (0,X), A       \n");  // Red
  goto CONT;

H001:
    // 001  case 1: // O -> Y
  asm("LD    A, #170         \n");
if( YB == 1 ) { // return CRGB( 170        , 85 + third,           0);
  asm("LD    (0,X), A        \n"   // Red
      "LD    A, #85          \n"
      "ADD   A, s:VR+5       \n"   // +third
      "LD    (1,X), A        \n"); // Green
} else {   // return CRGB( 170 + third, 85 - third + offset8, 0);
  asm("ADD   A, VR+5         \n"
      "LD    (0,X), A        \n"
      "CPL   A               \n"   // ~(170+third)=255-170-third
      "ADD   A, s:VR+4       \n"   // +offset8
      "LD    (1,X), A        \n");
}
  goto CONT;

H1XX:
  if( ( VR[1] & 0x40) ) goto H11X;
  if( ( VR[1] & 0x20) ) goto H101;

  // 100  case 4: // A -> B
  // return CRGB( 0, 170 + third - offset8, 85 - third + offset8);
  asm("CLR   (0,X)          \n"
      "LD    A, #170        \n"
      "ADD   A, s:VR+5      \n"
      "SUB   A, s:VR+4      \n"
      "LD    (1,X), A       \n"
      "CPL   A              \n"   // ~(170+third-offset8)=255-170-third+offset8
      "LD    (2,X), A       \n");
  goto CONT;

H101:
  // 101  case 5: // B -> P
  // return CRGB( third, 0, 255 - third);
  asm("LD    A, s:VR+5      \n"   // third
      "LD    (0,X), A       \n"   // Red
      "CLR   (1,X)          \n"   // Green
      "CPL   A              \n"   // 255-third
      "LD    (3,X), A       \n"); // Blue
  goto CONT;

H11X:
  asm("CLR   (1,X)          \n"); // Green
  if( ( VR[1] & 0x20)  ) goto H111;

  // 110  case 6: // P -- K
  //return CRGB( 85 + third, 0, 170 - third);
  asm("LD    A, #85         \n"
      "ADD   A, s:VR+5      \n"
      "LD    (0,X), A       \n"   // Red
      "CPL   A              \n"   // ~(85+third)=255-85-third=170-third
      "LD    (2,X), A       \n"); // Blue
  goto CONT;

H111:
  // 111  case 7: // K -> R
  //return CRGB( 170 + third, 0, 85 - third);
  asm("LD    A, #170        \n"
      "ADD   A, s:VR+5      \n"
      "LD    (0,X), A       \n"   // Red
      "CPL   A              \n"   // ~(170+third)=255-170-third=85-third
      "LD    (2,X), A       \n");
  //goto CONT:

  //////////////////////////////////////////////////

CONT:

if( GS == 128 ) {
  asm("SRL   (1,X)          \n"); //rgb.g >>= 1;
}
else if( GS < 256 )
{
  //rgb.g = scale8_video( rgb.g, (uint8_t)GS);
  VR[1] = (uint8_t)GS;
  asm("LD    A, (1,X)       \n"   // rgb.g
      "LDW   X, s:VR+0      \n"
      "MUL   X, A           \n"
      "ADDW  #$FF           \n"   // round up
      "EXG   A, XH          \n"
      "LDW   X, Y           \n"   // restore X=&rgb
      "LD    (1,X), A       \n"); // rgb.g scaled
}

  // Scale down colors if we're desaturated at all
  // and add the brightness_floor to r, g, and b.

  asm("LD    A, s:VR+2      \n"   //sat
      "JREQ  __null         \n"   // if(sat==0) goto null
      "INC   A              \n"
      "JREQ  __cont         \n"   // if(sat==255) goto cont

      // const uint8_t brightness_floor = dim8_raw( 255 - hsv.sat);
      "NEG   A              \n"   // 256-(sat+1)
      "LD    XL, A          \n"   // duplicate arg
      "MUL   X, A           \n"
      "ADDW  X, #$80        \n"   // round up/down mathematically
      "LDW   s:VR+4, X      \n"   //  => VR4/5 for add+round

      //rgb.nscale8( hsv.sat).addRaw( brightness_floor );
      // scale+add, R
      "LD    A, (0,Y)       \n"
      "EXG   A, XL          \n"
      "LD    A, s:VR+2      \n"   // sat/scale is now in A
      "MUL   X, A           \n"
      "ADDW  X, s:VR+4      \n"   // add+round
      "RLWA  X,A            \n"   // sat/scale is now in XL
      "LD    (0,Y), A       \n"

      // scale+add, G
      "LD    A, (1,Y)       \n"
      "EXG   A, XL          \n"   // sat/scale is now in A
      "MUL   X, A           \n"
      "ADDW  X, s:VR+4      \n"   // add+round
      "RLWA  X,A            \n"   // sat/scale is now in XL
      "LD    (1,Y), A       \n"

      // scale+add, B
      "LD    A, (2,Y)       \n"
      "EXG   A, XL          \n"   // sat/scale is now in A
      "MUL   X, A           \n"
      "ADDW  X, s:VR+4      \n"   // add+round
      "RLWA  X,A            \n"   // sat/scale is now in XL
      "LD    (2,Y), A       \n"

      "JRA   __cont         \n"
      "__null:              \n"
      "LD    A, #$FF        \n"   // setRGB(#FFFFFF);
      "LD    (0,Y), A       \n"
      "LD    (1,Y), A       \n"
      "LD    (2,Y), A       \n"
      "__cont:              \n");

  // Now scale everything down if we're at value < 255.
  asm("LD    A, s:VR+3      \n"   //val
      "JREQ  __null         \n"   // if(val==0) goto null
      "CP    A, #$FF        \n"
      "JREQ  __cont         \n"   // if(val==255) goto cont

      // const uint8_t scale = dim8_raw( hsv.val);
      "LD    XL, A          \n"   // duplicate arg
      "MUL   X, A           \n"
      "ADDW  X, #$80        \n"   // round up/down mathematically
      "SWAPW X              \n"   // scale is now in XL

      //rgb.nscale8( scale)
      // scale R
      "LD    A, (0,Y)       \n"
      "EXG   A, XL          \n"   // scale is now in A
      "MUL   X, A           \n"
      "ADDW  X, #$80        \n"   // add+round
      "RLWA  X,A            \n"   // scale is now in XL
      "LD    (0,Y), A       \n"

      // scale+add, G
      "LD    A, (1,Y)       \n"
      "EXG   A, XL          \n"   // scale is now in A
      "MUL   X, A           \n"
      "ADDW  X, #$80        \n"   // add+round
      "RLWA  X,A            \n"   // scale is now in XL
      "LD    (1,Y), A       \n"

      // scale+add, B
      "LD    A, (2,Y)       \n"
      "EXG   A, XL          \n"   // scale is now in A
      "MUL   X, A           \n"
      "ADDW  X, #$80        \n"   // add+round
      "RLWA  X,A            \n"   // scale is now in XL
      "LD    (2,Y), A       \n"

      "JRA   __cont         \n"
      "__null:              \n"   // setRGB(#000000);
      "CLR   (0,Y)          \n"
      "CLR   (1,Y)          \n"
      "CLR   (2,Y)          \n"
      "__cont:              \n");

  asm("POPW  X              \n");
}


FASTLED_NAMESPACE_END
