/* -*- c++ -*- */
// $Id: HTTP_States.h 91730 2010-09-13 09:31:11Z johnnyw $

#ifndef JAWS_HTTP_STATES_H
#define JAWS_HTTP_STATES_H

#include "ace/Singleton.h"

#include "jaws3/Protocol_Handler.h"

#define JAWS_HTTP_STATE_MACRO(JAWS_HTTP_STATE_NAME)              \
                                                                 \
class JAWS_HTTP_STATE_NAME : protected JAWS_Protocol_State       \
{                                                                \
public:                                                          \
                                                                 \
  int service (JAWS_Event_Completer *, JAWS_HTTP_Data *);        \
                                                                 \
  JAWS_Protocol_State * transition ( const JAWS_Event_Result &   \
                                   , JAWS_HTTP_Data *            \
                                   , void *                      \
                                   );                            \
                                                                 \
  static JAWS_Protocol_State * instance (void)                   \
  {                                                              \
    return ACE_Singleton<JAWS_HTTP_STATE_NAME, ACE_SYNCH_MUTEX>  \
           ::instance ();                                        \
  }                                                              \
                                                                 \
private:                                                         \
                                                                 \
  int service (JAWS_Event_Completer *ec, void *d)                \
  {                                                              \
    JAWS_HTTP_Data *hd = static_cast<JAWS_HTTP_Data *> (d);  \
    return this->service (ec, hd);                               \
  }                                                              \
                                                                 \
  JAWS_Protocol_State * transition ( const JAWS_Event_Result &r  \
                                   , void d*                     \
                                   , void a*                     \
                                   )                             \
  {                                                              \
    JAWS_HTTP_Data *hd = static_cast<JAWS_HTTP_Data *> (d);  \
    return this->transition (r, hd, a);                          \
  }                                                              \
                                                                 \
}

JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Read_Request);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Read_Headers);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Parse_Error);

JAWS_HTTP_STATE_MACRO (JAWS_HTTP_GET);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_HEAD);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_PUT);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_POST);

JAWS_HTTP_STATE_MACRO (JAWS_HTTP_GET_Response);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_HEAD_Response);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_PUT_Response);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_POST_Response);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Auth_Response);

JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Transmit_File);

JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Send_Message);

JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Spawn_CGI);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Send_CGI_Status);
JAWS_HTTP_STATE_MACRO (JAWS_HTTP_Wait_CGI);

JAWS_HTTP_STATE_MACRO (JAWS_HTTP_DONE);

#endif /* JAWS_HTTP_STATES_H */
