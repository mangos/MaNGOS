/* -*- c++ -*- */
// Hey, Emacs!  This is a C++ file!
// $Id: IO_Handler.h 80826 2008-03-04 14:51:23Z wotte $

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

#ifndef JAWS_IO_HANDLER_H
#define JAWS_IO_HANDLER_H

#include "ace/Asynch_IO.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Singleton.h"
#include "ace/Synch_Traits.h"
#include "ace/RW_Thread_Mutex.h"

#include "JAWS/Export.h"

// #include "JAWS/IO.h"
class JAWS_IO;
class JAWS_Synch_IO;
class JAWS_Asynch_IO;
class JAWS_IO_Handler;
class JAWS_IO_Handler_Factory;
class JAWS_Data_Block;
class JAWS_Pipeline_Handler;
class JAWS_Waiter;

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

class JAWS_Export JAWS_Abstract_IO_Handler
  // = TITLE
  //
  //     This class defines the abstract interface for an I/O handler
  //     class in the context of Web-likes servers
  //
  // = DESCRIPTION
{
public:
  virtual ~JAWS_Abstract_IO_Handler (void);

  virtual void task (JAWS_Pipeline_Handler *ph) = 0;
  virtual JAWS_Pipeline_Handler *task (void) = 0;

  virtual void message_block (JAWS_Data_Block *mb) = 0;
  virtual JAWS_Data_Block *message_block (void) = 0;

  virtual void accept_complete (ACE_HANDLE handle) = 0;
  // This method is called by the IO class when new passive connection has
  // been established.

  virtual void accept_error (void) = 0;
  // This method is called by the IO class when new passive connection has
  // been established.

#if 0
  virtual void connect_complete (ACE_Message_Block *) = 0;
  // This method is called by the IO class when new active connection has
  // been established.

  virtual void connect_error (ACE_Message_Block *) = 0;
  // This method is called by the IO class when new active connection has
  // been established.
#endif

  virtual void read_complete (ACE_Message_Block *data) = 0;
  // This method is called by the IO class when new client data shows
  // up.

  virtual void read_error (void) = 0;
  // This method is called by the IO class when there was an error in
  // reading new data from the client.

  virtual void transmit_file_complete (void) = 0;
  // This method is called by the IO class when the requested file has
  // been successfully transmitted to the client.

  virtual void transmit_file_error (int result) = 0;
  // This method is called by the IO class when there was an error in
  // transmitting the requested file to the client.

  virtual void receive_file_complete (void) = 0;
  // This method is called by the IO class when the requested file has
  // been successfully received from the client.

  virtual void receive_file_error (int result) = 0;
  // This method is called by the IO class when there was an error in
  // receiving the requested file from the client.

  virtual void write_error (void) = 0;
  // This method is called by the IO class when there was an error in
  // writing data to the client.

  virtual void confirmation_message_complete (void) = 0;
  // This method is called by the IO class when the confirmation
  // message has been delivered to the client.

  virtual void error_message_complete (void) = 0;
  // This method is called by the IO class when the error message has
  // been delivered to the client.

  virtual JAWS_IO_Handler_Factory *factory (void) = 0;
  // Returns the factory for this IO handler

  virtual ACE_HANDLE handle (void) const = 0;
  // Returns the socket handle for this handler

  virtual void done (void) = 0;
  // Cleans up the handler.

  virtual int status (void) = 0;
  // Returns the status of the handler

  virtual void idle (void) = 0;
  // puts handler in an idle state

  enum { IDLE = 0, IDLE_A = 1,
         ACCEPT_OK = 2, ACCEPT_OK_A = 3,
         ACCEPT_ERROR = 4, ACCEPT_ERROR_A = 5,
         READ_OK = 6, READ_OK_A = 7,
         READ_ERROR = 8, READ_ERROR_A = 9,
         WRITE_OK = 10, WRITE_OK_A = 11,
         WRITE_ERROR = 12, WRITE_ERROR_A = 13,
         TRANSMIT_OK = 14, TRANSMIT_OK_A = 15,
         TRANSMIT_ERROR = 16, TRANSMIT_ERROR_A = 17,
         RECEIVE_OK = 18, RECEIVE_OK_A = 19,
         RECEIVE_ERROR = 20, RECEIVE_ERROR_A = 21 };
  // The different states of the handler

};

#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined(ACE_HAS_AIO_CALLS)

// Forward reference.
class JAWS_Asynch_IO_Handler;

class JAWS_Export JAWS_Asynch_Handler : public ACE_Service_Handler
{
public:
  JAWS_Asynch_Handler (void);
  virtual ~JAWS_Asynch_Handler (void);

  virtual void handle_read_stream (const ACE_Asynch_Read_Stream::Result
                                   &result);
  // This method will be called when an asynchronous read completes on
  // a stream.

  virtual void handle_write_stream (const ACE_Asynch_Write_Stream::Result
                                    &result);
  // This method will be called when an asynchronous write completes
  // on a stream.

  virtual void handle_transmit_file (const ACE_Asynch_Transmit_File::Result
                                     &result);
  // This method will be called when an asynchronous transmit file
  // completes.

  virtual void handle_accept (const ACE_Asynch_Accept::Result &result);
  // This method will be called when an asynchronous accept completes.

  virtual void handler (JAWS_Asynch_IO_Handler *ioh);
  virtual JAWS_Asynch_IO_Handler * handler (void);

  virtual void dispatch_handler (void);

  virtual void open (ACE_HANDLE h, ACE_Message_Block &mb);
  // Call back entry point for ACE_Asynch_Acceptor

  virtual void act (const void *act_ref);
  // Receives the ACT.

  //virtual ACE_HANDLE handle (void) const;

private:
  JAWS_Asynch_IO_Handler *ioh_;
};
#endif /* defined(ACE_HAS_WIN32_OVERLAPPED_IO) || defined(ACE_HAS_AIO_CALLS) */


class JAWS_Export JAWS_IO_Handler : public JAWS_Abstract_IO_Handler
{
public:
  JAWS_IO_Handler (JAWS_IO_Handler_Factory *factory);
  virtual ~JAWS_IO_Handler (void);

  // Inherited from JAWS_IO_Handler

  virtual void accept_complete (ACE_HANDLE handle);
  virtual void accept_error (void);
  virtual void read_complete (ACE_Message_Block *data);
  virtual void read_error (void);
  virtual void transmit_file_complete (void);
  virtual void transmit_file_error (int result);
  virtual void receive_file_complete (void);
  virtual void receive_file_error (int result);
  virtual void write_error (void);
  virtual void confirmation_message_complete (void);
  virtual void error_message_complete (void);

  virtual JAWS_IO_Handler_Factory *factory (void);
  virtual ACE_HANDLE handle (void) const;

  virtual void done (void);
  virtual int status (void);
  virtual void idle (void);

  virtual void acquire (void);
  virtual void lock (void);
  virtual void release (void);

  virtual void task (JAWS_Pipeline_Handler *ph);
  virtual JAWS_Pipeline_Handler *task (void);

  virtual void message_block (JAWS_Data_Block *mb);
  virtual JAWS_Data_Block *message_block (void);

protected:
  int status_;
  // The state of the handler.

  JAWS_Data_Block *mb_;
  // This maintains the state of the request.

  ACE_HANDLE handle_;
  // The socket handle returned from accept.

  JAWS_Pipeline_Handler *task_;
  // This is a reference to the next stage of the pipeline when the IO
  // request completes.

  JAWS_IO_Handler_Factory *factory_;
  // The reference to the handler's factory.
};

class JAWS_Export JAWS_IO_Handler_Factory
{
public:
  virtual ~JAWS_IO_Handler_Factory (void);
  // Destructor

  virtual JAWS_IO_Handler *create_io_handler (void);
  // This creates a new JAWS_IO_Handler

  virtual void destroy_io_handler (JAWS_IO_Handler *handler);
  // This deletes a JAWS_IO_Handler
};

typedef JAWS_IO_Handler JAWS_Synch_IO_Handler;
typedef JAWS_IO_Handler_Factory JAWS_Synch_IO_Handler_Factory;

typedef ACE_Singleton<JAWS_Synch_IO_Handler_Factory, ACE_SYNCH_MUTEX>
        JAWS_Synch_IO_Handler_Factory_Singleton;

#if defined(ACE_HAS_WIN32_OVERLAPPED_IO) || defined(ACE_HAS_AIO_CALLS)

class JAWS_Export JAWS_Asynch_IO_Handler_Factory : public JAWS_IO_Handler_Factory
{
public:
  virtual ~JAWS_Asynch_IO_Handler_Factory (void);
  // Destructor

  virtual JAWS_IO_Handler *create_io_handler (void);
  // This creates a new JAWS_IO_Handler

  virtual void destroy_io_handler (JAWS_IO_Handler *handler);
  // This deletes a JAWS_IO_Handler
};

class JAWS_Export JAWS_Asynch_IO_Handler : public JAWS_IO_Handler
{
friend class JAWS_Asynch_Handler;
friend class JAWS_Asynch_IO_Handler_Factory;
friend class JAWS_Waiter;

  // Provide implementations for the common functions.
public:
  explicit JAWS_Asynch_IO_Handler (JAWS_Asynch_IO_Handler_Factory *factory);
  virtual ~JAWS_Asynch_IO_Handler (void);

  virtual ACE_Handler *handler (void);

  virtual void acquire (void);
  virtual void lock (void);
  virtual void release (void);

protected:

  JAWS_Asynch_Handler *handler_;
  ACE_SYNCH_RW_MUTEX count_;
};

#else

typedef JAWS_IO_Handler JAWS_Asynch_IO_Handler;
typedef JAWS_IO_Handler_Factory JAWS_Asynch_IO_Handler_Factory;

#endif /* defined(ACE_HAS_WIN32_OVERLAPPED_IO) || defined(ACE_HAS_AIO_CALLS) */

typedef ACE_Singleton<JAWS_Asynch_IO_Handler_Factory, ACE_SYNCH_MUTEX>
        JAWS_Asynch_IO_Handler_Factory_Singleton;

#endif /* JAWS_IO_HANDLER_H */
