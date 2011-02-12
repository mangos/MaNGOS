/*
 * $Id: crit.h 80826 2008-03-04 14:51:23Z wotte $
 *
 * Copyright (c) 1994, 1995.  Netscape Communications Corporation.  All
 * rights reserved.
 *
 * Use of this software is governed by the terms of the license agreement for
 * the Netscape Communications or Netscape Comemrce Server between the
 * parties.
 */


/* ------------------------------------------------------------------------ */


/*
 * crit.h: Critical section abstraction. Used in threaded servers to protect
 *         areas where two threads can interfere with each other.
 *
 *         Condvars are condition variables that are used for thread-thread
 *         synchronization.
 *
 * Rob McCool
 */

#ifndef CRIT_H
#define CRIT_H


#ifdef USE_NSPR
#include <nspr/prmon.h>
typedef PRMonitor* CRITICAL;
#else
typedef void *CRITICAL;
#endif

/*
 * crit_init creates and returns a new critical section variable. At the
 * time of creation no one has entered it.
 */
#ifdef USE_NSPR
#define crit_init() PR_NewMonitor(0)
#else
#define crit_init() (0)
#endif

/*
 * crit_enter enters a critical section. If someone is already in the
 * section, the calling thread is blocked until that thread exits.
 */
#ifdef USE_NSPR
#define crit_enter(id) PR_EnterMonitor(id)
#else
#define crit_enter(id) (0)
#endif

/*
 * crit_exit exits a critical section. If another thread is blocked waiting
 * to enter, it will be unblocked and given ownership of the section.
 */
#ifdef USE_NSPR
#define crit_exit(id) PR_ExitMonitor(id)
#else
#define crit_exit(id) (0)
#endif

/*
 * crit_terminate removes a previously allocated critical section variable.
 */
#ifdef USE_NSPR
#define crit_terminate(id) PR_DestroyMonitor(id)
#else
#define crit_terminate(id) (0)
#endif


#ifdef USE_NSPR
typedef PRMonitor* CONDVAR;
#else
typedef void* CONDVAR;
#endif

/*
 * condvar_init initializes and returns a new condition variable. You
 * must provide a critical section to be associated with this condition
 * variable.
 */
#ifdef USE_NSPR
#define condvar_init(crit) (crit)
#else
#define condvar_init(crit) (crit)
#endif

/*
 * condvar_wait blocks on the given condition variable. The calling thread
 * will be blocked until another thread calls condvar_notify on this variable.
 * The caller must have entered the critical section associated with this
 * condition variable prior to waiting for it.
 */
#ifdef USE_NSPR
#define condvar_wait(cv) (PR_Wait(cv, LL_MAXINT))
#else
#define condvar_wait(cv) (0)
#endif

/*
 * condvar_notify awakens any threads blocked on the given condition
 * variable. The caller must have entered the critical section associated
 * with this variable first.
 */
#ifdef USE_NSPR
#define condvar_notify(cv) (PR_Notify(cv))
#else
#define condvar_notify(cv) (0)
#endif

/*
 * condvar_terminate frees the given previously allocated condition variable
 */
#ifdef USE_NSPR
#define condvar_terminate(cv) (0)
#else
#define condvar_terminate(cv) (0)
#endif


#endif
