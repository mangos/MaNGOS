/* -*- C++ -*- */
// $Id: SM_Server.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    SM_Server.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _SM_SERVER_H
#define _SM_SERVER_H

#include "PM_Server.h"
#include "CM_Server.h"
#include "Select_Manager.h"

class SM_Server : public Select_Manager, public CM_Server
{
public:
  SM_Server (void);
  virtual ~SM_Server (void);
  virtual int mux (char *packet, int &packet_length);
  virtual int demux (char *packet, int &packet_length);

private:
  PM_Server *pm_server;
};

#endif /* _SM_SERVER_H */
