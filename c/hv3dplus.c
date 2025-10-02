/******************************************************************************
 HV3D+ algorithm.
 ------------------------------------------------------------------------------

                        Copyright (c) 2013, 2016, 2017
                     Andreia P. Guerreiro <apg@dei.uc.pt>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ------------------------------------------------------------------------------

 Reference:

 [1] Andreia P. Guerreiro and Carlos M. Fonseca. Computing and Updating
     Hypervolume Contributions in Up to Four Dimensions. IEEE Transactions on
     Evolutionary Computation, 22(3):449–463, June 2018.

 ------------------------------------------------------------------------------

 Modified by Manuel López-Ibáñez (2025):
  - Integration within moocore.
  - Correct handling of weakly dominated points and repeated coordinates during
    preprocessing().
  - More efficient setup_cdllist() and preprocessing() in terms of time and memory.

******************************************************************************/

#include <float.h>
#include <string.h>
#include "common.h"
#define HV_DIMENSION 3
#include "hv_priv.h"


static inline double
compute_area3d_simple(const double * px, const dlnode_t * q)
{
    return compute_area_no_inners(px, q, 1);
}

static double
hv3dplus_list(dlnode_t * list)
{
    restart_list_y(list);
    assert(list + 2 == list->prev[0]);

    double area = 0, volume = 0;
    dlnode_t * p = (list + 1)->next[0];
    const dlnode_t * stop = list + 2;
    while (p != stop) {
        area += compute_area3d_simple(p->x, p->cnext[0]);
        p->cnext[0]->cnext[1] = p;
        p->cnext[1]->cnext[0] = p;
        ASSERT_OR_DO((p->next[0]->x[2] > p->x[2]) ||
                         (p->next[0]->x[0] < p->x[0]) ||
                         (p->next[0]->x[1] < p->x[1]),
                     print_x(p);
                     print_x(p->next[0]););
        assert(area > 0);
        /* It is possible to have two points with the same z-value, e.g.,
           (1,2,3) and (2,1,3). */
        volume += area * (p->next[0]->x[2] - p->x[2]);
        p = p->next[0];
    }
    return volume;
}

double
hv3d(const double * restrict data, size_t n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref);
    double hv = hv3dplus_list(list);
    free_cdllist(list);
    return hv;
}
