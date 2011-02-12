/* -*- c++ -*- */
// $Id: Datagram.h 91743 2010-09-13 18:24:51Z johnnyw $

#ifndef JAWS_DATAGRAM_H
#define JAWS_DATAGRAM_H

#include "ace/Addr.h"
#include "ace/Synch.h"
#include "ace/Singleton.h"
#include "ace/SOCK_Dgram.h"

#include "jaws3/Export.h"
#include "jaws3/Event_Completer.h"

class JAWS_Export JAWS_Datagram_Impl
{
public:

  virtual ~JAWS_Datagram_Impl (void) {}

  virtual void accept ( const ACE_Addr &local_sap
                      , ACE_SOCK_Dgram &new_dgram
                      , JAWS_Event_Completer *completer
                      , void *act = 0
                      ) = 0;
  // The address to new_dgram is passed back as the data member of
  // the JAWS_Event_Result that is returned to the completer.

  virtual void connect ( const ACE_Addr &remote_sap
                       , ACE_SOCK_Dgram &new_dgram
                       , JAWS_Event_Completer *completer
                       , void *act = 0
                       ) = 0;
  // The address to new_dgram is passed back as the data member of
  // the JAWS_Event_Result that is returned to the completer.
  // ADDR_ANY is assumed for the local access point.

  virtual void connect ( const ACE_Addr &remote_sap
                       , ACE_SOCK_Dgram &new_dgram
                       , const ACE_Addr &local_sap
                       , JAWS_Event_Completer *completer
                       , void *act = 0
                       ) = 0;
  // The address to new_dgram is passed back as the data member of
  // the JAWS_Event_Result that is returned to the completer.
  // Use the specified local access point.

  virtual void accept ( const ACE_Addr &local_sap
                      , ACE_SOCK_Dgram &new_dgram
                      , JAWS_Event_Completer *completer
                      , const ACE_Time_Value &timeout
                      , void *act = 0
                      ) = 0;
  // The address to new_dgram is passed back as the data member of
  // the JAWS_Event_Result that is returned to the completer.

  virtual void connect ( const ACE_Addr &remote_sap
                       , ACE_SOCK_Dgram &new_dgram
                       , JAWS_Event_Completer *completer
                       , const ACE_Time_Value &timeout
                       , void *act = 0
                       ) = 0;
  // The address to new_dgram is passed back as the data member of
  // the JAWS_Event_Result that is returned to the completer.
  // ADDR_ANY is assumed for the local access point.

  virtual void connect ( const ACE_Addr &remote_sap
                       , ACE_SOCK_Dgram &new_dgram
                       , const ACE_Addr &local_sap
                       , JAWS_Event_Completer *completer
                       , const ACE_Time_Value &timeout
                       , void *act = 0
                       ) = 0;
  // The address to new_dgram is passed back as the data member of
  // the JAWS_Event_Result that is returned to the completer.
  // Use the specified local access point.

};


class JAWS_Export JAWS_Datagram
{
public:

  JAWS_Datagram (JAWS_Datagram_Impl *impl = 0);

  static JAWS_Datagram * instance (void)
  {
    return ACE_Singleton<JAWS_Datagram, ACE_SYNCH_MUTEX>::instance ();
  }

  void accept ( const ACE_Addr &local_sap
              , ACE_SOCK_Dgram &new_dgram
              , JAWS_Event_Completer *completer
              , void *act = 0
              );

  void connect ( const ACE_Addr &remote_sap
               , ACE_SOCK_Dgram &new_dgram
               , JAWS_Event_Completer *completer
               , void *act = 0
               );

  void connect ( const ACE_Addr &remote_sap
               , ACE_SOCK_Dgram &new_dgram
               , const ACE_Addr &local_sap
               , JAWS_Event_Completer *completer
               , void *act = 0
               );

  void accept ( const ACE_Addr &local_sap
              , ACE_SOCK_Dgram &new_dgram
              , JAWS_Event_Completer *completer
              , const ACE_Time_Value &timeout
              , void *act = 0
              );

  void connect ( const ACE_Addr &remote_sap
               , ACE_SOCK_Dgram &new_dgram
               , JAWS_Event_Completer *completer
               , const ACE_Time_Value &timeout
               , void *act = 0
               );

  void connect ( const ACE_Addr &remote_sap
               , ACE_SOCK_Dgram &new_dgram
               , const ACE_Addr &local_sap
               , JAWS_Event_Completer *completer
               , const ACE_Time_Value &timeout
               , void *act = 0
               );

private:

  JAWS_Datagram_Impl *impl_;

};

#endif /* JAWS_DATAGRAM_H */
