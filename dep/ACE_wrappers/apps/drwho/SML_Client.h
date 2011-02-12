/* -*- C++ -*- */
// $Id: SML_Client.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    SML_Client.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _SML_CLIENT_H
#define _SML_CLIENT_H

#include "SM_Client.h"
#include "SML_Server.h"

class SML_Client : public SM_Client
{
public:
  SML_Client (void);
  virtual ~SML_Client (void);
  virtual int receive (int timeout = 0);
  virtual int send (void);

private:
  SML_Server sml_server;
  int packet_length;
};

#endif /* _SML_CLIENT_H */
