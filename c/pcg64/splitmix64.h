#ifndef SPLITMIX64_H
#define SPLITMIX64_H
/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
// Original documentation by Vigna:
/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See https://doi.org/10.1145/2714064.2660195 and
   https://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html

   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state; otherwise, we
   rather suggest to use a xoroshiro128+ (for moderately parallel
   computations) or xorshift1024* (for massively parallel computations)
   generator. */

#include <stdint.h>
// Returns random number, modifies *state.
static inline uint64_t
splitmix64_r(uint64_t *state)
{
    // floor( ( (1+sqrt(5))/2 ) * 2**64 MOD 2**64) = 0x9E3779B97F4A7C15
    uint64_t z = (*state += UINT64_C(0x9E3779B97F4A7C15));
    // David Stafford's Mix13 for MurmurHash3's 64-bit finalizer.
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

#endif // SPLITMIX64_H
