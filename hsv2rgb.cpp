#define FASTLED_INTERNAL
#include <stdint.h>

#include "FastLED.h"

FASTLED_NAMESPACE_BEGIN

// Functions to convert HSV colors to RGB colors.
//
//  The basically fall into two groups: spectra, and rainbows.
//  Spectra and rainbows are not the same thing.  Wikipedia has a good
//  illustration here
//   http://upload.wikimedia.org/wikipedia/commons/f/f6/Prism_compare_rainbow_01.png
//  from this article
//   http://en.wikipedia.org/wiki/Rainbow#Number_of_colours_in_spectrum_or_rainbow
//  that shows a 'spectrum' and a 'rainbow' side by side.  Among other
//  differences, you'll see that a 'rainbow' has much more yellow than
//  a plain spectrum.  "Classic" LED color washes are spectrum based, and
//  usually show very little yellow.
//
//  Wikipedia's page on HSV color space, with pseudocode for conversion
//  to RGB color space
//   http://en.wikipedia.org/wiki/HSL_and_HSV
//  Note that their conversion algorithm, which is (naturally) very popular
//  is in the "maximum brightness at any given hue" style, vs the "uniform
//  brightness for all hues" style.
//
//  You can't have both; either purple is the same brightness as red, e.g
//    red = #FF0000 and purple = #800080 -> same "total light" output
//  OR purple is 'as bright as it can be', e.g.
//    red = #FF0000 and purple = #FF00FF -> purple is much brighter than red.
//  The colorspace conversions here try to keep the apparent brightness
//  constant even as the hue varies.
//
//  Adafruit's "Wheel" function, discussed here
//   http://forums.adafruit.com/viewtopic.php?f=47&t=22483
//  is also of the "constant apparent brightness" variety.
//
//  TODO: provide the 'maximum brightness no matter what' variation.
//
//  See also some good, clear Arduino C code from Kasper Kamperman
//   http://www.kasperkamperman.com/blog/arduino/arduino-programming-hsb-to-rgb/
//  which in turn was was based on Windows C code from "nico80"
//   http://www.codeproject.com/Articles/9207/An-HSB-RGBA-colour-picker





void hsv2rgb_raw_C (const struct CHSV & hsv, struct CRGB & rgb);
void hsv2rgb_raw_avr(const struct CHSV & hsv, struct CRGB & rgb);

#if defined(__AVR__) && !defined( LIB8_ATTINY )
void hsv2rgb_raw(const struct CHSV & hsv, struct CRGB & rgb)
{
    hsv2rgb_raw_avr( hsv, rgb);
}
#else
void hsv2rgb_raw(const struct CHSV & hsv, struct CRGB & rgb)
{
    hsv2rgb_raw_C( hsv, rgb);
}
#endif



#define APPLY_DIMMING(X) (X)
#define HSV_SECTION_6 (0x20)
#define HSV_SECTION_3 (0x40)

void hsv2rgb_raw_C (const struct CHSV & hsv, struct CRGB & rgb)
{
    // Convert hue, saturation and brightness ( HSV/HSB ) to RGB
    // "Dimming" is used on saturation and brightness to make
    // the output more visually linear.

    // Apply dimming curves
    uint8_t value = APPLY_DIMMING( hsv.val);
    uint8_t saturation = hsv.sat;

    // The brightness floor is minimum number that all of
    // R, G, and B will be set to.
    uint8_t invsat = APPLY_DIMMING( 255 - saturation);
    uint8_t brightness_floor = (value * invsat) / 256;

    // The color amplitude is the maximum amount of R, G, and B
    // that will be added on top of the brightness_floor to
    // create the specific hue desired.
    uint8_t color_amplitude = value - brightness_floor;

    // Figure out which section of the hue wheel we're in,
    // and how far offset we are withing that section
    uint8_t section = hsv.hue / HSV_SECTION_3; // 0..2
    uint8_t offset = hsv.hue % HSV_SECTION_3;  // 0..63

    uint8_t rampup = offset; // 0..63
    uint8_t rampdown = (HSV_SECTION_3 - 1) - offset; // 63..0

    // We now scale rampup and rampdown to a 0-255 range -- at least
    // in theory, but here's where architecture-specific decsions
    // come in to play:
    // To scale them up to 0-255, we'd want to multiply by 4.
    // But in the very next step, we multiply the ramps by other
    // values and then divide the resulting product by 256.
    // So which is faster?
    //   ((ramp * 4) * othervalue) / 256
    // or
    //   ((ramp    ) * othervalue) /  64
    // It depends on your processor architecture.
    // On 8-bit AVR, the "/ 256" is just a one-cycle register move,
    // but the "/ 64" might be a multicycle shift process. So on AVR
    // it's faster do multiply the ramp values by four, and then
    // divide by 256.
    // On ARM, the "/ 256" and "/ 64" are one cycle each, so it's
    // faster to NOT multiply the ramp values by four, and just to
    // divide the resulting product by 64 (instead of 256).
    // Moral of the story: trust your profiler, not your insticts.

    // Since there's an AVR assembly version elsewhere, we'll
    // assume what we're on an architecture where any number of
    // bit shifts has roughly the same cost, and we'll remove the
    // redundant math at the source level:

    //  // scale up to 255 range
    //  //rampup *= 4; // 0..252
    //  //rampdown *= 4; // 0..252

    // compute color-amplitude-scaled-down versions of rampup and rampdown
    uint8_t rampup_amp_adj   = (rampup   * color_amplitude) / (256 / 4);
    uint8_t rampdown_amp_adj = (rampdown * color_amplitude) / (256 / 4);

    // TODO: Can't we just calculate rampdown_amp_adj from rampup_amp_adj
    // by simply subtracting from 255?

    // add brightness_floor offset to everything
    uint8_t rampup_adj_with_floor   = rampup_amp_adj   + brightness_floor;
    uint8_t rampdown_adj_with_floor = rampdown_amp_adj + brightness_floor;


    if( section ) {
        if( section == 1) {
            // section 1: 0x40..0x7F
            rgb.r = brightness_floor;
            rgb.g = rampdown_adj_with_floor;
            rgb.b = rampup_adj_with_floor;
        } else {
            // section 2; 0x80..0xBF
            rgb.r = rampup_adj_with_floor;
            rgb.g = brightness_floor;
            rgb.b = rampdown_adj_with_floor;
        }
    } else {
        // section 0: 0x00..0x3F
        rgb.r = rampdown_adj_with_floor;
        rgb.g = rampup_adj_with_floor;
        rgb.b = brightness_floor;
    }
}



#if defined(__AVR__) && !defined( LIB8_ATTINY )
void hsv2rgb_raw_avr(const struct CHSV & hsv, struct CRGB & rgb)
{
    uint8_t hue, saturation, value;

    hue =        hsv.hue;
    saturation = hsv.sat;
    value =      hsv.val;

    // Saturation more useful the other way around
    saturation = 255 - saturation;
    uint8_t invsat = APPLY_DIMMING( saturation );

    // Apply dimming curves
    value = APPLY_DIMMING( value );

    // The brightness floor is minimum number that all of
    // R, G, and B will be set to, which is value * invsat
    uint8_t brightness_floor;

    asm volatile(
                 "mul %[value], %[invsat]            \n"
                 "mov %[brightness_floor], r1        \n"
                 : [brightness_floor] "=r" (brightness_floor)
                 : [value] "r" (value),
                 [invsat] "r" (invsat)
                 : "r0", "r1"
                 );

    // The color amplitude is the maximum amount of R, G, and B
    // that will be added on top of the brightness_floor to
    // create the specific hue desired.
    uint8_t color_amplitude = value - brightness_floor;

    // Figure how far we are offset into the section of the
    // color wheel that we're in
    uint8_t offset = hsv.hue & (HSV_SECTION_3 - 1);  // 0..63
    uint8_t rampup = offset * 4; // 0..252


    // compute color-amplitude-scaled-down versions of rampup and rampdown
    uint8_t rampup_amp_adj;
    uint8_t rampdown_amp_adj;

    asm volatile(
                 "mul %[rampup], %[color_amplitude]       \n"
                 "mov %[rampup_amp_adj], r1               \n"
                 "com %[rampup]                           \n"
                 "mul %[rampup], %[color_amplitude]       \n"
                 "mov %[rampdown_amp_adj], r1             \n"
                 : [rampup_amp_adj] "=&r" (rampup_amp_adj),
                 [rampdown_amp_adj] "=&r" (rampdown_amp_adj),
                 [rampup] "+r" (rampup)
                 : [color_amplitude] "r" (color_amplitude)
                 : "r0", "r1"
                 );


    // add brightness_floor offset to everything
    uint8_t rampup_adj_with_floor   = rampup_amp_adj   + brightness_floor;
    uint8_t rampdown_adj_with_floor = rampdown_amp_adj + brightness_floor;


    // keep gcc from using "X" as the index register for storing
    // results back in the return structure.  AVR's X register can't
    // do "std X+q, rnn", but the Y and Z registers can.
    // if the pointer to 'rgb' is in X, gcc will add all kinds of crazy
    // extra instructions.  Simply killing X here seems to help it
    // try Y or Z first.
    asm volatile(  ""  :  :  : "r26", "r27" );


    if( hue & 0x80 ) {
        // section 2: 0x80..0xBF
        rgb.r = rampup_adj_with_floor;
        rgb.g = brightness_floor;
        rgb.b = rampdown_adj_with_floor;
    } else {
        if( hue & 0x40) {
            // section 1: 0x40..0x7F
            rgb.r = brightness_floor;
            rgb.g = rampdown_adj_with_floor;
            rgb.b = rampup_adj_with_floor;
        } else {
            // section 0: 0x00..0x3F
            rgb.r = rampdown_adj_with_floor;
            rgb.g = rampup_adj_with_floor;
            rgb.b = brightness_floor;
        }
    }

    cleanup_R1();
}
// End of AVR asm implementation

#endif

void hsv2rgb_spectrum( const CHSV& hsv, CRGB& rgb)
{
    CHSV hsv2(hsv);
    hsv2.hue = scale8( hsv2.hue, 191);
    hsv2rgb_raw(hsv2, rgb);
}






// TODO: Move this to AVR platform code

#if defined(__AVR__)
// Left to its own devices, gcc turns "x <<= 3" into a loop
// It's much faster and smaller to just do three single-bit shifts
// So this business is to force that.
CONST
template <int BITS, typename UINT>
ALWAYS_INLINE
inline UINT shift_left(UINT a)
{
  UINT tmp = shift_left< (BITS-1) >(a);
//  asm volatile("");
  tmp <<= 1;
  return tmp;
}

CONST
template <>
ALWAYS_INLINE
inline uint8_t  shift_left<0, uint8_t >(uint8_t  a) { return a; }

CONST
template <>
ALWAYS_INLINE
inline uint16_t shift_left<0, uint16_t>(uint16_t a) { return a; }

CONST
template <>
ALWAYS_INLINE
inline uint32_t shift_left<0, uint32_t>(uint32_t a) { return a; }

#define FASTLED_HAVE_SHIFT_LEFT shift_left

CONST
template <int BITS, typename UINT>
ALWAYS_INLINE
inline UINT shift_right(UINT a)
{
  UINT tmp = shift_right< (BITS-1) >(a);
//  asm volatile("");
  tmp <<= 1;
  return tmp;
}

CONST
template <>
ALWAYS_INLINE
inline uint8_t  shift_right<0, uint8_t >(uint8_t  a) { return a; }

CONST
template <>
ALWAYS_INLINE
inline uint16_t shift_right<0, uint16_t>(uint16_t a) { return a; }

CONST
template <>
ALWAYS_INLINE
inline uint32_t shift_right<0, uint32_t>(uint32_t a) { return a; }

#define FASTLED_HAVE_SHIFT_RIGHT shift_right

#endif // defined(__AVR__)











#ifndef FASTLED_HAVE_HSV2RGB_RAINBOW

// See: https://user-images.githubusercontent.com/2461547/31694367-b9010528-b358-11e7-819a-b65a3ac14c2f.jpg
//
//  0       2       4       6       8       A       C       E       0
//  0       0       0       0       0       0       0       0       0
//  -----------------------------------------------------------------
//  R       O       Y       G       A       B       P       P       R
//  E       R       E       R       Q       L       U       I       E
//  D       A       L       E       U       U       R       N       D
//          N       L       E       A       E       P       K
//          G       O       N                       L
//          E       W                               E
//  |=======|=======|=======|=======|=======|=======|=======|=======|
//  R               |       G       |       B       |               R
//  |  R    |       |    G    G     |     B    B    |       |    R  |
//  |     R         | G     |    G  |    B       B  |          R    |
//  - - - - RRRRRRRRG - - - - - - - G - B - - - - - B - - - R - - - -
//  |             G  R              | GB            |  B  R         |
//  |          G    |  R            |B G            |  R  B         |
//  - - - - G - - - - - R - - - - - B - G - - - - - R - - - B - - - -
//  |    G          |    R       B  |    G       R  |          B    |
//  | G     |       |      R  B     |      G  R     |       |     B |
//  GBBBBBBBBBBBBBBBBBBBBBBBBRRRRRRRRRRRRRRRGGGGGGGGGGGGGGGGGGGGGGGGB
//  |=======|=======|=======|=======|=======|=======|=======|=======|

// Yellow has a higher inherent brightness than any other color; 'pure' yellow
// is perceived to be 93% as bright as white.  In order to make yellow appear
// the correct relative brightness, it has to be rendered brighter than all
// other colors.
//    YB=1 is a moderate boost, the default, as documented
//    YB=2 is a strong boost.
template< uint8_t YB = 1>
inline CRGB hue2rgb_rainbow( uint8_t hue )
{
    // The first 3 bit of hue determine the section
    // 000    001      010       011      100     101    110       111
    // Red -> Orange > Yellow -> Green -> Aqua -> Blue > Purple -> pinK -> Red
    //
    // The last 5 bits are used to interpolate between the two colors
    // For this, we re-map the values 0..31 to 0..255, i.e. looking at bits:
    //
    //   hue = ...abcde => offset8 = abcdeabc

    const uint8_t offset = hue & 0x1F; // 0..31
#if defined(HAVE_SHIFT_LEFT) && defined(HAVE_SHIFT_RIGHT)
    const uint8_t offset8 = shift_left<3>( offset ) | shift_right<2>( offset );
#else
    const uint8_t offset8 = (offset << 3) | (offset >> 2);
#endif

    const uint8_t third = scale8( offset8, (256 / 3));   // max = 85
    // twothirds             = offset8 - third;          // max = 170
    // 255 - third           = ~third;
    // 255 - offset8         = ~offset8;
    // 170 + third           = ~(85 - third);
    // 170 - third           = ~(85 + third);
    // 170 + third - offset8 = ~(85 - third + offset8);

    if( ! (hue & 0x80) ) {
        if( ! (hue & 0x40) ) {
            if( ! (hue & 0x20) ) // 000 / case 0 / Red (255, 0, 0) -> Orange (170, 85, 0)
                return CRGB( ~(unsigned)third, third, 0);
                                 // 001 / case 1 / Orange (170, 85, 0) -> Yellow (r, g, 0)
            if( YB == 1)
                return CRGB( 170, 85 + third, 0);
            else {
                const uint8_t t = 170 + third;
                return CRGB( t, ~(unsigned)t + offset8, 0);
            }
        }
        if( ! (hue & 0x20) ) {   // 010 / case 2 / Yellow ( r, g, 0) -> Green (0, 255, 0)
            if( YB == 1) {
                const uint8_t t = 170 + third;
                return CRGB( t - offset8, t, 0);
            } else
                return CRGB( ~(unsigned)offset8, 255, 0);
        }
                                 // 011 / case 3 / Green (0, 255, 0) -> Aqua (0, 170, 85)
        return CRGB( 0, ~(unsigned)third, third);
    }
    if( ! (hue & 0x40) ) {
        if( ! ( hue & 0x20) ) {  // 100 / case 4 / Aqua (0, 170, 85) -> Blue (0, 0, 255)
            const uint8_t t = 170 + third - offset8;
            return CRGB( 0, t, ~(unsigned)t);
        } else                   // 101 / case 5 / Blue (0, 0, 255) -> Purple (85, 0, 170)
            return CRGB( third, 0, ~(unsigned)third);
    }
    if( ! (hue & 0x20) ) {       // 110 / case 6 / Purple (85, 0, 170) -> Pink (170, 0, 85)
        const uint8_t t = 85 + third;
        return CRGB( t, 0, ~(unsigned)t);
    } else {                     // 111 / case 7 / Pink (170, 0, 85) -> Red (255, 0, 0)
        const uint8_t t = 170 + third;
        return CRGB( t, 0, ~(unsigned)t);
    }
}





// Yellow has a higher inherent brightness than any other color; 'pure' yellow
// is perceived to be 93% as bright as white.  In order to make yellow appear
// the correct relative brightness, it has to be rendered brighter than all
// other colors.
//    YB=1 is a moderate boost, the default.
//    YB=2 is a strong boost.
// To scale green down, provide a scale (in GS/256th).
//    GS=0..255, 128 is optimized for speed; default is GS=256, i.e. no scaling
template< uint8_t YB, uint16_t GS>
inline CRGB hsv2rgb_rainbow( const CHSV& hsv)
{
    static_assert( GS <= 256, "Invalid hsv2rgb_rainbow parameters" );

    CRGB rgb = hue2rgb_rainbow< YB >( hsv.hue);

    // This is one of the good places to scale the green down,
    // although the client can scale green down as well.
    if( GS == 128 ) rgb.g >>= 1;
    else if( GS < 256 ) rgb.g = scale8_video( rgb.g, (uint8_t)GS);

    // Scale down colors if we're desaturated at all
    // and add the brightness_floor to r, g, and b.

    if( hsv.sat == 0) {
      rgb = CRGB::White;
    } else if( hsv.sat != 255 ) {
      const uint8_t brightness_floor = dim8_raw( ~(unsigned)hsv.sat);
      rgb.nscale8( hsv.sat).addRaw( brightness_floor );
    }

    // Now scale everything down if we're at value < 255.

    if( hsv.val == 0 ) {
        rgb.zero(); //rgb.setRGB( 0, 0, 0);
    } else if( hsv.val != 255 ) {
        rgb.nscale8( dim8_raw( hsv.val) );
    }

    return rgb;
}

template< uint8_t YB = 1, uint16_t GS = 256>
void hsv2rgb_rainbow( const struct CHSV& hsv, struct CRGB& rgb)
{
  rgb = hsv2rgb_rainbow<YB,GS>( hsv);
}

#endif // FASTLED_HAVE_HSV2RGB_RAINBOW

void hsv2rgb_raw(const struct CHSV * phsv, struct CRGB * prgb, int numLeds) {
    for(int i = 0; i < numLeds; i++) {
        hsv2rgb_raw(phsv[i], prgb[i]);
    }
}

void hsv2rgb_rainbow( const struct CHSV* phsv, struct CRGB * prgb, int numLeds) {
    for(int i = 0; i < numLeds; i++) {
        hsv2rgb_rainbow(phsv[i], prgb[i]);
    }
}

void hsv2rgb_spectrum( const struct CHSV* phsv, struct CRGB * prgb, int numLeds) {
    for(int i = 0; i < numLeds; i++) {
        hsv2rgb_spectrum(phsv[i], prgb[i]);
    }
}



#define FIXFRAC8(N,D) (((N)*256)/(D))

// This function is only an approximation, and it is not
// nearly as fast as the normal HSV-to-RGB conversion.
// See extended notes in the .h file.
CHSV rgb2hsv_approximate( const CRGB& rgb)
{
    uint8_t r = rgb.r;
    uint8_t g = rgb.g;
    uint8_t b = rgb.b;
    uint8_t h, s, v;
    
    // find desaturation
    uint8_t desat = 255;
    if( r < desat) desat = r;
    if( g < desat) desat = g;
    if( b < desat) desat = b;
    
    // remove saturation from all channels
    r -= desat;
    g -= desat;
    b -= desat;
    
    //Serial.print("desat="); Serial.print(desat); Serial.println("");
    
    //uint8_t orig_desat = sqrt16( desat * 256);
    //Serial.print("orig_desat="); Serial.print(orig_desat); Serial.println("");
    
    // saturation is opposite of desaturation
    s = 255 - desat;
    //Serial.print("s.1="); Serial.print(s); Serial.println("");
    
    if( s != 255 ) {
        // undo 'dimming' of saturation
        s = 255 - sqrt16( (255-s) * 256);
    }
    // without lib8tion: float ... ew ... sqrt... double ew, or rather, ew ^ 0.5
    // if( s != 255 ) s = (255 - (256.0 * sqrt( (float)(255-s) / 256.0)));
    //Serial.print("s.2="); Serial.print(s); Serial.println("");
    
    
    // at least one channel is now zero
    // if all three channels are zero, we had a
    // shade of gray.
    if( (r + g + b) == 0) {
        // we pick hue zero for no special reason
        return CHSV( 0, 0, 255 - s);
    }
    
    // scale all channels up to compensate for desaturation
    if( s < 255) {
        if( s == 0) s = 1;
        uint32_t scaleup = 65535 / (s);
        r = ((uint32_t)(r) * scaleup) / 256;
        g = ((uint32_t)(g) * scaleup) / 256;
        b = ((uint32_t)(b) * scaleup) / 256;
    }
    //Serial.print("r.2="); Serial.print(r); Serial.println("");
    //Serial.print("g.2="); Serial.print(g); Serial.println("");
    //Serial.print("b.2="); Serial.print(b); Serial.println("");
    
    uint16_t total = r + g + b;
    
    //Serial.print("total="); Serial.print(total); Serial.println("");
    
    // scale all channels up to compensate for low values
    if( total < 255) {
        if( total == 0) total = 1;
        uint32_t scaleup = 65535 / (total);
        r = ((uint32_t)(r) * scaleup) / 256;
        g = ((uint32_t)(g) * scaleup) / 256;
        b = ((uint32_t)(b) * scaleup) / 256;
    }
    //Serial.print("r.3="); Serial.print(r); Serial.println("");
    //Serial.print("g.3="); Serial.print(g); Serial.println("");
    //Serial.print("b.3="); Serial.print(b); Serial.println("");
    
    if( total > 255 ) {
        v = 255;
    } else {
        v = qadd8(desat,total);
        // undo 'dimming' of brightness
        if( v != 255) v = sqrt16( v * 256);
        // without lib8tion: float ... ew ... sqrt... double ew, or rather, ew ^ 0.5
        // if( v != 255) v = (256.0 * sqrt( (float)(v) / 256.0));
        
    }
    
    //Serial.print("v="); Serial.print(v); Serial.println("");
    
    
#if 0
    
    //#else
    if( v != 255) {
        // this part could probably use refinement/rethinking,
        // (but it doesn't overflow & wrap anymore)
        uint16_t s16;
        s16 = (s * 256);
        s16 /= v;
        //Serial.print("s16="); Serial.print(s16); Serial.println("");
        if( s16 < 256) {
            s = s16;
        } else {
            s = 255; // clamp to prevent overflow
        }
    }
#endif
    
    //Serial.print("s.3="); Serial.print(s); Serial.println("");
    
    
    // since this wasn't a pure shade of gray,
    // the interesting question is what hue is it
    
    
    
    // start with which channel is highest
    // (ties don't matter)
    uint8_t highest = r;
    if( g > highest) highest = g;
    if( b > highest) highest = b;
    
    if( highest == r ) {
        // Red is highest.
        // Hue could be Purple/Pink-Red,Red-Orange,Orange-Yellow
        if( g == 0 ) {
            // if green is zero, we're in Purple/Pink-Red
            h = (HUE_PURPLE + HUE_PINK) / 2;
            h += scale8( qsub8(r, 128), FIXFRAC8(48,128));
        } else if ( (r - g) > g) {
            // if R-G > G then we're in Red-Orange
            h = HUE_RED;
            h += scale8( g, FIXFRAC8(32,85));
        } else {
            // R-G < G, we're in Orange-Yellow
            h = HUE_ORANGE;
            h += scale8( qsub8((g - 85) + (171 - r), 4), FIXFRAC8(32,85)); //221
        }
        
    } else if ( highest == g) {
        // Green is highest
        // Hue could be Yellow-Green, Green-Aqua
        if( b == 0) {
            // if Blue is zero, we're in Yellow-Green
            //   G = 171..255
            //   R = 171..  0
            h = HUE_YELLOW;
            uint8_t radj = scale8( qsub8(171,r),   47); //171..0 -> 0..171 -> 0..31
            uint8_t gadj = scale8( qsub8(g,171),   96); //171..255 -> 0..84 -> 0..31;
            uint8_t rgadj = radj + gadj;
            uint8_t hueadv = rgadj / 2;
            h += hueadv;
            //h += scale8( qadd8( 4, qadd8((g - 128), (128 - r))),
            //             FIXFRAC8(32,255)); //
        } else {
            // if Blue is nonzero we're in Green-Aqua
            if( (g-b) > b) {
                h = HUE_GREEN;
                h += scale8( b, FIXFRAC8(32,85));
            } else {
                h = HUE_AQUA;
                h += scale8( qsub8(b, 85), FIXFRAC8(8,42));
            }
        }
        
    } else /* highest == b */ {
        // Blue is highest
        // Hue could be Aqua/Blue-Blue, Blue-Purple, Purple-Pink
        if( r == 0) {
            // if red is zero, we're in Aqua/Blue-Blue
            h = HUE_AQUA + ((HUE_BLUE - HUE_AQUA) / 4);
            h += scale8( qsub8(b, 128), FIXFRAC8(24,128));
        } else if ( (b-r) > r) {
            // B-R > R, we're in Blue-Purple
            h = HUE_BLUE;
            h += scale8( r, FIXFRAC8(32,85));
        } else {
            // B-R < R, we're in Purple-Pink
            h = HUE_PURPLE;
            h += scale8( qsub8(r, 85), FIXFRAC8(32,85));
        }
    }
    
    h += 1;
    return CHSV( h, s, v);
}

// Examples that need work:
//   0,192,192
//   192,64,64
//   224,32,32
//   252,0,126
//   252,252,0
//   252,252,126

FASTLED_NAMESPACE_END
