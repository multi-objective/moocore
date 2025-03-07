#include <stdint.h>
#include "common.h"

// Fast power function for exp between 0 and 32
_attr_const_func static inline double
fast_pow_uint_max32(double base, uint_fast8_t exp)
{
    ASSUME(exp <= 32);
    double b2, b3, b4, b5, b6, b8, b9, b10, b12, b13, b14, b15, b16;

    switch (exp) {
      case 0: return 1;
      case 1: return base;
      case 2: return base * base;
      case 3: return base * base * base;
      case 4: { b2 = base * base; return b2 * b2; }
      case 5: { b2 = base * base; return b2 * b2 * base; }
      case 6: { b2 = base * base; return b2 * b2 * b2; }
      case 7: { b2 = base * base; return b2 * b2 * b2 * base; }
      case 8: { b2 = base * base; b4 = b2 * b2; return b4 * b4; }
      case 9: { b3 = base * base * base; return b3 * b3 * b3; }
      case 10: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b2; }
      case 11: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b2 * base; }
      case 12: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b4; }
      case 13: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b4 * base; }
      case 14: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b4 * b2; }
      case 15: { b2 = base * base; b5 = b2 * b2 * base; return b5 * b5 * b5; }
      case 16: { b2 = base * base; b4 = b2 * b2; b8 = b4 * b4; return b8 * b8; }
      case 17: { b16 = fast_pow_uint_max32(base, 16); return b16 * base; }
      case 18: { b9  = fast_pow_uint_max32(base, 9); return b9 * b9; }
      case 19: { b9  = fast_pow_uint_max32(base, 9); return b9 * b9 * base; }
      case 20: { b10 = fast_pow_uint_max32(base, 10); return b10 * b10; }
      case 21: { b10 = fast_pow_uint_max32(base, 10); return b10 * b10 * base; }
      case 22: { b2 = base * base; b5 = b2 * b2 * base; b10 = b5 * b5; return b10 * b10 * b2; }
      case 23: { b2 = base * base; b3 = b2 * base; b5 = b2 * b3; b10 = b5 * b5; return b10 * b10 * b3; }
      case 24: { b12 = fast_pow_uint_max32(base, 12); return b12 * b12; }
      case 25: { b12 = fast_pow_uint_max32(base, 12); return b12 * b12 * base; }
      case 26: { b13 = fast_pow_uint_max32(base, 13); return b13 * b13; }
      case 27: { b3 = base * base * base; b6 = b3 * b3; b12 = b6 * b6; return b12 * b12 * b3; }
      case 28: { b14 = fast_pow_uint_max32(base, 14); return b14 * b14; }
      case 29: { b14 = fast_pow_uint_max32(base, 14); return b14 * b14 * base; }
      case 30: { b15 = fast_pow_uint_max32(base, 15); return b15 * b15; }
      case 31: { b15 = fast_pow_uint_max32(base, 15); return b15 * b15 * base; }
      case 32: { b16 = fast_pow_uint_max32(base, 16); return b16 * b16; }
      default: unreachable();
    }
}

_attr_const_func static inline double
pow_uint(double base, unsigned int exp)
{
    if (exp <= 32)
        return fast_pow_uint_max32(base, (uint_fast8_t) exp); // Use lookup table for small values

    double result = (exp & 1) ? base : 1;
    exp >>= 1;
    while (exp) {
        base *= base;
        if (exp & 1)
            result *= base;
        exp >>= 1;
    }
    return result;
}

_attr_const_func static inline long double
fast_powl_uint_max32(long double base, uint_fast8_t exp)
{
    ASSUME(exp <= 32);
    long double b2, b3, b4, b5, b6, b8, b9, b10, b12, b13, b14, b15, b16;

    switch (exp) {
      case 0: return 1;
      case 1: return base;
      case 2: return base * base;
      case 3: return base * base * base;
      case 4: { b2 = base * base; return b2 * b2; }
      case 5: { b2 = base * base; return b2 * b2 * base; }
      case 6: { b2 = base * base; return b2 * b2 * b2; }
      case 7: { b2 = base * base; return b2 * b2 * b2 * base; }
      case 8: { b2 = base * base; b4 = b2 * b2; return b4 * b4; }
      case 9: { b3 = base * base * base; return b3 * b3 * b3; }
      case 10: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b2; }
      case 11: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b2 * base; }
      case 12: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b4; }
      case 13: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b4 * base; }
      case 14: { b2 = base * base; b4 = b2 * b2; return b4 * b4 * b4 * b2; }
      case 15: { b2 = base * base; b5 = b2 * b2 * base; return b5 * b5 * b5; }
      case 16: { b2 = base * base; b4 = b2 * b2; b8 = b4 * b4; return b8 * b8; }
      case 17: { b16 = fast_powl_uint_max32(base, 16); return b16 * base; }
      case 18: { b9  = fast_powl_uint_max32(base, 9); return b9 * b9; }
      case 19: { b9  = fast_powl_uint_max32(base, 9); return b9 * b9 * base; }
      case 20: { b10 = fast_powl_uint_max32(base, 10); return b10 * b10; }
      case 21: { b10 = fast_powl_uint_max32(base, 10); return b10 * b10 * base; }
      case 22: { b2 = base * base; b5 = b2 * b2 * base; b10 = b5 * b5; return b10 * b10 * b2; }
      case 23: { b2 = base * base; b3 = b2 * base; b5 = b2 * b3; b10 = b5 * b5; return b10 * b10 * b3; }
      case 24: { b12 = fast_powl_uint_max32(base, 12); return b12 * b12; }
      case 25: { b12 = fast_powl_uint_max32(base, 12); return b12 * b12 * base; }
      case 26: { b13 = fast_powl_uint_max32(base, 13); return b13 * b13; }
      case 27: { b3 = base * base * base; b6 = b3 * b3; b12 = b6 * b6; return b12 * b12 * b3; }
      case 28: { b14 = fast_powl_uint_max32(base, 14); return b14 * b14; }
      case 29: { b14 = fast_powl_uint_max32(base, 14); return b14 * b14 * base; }
      case 30: { b15 = fast_powl_uint_max32(base, 15); return b15 * b15; }
      case 31: { b15 = fast_powl_uint_max32(base, 15); return b15 * b15 * base; }
      case 32: { b16 = fast_powl_uint_max32(base, 16); return b16 * b16; }
      default: unreachable();
    }
}

_attr_const_func static inline long double
powl_uint(long double base, unsigned int exp)
{
    if (exp <= 32)
        return fast_powl_uint_max32(base, (uint_fast8_t) exp); // Use lookup table for small values

    long double result = (exp & 1) ? base : 1;
    exp >>= 1;
    while (exp) {
        base *= base;
        if (exp & 1)
            result *= base;
        exp >>= 1;
    }
    return result;
}
