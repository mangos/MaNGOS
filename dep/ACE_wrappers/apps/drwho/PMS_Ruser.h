/* -*- C++ -*- */
// $Id: PMS_Ruser.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    PMS_Ruser.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _PMS_RUSER_H
#define _PMS_RUSER_H

#include "PM_Server.h"

class PMS_Ruser : public PM_Server
{
  // = TITLE
  //   Provides the server's lookup table abstraction for `ruser' users...

public:
  PMS_Ruser (void);

protected:
  virtual char *handle_protocol_entries (char *bp, Drwho_Node *hp);
  virtual Protocol_Record *insert_protocol_info (Protocol_Record &protocol_record);
  virtual int encode (char *packet, int &total_bytes);
  virtual int decode (char *packet, int &total_bytes);
};

#endif /* _PMS_RUSER_H */
