/* -*- C++ -*- */
// $Id: PM_Server.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    PM_Server.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _PM_SERVER_H
#define _PM_SERVER_H

#include "Protocol_Manager.h"

class PM_Server : public Protocol_Manager
{
  // = TITLE
  //   Handle the server's lookup table abstraction.

public:
  PM_Server (void);
  virtual ~PM_Server (void);

  virtual int encode (char *packet, int &total_bytes) = 0;
  virtual int decode (char *packet, int &total_bytes) = 0;
  virtual int process (void);

protected:
  virtual char *handle_protocol_entries (char *bp,
                                         Drwho_Node *hp);
  virtual Protocol_Record *insert_protocol_info (Protocol_Record &protocol_record);
};

#endif /* _PM_SERVER_H */
