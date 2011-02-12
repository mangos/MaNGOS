/* -*- c++ -*- */

//=============================================================================
/**
 *  @file    HTTP_Handler.h
 *
 *  $Id: HTTP_Handler.h 85430 2009-05-25 11:26:46Z johnnyw $
 *
 *  @author James Hu and Irfan Pyarali
 */
//=============================================================================


#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

// = Forward declarations
class Message_Block;
class HTTP_Handler_Factory;

#include "ace/Asynch_IO.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "HTTP_Request.h"
#include "HTTP_Response.h"
#include "JAWS_IO.h"

/**
 * @class HTTP_Handler
 *
 * @brief This class is used to implement the HTTP protocol
 *
 * The HTTP_Handler class is a state based implementation of the
 * HTTP protocol. Therefore, it can be used synchronously and
 * asynchronously. It uses an abstract IO class to move between
 * different HTTP protocol states. It is up to the IO class to
 * decide on synchronous or asynchronous I/O.
 */
class HTTP_Handler : protected JAWS_IO_Handler
{
  // Friend I/O classes. Can call protected methods.
  friend class JAWS_Synch_IO;
  friend class JAWS_Synch_IO_No_Cache;
  friend class JAWS_Asynch_IO;

  // Factories
  friend class Asynch_HTTP_Handler_Factory;
  friend class Synch_HTTP_Handler_Factory;
  friend class No_Cache_Synch_HTTP_Handler_Factory;

public:
  /**
   * The handler is initialized with a connection <handle> of a new
   * client and any <initial_data> that came across. The
   * <initial_data> block will be of MAX_REQUEST_SIZE and the number
   * of bytes in <initial_data> can be found from
   * <initial_data>.length ()
   */
  virtual void open (ACE_HANDLE handle,
                     ACE_Message_Block &initial_data);

protected:
  /// The constructor is passed the factory that created <this> and the
  /// IO mechanism that the handler should use.
  HTTP_Handler (JAWS_IO &io,
                HTTP_Handler_Factory &factory);

  /// Destructor
  virtual ~HTTP_Handler (void);

  /// This method is called by the framework when there is a timeout.
  virtual void timeout (void);

  /**
   * This is the termination state of the handler. After successful or
   * unsuccessful completions, the handler will end up in this state
   * (method).
   */
  virtual void done (void);

  /// Request too long.
  virtual void request_too_long (void);

  /// Reference to the creating factory.
  HTTP_Handler_Factory &factory_;

protected:
  // = Completion methods inherited from <JAWS_IO_Handler>.

  virtual void read_complete (ACE_Message_Block &data);
  virtual void read_error (void);
  virtual void transmit_file_complete (void);
  virtual void transmit_file_error (int result);
  virtual void receive_file_complete (void);
  virtual void receive_file_error (int result);
  virtual void write_error (void);
  virtual void confirmation_message_complete (void);
  virtual void error_message_complete (void);

public:
  enum
  {
    MAX_SOCKBUFSIZE = 64 * 1024,
    MAX_REQUEST_SIZE = 8192,
    METHODSIZ = 10,
    VERSIONSIZ = 10
  };

private:
  /// This points to the request sent by the client
  ACE_Message_Block *request_data_;

  /// I/O handle to the client
  ACE_HANDLE handle_;

  HTTP_Request request_;
  HTTP_Response response_;

  /// IO class used by the handler
  JAWS_IO &io_;
};

/**
 * @class HTTP_Handler_Factory
 *
 * @brief This class is used to create new HTTP handlers
 *
 * This is an abstract factory for creating new HTTP handlers.
 */
class HTTP_Handler_Factory
{
public:
  /// Destructor
  virtual ~HTTP_Handler_Factory (void);

  /// This creates a new HTTP_Handler
  virtual HTTP_Handler *create_http_handler (void) = 0;

  /**
   * The HTTP handler will call this method from HTTP_Handler::done to
   * tell the factory to reap up the handler as it is now done with
   * the protocol
   */
  virtual void destroy_http_handler (HTTP_Handler &handler,
                                     JAWS_IO &io) = 0;
};

/**
 * @class Synch_HTTP_Handler_Factory
 *
 * @brief This class is used to create new HTTP handlers that will use
 * Synch IO
 *
 */
class Synch_HTTP_Handler_Factory : public HTTP_Handler_Factory
{
public:
  /// This creates a new HTTP_Handler
  HTTP_Handler *create_http_handler (void);

  /**
   * The HTTP handler will call this method from HTTP_Handler::done to
   * tell the factory to reap up the handler as it is now done with
   * the protocol
   */
  void destroy_http_handler (HTTP_Handler &handler,
                             JAWS_IO &io);
};

//--------------Added a factory for SYNCH IO without caching

/**
 * @class No_Cache_Synch_HTTP_Handler_Factory
 *
 * @brief This class is used to create new HTTP handlers that will use
 * Synch IO without caching
 *
 */
class No_Cache_Synch_HTTP_Handler_Factory : public HTTP_Handler_Factory
{
public:
  /// This creates a new HTTP_Handler
  HTTP_Handler *create_http_handler (void);

  /**
   * The HTTP handler will call this method from HTTP_Handler::done to
   * tell the factory to reap up the handler as it is now done with
   * the protocol
   */
  void destroy_http_handler (HTTP_Handler &handler,
                             JAWS_IO &io);
};

//--------------

#if defined (ACE_HAS_WIN32_OVERLAPPED_IO)
/**
 * @class Asynch_HTTP_Handler_Factory
 *
 * @brief This class is used to create new HTTP handlers that will use
 * Asynchronous IO.  This only works on Win32.
 *
 */
class Asynch_HTTP_Handler_Factory : public HTTP_Handler_Factory, public ACE_Service_Handler
{
public:
  /**
   * The HTTP handler will call this method from HTTP_Handler::done to
   * tell the factory to reap up the handler as it is now done with
   * the protocol
   */
  void destroy_http_handler (HTTP_Handler &handler,
                             JAWS_IO &io);

  /**
   * <open> is called by <ACE_Asynch_Acceptor> to initialize a new
   * instance of ACE_Service_Handler that has been created after the a
   * new connection is accepted.
   *
   * This will act as a creation point for new handlers.
   */
  virtual void open (ACE_HANDLE handle,
                     ACE_Message_Block &message_block);

private:
  /**
   * This method is private as users are not allowed to create new
   * handlers. New handlers can only be created by the framework when
   * new client connections arrive.
   */
  HTTP_Handler *create_http_handler (void);
};
#endif /* ACE_HAS_WIN32_OVERLAPPED_IO */
#endif /* HTTP_HANDLER_H */
