/*************************************************************************

 r2_exact.h

 ---------------------------------------------------------------------

                          Copyright (C) 2026
        Lennart Schäpermeier <lennart.schaepermeier@uni-muenster.de>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------


*************************************************************************/
#ifndef R2_EXACT_H_
#define R2_EXACT_H_

#include <stdbool.h>
#include "libmoocore-config.h"

// C++ needs to know that types and declarations are C, not C++.
BEGIN_C_DECLS

MOOCORE_API double r2_exact(const double * restrict data, size_t n, dimension_t d, const double * restrict ref);

END_C_DECLS

#endif // R2_EXACT_H_
