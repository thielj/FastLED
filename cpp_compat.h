#ifndef __INC_CPP_COMPAT_H
#define __INC_CPP_COMPAT_H

#include "FastLED.h"

#if __cplusplus <= 199711L
#define STATIC_ASSERT(expression, message)
#define constexpr /*const*/
#else
#define STATIC_ASSERT(expression, message) static_assert(expression, message)
#endif // __cplusplus <= 199711L


#if defined(__GNUC__)
#  define ALWAYS_INLINE  __attribute__((always_inline))
#  define HOT            __attribute__((hot))
#  define NO_INLINE      __attribute__((noinline))
// do not initialize global or static variable
// this depends on compiler support or a NOLOAD section in the linker script
// e.g. __attribute__ ((section (".noinit")))
#  define NO_INIT
#  define PREVENT_OPTIMIZATION()  asm volatile("")
#  define BEGIN_CRITICAL  __critical {
#  define END_CRITICAL    }
  // Sometimes the compiler will do clever things to reduce
  // code size that result in a net slowdown, if it thinks that
  // a variable is not used in a certain location.
  // This macro does its best to convince the compiler that
  // the variable is used in this location, to help control
  // code motion and de-duplication that would result in a slowdown.
#  define FORCE_REFERENCE(var)  asm volatile( "" : : "r" (var) )
  // #define FORCE_REFERENCE(x) { void* volatile dummy = &x; }
#elif defined(__ICCSTM8__) // IAR STM8
  // see platforms/stm8/cc.h
#endif

#ifndef COUNT_OF
#define COUNT_OF(array) ((sizeof(array)/sizeof(0[array])) / ((size_t)(!(sizeof(array) % sizeof(0[array])))))
#endif



#endif // __INC_CPP_COMPAT_H
