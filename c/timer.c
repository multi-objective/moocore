/*************************************************************************

 Global timer functions.

*************************************************************************/
#include <stddef.h>
#if DEBUG >= 1
#include <stdio.h>
#endif
#include <sys/time.h> /* for struct timeval */
#ifndef WIN32
#include <sys/resource.h> /* for getrusage */
#else
/*
  getrusage
  Implementation according to:
  The Open Group Base Specifications Issue 6
  IEEE Std 1003.1, 2004 Edition

  THIS SOFTWARE IS NOT COPYRIGHTED

  This source code is offered for use in the public domain. You may
  use, modify or distribute it freely.

  This code is distributed in the hope that it will be useful but
  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
  DISCLAIMED. This includes but is not limited to warranties of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Contributed by:
  Ramiro Polla <ramiro@lisha.ufsc.br>

  Modified by:

  Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>
    * Remove dependency with <sys/resources.h>

**************************************************************************/
/*
 * resource.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Based on:
 * http://www.opengroup.org/onlinepubs/000095399/basedefs/sys/resource.h.html
 */
#define RUSAGE_SELF     (1<<0)
#define RUSAGE_CHILDREN (1<<1)

struct rusage
{
    struct timeval ru_utime;	/* user time used */
    struct timeval ru_stime;	/* system time used */
};

/* Include only the minimum from windows.h */
#define WIN32_LEAN_AND_MEAN
#include <errno.h>
#include <stdint.h>
#include <windows.h>

static void
FILETIME_to_timeval (struct timeval *tv, FILETIME *ft)
{
    int64_t fulltime = ((((int64_t) ft->dwHighDateTime) << 32)
                        | ((int64_t) ft->dwLowDateTime));
    fulltime /= 10LL; /* 100-ns -> us */
    tv->tv_sec  = (__typeof__(tv->tv_sec)) fulltime / 1000000L;
    tv->tv_usec = (__typeof__(tv->tv_sec)) fulltime % 1000000L;
}

static int __cdecl
getrusage(int who, struct rusage *r_usage)
{
    FILETIME starttime;
    FILETIME exittime;
    FILETIME kerneltime;
    FILETIME usertime;

    if (!r_usage) {
        errno = EFAULT;
        return -1;
    }

    if (who != RUSAGE_SELF) {
        errno = EINVAL;
        return -1;
    }

    if (GetProcessTimes (GetCurrentProcess (),
                         &starttime, &exittime,
                         &kerneltime, &usertime) == 0) {
        return -1;
    }
    FILETIME_to_timeval (&r_usage->ru_stime, &kerneltime);
    FILETIME_to_timeval (&r_usage->ru_utime, &usertime);
    return 0;
}
#endif

#include "timer.h"

#define TIMER_CPUTIME(X) ( (double)X.ru_utime.tv_sec  +         \
                           (double)X.ru_stime.tv_sec  +         \
                          ((double)X.ru_utime.tv_usec +         \
                           (double)X.ru_stime.tv_usec ) * 1.0E-6)

#define TIMER_WALLTIME(X)  ( (double)X.tv_sec +         \
                             (double)X.tv_usec * 1.0E-6 )

static struct rusage res;
static struct timeval tp;
static double virtual_time, real_time;
static double stop_virtual_time, stop_real_time;

/*
 *  The virtual time of day and the real time of day are calculated and
 *  stored for future use.  The future use consists of subtracting these
 *  values from similar values obtained at a later time to allow the user
 *  to get the amount of time used by the backtracking routine.
 */

void Timer_start(void)
{
    gettimeofday (&tp, NULL );
    real_time =   TIMER_WALLTIME(tp);

    getrusage (RUSAGE_SELF, &res );
    virtual_time = TIMER_CPUTIME(res);
}

/*
 *  Return the time used in seconds (either
 *  REAL or VIRTUAL time, depending on ``type'').
 */
double Timer_elapsed_virtual (void)
{
    getrusage (RUSAGE_SELF, &res);
    double timer_tmp_time = TIMER_CPUTIME(res) - virtual_time;

#if DEBUG >= 4
    if (timer_tmp_time  < 0.0) {
        fprintf(stderr, "%s: Timer_elapsed(): warning: "
                "negative increase in time ", __FILE__);
        fprintf(stderr, "(%.6g - %.6g = ",
                TIMER_CPUTIME(res) , virtual_time);
        fprintf(stderr, "%.6g)\n", timer_tmp_time);
    }
#endif

    return (timer_tmp_time < 0.0) ? 0 : timer_tmp_time;
}

double Timer_elapsed_real (void)
{

    gettimeofday (&tp, NULL);
    double timer_tmp_time = TIMER_WALLTIME(tp) - real_time;

#if DEBUG >= 2
    if (timer_tmp_time  < 0.0) {
        fprintf(stderr, "%s: Timer_elapsed(): warning: "
                "negative increase in time ", __FILE__);
        fprintf(stderr, "(%.6g - %.6g = ",
                TIMER_WALLTIME(tp) , real_time);
        fprintf(stderr, "%.6g)\n", timer_tmp_time);
    }
#endif

    return (timer_tmp_time < 0.0) ? 0 : timer_tmp_time;
}

double Timer_elapsed( TIMER_TYPE type )
{
    return (type == REAL_TIME)
        ? Timer_elapsed_real ()
        : Timer_elapsed_virtual ();
}

void Timer_stop(void)
{
    gettimeofday( &tp, NULL );
    stop_real_time =  TIMER_WALLTIME(tp);

    getrusage( RUSAGE_SELF, &res );
    stop_virtual_time = TIMER_CPUTIME(res);
}

void Timer_continue(void)
{
    gettimeofday( &tp, NULL );
    double timer_tmp_time = TIMER_WALLTIME(tp) - stop_real_time;

#if DEBUG >= 2
    if (timer_tmp_time  < 0.0) {
        fprintf(stderr, "%s: Timer_continue(): warning: "
                "negative increase in time (%.6g - %.6g = %.6g)\n",
                __FILE__, TIMER_WALLTIME(tp), stop_real_time, timer_tmp_time);
    }
#endif

    if (timer_tmp_time > 0.0) real_time += timer_tmp_time;

    getrusage( RUSAGE_SELF, &res );
    timer_tmp_time =  TIMER_CPUTIME(res) - stop_virtual_time;

#if DEBUG >= 2
    if (timer_tmp_time  < 0.0) {
        fprintf(stderr, "%s: Timer_continue(): warning: "
                "negative increase in time (%.6g - %.6g = %.6g)\n",
                __FILE__, TIMER_CPUTIME(res),stop_virtual_time,timer_tmp_time);
    }
#endif

    if (timer_tmp_time > 0.0) virtual_time += timer_tmp_time;
}
