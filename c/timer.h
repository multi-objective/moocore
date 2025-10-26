/*************************************************************************

 Simple timer functions.

 ---------------------------------------------------------------------

 Copyright (C) 2005-2007, 2025  Manuel Lopez-Ibanez

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------
*************************************************************************/
#ifndef TIMER_H_
#define TIMER_H_

typedef enum type_timer {REAL_TIME, VIRTUAL_TIME} TIMER_TYPE;
typedef struct Timer_t {
    double start_time;
    double stop_time;
    TIMER_TYPE type;
} Timer_t;

// Global timer functions
void Timer_start(void);
double Timer_elapsed_virtual(void);
double Timer_elapsed_real(void);
double Timer_elapsed(TIMER_TYPE type);
void Timer_stop(void);
void Timer_continue(void);

// Object oriented functions
Timer_t timer_start(TIMER_TYPE type);
double timer_elapsed(const Timer_t * restrict);
double timer_reset(Timer_t * restrict);
void timer_stop(Timer_t * restrict);
void timer_continue(Timer_t * restrict);


#endif // TIMER_H_
