/*
   From https://github.com/numpy/numpy/blob/b79be4888136c43805307c0151068089c68dd43c/numpy/random/src/mt19937/mt19937.c
*/
#include "mt19937.h"

void mt19937_seed(mt19937_state *state, uint32_t seed) {
  int pos;
  seed &= 0xffffffffUL;

  /* Knuth's PRNG as used in the Mersenne Twister reference implementation */
  for (pos = 0; pos < RK_STATE_LEN; pos++) {
    state->key[pos] = seed;
    seed = (1812433253UL * (seed ^ (seed >> 30)) + pos + 1) & 0xffffffffUL;
  }
  state->pos = RK_STATE_LEN;
}

/* initializes mt[RK_STATE_LEN] with a seed */
static void init_genrand(mt19937_state *state, uint32_t s) {
  int mti;
  uint32_t *mt = state->key;

  mt[0] = s & 0xffffffffUL;
  for (mti = 1; mti < RK_STATE_LEN; mti++) {
    /*
     * See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
     * In the previous versions, MSBs of the seed affect
     * only MSBs of the array mt[].
     * 2002/01/09 modified by Makoto Matsumoto
     */
      /* This code triggers conversions between uint32_t and unsigned long,
         which various compilers warn about. */
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wconversion"
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#endif
    mt[mti] = (1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
#  pragma GCC diagnostic pop
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
    /* for > 32 bit machines */
    mt[mti] &= 0xffffffffUL;
  }
  state->pos = mti;
  return;
}

/*
 * initialize by an array with array-length
 * init_key is the array for initializing keys
 * key_length is its length
 */
void mt19937_init_by_array(mt19937_state *state, uint32_t *init_key,
                           int key_length) {
  /* was signed in the original code. RDH 12/16/2002 */
  int i = 1;
  int j = 0;
  uint32_t *mt = state->key;
  int k;

  init_genrand(state, 19650218UL);
  k = (RK_STATE_LEN > key_length ? RK_STATE_LEN : key_length);
  for (; k; k--) {
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wconversion"
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#endif
    /* non linear */
    mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525UL)) + init_key[j] + j;
#  pragma GCC diagnostic pop
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
    /* for > 32 bit machines */
    mt[i] &= 0xffffffffUL;
    i++;
    j++;
    if (i >= RK_STATE_LEN) {
      mt[0] = mt[RK_STATE_LEN - 1];
      i = 1;
    }
    if (j >= key_length) {
      j = 0;
    }
  }
  for (k = RK_STATE_LEN - 1; k; k--) {
      /* This code triggers conversions between uint32_t and unsigned long,
         which various compilers warn about. */
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wconversion"
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#endif
    mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941UL)) - i; /* non linear */
#  pragma GCC diagnostic pop
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
    mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
    i++;
    if (i >= RK_STATE_LEN) {
      mt[0] = mt[RK_STATE_LEN - 1];
      i = 1;
    }
  }

  mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */
}

void mt19937_gen(mt19937_state *state) {
  uint32_t y;
  int i;

/* Not sure how to fix these warnings::

mt19937/mt19937.c:90:53: warning: higher order bits are zeroes after implicit conversion [-Wimplicit-int-conversion]
   90 |     state->key[i] = state->key[i + M] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);
      |                                                     ^~~~~~~~ ~
mt19937/mt19937.c:94:59: warning: higher order bits are zeroes after implicit conversion [-Wimplicit-int-conversion]
   94 |     state->key[i] = state->key[i + (M - N)] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);
      |                                                           ^~~~~~~~ ~
mt19937/mt19937.c:97:55: warning: higher order bits are zeroes after implicit conversion [-Wimplicit-int-conversion]
   97 |   state->key[N - 1] = state->key[M - 1] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);
      |                                                       ^~~~~~~~ ~
*/
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#endif

  for (i = 0; i < N - M; i++) {
    y = (state->key[i] & UPPER_MASK) | (state->key[i + 1] & LOWER_MASK);
    state->key[i] = state->key[i + M] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);
  }
  for (; i < N - 1; i++) {
    y = (state->key[i] & UPPER_MASK) | (state->key[i + 1] & LOWER_MASK);
    state->key[i] = state->key[i + (M - N)] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);
  }
  y = (state->key[N - 1] & UPPER_MASK) | (state->key[0] & LOWER_MASK);
  state->key[N - 1] = state->key[M - 1] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

  state->pos = 0;
}

extern inline uint64_t mt19937_next64(mt19937_state *state);

extern inline uint32_t mt19937_next32(mt19937_state *state);

extern inline double mt19937_next_double(mt19937_state *state);
