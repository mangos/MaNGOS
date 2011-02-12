/*
 * $Id: dll.h 80826 2008-03-04 14:51:23Z wotte $
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
 * dll.h: Handle dynamically linked libraries
 *
 * Rob McCool
 */

#ifndef _DLL_H
#define _DLL_H

#include "systems.h"

#if defined(DLL_CAPABLE)

/* --------------------------- Data structures ---------------------------- */


#if defined(USE_NSPR)
#include <nspr/prlink.h>
typedef int DLHANDLE;

#elif defined(DLL_DLOPEN)
#include <dlfcn.h>
typedef void *DLHANDLE;  /* DLOPEN */

#elif defined(DLL_HPSHL)
#include <dl.h>
typedef shl_t DLHANDLE;  /* HP_SHL */

#elif defined(DLL_WIN32)
typedef HINSTANCE DLHANDLE; /* WIN32 */
#endif


/* ------------------------------ Prototypes ------------------------------ */


/*
 * dll_open loads the library at the given path into memory, and returns
 * a handle to be used in later calls to dll_findsym and dll_close.
 */
#if defined(USE_NSPR)
#define dll_open(libfn) PR_LoadLibrary(libfn)

#elif defined(DLL_DLOPEN)
#define dll_open(libfn) dlopen(libfn, DLL_DLOPEN_FLAGS)

#elif defined(DLL_HPSHL)
#define dll_open(libfn) shl_load((libfn), BIND_IMMEDIATE, 0)

#elif defined(DLL_WIN32)
DLHANDLE dll_open(char *libfn);
#endif


/*
 * dll_findsym looks for a symbol with the given name in the library
 * pointed to by the given handle. Returns a pointer to the named function.
 */

#if defined(USE_NSPR)
#define dll_findsym(dlp, name) PR_FindSymbol(name)

#elif defined(DLL_DLOPEN)
#define dll_findsym(dlp, name) dlsym(dlp, name)

#elif defined(DLL_HPSHL)
void *dll_findsym(DLHANDLE dlp, char *name);

#elif defined(DLL_WIN32)
#define dll_findsym(dlp, name) GetProcAddress(dlp, name)
#endif


/*
 * dll_error returns a string describing the last error on the given handle
 */
#if defined(USE_NSPR)
#define dll_error(dlp) system_errmsg(0)

#elif defined(DLL_DLOPEN)
#define dll_error(dlp) dlerror()

#elif defined(DLL_HPSHL)
#define dll_error(dlp) system_errmsg(0)

#elif defined(DLL_WIN32)
#define dll_error(dlp) system_errmsg(0)
#endif


/*
 * dll_close closes the previously opened library given by handle
 */
#if defined(USE_NSPR)
int dll_close(void *arg);

#elif defined(DLL_DLOPEN)
#define dll_close dlclose

#elif defined (DLL_HPSHL)
#define dll_close shl_unload

#elif defined(DLL_WIN32)
#define dll_close FreeLibrary
#endif


#endif /* DLL_CAPABLE */
#endif
