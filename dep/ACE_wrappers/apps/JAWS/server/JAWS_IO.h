/* -*- c++ -*- */

//=============================================================================
/**
 *  @file    JAWS_IO.h
 *
 *  $Id: JAWS_IO.h 85433 2009-05-25 14:42:29Z johnnyw $
 *
 *  @author James Hu
 */
//=============================================================================


#ifndef JAWS_IO_H
#define JAWS_IO_H

#include "ace/ACE.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Asynch_IO.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

class JAWS_IO_Handler;


/**
 * @class JAWS_IO
 *
 * @brief This class defines the abstract interface for an I/O class in
 * the context of Web-likes servers
 *
 * An I/O class should have the following interface. Derived
 * classes will define the exactly how the I/O will take place
 * (Asynchronous, Synchronous, Reactive)
 */
class JAWS_IO
{
public:
  JAWS_IO (void);
  virtual ~JAWS_IO (void);
  void handler (JAWS_IO_Handler *handler);

  virtual void handle (ACE_HANDLE h) = 0;
  virtual ACE_HANDLE handle (void) const = 0;

  // James, please add documentation here.

  /// read from the handle size bytes into the message block.
  virtual void read (ACE_Message_Block& mb, int size) = 0;

  /// send header, filename, trailer to the handle.
  virtual void transmit_file (const char *filename,
                              const char *header,
                              int header_size,
                              const char *trailer,
                              int trailer_size) = 0;

  /// read data from the handle and store in filename.
  virtual void receive_file (const char *filename,
                             void *initial_data,
                             int initial_data_length,
                             int entire_length) = 0;

  /// send a confirmation message to the handle.
  virtual void send_confirmation_message (const char *buffer, int length) = 0;

  /// send an error message to the handle.
  virtual void send_error_message (const char *buffer, int length) = 0;

protected:
  JAWS_IO_Handler *handler_;
};

/**
 * @class JAWS_IO_Handler
 *
 * @brief This class defines the abstract interface for an I/O handler class in
 * the context of Web-likes servers
 *
 */
class JAWS_IO_Handler
{
public:

  /// Destructor.
  virtual ~JAWS_IO_Handler (void);

  /// This method is called by the IO class when new client data shows
  /// up.
  virtual void read_complete (ACE_Message_Block &data) = 0;

  /// This method is called by the IO class when there was an error in
  /// reading new data from the client.
  virtual void read_error (void) = 0;

  /// This method is called by the IO class when the requested file has
  /// been successfully transmitted to the client.
  virtual void transmit_file_complete (void) = 0;

  /// This method is called by the IO class when there was an error in
  /// transmitting the requested file to the client.
  virtual void transmit_file_error (int result) = 0;

  /// This method is called by the IO class when the requested file has
  /// been successfully received from the client.
  virtual void receive_file_complete (void) = 0;

  /// This method is called by the IO class when there was an error in
  /// receiving the requested file from the client.
  virtual void receive_file_error (int result) = 0;

  /// This method is called by the IO class when there was an error in
  /// writing data to the client.
  virtual void write_error (void) = 0;

  /// This method is called by the IO class when the confirmation
  /// message has been delivered to the client.
  virtual void confirmation_message_complete (void) = 0;

  /// This method is called by the IO class when the error message has
  /// been delivered to the client.
  virtual void error_message_complete (void) = 0;

};

/**
 * @class JAWS_Synch_IO
 *
 * @brief This class defines the interface for a Synchronous I/O class.
 *
 */
class JAWS_Synch_IO : public JAWS_IO
{
public:
  JAWS_Synch_IO (void);

  ~JAWS_Synch_IO (void);

  virtual void handle (ACE_HANDLE h);
  virtual ACE_HANDLE handle (void) const;

  void read (ACE_Message_Block& mb, int size);

  void transmit_file (const char *filename,
                      const char *header,
                      int header_size,
                      const char *trailer,
                      int trailer_size);

  void receive_file (const char *filename,
                     void *initial_data,
                     int initial_data_length,
                     int entire_length);

  void send_confirmation_message (const char *buffer,
                                  int length);

  void send_error_message (const char *buffer,
                           int length);

protected:
  virtual void send_message (const char *buffer,
                             int length);

  ACE_HANDLE handle_;
};

// This only works on Win32
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO)

/**
 * @class JAWS_Asynch_IO
 *
 * @brief This class defines the interface for a Asynchronous I/O class.
 *
 */
class JAWS_Asynch_IO : public JAWS_IO, public ACE_Handler
{
public:
  JAWS_Asynch_IO (void);

  ~JAWS_Asynch_IO (void);

  virtual void handle (ACE_HANDLE h) { ACE_Handler::handle (h); };
  virtual ACE_HANDLE handle (void) const { return ACE_Handler::handle (); };

  void read (ACE_Message_Block& mb, int size);

  void transmit_file (const char *filename,
                      const char *header,
                      int header_size,
                      const char *trailer,
                      int trailer_size);

  void receive_file (const char *filename,
                     void *initial_data,
                     int initial_data_length,
                     int entire_length);

  void send_confirmation_message (const char *buffer,
                                  int length);

  void send_error_message (const char *buffer,
                           int length);

protected:
  enum Message_Types
  {
    CONFORMATION,
    ERROR_MESSAGE
  };

  virtual void send_message (const char *buffer,
                             int length,
                             int act);

  /// This method will be called when an asynchronous read completes on
  /// a stream.
  virtual void handle_read_stream (const ACE_Asynch_Read_Stream::Result &result);

  /// This method will be called when an asynchronous write completes
  /// on a stream.
  virtual void handle_write_stream (const ACE_Asynch_Write_Stream::Result &result);

  /// This method will be called when an asynchronous transmit file
  /// completes.
  virtual void handle_transmit_file (const ACE_Asynch_Transmit_File::Result &result);
};

#endif /* ACE_HAS_WIN32_OVERLAPPED_IO */


//-------------------Adding SYNCH IO no Caching

/**
 * @class JAWS_Synch_IO_No_Cache
 *
 * @brief This class defines the interface for a Synchronous I/O class,
 * however in this class we do not use any caching.
 *
 * Wondering how this is useful?
 * The ACE_Filecache ACE_NOMAP option is broken and even if it were not, there
 * are other use cases in which we want to avoid caching altogether. For example,
 * we use JAWS in conjunction with the CIAO Repository Manager, however the two
 * do not have any explicit knowledge of each other. Therefore if the RM tried
 * to remove a package and its files from disk, its operation would [partially]
 * fail if JAWS still holds some of the files in its cache.
 */
class JAWS_Synch_IO_No_Cache : public JAWS_IO
{
public:
  JAWS_Synch_IO_No_Cache (void);

  ~JAWS_Synch_IO_No_Cache (void);

  virtual void handle (ACE_HANDLE h);
  virtual ACE_HANDLE handle (void) const;

  void read (ACE_Message_Block& mb, int size);

  void transmit_file (const char *filename,
                      const char *header,
                      int header_size,
                      const char *trailer,
                      int trailer_size);

  void receive_file (const char *filename,
                     void *initial_data,
                     int initial_data_length,
                     int entire_length);

  void send_confirmation_message (const char *buffer,
                                  int length);

  void send_error_message (const char *buffer,
                           int length);

protected:
  virtual void send_message (const char *buffer, int length);

  ACE_HANDLE handle_;
};

//-------------------

#endif /* JAWS_IO_H */

