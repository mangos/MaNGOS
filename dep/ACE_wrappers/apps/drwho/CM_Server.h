/* -*- C++ -*- */
// $Id: CM_Server.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    CM_Server.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _CM_SERVER_H
#define _CM_SERVER_H

#include "Options.h"
#include "global.h"
#include "Comm_Manager.h"

class CM_Server : public Comm_Manager
{
  // = TITLE
  //    Provides a virtual communcations layer for the server in drwho.
public:
  CM_Server (void);
  virtual ~CM_Server (void);

  virtual int open (short port_number);
  virtual int receive (int timeout = 0);
  virtual int send (void);
  virtual int mux (char *packet, int &packet_length)   = 0;
  virtual int demux (char *packet, int &packet_length) = 0;
};

#endif /* _CM_SERVER_H */
