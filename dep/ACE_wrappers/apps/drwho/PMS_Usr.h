/* -*- C++ -*- */
// $Id: PMS_Usr.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    PMS_Usr.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _PMS_USR_H
#define _PMS_USR_H

#include "PM_Server.h"

class PMS_Usr : public PM_Server
{
  // = TITLE
  //   Provides the client's lookup table abstraction for `Usr' users...

public:
  PMS_Usr (void);

protected:
  virtual int encode (char *packet, int &total_bytes);
  virtual int decode (char *packet, int &total_bytes);
};

#endif /* _PMS_USR_H */
