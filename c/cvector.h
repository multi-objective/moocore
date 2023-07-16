#ifndef __CVECTOR_H__
#define __CVECTOR_H__

#ifndef cvector_assert
#include <assert.h>
#define cvector_assert(X) assert(X)
#endif

#include <stdbool.h>
#include <limits.h> // SIZE_MAX

#define vector_define(VECTOR_TYPE, BASE_TYPE)                                  \
struct VECTOR_TYPE;                                                            \
typedef struct VECTOR_TYPE VECTOR_TYPE;                                        \
                                                                               \
struct VECTOR_TYPE {                                                           \
    BASE_TYPE *_begin;                                                         \
    BASE_TYPE *_end;                                                           \
    BASE_TYPE *_capacity;                                                      \
};                                                                             \
                                                                               \
static inline                                                                  \
void VECTOR_TYPE##_reserve(VECTOR_TYPE * v, size_t n);                         \
                                                                               \
/* Default constructor: constructs an empty vector, with no content
   and a size of zero. */                                                     \
static inline                                                                 \
void VECTOR_TYPE##_ctor_zero(VECTOR_TYPE * v)                                 \
{									      \
    v->_begin = NULL;                                                         \
    v->_end = NULL;                                                           \
    v->_capacity = NULL;						      \
}									      \
                                                                              \
/* Default constructor: constructs an empty vector, with no content
   and a size of zero. */                                                     \
static inline                                                                 \
void VECTOR_TYPE##_ctor(VECTOR_TYPE * v, size_t n)                            \
{									      \
    VECTOR_TYPE##_ctor_zero (v);                                              \
    VECTOR_TYPE##_reserve (v, n);                                             \
}									      \
                                                                              \
/* Default constructor: dynamically allocates an empty vector, with no
   content and a size of zero. */                                             \
static inline                                                                 \
VECTOR_TYPE * VECTOR_TYPE##_new(size_t n)                                     \
{									      \
    VECTOR_TYPE * v = malloc (sizeof(VECTOR_TYPE));                           \
    VECTOR_TYPE##_ctor (v, n);                                                \
    return v;                                                                 \
}									      \
                                                                              \
/* Destructor. It frees the memory allocated to the vector, but it
   does not try to free each of its elements or the vector itself. */          \
static inline                                                                  \
void VECTOR_TYPE##_dtor(VECTOR_TYPE * v)                                       \
{ free(v->_begin); }                                                           \
                                                                               \
/* Destructor. It frees the memory allocated to a dynamically
   allocated vector, but it does not try to free each of its
   elements. */                                                               \
static inline                                                                 \
void VECTOR_TYPE##_delete(VECTOR_TYPE * v)                                    \
{ VECTOR_TYPE##_dtor(v); free(v); }                                           \
                                                                              \
                                                                              \
/* Returns the number of elements in the vector container. */                 \
static inline                                                                 \
size_t VECTOR_TYPE##_size(const VECTOR_TYPE * v)			      \
{ return v->_end - v->_begin; }						      \
                                                                              \
/* Return size of allocated storage capacity. */                              \
static inline                                                                 \
size_t VECTOR_TYPE##_capacity(const VECTOR_TYPE * v)                          \
{ return v->_capacity - v->_begin; }                                          \
                                                                              \
/* Returns whether the vector container is empty, i.e. whether its
   size is 0. */                                                              \
static inline                                                                 \
bool VECTOR_TYPE##_empty(const VECTOR_TYPE * v)                               \
{ return v->_begin == v->_end; }                                              \
                                                                              \
/* Requests that the capacity of the allocated storage space for the
   elements of the vector container be at least enough to hold n
   elements.

   This informs the vector of a planned increase in size, although
   notice that the parameter n informs of a minimum, so the resulting
   capacity may be any capacity equal or larger than this.

   When n is greater than the current capacity, a reallocation is
   attempted during the call to this function. If successful, it
   grants that no further automatic reallocations will happen because
   of a call to vector::insert or vector::push_back until the vector
   size surpasses at least n (this preserves the validity of iterators
   on all these future calls).

   A reallocation invalidates all previously obtained iterators,
   references and pointers to elements of the vector.

   In any case, a call to this function never affects the elements
   contained in the vector, nor the vector size. */                           \
static inline                                                                 \
void VECTOR_TYPE##_reserve(VECTOR_TYPE * v, size_t n)                         \
{									      \
    size_t old_capacity = VECTOR_TYPE##_capacity(v);			      \
    size_t old_size = VECTOR_TYPE##_size(v);                                  \
    if (n > old_capacity) {                                                   \
        cvector_assert(SIZE_MAX / sizeof(BASE_TYPE) >= n);                    \
        v->_begin = realloc(v->_begin, sizeof(BASE_TYPE) * n);                \
        cvector_assert(v->_begin != NULL);                                    \
        v->_end = v->_begin + old_size;                                       \
        v->_capacity = v->_begin + n;                                         \
    }									      \
}      									      \
                                                                              \
/* Adds a new element at the end of the vector, after its current last
   element. The content of this new element is initialized to a copy
   of x.

   This effectively increases the vector size by one, which causes a
   reallocation of the internal allocated storage if the vector size
   was equal to the vector capacity before the call. Reallocations
   invalidate all previously obtained iterators, references and
   pointers. */                                                               \
static inline                                                                 \
void VECTOR_TYPE##_push_back(VECTOR_TYPE * v, BASE_TYPE x)                    \
{                                                                             \
    if (v->_end == v->_capacity)                                              \
        VECTOR_TYPE##_reserve (v,                                             \
                               (VECTOR_TYPE##_capacity (v) == 0)              \
                               ? 8                                            \
                               : 2 * VECTOR_TYPE##_capacity (v));             \
    *(v->_end) = x;                                                           \
    v->_end++;                                                                \
}									      \
                                                                              \
/* Removes the last element in the vector, effectively reducing the
   vector size by one and invalidating all iterators and references to
   it. Note that the element is not freed. */                                  \
static inline                                                                  \
BASE_TYPE VECTOR_TYPE##_pop_back(VECTOR_TYPE * v)                              \
{                                                                              \
    cvector_assert(!VECTOR_TYPE##_empty(v));                                   \
    v->_end--;                                                                 \
    return *(v->_end);                                                         \
}                                                                              \
                                                                               \
/* Returns the first element in the vector container.  This is not a
   reference like in C++! */                                                   \
static inline                                                                  \
BASE_TYPE VECTOR_TYPE##_front(const VECTOR_TYPE * v)                           \
{                                                                              \
    cvector_assert (!VECTOR_TYPE##_empty(v));                                  \
    return v->_begin[0];                                                       \
}                                                                              \
                                                                               \
/* Returns the last element in the vector container.  This is not a
   reference like in C++! */                                                   \
static inline                                                                  \
BASE_TYPE VECTOR_TYPE##_back(const VECTOR_TYPE * v)                            \
{                                                                              \
    cvector_assert (!VECTOR_TYPE##_empty(v));                                  \
    return *(v->_end - 1);                                                     \
}                                                                              \
                                                                               \
/* Returns an iterator (pointer) referring to the first element in the
   vector container. */                                                       \
static inline                                                                 \
BASE_TYPE * VECTOR_TYPE##_begin(const VECTOR_TYPE * v)                        \
{ return v->_begin; }							      \
                                                                              \
/* Returns an iterator (pointer) referring to the past-the-end element
   in the vector container. */                                                \
static inline                                                                 \
BASE_TYPE * VECTOR_TYPE##_end(const VECTOR_TYPE * v)                          \
{ return v->_end; }							      \
                                                                              \
/* Returns the element at position pos. */                                    \
static inline                                                                 \
BASE_TYPE VECTOR_TYPE##_at(const VECTOR_TYPE * v, size_t pos)                 \
{                                                                             \
    cvector_assert(pos <= VECTOR_TYPE##_size(v));                             \
    return v->_begin[pos];                                                    \
}                                                                              \
                                                                               \
/* Set the element at position pos to value e and return it. */                \
static inline                                                                  \
BASE_TYPE VECTOR_TYPE##_set(const VECTOR_TYPE * v, size_t pos, BASE_TYPE e)    \
{                                                                              \
    cvector_assert(pos <= VECTOR_TYPE##_size(v));                              \
    return (v->_begin[pos] = e);                                               \
}                                                                              \


#endif
