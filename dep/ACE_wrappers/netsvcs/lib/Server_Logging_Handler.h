/* -*- C++ -*- */

//=============================================================================
/**
 *  @file    Server_Logging_Handler.h
 *
 *  $Id: Server_Logging_Handler.h 84498 2009-02-17 18:08:47Z johnnyw $
 *
 *  @author Doug Schmidt and Per Andersson <Per.Andersson@hfera.ericsson.se>
 */
//=============================================================================


#ifndef ACE_SERVER_LOGGING_HANDLER_H
#define ACE_SERVER_LOGGING_HANDLER_H

#include "Log_Message_Receiver.h"
#include "Server_Logging_Handler_T.h"
#include "ace/svc_export.h"

// Typedefs for Logging Handlers & acceptors using a static type based
// log message receivers.

// Synched and NULL synched message receivers
typedef Static_Log_Message_Receiver<ACE_NULL_SYNCH>
        Null_Synch_Static_Receiver;
typedef Static_Log_Message_Receiver<ACE_LOGGER_SYNCH>
        Synch_Static_Receiver;

// NULL synched logging handler
typedef ACE_Server_Logging_Handler<Null_Synch_Static_Receiver>
        Null_Synch_Logging_Handler_Static_Receiver;

// synched logging handlers
typedef ACE_Server_Logging_Handler<Synch_Static_Receiver>
        Synch_Logging_Handler_Static_Receiver;
typedef ACE_Thr_Server_Logging_Handler<Synch_Static_Receiver>
        Synch_Thr_Logging_Handler_Static_Receiver;

// NULL synched logging acceptor
typedef ACE_Server_Logging_Acceptor_T<Null_Synch_Logging_Handler_Static_Receiver,
                                      Null_Synch_Static_Receiver,
                                      ACE_Schedule_All_Reactive_Strategy<Null_Synch_Logging_Handler_Static_Receiver> >
        Null_Synch_Logging_Handler_Static_Receiver_Acceptor;

// NULL synched logging acceptors
typedef ACE_Server_Logging_Acceptor_T<Synch_Logging_Handler_Static_Receiver,
                                      Synch_Static_Receiver,
                                      ACE_Schedule_All_Reactive_Strategy<Synch_Logging_Handler_Static_Receiver> >
        Synch_Logging_Handler_Static_Receiver_Acceptor;

typedef ACE_Server_Logging_Acceptor_T<Synch_Thr_Logging_Handler_Static_Receiver,
                                      Synch_Static_Receiver,
                                      ACE_Schedule_All_Threaded_Strategy<Synch_Thr_Logging_Handler_Static_Receiver> >
        Synch_Thr_Logging_Handler_Static_Receiver_Acceptor;

// typedefs for Logging Handlers & acceptors using a instance based
// log message receivers.

// Synched message receivers
typedef Log_Message_Receiver<ACE_LOGGER_SYNCH>
        Synch_Receiver;

// synched logging handlers
typedef ACE_Server_Logging_Handler<Synch_Receiver>
        Synch_Logging_Handler_Receiver;
typedef ACE_Thr_Server_Logging_Handler<Synch_Receiver>
        Synch_Thr_Logging_Handler_Receiver;

// synched logging acceptors
typedef ACE_Server_Logging_Acceptor_T<Synch_Logging_Handler_Receiver,
                                      Synch_Receiver,
                                      ACE_Schedule_All_Reactive_Strategy<Synch_Logging_Handler_Receiver> >
        Synch_Logging_Handler_Receiver_Acceptor;

typedef ACE_Server_Logging_Acceptor_T<Synch_Thr_Logging_Handler_Receiver,
                                      Synch_Receiver,
                                      ACE_Schedule_All_Threaded_Strategy<Synch_Thr_Logging_Handler_Receiver> >
        Synch_Thr_Logging_Handler_Receiver_Acceptor;


// Define external acceptors

// Acceptors that use static/type based log message receiver.
typedef Null_Synch_Logging_Handler_Static_Receiver_Acceptor
        ACE_Server_Logging_Acceptor;
typedef Synch_Thr_Logging_Handler_Static_Receiver_Acceptor
        ACE_Thr_Server_Logging_Acceptor;

ACE_SVC_FACTORY_DECLARE (ACE_Server_Logging_Acceptor)
ACE_SVC_FACTORY_DECLARE (ACE_Thr_Server_Logging_Acceptor)

#endif /* ACE_SERVER_LOGGING_HANDLER_H */
