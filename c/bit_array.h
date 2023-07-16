/* A better implementation
   https://github.com/noporpoise/BitArray
*/
#include <inttypes.h>
#include <stdlib.h> // malloc, calloc
#include <string.h> // memcpy
#include "common.h" // fatal_error

typedef uint64_t bit_array, bit_array_word_t;
typedef uint8_t word_offset_t; // Offset within a 64 bit word

// number of bits per word
static const size_t bit_array_word_size = sizeof(bit_array_word_t) * 8;

static inline size_t
bitset_index(const bit_array *b, size_t pos) { return(pos % (sizeof(*b)*8)); }

static inline size_t
bitset_word(const bit_array *b, size_t pos) { return(pos / (sizeof(*b)*8)); }

static inline bit_array
bitset_mask(size_t k) { return (bit_array)((bit_array)1 << k); }

static inline void
bit_array_set_one(bit_array * b, size_t i) {
    b[bitset_word(b, i)] |= bitset_mask(bitset_index(b,i));
}

static inline void
bit_array_set_zero(bit_array * b, size_t i) {
    b[bitset_word(b,i)] &=~ bitset_mask(bitset_index(b,i));
}

static inline char
bit_array_get(const bit_array * b, size_t i) {
    return (b[bitset_word(b,i)] >> bitset_index(b,i)) & 0x1;
}

static inline size_t
bit_nwords(size_t nbits) {
    return  (nbits + bit_array_word_size - 1) / bit_array_word_size;
}

static inline size_t bit_array_bytesize(size_t n)
{
    return sizeof(bit_array_word_t) * bit_nwords(n);
}

static inline bit_array *
bit_array_alloc(size_t n)
{
    //fprintf(stderr, "n= %ld, word_size = %ld size = %ld\n", n, bit_array_word_size, bit_nwords(n));
    return (bit_array *) malloc(bit_array_bytesize(n));
}

static inline bit_array *
bit_array_calloc(size_t n)
{
    //fprintf(stderr, "word_size = %ld size = %ld", bit_array_word_size, bit_nwords(n));
    return (bit_array *) calloc(bit_nwords(n), sizeof(bit_array_word_t));
}

static inline bit_array *
bit_array_realloc(bit_array *b, size_t n)
{
    //fprintf(stderr, "word_size = %ld size = %ld", bit_array_word_size, bit_nwords(n));
    return (bit_array *) realloc(b, bit_array_bytesize(n));
}

static inline void
bit_array_zero_all(bit_array *b, size_t n)
{
    memset(b, 0, sizeof(bit_array_word_t) * bit_nwords(n));
}

static inline void
bit_array_set(bit_array * b, size_t i, bool x) {
    x ? bit_array_set_one(b, i) : bit_array_set_zero(b, i);
}


static inline void
bitset_check(const bit_array *b _no_warn_unused, const bool * ref _no_warn_unused, size_t n)
{
    for(size_t i = 0; i < n; i++) {
        if (bit_array_get(b, i) != ref[i]) {
            fatal_error("bit_array_check_failed at %lu: %d != %d : %lu!\n",
                        (unsigned long) i, bit_array_get(b, i), ref[i], (unsigned long) n);
        }
    }
}
static inline bit_array *
bit_array_offset(bit_array *b, size_t k, size_t nbits)
{
    return b + k * bit_nwords(nbits);
}

static inline void
bit_array_check (bit_array *b, const bool * ref, size_t n, size_t nbits)
{
    for (size_t k = 0; k < n; k++) {
        // fprintf(stderr,"checking: k=%ld n=%ld, nbits=%ld offset=%ld\n",
        //         k, n, nbits, k * bit_array_bytesize(nbits));
        bitset_check(bit_array_offset(b, k, nbits), ref + k * nbits, nbits);
    }
}

static inline void
bit_array_copy(bit_array * dest, const bit_array *src, size_t n) {
    memcpy(dest, src, sizeof(bit_array_word_t) * bit_nwords(n));
}

static inline void
bit_array_fprintf(FILE *stream, bit_array *b, size_t n)
{
    for (size_t k = 0; k < n; k++) {
        fprintf(stream, "%d", bit_array_get(b, k) ? 1 : 0);
    }
}
