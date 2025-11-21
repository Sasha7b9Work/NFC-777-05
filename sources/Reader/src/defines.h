// (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include <cstring>

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_BUILD 1595

#define DATE_BUILD "2025-11-21 22:58:45"
 

#define VERSION_SCPI  "1.0"


#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
    #pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
    #pragma clang diagnostic ignored "-Wwritable-strings"
    #pragma clang diagnostic ignored "-Wcast-align"
    #pragma clang diagnostic ignored "-Wcast-qual"
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    #pragma clang diagnostic ignored "-Wold-style-cast"
    #pragma clang diagnostic ignored "-Wpadded"
    #pragma clang diagnostic ignored "-Wglobal-constructors"
    #pragma clang diagnostic ignored "-Winvalid-utf8"
    #pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    #pragma clang diagnostic ignored "-Wmissing-prototypes"
    #pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#else
    #ifdef WIN32
        #define __disable_irq()
        #define __enable_irq()
        #define __DSB()
        #define __ISB()
        #define __attribute__(x)
        #define WIN32_LEAN_AND_MEAN
        #ifndef GUI
            #define asm()
        #endif
    #endif
#endif  

#include "Types.h"

#ifdef WIN32
#define __asm(x)
#else
//    #pragma anon_unions
#endif


#define _GET_BIT(value, bit)   ((value) & (1 << (bit)))
#define _SET_BIT(value, bit)   ((value) |= (1 << (bit)))
#define _CLEAR_BIT(value, bit) ((value) &= (uint)(~(1 << (bit))))


#define ERROR_VALUE_FLOAT   1.111e29f


#define _bitset(bits)                               \
  ((uint8)(                                         \
  (((uint8)((uint)bits / 01)        % 010) << 0) |  \
  (((uint8)((uint)bits / 010)       % 010) << 1) |  \
  (((uint8)((uint)bits / 0100)      % 010) << 2) |  \
  (((uint8)((uint)bits / 01000)     % 010) << 3) |  \
  (((uint8)((uint)bits / 010000)    % 010) << 4) |  \
  (((uint8)((uint)bits / 0100000)   % 010) << 5) |  \
  (((uint8)((uint)bits / 01000000)  % 010) << 6) |  \
  (((uint8)((uint)bits / 010000000) % 010) << 7)))

#define BINARY_U8( bits ) ((uint8)_bitset(0##bits))


#include "opt.h"
//#include "Utils/Log.h"


#ifdef GUI
#include "defines_GUI.h"
#endif


//#include "Utils/Debug.h"
