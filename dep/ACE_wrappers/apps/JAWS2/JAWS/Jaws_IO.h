/* -*- c++ -*- */
// Hey, Emacs!  This is a C++ file!
// $Id: Jaws_IO.h 85415 2009-05-22 07:26:32Z johnnyw $

// ============================================================================
//
// = LIBRARY
//   jaws
//
// = FILENAME
//    IO.h
//
// = AUTHOR
//    James Hu
//
// ============================================================================

#ifndef JAWS_IO_H
#define JAWS_IO_H

#include "ace/ACE.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Asynch_IO.h"
#include "ace/SOCK_Stream.h"
#include "ace/Singleton.h"
#include "ace/Synch_Traits.h"

#include "JAWS/Export.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

class JAWS_IO_Handler;
class JAWS_IO_Acceptor;

class JAWS_Export JAWS_IO
  // = TITLE
  //
  //     This class defines the abstract interface for an I/O class in
  //     the context of Web-likes servers
  //
  // = DESCRIPTION
  //
  //     An I/O class should have the following interface. Derived
  //     classes will define the exactly how the I/O will take place
  //     (Asynchronous, Synchronous, Reactive)
{
public:
  JAWS_IO (void);
  virtual ~JAWS_IO (void);

  //  void acceptor (JAWS_IO_Acceptor *acceptor);
  //  void handler (JAWS_IO_Handler *handler);
  //  void handle (ACE_HANDLE h);
  //  ACE_HANDLE handle (void);

  // James, please add documentation here.

  virtual void accept (JAWS_IO_Handler *ioh,
                       ACE_Message_Block *mb = 0,
                       unsigned int size = 0) = 0;
  // accept a passive connection

  virtual void read (JAWS_IO_Handler *ioh,
                     ACE_Message_Block *mb,
                     unsigned int size) = 0;
  // read from the handle size bytes into the message block.

  virtual void transmit_file (JAWS_IO_Handler *ioh,
                              ACE_HANDLE file,
                              const char *header,
                              unsigned int header_size,
                              const char *trailer,
                              unsigned int trailer_size) = 0;
  // send header, filename, trailer to the handle.

  virtual void transmit_file (JAWS_IO_Handler *ioh,
                              const char *filename,
                              const char *header,
                              unsigned int header_size,
                              const char *trailer,
                              unsigned int trailer_size) = 0;
  // send header, filename, trailer to the handle.

  virtual void receive_file (JAWS_IO_Handler *ioh,
                             const char *filename,
                             void *initial_data,
                             unsigned int initial_data_length,
                             unsigned int entire_length) = 0;
  // read data from the handle and store in filename.

  virtual void send_confirmation_message (JAWS_IO_Handler *ioh,
                                          const char *buffer,
                                          unsigned int length) = 0;
  // send a confirmation message to the handle.

  virtual void send_error_message (JAWS_IO_Handler *ioh,
                                   const char *buffer,
                                   unsigned int length) = 0;
  // send an error message to the handle.

protected:
  ACE_HANDLE handle_;
  JAWS_IO_Handler *handler_;
  ACE_INET_Addr *inet_addr_;
  JAWS_IO_Acceptor *acceptor_;
};

class JAWS_Export JAWS_Synch_IO : public JAWS_IO
  // = TITLE
  //
  //     This class defines the interface for a Synchronous I/O class.
  //
  // = DESCRIPTION
{
public:
  JAWS_Synch_IO (void);

  virtual ~JAWS_Synch_IO (void);

  virtual void accept (JAWS_IO_Handler *ioh,
                       ACE_Message_Block *mb = 0,
                       unsigned int size = 0);

  virtual void read (JAWS_IO_Handler *ioh,
                     ACE_Message_Block *mb,
                     unsigned int size);

  virtual void transmit_file (JAWS_IO_Handler *ioh,
                              ACE_HANDLE handle,
                              const char *header,
                              unsigned int header_size,
                              const char *trailer,
                              unsigned int trailer_size);

  virtual void transmit_file (JAWS_IO_Handler *ioh,
                              const char *filename,
                              const char *header,
                              unsigned int header_size,
                              const char *trailer,
                              unsigned int trailer_size);

  virtual void receive_file (JAWS_IO_Handler *ioh,
                             const char *filename,
                             void *initial_data,
                             unsigned int initial_data_length,
                             unsigned int entire_length);

  virtual void send_confirmation_message (JAWS_IO_Handler *ioh,
                                          const char *buffer,
                                          unsigned int length);

  virtual void send_error_message (JAWS_IO_Handler *ioh,
                                   const char *buffer,
                                   unsigned int length);

protected:
  virtual void send_message (JAWS_IO_Handler *ioh,
                             const char *buffer,
                             unsigned int length);
};

typedef ACE_Singleton<JAWS_Synch_IO, ACE_SYNCH_MUTEX>
        JAWS_Synch_IO_Singleton;

// This only works on asynch I/O-capable systems.
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)

class JAWS_Export JAWS_Asynch_IO : public JAWS_IO
  // = TITLE
  //
  //     This class defines the interface for a Asynchronous I/O class.
  //
  // = DESCRIPTION
{
public:
  JAWS_Asynch_IO (void);

  virtual ~JAWS_Asynch_IO (void);

  virtual void accept (JAWS_IO_Handler *ioh,
                       ACE_Message_Block *mb = 0,
                       unsigned int size = 0);

  virtual void read (JAWS_IO_Handler *ioh,
                     ACE_Message_Block *mb,
                     unsigned int size);

  virtual void transmit_file (JAWS_IO_Handler *ioh,
                              ACE_HANDLE handle,
                              const char *header,
                              unsigned int header_size,
                              const char *trailer,
                              unsigned int trailer_size);

  virtual void transmit_file (JAWS_IO_Handler *ioh,
                              const char *filename,
                              const char *header,
                              unsigned int header_size,
                              const char *trailer,
                              unsigned int trailer_size);

  virtual void receive_file (JAWS_IO_Handler *ioh,
                             const char *filename,
                             void *initial_data,
                             unsigned int initial_data_length,
                             unsigned int entire_length);

  virtual void send_confirmation_message (JAWS_IO_Handler *ioh,
                                          const char *buffer,
                                          unsigned int length);

  virtual void send_error_message (JAWS_IO_Handler *ioh,
                                   const char *buffer,
                                   unsigned int length);

#ifdef ACE_LYNXOS_MAJOR
#ifdef ERROR_MESSAGE
#undef ERROR_MESSAGE
#endif /* ERROR_MESSAGE */
#endif /* ACE_LYNXOS_MAJOR */

  enum Message_Types
  {
    CONFIRMATION,
    ERROR_MESSAGE
  };

protected:

  virtual void send_message (JAWS_IO_Handler *ioh,
                             const char *buffer,
                             unsigned int length,
                             long act);
};

typedef ACE_Singleton<JAWS_Asynch_IO, ACE_SYNCH_MUTEX>
        JAWS_Asynch_IO_Singleton;

class JAWS_Export JAWS_Asynch2_IO : public JAWS_Asynch_IO
{
  // This version of Asynch_IO has a do nothing accept() implementation.
public:
  virtual void accept (JAWS_IO_Handler *ioh,
                       ACE_Message_Block *mb = 0,
                       unsigned int size = 0);
  // does nothing

};

typedef ACE_Singleton<JAWS_Asynch2_IO, ACE_SYNCH_MUTEX>
        JAWS_Asynch2_IO_Singleton;

#endif /* ACE_HAS_WIN32_OVERLAPPED_IO || ACE_HAS_AIO_CALLS */

#endif /* JAWS_IO_H */
