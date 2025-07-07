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

#ifdef __cplusplus
extern "C" {
#endif

double fpli_hv(const double *data, int d, int n, const double *ref);
void hv_contributions (double *hvc, double *points, int dim, int size, const double * ref);
#ifdef __cplusplus
}
#endif

#endif // HV_H_
