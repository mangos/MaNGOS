/* -*- c++ -*- */
// $Id: HTTP_Service_Handler.h 91863 2010-09-20 13:33:11Z johnnyw $

#ifndef JAWS_HTTP_SERVICE_HANDLER_H
#define JAWS_HTTP_SERVICE_HANDLER_H

#include "ace/Synch.h"
#include "ace/Acceptor.h"
#include "ace/Svc_Handler.h"
#include "ace/SOCK_Acceptor.h"

#include "jaws3/Protocol_Handler.h"

#include "HTTP_Data.h"

class JAWS_HTTP_Service_Handler
  : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
  , public JAWS_Protocol_Handler
// = TITLE
//     An HTTP Server service handler.
//
// = DESCRIPTION
//     This class is responsible for initializing the JAWS_Protocol_Handler
//     with the correct state and data so that the HTTP Server can
//     be properly serviced by the underlying framework.
//
//     This class should perhaps be factored out into a template so that
//     there is less programming effort.
{
public:

  JAWS_HTTP_Service_Handler (void);

  int open (void *);

  int close (unsigned long);

private:

  JAWS_HTTP_Data data_;

};


class ACE_Svc_Export JAWS_HTTP_Acceptor
  : public ACE_Acceptor<JAWS_HTTP_Service_Handler, ACE_SOCK_ACCEPTOR>
// = TITLE
//     An HTTP Server acceptor.
//
// = DESCRIPTION
//     The role of this class is to serve the acceptor role in the
//     acceptor pattern.  It interacts with the Reactor to perform
//     accepts asynchronously.  Upon completion, the service handler
//     is created.
{
public:

  int init (int argc, ACE_TCHAR *argv[]);

};

ACE_SVC_FACTORY_DECLARE (JAWS_HTTP_Acceptor)


#endif /* JAWS_HTTP_SERVICE_HANDLER_H */
