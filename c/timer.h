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
void Timer_start(void);
double Timer_elapsed_virtual(void);
double Timer_elapsed_real(void);
double Timer_elapsed(TIMER_TYPE type);
void Timer_stop(void);
void Timer_continue(void);

#endif // TIMER_H_
