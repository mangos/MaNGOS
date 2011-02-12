/* -*- C++ -*- */

//=============================================================================
/**
 *  @file    Log_Message_Receiver.h
 *
 *  $Id: Log_Message_Receiver.h 91626 2010-09-07 10:59:20Z johnnyw $
 *
 *  @author Per Andersson
 */
//=============================================================================

#ifndef LOG_MESSAGE_RECEIVER_H
#define LOG_MESSAGE_RECEIVER_H

#include "ace/Log_Record.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Synch_Traits.h"
#include "ace/Guard_T.h"
#if defined (ACE_HAS_THREADS)
#  include "ace/Thread_Mutex.h"
#else
#  include "ace/Null_Mutex.h"
#endif /* ACE_HAS_THREADS */

// ==========================================================================//
//------------- General Requirements on a Log Message Receiver --------------//
// ==========================================================================//
//
//  The requirements on a log manager receiver, T, are quite simple.
//  1: There must exist one "log_record" member function with the following
//     prototype:
//        void log_record(const ACE_TCHAR *hostname,
//                        ACE_Log_Record &record);
//
//  2: There must exist a public destructor.
//  3: There must exist a public copy constructor.
//  4: There must exist a default constructor. (for now)
//
//  The semantics are also simple. A log message receiver should
//  behave as an accessor object (smart pointer or envelope class).
//  It should be very cheap to copy and the should be no noticeable
//  difference when using either the new copy or the old log message
//  receiver.
//
//  Methods:
//    void log_record(const ACE_TCHAR* hostname,
//                    ACE_Log_Record& record)
//  Description:
//    Processes the log record "record" from the host "hostname"
//  Precondition:
//    hostname != 0;
//  Requirements:
//    Record must be a valid ACE_Log_Record.
//
// ==========================================================================//

// ==========================================================================//
// ------------ General Description of a Log Message Receiver -------------- //
// ==========================================================================//
//
//  Log Message Receivers, LRMs, are processing log records. It is the
//  LRM that writes a log message to stderr, stdout, a log file and maybee
//  converts some of the log messages to notifications, warnings, alarms
//  and forwards them to some operation and maintenance system (PATROL).
//
//  The client logging handler and server logging handler are responsible
//  for forwarding, receiving, framing, processing log records.
//  That is a very usable service, but it should also be possible to change
//  how log records are processed without having to rewrite code in
//  the server log handler. This code should instead be written as a
//  separate entity, a Log Message Receiver.
//
//  A simple LMR should be very easy to write but it should also
//  be possible to write more complex LMRs, like one that creates
//  a new log file each day or keeps a fixed size, round robin,
//  log file. It should also be possible to have separate LMRs
//  of the same type that uses differnt log files.
//
// ==========================================================================//


// Type based log message receiver
/**
 * @class Static_Log_Message_Receiver
 *
 * @brief Static_Log_Message_Receiver is a simple log message receiver. It
 * has no instance data and only static member
 * functions. Static/typed based receivers are best when all LMR
 * should do exactly the same thing.
 *
 * This class contains a static log_record member function that
 * prints the content of log_records on stderr.
 */
template<ACE_SYNCH_DECL>
class Static_Log_Message_Receiver
{

public:
  /// Prints the log_record to stderr using record.print (hostname, 0, stderr).
  /// Serializes the output by using a ACE_SYNCH_MUTEX.
  static void log_record(const ACE_TCHAR *hostname,
                         ACE_Log_Record &record);

  /// Prints the log_record to a user specified ostream.
  static void log_output(const ACE_TCHAR *hostname,
                         ACE_Log_Record &record,
                         ostream *output);
};

// Instance based log message receiver

// ------------------------ Log_Message_Receiver --------------------------- //
//
//  Log_Message_Receiver is little more complicated log message receiver.
//  It is instance based and have a reference counted implementation.
//  Log_Message_Receiver is the envelope class for Log_Message_Receiver_Impl.
//
// ------------------------------------------------------------------------- //


//Forward declaration
template<ACE_SYNCH_DECL> class Log_Message_Receiver_Impl;

/**
 * @class Log_Message_Receiver
 *
 * @brief Log_Message_Receiver is a little more complicated log message
 * receiver.  It is instance based and have a reference counted
 * implementation.  Log_Message_Receiver is the envelope class for
 * Log_Message_Receiver_Impl.  The difference between
 * Static_Log_Message_Receiver and Log_Message_Receiver is that is
 * possible to have instance data in Log_Message_Receiver.
 * Comment:
 * The practical usage of this is limited with the current
 * ACE_Server_Logging_Acceptor_T design. Since
 * ACE_Server_Logging_Acceptor_T will create the
 * Log_Message_Receiver using the default constructor.  The main
 * reason for inclusion right now is to ensure that the code in
 * ACE_Server_Logging_Handler_T works both with type and instance
 * based LMRs.
 *
 * This class contains a log_record member function that prints the
 * content of log_records on stderr.
 */
template<ACE_SYNCH_DECL>
class Log_Message_Receiver
{
public:
  /// Creates a new Log_Message_Receiver
  Log_Message_Receiver (void);
  Log_Message_Receiver(Log_Message_Receiver<ACE_SYNCH_USE> const &rhs);
  ~Log_Message_Receiver (void);

  void log_record (const ACE_TCHAR *hostname,
                   ACE_Log_Record &record);

  void log_output(const ACE_TCHAR *hostname,
                  ACE_Log_Record &record,
                  ostream *output);
private:
  ACE_UNIMPLEMENTED_FUNC (void operator= (const Log_Message_Receiver<ACE_SYNCH_USE> &rhs))

  // Attributes.
  Log_Message_Receiver_Impl<ACE_SYNCH_USE> *receiver_impl_;
};

/**
 * @class Log_Message_Receiver_Impl
 *
 * @brief Implementation with reference count.
 */
template<ACE_SYNCH_DECL>
class Log_Message_Receiver_Impl : private ACE_Copy_Disabled
{
public:
  // Methods for handling reference count and instance lifetime
  static Log_Message_Receiver_Impl *create (void);
  static Log_Message_Receiver_Impl *attach (Log_Message_Receiver_Impl<ACE_SYNCH_USE> *body);
  static void detach (Log_Message_Receiver_Impl<ACE_SYNCH_USE> *body);

  void log_record (const ACE_TCHAR *hostname, ACE_Log_Record &record);

  void log_output(const ACE_TCHAR *hostname,
                  ACE_Log_Record &record,
                  ostream *output);

protected:
  Log_Message_Receiver_Impl (void);
  ~Log_Message_Receiver_Impl (void);

  /// Attributes
  int count_;
  ACE_SYNCH_MUTEX_T print_lock_;

private:
#if !defined (ACE_LACKS_STATIC_DATA_MEMBER_TEMPLATES)
  static ACE_SYNCH_MUTEX_T copy_lock_;
#endif /* ACE_LACKS_STATIC_DATA_MEMBER_TEMPLATES */
};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "Log_Message_Receiver.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("Log_Message_Receiver.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif /* LOG_MESSAGE_RECEIVER_H */
