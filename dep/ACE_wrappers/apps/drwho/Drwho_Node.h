/* -*- C++ -*- */
// $Id: Drwho_Node.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    Drwho_Node.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _DRWHO_NODE_H
#define _DRWHO_NODE_H

#include "global.h"

class Drwho_Node
{
  // = TITLE
  //   Stores information about a host for a specific friend.
public:
  Drwho_Node (const char *host, Drwho_Node *next);
  Drwho_Node (void);

  int get_active_count (void);
  int get_inactive_count (void);
  int set_active_count (int count);
  int set_inactive_count (int count);
  int set_idle_time (int idle_time);
  int get_idle_time (void);
  const char *get_host_name (void);
  const char *set_host_name (const char *str);
  const char *get_login_name (void);
  const char *set_login_name (const char *);
  const char *get_real_name (void);
  const char *set_real_name (const char *);

  const char *key_name1_;
  const char *key_name2_;
  const char *tty_name_;
  int idle_time_;
  int active_count_;
  int inactive_count_;
  Drwho_Node *next_;
};

#endif /* _DRWHO_NODE_H */
