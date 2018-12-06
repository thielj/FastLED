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


#ifndef __INC_COLOR_STM8_H
#define __INC_COLOR_STM8_H

FASTLED_NAMESPACE_BEGIN

// hsv2rgb_rainbow - convert a hue, saturation, and value to RGB
//                   using a visually balanced rainbow (vs a straight
//                   mathematical spectrum).
//                   This 'rainbow' yields better yellow and orange
//                   than a straight 'spectrum'.
//
//                   NOTE: here hue is 0-255, not just 0-191
template< uint8_t YB, uint16_t GS> // defaults: 1, 256
void hsv2rgb_rainbow( const CHSV& hsv, CRGB& rgb);

#define FASTLED_HAVE_HSV2RGB_RAINBOW

FASTLED_NAMESPACE_END

#endif // __INC_COLOR_STM8_H
