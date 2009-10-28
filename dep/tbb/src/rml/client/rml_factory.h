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

// No ifndef guard because this file is not a normal include file.

// FIXME - resolve whether _debug version of the RML should have different suffix. */

#if TBB_USE_DEBUG
#define DEBUG_SUFFIX "_debug"
#else
#define DEBUG_SUFFIX
#endif /* TBB_USE_DEBUG */

// RML_SERVER_NAME is the name of the RML server library.
#if _WIN32||_WIN64
#define RML_SERVER_NAME "irml" DEBUG_SUFFIX ".dll"
#elif __APPLE__
#define RML_SERVER_NAME "libirml" DEBUG_SUFFIX ".dylib"
#elif __linux__
#define RML_SERVER_NAME "libirml" DEBUG_SUFFIX ".so.1"
#elif __FreeBSD__ || __sun
#define RML_SERVER_NAME "libirml" DEBUG_SUFFIX ".so"
#else
#error Unknown OS
#endif

#include "library_assert.h"

const ::rml::versioned_object::version_type CLIENT_VERSION = 1;

::rml::factory::status_type FACTORY::open() {
    // Failure of following assertion indicates that factory is already open, or not zero-inited.
    LIBRARY_ASSERT( !library_handle, NULL );
    status_type (*open_factory_routine)( factory&, version_type&, version_type );
    dynamic_link_descriptor server_link_table[4] = {
        DLD(__RML_open_factory,open_factory_routine),
        MAKE_SERVER(my_make_server_routine),
        DLD(__RML_close_factory,my_wait_to_close_routine),
        GET_INFO(my_call_with_server_info_routine),
    };
    status_type result;
    dynamic_link_handle h;
    if( dynamic_link( RML_SERVER_NAME, server_link_table, 4, 4, &h ) ) {
        library_handle = h; 
        version_type server_version;
        status_type result = (*open_factory_routine)( *this, server_version, CLIENT_VERSION );
        // server_version can be checked here for incompatibility here if necessary.
        return result;
    } else {
        library_handle = NULL;
        result = st_not_found;
    }
    return result;
}

void FACTORY::close() {
    if( library_handle ) {
        (*my_wait_to_close_routine)(*this);
        dynamic_link_handle h = library_handle;
        dynamic_unlink(h);
        library_handle = NULL;
    }
}

::rml::factory::status_type FACTORY::make_server( SERVER*& s, CLIENT& c) {
    // Failure of following assertion means that factory was not successfully opened.
    LIBRARY_ASSERT( my_make_server_routine, NULL );
    return (*my_make_server_routine)(*this,s,c);
}

void FACTORY::call_with_server_info( ::rml::server_info_callback_t cb, void* arg ) const {
    // Failure of following assertion means that factory was not successfully opened.
    LIBRARY_ASSERT( my_call_with_server_info_routine, NULL );
    (*my_call_with_server_info_routine)( cb, arg );
}
