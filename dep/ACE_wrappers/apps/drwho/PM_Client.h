/* -*- C++ -*- */
// $Id: PM_Client.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    PM_Client.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _PM_CLIENT_H
#define _PM_CLIENT_H

#include "Protocol_Manager.h"

class PM_Client : public Protocol_Manager
{
  // = TITLE
  //   Provides the client side of the friend manager lookup table abstraction.
public:
  PM_Client (void);
  virtual ~PM_Client (void);

  virtual int encode (char *packet, int &total_bytes) = 0;
  virtual int decode (char *packet, int &total_bytes) = 0;
  virtual void process (void);

protected:
  int max_key_length;

  virtual char *handle_protocol_entries (const char *cp,
                                         const char *key_name1,
                                         const char *key_name2 = 0);

  virtual Protocol_Record *insert_protocol_info (Protocol_Record &protocol_record);
};

#endif /* _PM_CLIENT_H */

