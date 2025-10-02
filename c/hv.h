/*************************************************************************

 hv.h

 ---------------------------------------------------------------------

                          Copyright (C) 2005-2006, 2025
                    Carlos M. Fonseca <cmfonsec@dei.uc.pt>
          Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>
                       Luis Paquete <paquete@dei.uc.pt>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------


*************************************************************************/
#ifndef HV_H_
#define HV_H_

#include "libmoocore-config.h"

// C++ needs to know that types and declarations are C, not C++.
BEGIN_C_DECLS

MOOCORE_API double fpli_hv(const double * data, int d, int n,
                           const double * ref);
MOOCORE_API double hv_contributions(double * hvc, double * points, int dim,
                                    int size, const double * ref,
                                    bool ignore_dominated);
END_C_DECLS

#endif // HV_H_
