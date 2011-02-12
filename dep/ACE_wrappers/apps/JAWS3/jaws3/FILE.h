// -*- c++ -*-
//
// $Id: FILE.h 80826 2008-03-04 14:51:23Z wotte $


#ifndef JAWS_FILE_H
#define JAWS_FILE_H

#include "ace/FILE_IO.h"
#include "ace/Mem_Map.h"

#include "jaws3/Export.h"

class JAWS_Export JAWS_FILE : public ACE_FILE_IO
//
// Like ACE_FILE_IO, but support for ACE_Mem_Map;
{
public:

  JAWS_FILE (void);

  ~JAWS_FILE (void);

  ACE_Mem_Map *mem_map (int length = -1,
                        int prot = PROT_RDWR,
                        int share = ACE_MAP_PRIVATE,
                        void *addr = 0,
                        ACE_OFF_T offset = 0,
                        LPSECURITY_ATTRIBUTES sa = 0);
  ACE_Mem_Map *mem_map (int length = -1,
                        int prot = PROT_RDWR,
                        int share = ACE_MAP_PRIVATE,
                        void *addr = 0,
                        ACE_OFF_T offset = 0,
                        LPSECURITY_ATTRIBUTES sa = 0) const;

  ACE_Mem_Map *map (void) const;

  void can_map (int);

private:

  ACE_SYNCH_MUTEX lock_;
  ACE_Mem_Map *map_;

  int can_map_;

};

#endif /* JAWS_FILE_H */
