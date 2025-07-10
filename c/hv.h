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

#if defined(_WIN32) && !(defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__))
# ifndef MOOCORE_STATIC_LIB
#  ifdef MOOCORE_SHARED_LIB
#   define MOOCORE_API extern __declspec(dllexport)
#  else
#   define MOOCORE_API extern __declspec(dllimport)
#  endif
# endif
#elif __GNUC__ >= 4 && defined(MOOCORE_SHARED_LIB)
# define MOOCORE_API extern __attribute__((visibility("default")))
#endif

#ifndef MOOCORE_API
# define MOOCORE_API extern
#endif

// C++ needs to know that types and declarations are C, not C++.
#ifdef	__cplusplus
extern "C" {
#endif

MOOCORE_API double fpli_hv(const double *data, int d, int n, const double *ref);
MOOCORE_API double hv_contributions(double *hvc, double *points, int dim, int size, const double * ref);

#ifdef	__cplusplus
}
#endif

#endif // HV_H_
