/*******************************************************************************

Copyright (c) 2017-present J Thiel
Copyright (c) 2013-present FastLED

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
#ifndef __INC_FASTPIN_STM8_H
#define __INC_FASTPIN_STM8_H

#if defined(FASTLED_FORCE_SOFTWARE_PINS)
#warning "Software pin support forced, pin access will be slightly slower."
#define NO_HARDWARE_PIN_SUPPORT
#undef HAS_HARDWARE_PIN_SUPPORT
#else

#define HAS_HARDWARE_PIN_SUPPORT

FASTLED_NAMESPACE_BEGIN

// OUTPUT PIN: cr1=1 for push-pull (0=open drain), cr2=1 for fast up to 10MHz (0=slow up to 2MHz)
enum STM8OutputMode {
  OpenDrain = 0,
  PushPull  = 1,
  FastMode  = 2,
  OpenDrain_Fast = OpenDrain | FastMode,
  PushPull_Fast  = PushPull  | FastMode
};

// INPUT PIN: cr=1 for pull-up; cr2=1 to enable external interrupt
enum STM8InputMode {
  Floating           = 0,
  PullUp             = 1,
  ExternalInterrupt  = 2,
  Floating_Interrupt = Floating | ExternalInterrupt,
  PullUp_Interrupt   = PullUp   | ExternalInterrupt
};

/// Class definition for a Pin where we know the port registers at compile time for said pin.  This allows us to make
/// a lot of optimizations, as the inlined hi/lo methods will devolve to a single io register write/bitset.
template<uint8_t PIN, uint8_t _BIT, uint32_t _MASK, typename _GPIO> struct _STM8_PIN
{
  typedef volatile uint8_t * port_ptr_t;
  typedef uint8_t port_t;

  ALWAYS_INLINE
  inline static void setOutput( STM8OutputMode mode = PushPull_Fast )
  {
    // make sure external interrupts are off
    bres( _GPIO::r()->CR2, _BIT); //_GPIO::r()->CR2 &= ~_MASK;
    bset( _GPIO::r()->DDR, _BIT); //_GPIO::r()->DDR |=  _MASK;
    if( mode & PushPull )
    {
      bset( _GPIO::r()->CR1, _BIT); //_GPIO::r()->CR1 |=  _MASK;
    } else {
      bres( _GPIO::r()->CR1, _BIT); //_GPIO::r()->CR1 &= ~_MASK;
    }
    if( mode & FastMode ) // enable fast mode if requested
    {
      bset( _GPIO::r()->CR2, _BIT); //_GPIO::r()->CR2 |=  _MASK;
    }
  }
  ALWAYS_INLINE
  inline static void setInput( STM8InputMode mode = Floating )
  {
    _GPIO::r()->CR2 &= ~_MASK; // make sure external interrupts are off
    if( mode & PullUp )            { _GPIO::r()->CR1 |=  _MASK; } else { _GPIO::r()->CR1 &= ~_MASK; };
    _GPIO::r()->DDR &= ~_MASK; // switch to input
    if( mode & ExternalInterrupt ) { _GPIO::r()->CR2 |=  _MASK; }; // enable EXT IRQs if requested
  }

  ALWAYS_INLINE
  inline static void hi() { _GPIO::r()->ODR |=  _MASK; }
  ALWAYS_INLINE
  inline static void lo() { _GPIO::r()->ODR &= ~_MASK; }
  ALWAYS_INLINE
  inline static void set(register port_t val) { _GPIO::r()->ODR = val; }
  ALWAYS_INLINE
  inline static void strobe() { toggle(); toggle(); }
  ALWAYS_INLINE
  inline static void toggle()
  {
#ifdef NDEBUG // the intrinsic doesnt work when not inlining the call
    bcpl(_GPIO::r()->ODR, _BIT);
#else
    _GPIO::r()->ODR ^= _MASK;
#endif
  }

  ALWAYS_INLINE
  inline static void hi(register port_ptr_t port) { hi(); }
  ALWAYS_INLINE
  inline static void lo(register port_ptr_t port) { lo(); }
  ALWAYS_INLINE
  inline static void fastset(register port_ptr_t port, register port_t val) { *port = val; }

  ALWAYS_INLINE
  inline static port_t hival() { return _GPIO::r()->ODR | _MASK; }
  ALWAYS_INLINE
  inline static port_t loval() { return _GPIO::r()->ODR & ~_MASK; }
  ALWAYS_INLINE
  inline static port_ptr_t port() { return &_GPIO::r()->ODR; }
  ALWAYS_INLINE
  inline static constexpr port_t mask() { return _MASK; }
  ALWAYS_INLINE
  inline static constexpr uint8_t pin() { return PIN; }
};


typedef struct
{
  volatile uint8_t ODR; /*!< Output Data Register */
  volatile uint8_t IDR; /*!< Input Data Register */
  volatile uint8_t DDR; /*!< Data Direction Register */
  volatile uint8_t CR1; /*!< Configuration Register 1 */
  volatile uint8_t CR2; /*!< Configuration Register 2 */
}
_STM8_GPIO;

#if defined(__IAR_SYSTEMS_ICC__)
//__near __no_init volatile unsigned char PA_ODR @ 0x5000;
// it's important that they are referenced in C++ before we can use them with the inline assembler
#define _STM8_PA(L,A) &P ## L ## _ODR
#elif defined(GPIOA_BaseAddress)
#define _STM8_PA(L,A) GPIO ## L ## _BaseAddress
#else
#define _STM8_PA(L,A) A
#endif

#define _STM8_BLOCK(L,A) struct _stm8_gpio_ ## L { \
  ALWAYS_INLINE \
  static inline volatile _STM8_GPIO * r() { return (_STM8_GPIO *) _STM8_PA(L,A) ; } \
}

// bccm() is used by the clockless controllers
#define _STM8_DEF_PIN(L, BIT) template<> struct FastPin<P ## L ## BIT> \
  : public _STM8_PIN<P ## L ## BIT, BIT, 1 << BIT, _stm8_gpio_ ## L > { \
  ALWAYS_INLINE \
  static inline void bccm() { __asm( "BCCM P" #L "_ODR, #" #BIT " \n" ) ; } \
}

// blocks A..I
// with IAR, we can detect presence of PA_ODR by #if defined(PA_ODR_ODR0)
// in the system headers, GPIOA is #define'd if present, and GPIOA_BaseAddress uncondtionally

#if defined(PA_ODR_ODR0) || defined(GPIOA)
_STM8_BLOCK(A,0x5000);
#endif
#if defined(PB_ODR_ODR0) || defined(GPIOB)
_STM8_BLOCK(B,0x5005);
#endif
#if defined(PC_ODR_ODR0) || defined(GPIOC)
_STM8_BLOCK(C,0x500A);
#endif
#if defined(PD_ODR_ODR0) || defined(GPIOD)
_STM8_BLOCK(D,0x500F);
#endif
#if defined(PE_ODR_ODR0) || defined(GPIOE)
_STM8_BLOCK(E,0x5014);
#endif
#if defined(PF_ODR_ODR0) || defined(GPIOF)
_STM8_BLOCK(F,0x5019);
#endif
#if defined(PG_ODR_ODR0) || defined(GPIOG)
_STM8_BLOCK(G,0x501E);
#endif
#if defined(PH_ODR_ODR0) || defined(GPIOH)
_STM8_BLOCK(H,0x5023);
#endif
#if defined(PI_ODR_ODR0) || defined(GPIOI)
_STM8_BLOCK(I,0x5028);
#endif

// #if (MCU==STM8S103F3) // Generic STM8S103F3

////////////////////////////////////////////////////////////////////////////////
// STM8S103F3
//
// PWM      (D 13) PD4  1|    |20  PD3 (D 12, AI 2) PWM
// TX (D 14, AI 3) PD5  2|    |19  PD2 (D 11, AI 1)
// RX (D 15, AI 4) PD6  3|    |18  PD1 (D 10)
//                NRST  4|    |17  PC7 (D 9)   MISO/PWM+
//           (D 0) PA1  5|    |16  PC6 (D 8)   MOSI/PWM+
//           (D 1) PA2  6|    |15  PC5 (D 7)    SCK
//                 GND  7|    |14  PC4 (D 6, AI 0)  PWM
//                Vcap  8|    |13  PC3 (D 5)        PWM
//                 Vdd  9|    |12  PB4 (D 4)        SCL
// PWM       (D 2) PA3 10|    |11  PB5 (D 3)   LED, SDA
//                       +----+
//
// (PWM+ indicates the additional PWM pins on alternate function pins)

enum STM8PortPin {
	PA1, /* 0 */
	PA2,
	PA3,
	PB5, /* 3 */
	PB4,
	PC3, /* 5 */
	PC4,
	PC5,
	PC6,
	PC7,
	PD1, /* 10 */
	PD2,
	PD3,
	PD4,
	PD5,
	PD6  /* 15 */
};

_STM8_DEF_PIN(A,1); _STM8_DEF_PIN(A,2); _STM8_DEF_PIN(A,3);
_STM8_DEF_PIN(B,5); _STM8_DEF_PIN(B,4);
_STM8_DEF_PIN(C,3); _STM8_DEF_PIN(C,4); _STM8_DEF_PIN(C,5);
_STM8_DEF_PIN(C,6); _STM8_DEF_PIN(C,7);
_STM8_DEF_PIN(D,1); _STM8_DEF_PIN(D,2); _STM8_DEF_PIN(D,3);
_STM8_DEF_PIN(D,4); _STM8_DEF_PIN(D,5); _STM8_DEF_PIN(D,6);

#define NUM_DIGITAL_PINS            16
#define MAX_PIN                     15

#define PIN_SPI_SS          (PA3)	// 2
#define PIN_SPI_MOSI        (PC6)	// 8
#define PIN_SPI_MISO        (PC7)	// 9
#define PIN_SPI_SCK         (PC5)	// 7
#define PIN_WIRE_SDA        (PB5)	// 3
#define PIN_WIRE_SCL        (PB4)	// 4
#define PIN_LED_BUILTIN     (PB5)	// 3
#define PIN_TX	            (PD5)	// 14
#define PIN_RX	            (PD6)	// 15

#define SPI_DATA            PIN_SPI_MOSI
#define SPI_CLOCK           PIN_SPI_SCK

//#endif // __STM8S103F3__

FASTLED_NAMESPACE_END

#endif // defined(FASTLED_FORCE_SOFTWARE_PINS)

#endif // __INC_FASTPIN_ARM_STM32
