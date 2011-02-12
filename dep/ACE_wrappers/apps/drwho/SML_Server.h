/* -*- C++ -*- */
// $Id: SML_Server.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    SML_Server.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _SML_SERVER_H
#define _SML_SERVER_H

#include "SM_Server.h"

class SML_Server : public SM_Server
{
public:
  SML_Server (void);
  virtual ~SML_Server (void);
};

#endif /* _SML_SERVER_H */
