/* -*- C++ -*- */
// $Id: Protocol_Record.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    Protocol_Record.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _PROTOCOL_RECORD_H
#define _PROTOCOL_RECORD_H

#include "Drwho_Node.h"

class Protocol_Record
{
  // = TITLE
  //   Stores information about a single friend's status.

public:
  Protocol_Record (void);
  Protocol_Record (int use_dummy);
  Protocol_Record (const char *key_name1,
                   Protocol_Record *next = 0);
  ~Protocol_Record (void);
  const char *get_host (void);
  const char *set_host (const char *str);
  const char *get_login (void);
  const char *set_login (const char *str);
  const char *get_real (void);
  const char *set_real (const char *str);
  Drwho_Node *get_drwho_list (void);

  static Drwho_Node drwho_node_;
  const char *key_name1_;
  const char *key_name2_;
  Drwho_Node *drwho_list_;
  Protocol_Record *next_;
  int is_active_;
};

#endif /* _PROTOCOL_RECORD_H */
