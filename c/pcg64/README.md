# PCG64

`pcg64.h` and `LICENSE.md` come from
[NumPy's PCG64 implementation](https://github.com/numpy/numpy/tree/98acfbde9876839be145d022beb76e7ff835e8e1/numpy/random/src/pcg64)
at revision `98acfbde9876839be145d022beb76e7ff835e8e1`.
The header adds explicit casts to silence conversion warnings.
moocore uses the inline functions in `pcg64.h`.

moocore initializes PCG64 with its 32-bit seed as the initial state and a
fixed stream identifier of 54; it does not use NumPy's `SeedSequence`.
