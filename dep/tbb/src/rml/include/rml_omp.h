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

// Header guard and namespace names follow OpenMP runtime conventions.

#ifndef KMP_RML_OMP_H
#define KMP_RML_OMP_H

#include "rml_base.h"

namespace __kmp {
namespace rml {

class omp_client;

//------------------------------------------------------------------------
// Classes instantiated by the server
//------------------------------------------------------------------------

//! Represents a set of worker threads provided by the server.
class omp_server: public ::rml::server {
public:
    //! A number of threads
    typedef unsigned size_type;

    //! Return the number of coins in the bank. (negative if machine is oversubscribed).
    virtual int current_balance() const RML_PURE(int);
  
    //! Request n coins.  Returns number of coins granted. Oversubscription amount if negative.
    /** Always granted if is_strict is true.
        - Positive or zero result indicates that the number of coins was taken from the bank.
        - Negative result indicates that no coins were taken, and that the bank has deficit 
          by that amount and the caller (if being a good citizen) should return that many coins.
     */
    virtual int try_increase_load( size_type /*n*/, bool /*strict*/ ) RML_PURE(size_type)

    //! Return n coins into the bank.
    virtual void decrease_load( size_type /*n*/ ) RML_PURE(void);

    //! Convert n coins into n threads.
    /** When a thread returns, it is converted back into a coin and the coin is returned to the bank. */
    virtual void get_threads( size_type /*m*/, void* /*cookie*/, job* /*array*/[] ) RML_PURE(void);

    /** Putting a thread to sleep - convert a thread into a coin
        Waking up a thread        - convert a coin into a thread
      
       Note: conversion between a coin and a thread does not affect the accounting.
     */
};


//------------------------------------------------------------------------
// Classes (or base classes thereof) instantiated by the client
//------------------------------------------------------------------------

class omp_client: public ::rml::client {
public:
    //! Called by server thread when it runs its part of a parallel region.  
    /** The index argument is a 0-origin index of this thread within the array
        returned by method get_threads.  Server decreases the load by 1 after this method returns. */
    virtual void process( job&, void* /*cookie*/, size_type /*index*/ ) RML_PURE(void)
};

/** Client must ensure that instance is zero-inited, typically by being a file-scope object. */
class omp_factory: public ::rml::factory {

    //! Pointer to routine that creates an RML server.
    status_type (*my_make_server_routine)( omp_factory&, omp_server*&, omp_client& );

    //! Pointer to routine that returns server version info.
    void (*my_call_with_server_info_routine)( ::rml::server_info_callback_t cb, void* arg );

public:
    typedef ::rml::versioned_object::version_type version_type;
    typedef omp_client client_type;
    typedef omp_server server_type;

    //! Open factory.
    /** Dynamically links against RML library. 
        Returns st_success, st_incompatible, or st_not_found. */
    status_type open();

    //! Factory method to be called by client to create a server object.
    /** Factory must be open. 
        Returns st_success or st_incompatible . */
    status_type make_server( server_type*&, client_type& );

    //! Close factory.
    void close();

    //! Call the callback with the server build info.
    void call_with_server_info( ::rml::server_info_callback_t cb, void* arg ) const;
};

} // namespace rml
} // namespace __kmp

#endif /* KMP_RML_OMP_H */
