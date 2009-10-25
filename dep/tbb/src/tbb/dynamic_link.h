/*
    Copyright 2005-2009 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#ifndef __TBB_dynamic_link
#define __TBB_dynamic_link

// Support for dynamically linking to a shared library.
// By default, the symbols defined here go in namespace tbb::internal.
// The symbols can be put in another namespace by defining the preprocessor
// symbols OPEN_INTERNAL_NAMESPACE and CLOSE_INTERNAL_NAMESPACE to open and
// close the other namespace.  See default definition below for an example.

#ifndef OPEN_INTERNAL_NAMESPACE
#define OPEN_INTERNAL_NAMESPACE namespace tbb { namespace internal {
#define CLOSE_INTERNAL_NAMESPACE }}
#endif /* OPEN_INTERNAL_NAMESPACE */

#include <stddef.h>
#if _WIN32||_WIN64
#include <windows.h>
#endif /* _WIN32||_WIN64 */

OPEN_INTERNAL_NAMESPACE

//! Type definition for a pointer to a void somefunc(void)
typedef void (*pointer_to_handler)();

// Double cast through the void* from func_ptr in DLD macro is necessary to 
// prevent warnings from some compilers (g++ 4.1)
#if __TBB_WEAK_SYMBOLS

#define DLD(s,h) {(pointer_to_handler)&s, (pointer_to_handler*)(void*)(&h)}
//! Association between a handler name and location of pointer to it.
struct dynamic_link_descriptor {
    //! pointer to the handler
    pointer_to_handler ptr;
    //! Pointer to the handler
    pointer_to_handler* handler;
};

#else /* !__TBB_WEAK_SYMBOLS */

#define DLD(s,h) {#s, (pointer_to_handler*)(void*)(&h)}
//! Association between a handler name and location of pointer to it.
struct dynamic_link_descriptor {
    //! Name of the handler
    const char* name;
    //! Pointer to the handler
    pointer_to_handler* handler;
};

#endif /* !__TBB_WEAK_SYMBOLS */

#if _WIN32||_WIN64
typedef HMODULE dynamic_link_handle;
#else 
typedef void* dynamic_link_handle;
#endif /* _WIN32||_WIN64 */

//! Fill in dynamically linked handlers.
/** 'n' is the length of the array descriptors[].
    'required' is the number of the initial entries in the array descriptors[] 
    that have to be found in order for the call to succeed. If the library and 
    all the required handlers are found, then the corresponding handler pointers 
    are set, and the return value is true.  Otherwise the original array of 
    descriptors is left untouched and the return value is false. **/
bool dynamic_link( const char* libraryname, 
                   const dynamic_link_descriptor descriptors[], 
                   size_t n, 
                   size_t required = ~(size_t)0,
                   dynamic_link_handle* handle = 0 );

void dynamic_unlink( dynamic_link_handle handle );

CLOSE_INTERNAL_NAMESPACE

#endif /* __TBB_dynamic_link */
