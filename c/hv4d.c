/*************************************************************************

 hv-plus.c

 ---------------------------------------------------------------------

                        Copyright (c) 2013, 2016, 2017
                Andreia P. Guerreiro <apg@dei.uc.pt>


 This program is free software (software libre); you can redistribute
 it and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of the
 License.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, you can obtain a copy of the GNU
 General Public License at:
                 http://www.gnu.org/copyleft/gpl.html
 or by writing to:
           Free Software Foundation, Inc., 59 Temple Place,
                 Suite 330, Boston, MA 02111-1307 USA

 ----------------------------------------------------------------------

 Reference:

 [1] A. P. Guerreiro, C. M. Fonseca, “Computing and Updating Hypervolume Contributions in Up to Four Dimensions”, CISUC Technical Report TR-2017-001, University of Coimbra, 2017

*************************************************************************/



#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include "common.h"
/* ---------------------------------- Auxiliar Functions ----------------------------------*/
//3D points
static inline bool lexicographic_less_3d(const double * a, const double * b)
{
    return (a[2] < b[2] || (a[2] == b[2] && (a[1] < b[1] || (a[1] == b[1] && a[0] <= b[0]))));
}




/* ---------------------------------- Data Structure ---------------------------------------*/

typedef struct dlnode {
  double x[4];                    /* The data vector              */
  struct dlnode * closest[2]; // closest[0] == cx, closest[1] == cy
  struct dlnode * cnext[2]; //current next

  struct dlnode * next[4]; //keeps the points sorted according to coordinates 2,3 and 4
                           // (in the case of 2 and 3, only the points swept by 4 are kept)
  struct dlnode *prev[4]; //keeps the points sorted according to coordinates 2 and 3 (except the sentinel 3)

  int ndomr;    //number of dominators
} hv4dlnode_t;




/* ---------------------------------- Data Structures Functions ---------------------------------------*/


static void
hv4d_initSentinels(hv4dlnode_t * list, const double * ref)
{
    hv4dlnode_t * s1 = list;
    hv4dlnode_t * s2 = list + 1;
    hv4dlnode_t * s3 = list + 2;

    s1->x[0] = -DBL_MAX;
    s1->x[1] = ref[1];
    s1->x[2] = -DBL_MAX;
    s1->x[3] = -DBL_MAX;
    s1->closest[0] = s2;
    s1->closest[1] = s1;

    s1->next[2] = s2;
    s1->next[3] = s2;
    s1->cnext[1] = NULL;
    s1->cnext[0] = NULL;

    s1->prev[2] = s3;
    s1->prev[3] = s3;
    s1->ndomr = 0;


    s2->x[0] = ref[0];
    s2->x[1] = -DBL_MAX;
    s2->x[2] = -DBL_MAX;
    s2->x[3] = -DBL_MAX;
    s2->closest[0] = s2;
    s2->closest[1] = s1;

    s2->next[2] = s3;
    s2->next[3] = s3;
    s2->cnext[1] = NULL;
    s2->cnext[0] = NULL;

    s2->prev[2] = s1;
    s2->prev[3] = s1;
    s2->ndomr = 0;



    s3->x[0] = -INT_MAX;
    s3->x[1] = -INT_MAX;
    s3->x[2] = ref[2];
    s3->x[3] = ref[3];
    s3->closest[0] = s2;
    s3->closest[1] = s1;

    s3->next[2] = s1;
    s3->next[3] = NULL;
    s3->cnext[1] = NULL;
    s3->cnext[0] = NULL;

    s3->prev[2] = s2;
    s3->prev[3] = s2;
    s3->ndomr = 0;
}





/* ---------------------------------- Update data structure ---------------------------------------*/




static void addToZ(hv4dlnode_t * new)
{
    new->next[2] = new->prev[2]->next[2]; //in case new->next[2] was removed for being dominated
    new->next[2]->prev[2] = new;
    new->prev[2]->next[2] = new;
}

static void removeFromz(hv4dlnode_t * old){
    old->prev[2]->next[2] = old->next[2];
    old->next[2]->prev[2] = old->prev[2];
}


/* check if new is dominated, find cx and cy of the 'new' point and find where to insert 'new' in the
 * list sorted by z
 */
static void
setupZandClosest(hv4dlnode_t * list, hv4dlnode_t * new)
{
    // FIXME: This breaks anti-aliasing rules and may break with optimization.
    double * closest1 = (double *) (list);
    double * closest0 = (double *) (list->next[2]);
    hv4dlnode_t * q = list->next[2]->next[2];
    const double * qx =  q->x;
    const double * newx = new->x;
    while (lexicographic_less_3d(qx, newx)) {
        if (qx[0] <= newx[0] && qx[1] <= newx[1]) {
            new->ndomr += 1;
            //new->domr = q;
            //return new;
        } else if (qx[1] < newx[1] && (qx[0] < closest0[0] || (qx[0] == closest0[0] && qx[1] < closest0[1]))) {
            closest0 = (double *) q;
        } else if (qx[0] < newx[0] && (qx[1] < closest1[1] || (qx[1] == closest1[1] && qx[0] < closest1[0]))){
            closest1 = (double *) q;
        }
        q = q->next[2];
        qx = q->x;
    }
    new->closest[0] = new->cnext[0] = (hv4dlnode_t *) closest0;
    new->closest[1] = new->cnext[1] = (hv4dlnode_t *) closest1;
    new->prev[2] = q->prev[2];
    new->next[2] = q;
}




static int updateLinks(hv4dlnode_t * list, hv4dlnode_t * new)
{
    hv4dlnode_t * p = new->next[2];
    hv4dlnode_t * stop = list->prev[2];
    int ndom = 0;
    bool allDelmtrsVisited = false;
    const double * newx = new->x;
    const double * px = p->x;
//     while(p != stop){
    while (p != stop && !allDelmtrsVisited) {
//         q = p->next[2];
        if (px[0] <= newx[0] && px[1] <= newx[1] && (px[0] < newx[0] || px[1] < newx[1])) {
            allDelmtrsVisited = true;
        } else if (newx[0] <= px[0]){
            //new <= p
            if (newx[1] <= px[1]){
                p->ndomr++;
                // p->domr = new;
                ndom += 1;
                removeFromz(p); //HV-ONLY (does not need dominated to compute HV)
            } else if (newx[0] < px[0] && (newx[1] < p->closest[1]->x[1] || (newx[1] == p->closest[1]->x[1] && (newx[0] < p->closest[1]->x[0] || (newx[0] == p->closest[1]->x[0] && newx[2] < p->closest[1]->x[2]))))){ // newx[1] > px[1]
                p->closest[1] = new;
            }
        } else if (newx[1] < px[1] && (newx[0] < p->closest[0]->x[0] || (newx[0] == p->closest[0]->x[0] && (newx[1] < p->closest[0]->x[1] || (newx[1] == p->closest[0]->x[1] && newx[2] < p->closest[0]->x[2]))))) {//newx[0] > px[0]
            p->closest[0] = new;
        }
        p = p->next[2];
        px = p->x;
    }
    return ndom;
}


static int compare_point4d(const void *p1, const void* p2)
{
    int i;
    for(i = 3; i >= 0; i--){
        double x1 = (*((const double **)p1))[i];
        double x2 = (*((const double **)p2))[i];

        if(x1 < x2)
            return -1;
        if(x1 > x2)
            return 1;
    }
    return 0;
}



typedef unsigned int dimension_t;

// FIXME: Move to header and avoid duplicated in hv.c
static bool
strongly_dominates(const double * x, const double * ref, dimension_t dim)
{
    ASSUME(dim >= 2);
    for (dimension_t i = 0; i < dim; i++)
        if (x[i] >= ref[i])
            return false;
    return true;
}

/*
 * Setup circular double-linked list in each dimension
 */
static hv4dlnode_t *
hv4d_setup_cdllist(const double * data, int n, const double *ref)
{
    ASSUME(n > 1);
    dimension_t d = 4;
    dimension_t di = d - 1;

    hv4dlnode_t * head = (hv4dlnode_t *) malloc((n+3) * sizeof(hv4dlnode_t));
    hv4dlnode_t * head3 = head+3;
    hv4d_initSentinels(head, ref);

    const double **scratchd = malloc(n * sizeof(const double*));
    int i, j;
    for (i = 0, j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to assure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (strongly_dominates(data + j * d, ref, d)) {
            scratchd[i] = data + j * d;
            i++;
        }
    }
    n = i;
    if (n == 0) {
        free(scratchd);
        goto finish;
    }
    qsort(scratchd, n, sizeof(const double*), compare_point4d);
    /* FIXME: This creates a copy of the data. */
    double * data2 = (double *) malloc(d * n * sizeof(double));
    for(i = 0; i < n; i++){
        for (j = 0; j < d; j++){
            data2[d * i + j] = scratchd[i][j];
        }
    }
    free(scratchd);

    hv4dlnode_t ** scratch = (hv4dlnode_t **) malloc(n * sizeof(hv4dlnode_t *));
    for (i = 0; i < n; i++) {
        /* FIXME: This creates yet another copy of the data. */
        hv4dlnode_t * p = head3+i;
        scratch[i] = p;
        for (j = 0; j < d; j++)
            p->x[j] = data2[i*d + j];
        // clearPoint:
        p->closest[1] = head;
        p->closest[0] = head->next[2];
        /* because of printfs */
        /* FIXME what does the comment mean????? */
        p->cnext[1] = head;
        p->cnext[0] = head->next[2];
        p->ndomr = 0;
        scratch[i] = p;
    }
    free(data2);

    // FIXME: do we need scratch?
    hv4dlnode_t * s = head->next[di];
    s->next[di] = scratch[0]; // s->next[di] = head3;
    scratch[0]->prev[di] = s; // head3->prev[di] = s;
    for (i = 0; i < n-1; i++){
        scratch[i]->next[di] = scratch[i+1]; // (head3+i)->next[di] = head3+i+1;
        scratch[i+1]->prev[di] = scratch[i]; // (head3+i+1)->prev[di] = head3+i;
    }
    s = head->prev[di];
    s->prev[di] = scratch[n-1]; // s->prev[di] = head3 + n - 1;
    scratch[n-1]->next[di] = s; // (head3 + n - 1)->next[di] = s;
    free(scratch);

finish:
    return head;
}



static void free_cdllist(hv4dlnode_t * list)
{
    free(list);
}




/* ----------------------Hypervolume Indicator Algorithms ---------------------------------------*/



static void restartListy(hv4dlnode_t * list){

    list->next[2]->cnext[1] = list; //link sentinels sentinels ((-inf ref[1] -inf) and (ref[0] -inf -inf))
    list->cnext[0] = list->next[2];

}

static double computeAreaSimple(double * p, int di, hv4dlnode_t * s, hv4dlnode_t * u)
{
    int dj = 1 - di;
    hv4dlnode_t * q = s;
    double area = (q->x[dj] - p[dj]) * (u->x[di] - p[di]);
    while (p[dj] < u->x[dj]) {
        q = u;
        u = u->cnext[di];
        area += (q->x[dj] - p[dj]) * (u->x[di] - q->x[di]);
    }
    return area;
}

static double hv3dplus(hv4dlnode_t * list)
{
    restartListy(list);
    hv4dlnode_t * p = list->next[2]->next[2];
    hv4dlnode_t * stop = list->prev[2];
    double area = 0;
    double volume = 0;
    while (p != stop) {
        if (p->ndomr < 1) {
            p->cnext[0] = p->closest[0];
            p->cnext[1] = p->closest[1];

            area += computeAreaSimple(p->x, 1, p->cnext[0], p->cnext[0]->cnext[1]);

            p->cnext[0]->cnext[1] = p;
            p->cnext[1]->cnext[0] = p;
        } else {
            removeFromz(p);
        }
        volume += area * (p->next[2]->x[2] - p->x[2]);
        p = p->next[2];
    }
    return volume;
}


/* Compute the hypervolume indicator in d=4 by iteratively
 * computing the hypervolume indicator in d=3 (using hv3d+)
 */
static double hv4dplusR(hv4dlnode_t * list)
{
    double hv = 0;
    hv4dlnode_t * last = list->prev[3];
    hv4dlnode_t * new = list->next[3]->next[3];
    // FIXME: Can this be false in the first iteration? Otherwise, use do {} while(new ! last);
    while (new != last) {
        setupZandClosest(list, new);          // compute cx and cy of 'new' and determine next and prev in z
        addToZ(new);                          // add to list sorted by z
        updateLinks(list, new); // update update cx and cy of the points above 'new' in z
                                              // and removes dominated points
        double volume = hv3dplus(list);              // compute hv indicator in d=3 in linear time
        double height = new->next[3]->x[3] - new->x[3];
        hv += volume * height;                // update hypervolume in d=4
        new = new->next[3];
    }
    return hv;
}


double hv4d(double *data, int n, const double *ref)
{
    hv4dlnode_t * list = hv4d_setup_cdllist(data, n, ref);
    double hv = hv4dplusR(list);
    free_cdllist(list);
    return hv;
}
