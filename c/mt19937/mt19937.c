/*
   From https://github.com/numpy/numpy/blob/5b00a4d86134b9c190dccd519ba500af776f2ab0/numpy/random/src/mt19937/mt19937.c
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

  for (i = 0; i < _MT19937_N - _MT19937_M; i++) {
    y = (state->key[i] & UPPER_MASK) | (state->key[i + 1] & LOWER_MASK);
    state->key[i] = state->key[i + _MT19937_M] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);
  }
  for (; i < _MT19937_N - 1; i++) {
    y = (state->key[i] & UPPER_MASK) | (state->key[i + 1] & LOWER_MASK);
    state->key[i] = state->key[i + (_MT19937_M - _MT19937_N)] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);
  }
  y = (state->key[_MT19937_N - 1] & UPPER_MASK) | (state->key[0] & LOWER_MASK);
  state->key[_MT19937_N - 1] = state->key[_MT19937_M - 1] ^ (y >> 1) ^ (-(y & 1) & MATRIX_A);

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

  state->pos = 0;
}

extern inline uint64_t mt19937_next64(mt19937_state *state);

extern inline uint32_t mt19937_next32(mt19937_state *state);

extern inline double mt19937_next_double(mt19937_state *state);
