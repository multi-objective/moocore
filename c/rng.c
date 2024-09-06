#include "rng.h"
#include "ziggurat_constants.h"
#include <math.h> /* log1p() exp() */

/* This is taken from random_standard_normal()
   https://github.com/numpy/numpy/blob/12aa98e0eca2d36358accb95253ccc5bfa9e788c/numpy/random/src/distributions/distributions.c#L137

## NumPy

Copyright (c) 2005-2017, NumPy Developers.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.

* Neither the name of the NumPy Developers nor the names of any
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


## Julia

The ziggurat methods were derived from Julia.

Copyright (c) 2009-2019: Jeff Bezanson, Stefan Karpinski, Viral B. Shah,
and other contributors:

https://github.com/JuliaLang/julia/contributors

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
double
rng_standard_normal(rng_state *rng)
{
    for (;;) {
        /* r = e3n52sb8 */
        uint64_t r = mt19937_next64(rng);
        int idx = r & 0xff;
        r >>= 8;
        int sign = r & 0x1;
        uint64_t rabs = (r >> 1) & 0x000fffffffffffff;
        double x = (double) rabs * wi_double[idx];
        if (sign & 0x1)
            x = -x;
        if (rabs < ki_double[idx])
            return x; /* 99.3% of the time return here */
        if (idx == 0) {
            for (;;) {
                /* Switch to 1.0 - U to avoid log(0.0), see GH 13361 */
                double xx = -ziggurat_nor_inv_r * log1p(-mt19937_next_double(rng));
                double yy = -log1p(-mt19937_next_double(rng));
                if (yy + yy > xx * xx)
                    return ((rabs >> 8) & 0x1) ? -(ziggurat_nor_r + xx)
                        : ziggurat_nor_r + xx;
            }
        } else if (((fi_double[idx - 1] - fi_double[idx]) * mt19937_next_double(rng) +
                    fi_double[idx]) < exp(-0.5 * x * x))
                return x;
    }
}

void
rng_bivariate_normal_fill(rng_state * rng,
                          double mu1, double mu2,
                          double sigma1, double sigma2, double rho,
                          double *out, int n)
{
    const double sigma2rho = sigma2 * rho;
    const double nu = sigma2 * sqrt(1 - rho*rho);
    for (int i = 0; i < n; i++) {
        const double x1 = rng_standard_normal(rng);
        *out = mu1 + x1 * sigma1;
        out++;
        *out = mu2 + x1 * sigma2rho + nu * rng_standard_normal(rng);
        out++;
    }
}
