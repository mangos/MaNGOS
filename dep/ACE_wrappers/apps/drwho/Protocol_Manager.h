/* -*- C++ -*- */
// $Id: Protocol_Manager.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    Protocol_Manager.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _PROTOCOL_MANAGER_H
#define _PROTOCOL_MANAGER_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Options.h"
#include "Search_Struct.h"
#include "Protocol_Record.h"

class Protocol_Manager
{
  // = TITLE
  //   A base class that consolidates friend management functionality
  //   shared by both clients and servers.
public:
  Protocol_Manager (void);
  virtual ~Protocol_Manager (void);

  virtual int encode (char *packet, int &total_bytes) = 0;
  virtual int decode (char *packet, int &total_bytes) = 0;

protected:
  int total_users;
  Search_Struct *ss;

  int friend_count (void);

  Drwho_Node *get_drwho_node (char *host_name, Drwho_Node *&head);
  int get_total_users (void);
  void increment_total_users (int remote_users = 1);

  Protocol_Record *get_next_friend (void);
  Protocol_Record *get_each_friend (void);

  virtual Protocol_Record *insert_protocol_info (Protocol_Record &protocol_record) = 0;
};

#endif /* _PROTOCOL_MANAGER_H */
