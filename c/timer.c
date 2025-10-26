/*************************************************************************

 Global timer functions.

*************************************************************************/
#include "config.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h> // HUGE_VAL
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

struct rusage {
    struct timeval ru_utime;	/* user time used */
    struct timeval ru_stime;	/* system time used */
};

/* Include only the minimum from windows.h */
#define WIN32_LEAN_AND_MEAN
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
    if (!r_usage) {
        // errno = EFAULT;
        return -1;
    }

    if (who != RUSAGE_SELF) {
        // errno = EINVAL;
        return -1;
    }

    FILETIME starttime, exittime, kerneltime, usertime;
    if (GetProcessTimes (GetCurrentProcess(),
                         &starttime, &exittime,
                         &kerneltime, &usertime) == 0) {
        return -1;
    }
    FILETIME_to_timeval(&r_usage->ru_stime, &kerneltime);
    FILETIME_to_timeval(&r_usage->ru_utime, &usertime);
    return 0;
}
#endif

#include "timer.h"
#include "maxminclamp.h"

static inline double
TIMER_CPUTIME(struct rusage res)
{
    // Convert everything to microseconds.
    int64_t total_usec = ((int64_t)res.ru_utime.tv_sec + (int64_t)res.ru_stime.tv_sec) * 1000000
        + (int64_t)res.ru_utime.tv_usec + (int64_t)res.ru_stime.tv_usec;
    // Convert total microseconds to seconds as double
    return (double)total_usec / 1e6;
}

static inline double
TIMER_WALLTIME(struct timeval tp)
{
    // Convert everything to microseconds.
    int64_t total_usec = (int64_t)tp.tv_sec * 1000000 + (int64_t)tp.tv_usec;
    // Convert total microseconds to seconds as double
    return (double)total_usec / 1e6;
}

static double virtual_time, real_time;
static double stop_virtual_time, stop_real_time;

static double get_wall_time(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return TIMER_WALLTIME(tp);
}

static double get_virtual_time(void)
{
    struct rusage res;
    if (getrusage(RUSAGE_SELF, &res) == 0)
        return TIMER_CPUTIME(res);
    assert(false);
    return HUGE_VAL;
}

static double get_time(TIMER_TYPE type)
{
    return (type == REAL_TIME) ? get_wall_time() : get_virtual_time();
}

/*
 *  The virtual time of day and the real time of day are calculated and
 *  stored for future use.  The future use consists of subtracting these
 *  values from similar values obtained at a later time to allow the user
 *  to get the amount of time used by the backtracking routine.
 */

void Timer_start(void)
{
    real_time = get_wall_time();
    virtual_time = get_virtual_time();
}

/*
 *  Return the time used in seconds (either
 *  REAL or VIRTUAL time, depending on ``type'').
 */
double Timer_elapsed_virtual(void)
{
    double now = get_virtual_time();
    double timer_tmp_time = now - virtual_time;

#if DEBUG >= 4
    if (timer_tmp_time < 0.0) {
        fprintf(stderr, "%s: Timer_elapsed(): warning: "
                "negative increase in time (%.6g - %.6g = %.6g)\n",
                __FILE__, now, virtual_time, timer_tmp_time);
    }
#endif
    return MAX(timer_tmp_time, 0.0);
}

double Timer_elapsed_real (void)
{
    double now = get_wall_time();
    double timer_tmp_time = now - real_time;

#if DEBUG >= 2
    if (timer_tmp_time < 0.0) {
        fprintf(stderr, "%s: Timer_elapsed(): warning: "
                "negative increase in time (%.6g - %.6g = %.6g)\n",
                __FILE__, now, real_time, timer_tmp_time);
    }
#endif
    return MAX(timer_tmp_time, 0.0);
}

double Timer_elapsed( TIMER_TYPE type )
{
    return (type == REAL_TIME)
        ? Timer_elapsed_real()
        : Timer_elapsed_virtual();
}

void Timer_stop(void)
{
    stop_real_time = get_wall_time();
    stop_virtual_time = get_virtual_time();
}

void Timer_continue(void)
{
    double now = get_wall_time();
    double timer_tmp_time = now - stop_real_time;
#if DEBUG >= 2
    if (timer_tmp_time < 0.0) {
        fprintf(stderr, "%s: Timer_continue(): warning: "
                "negative increase in time (%.6g - %.6g = %.6g)\n",
                __FILE__, now, stop_real_time, timer_tmp_time);
    }
#endif

    if (timer_tmp_time > 0.0) real_time += timer_tmp_time;

    now = get_virtual_time();
    timer_tmp_time =  now - stop_virtual_time;
#if DEBUG >= 2
    if (timer_tmp_time < 0.0) {
        fprintf(stderr, "%s: Timer_continue(): warning: "
                "negative increase in time (%.6g - %.6g = %.6g)\n",
                __FILE__, now, stop_virtual_time, timer_tmp_time);
    }
#endif

    if (timer_tmp_time > 0.0) virtual_time += timer_tmp_time;
}


double timer_reset(Timer_t * restrict timer)
{
    double old = timer->start_time;
    timer->start_time = get_time(timer->type);
    return MAX(timer->start_time - old, 0.0);
}

Timer_t timer_start(TIMER_TYPE type)
{
    Timer_t timer;
    assert(type == REAL_TIME || type == VIRTUAL_TIME);
    timer.type = type;
    timer.start_time = get_time(type);
    return timer;
}

double timer_elapsed(const Timer_t * restrict timer)
{
    double now = get_time(timer->type);
    return MAX(now - timer->start_time, 0.0);
}

void timer_stop(Timer_t * restrict timer)
{
    timer->stop_time = get_time(timer->type);
}

void timer_continue(Timer_t * restrict timer)
{
    double now = get_time(timer->type);
    timer->start_time += MAX(now - timer->stop_time, 0.0);
}
