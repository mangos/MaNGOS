/*
 * $Id: shmem.h 80826 2008-03-04 14:51:23Z wotte $
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
 * shmem.h: Portable abstraction for memory shared among a server's workers
 *
 * Rob McCool
 */


#ifndef _SHMEM_H
#define _SHMEM_H

#include "netsite.h"
#include "systems.h"


/* --------------------------- Data structures ---------------------------- */


#if defined (SHMEM_UNIX_MMAP) || defined (SHMEM_WIN32_MMAP)
#include "file.h" /* SYS_FILE */

typedef struct {
    void *data;   /* the data */
#ifdef SHMEM_WIN32_MMAP
    HANDLE fdmap;
#endif /* SHMEM_WIN32_MMAP */
    int size;     /* the maximum length of the data */

    char *name;   /* internal use: filename to unlink if exposed */
    SYS_FILE fd;  /* internal use: file descriptor for region */
} shmem_s;


/* ------------------------------ Prototypes ------------------------------ */


/*
 * shmem_alloc allocates a region of shared memory of the given size, using
 * the given name to avoid conflicts between multiple regions within the
 * program. The region will not be automatically grown if its boundaries
 * are over-run, use shmem_realloc for that.
 *
 * If expose is non-zero and the underlying system supports it, the
 * file used to create the shared region will be visible to other processes
 * running on the system.
 *
 * name should be unique to the program which calls this routine, otherwise
 * conflicts will arise.
 *
 * Returns a new shared memory region, with the data element being a
 * pointer to the shared memory. This function must be called before any
 * daemon workers are spawned, in order for the handle to the shared region
 * to be inherited by the children.
 *
 * Because of the requirement that the region must be inherited by the
 * children, the region cannot be re-allocated with a larger size when
 * necessary.
 */
shmem_s *shmem_alloc(char *name, int size, int expose);


/*
 * shmem_free de-allocates the specified region of shared memory.
 */
void shmem_free(shmem_s *region);

#endif  /* SHMEM_UNIX_MMAP || SHMEM_WIN32_MMAP */


#endif
