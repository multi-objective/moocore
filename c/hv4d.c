#include "hv4d_priv.h"

double
hv4d(const double * restrict data, size_t n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref);
    double hv = hv4dplusU(list);
    free_cdllist(list);
    return hv;
}
