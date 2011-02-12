/*
 * $Id: systhr.h 80826 2008-03-04 14:51:23Z wotte $
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
 * systhr.h: Abstracted threading mechanisms
 *
 * Rob McCool
 */

#ifndef _SYSTHR_H
#define _SYSTHR_H

#include "netsite.h"
#include "systems.h"

#ifdef THREAD_ANY

#ifdef USE_NSPR
#include <nspr/prthread.h>
#include <nspr/prglobal.h>

typedef PRThread* SYS_THREAD;
#endif

#ifdef THREAD_WIN32
#include <nspr/prthread.h>
#include <nspr/prglobal.h>
#include <process.h>
typedef struct {
    HANDLE hand;
    DWORD id;
} sys_thread_s;
typedef sys_thread_s *SYS_THREAD;
#endif

/*
 * systhread_start creates a thread with the given priority, will allocate
 * a stack of stksz bytes, and calls fn with arg as its argument. stksz
 * of zero will allocate a default stack size.
 *
 * XXX Priorities are system dependent
 */

SYS_THREAD systhread_start(int prio, int stksz, void (*fn)(void *), void *arg);

/*
 * systhread_current returns a pointer to the current thread.
 */
#ifdef USE_NSPR
#define systhread_current() PR_CurrentThread()
#elif defined(THREAD_WIN32)
#define systhread_current() GetCurrentThreadId()
#endif

/*
 * systhread_attach makes an existing thread an NSPR thread. Currently this
 * is used only in NT.
 */

SYS_THREAD systhread_attach();

/*
 * systhread_terminate terminates the thread that is passed in.
 */
void systhread_terminate(SYS_THREAD thr);


/*
 * systhread_sleep puts the calling thread to sleep for the given number
 * of milliseconds.
 */
void systhread_sleep(int milliseconds);

/*
 * systhread_init initializes the threading system. name is a name for the
 * program for debugging.
 */
void systhread_init(char *name);

/*
 * systhread_timerset starts or re-sets the interrupt timer for a thread
 * system. This should be considered a suggestion as most systems don't allow
 * the timer interval to be changed.
 */
#ifdef THREAD_NSPR_USER
#define systhread_timerset(usec) PR_StartEvents(usec)

#elif defined(USE_NSPR)
#define systhread_timerset(usec) (void)(usec)

#elif defined(THREAD_WIN32)
#define systhread_timerset(usec) (void)(usec)
#endif


/*
 * newkey allocates a new integer id for thread-private data. Use this
 * key to identify a variable which you want to appear differently
 * between threads, and then use setdata to associate a value with this
 * key for each thread.
 */
int systhread_newkey(void);

/*
 * Get data that has been previously associated with key in this thread.
 * Returns NULL if setkey has not been called with this key by this
 * thread previously, or the data that was previously used with setkey
 * by this thread with this key.
 */
void *systhread_getdata(int key);

/*
 * Associate data with the given key number in this thread.
 */
void systhread_setdata(int key, void *data);

#endif
#endif
