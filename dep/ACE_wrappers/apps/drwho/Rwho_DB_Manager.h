/* -*- C++ -*- */
// $Id: Rwho_DB_Manager.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    Rwho_DB_Manager.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _RWHO_DB_MANAGER_H
#define _RWHO_DB_MANAGER_H

#include <sys/types.h>
#include <protocols/rwhod.h>
#include "ace/Dirent.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Protocol_Record.h"

class RWho_DB_Manager
{
  // = TITLE
  //   This class returns the user/machine pairs one at a time from
  //   the rwho database.

public:
  RWho_DB_Manager (void);
  ~RWho_DB_Manager (void);
  int get_next_user (Protocol_Record &protocol_record);

private:
  ACE_Dirent rwho_dir;
  whod host_data;
  int number_of_users;
  int current_user;
  const int WHOD_HEADER_SIZE;
  char original_pathname[MAXPATHLEN + 1];
  const char *rwho_dir_name;

  int get_next_host (void);
};

#endif /* _RWHO_DB_MANAGER_H */
