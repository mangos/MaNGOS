/*
 * $Id: sem.h 80826 2008-03-04 14:51:23Z wotte $
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
 * sem.h: Attempt to provide multi-process semaphores across platforms
 *
 * Rob McCool
 */


#ifndef SEM_H
#define SEM_H

#include "systems.h"


/* All of the implementations currently use int as the semaphore type */
#ifdef SEM_WIN32
typedef HANDLE SEMAPHORE;
#define SEM_ERROR 0
/* That oughta hold them (I hope) */
#define SEM_MAXVALUE 32767

#else /* ! SEM_WIN32 */
typedef int SEMAPHORE;
#define SEM_ERROR -1
#endif /* SEM_WIN32 */

/*
 * sem_init creates a semaphore using the given name and unique
 * identification number. filename should be a file accessible to the
 * process. Returns SEM_ERROR on error.
 */

SEMAPHORE sem_init(char *name, int number);

/*
 * sem_terminate de-allocates the given semaphore.
 */

void sem_terminate(SEMAPHORE id);

/*
 * sem_grab attempts to gain exclusive access to the given semaphore. If
 * it can't get it, the caller will block. Returns -1 on error.
 */

int sem_grab(SEMAPHORE id);

/*
 * sem_release releases this process's exclusive control over the given
 * semaphore. Returns -1 on error.
 */

int sem_release(SEMAPHORE id);


#endif
