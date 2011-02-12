// $Id: FILE.cpp 91626 2010-09-07 10:59:20Z johnnyw $

#include "ace/Guard_T.h"

#include "JAWS/FILE.h"


JAWS_FILE::JAWS_FILE (void)
  : map_ (0)
{
}

JAWS_FILE::~JAWS_FILE (void)
{
  delete this->map_;
  this->map_ = 0;
}

ACE_Mem_Map *
JAWS_FILE::mem_map (int length,
                    int prot,
                    int share,
                    void *addr,
                    ACE_OFF_T offset,
                    LPSECURITY_ATTRIBUTES sa) const
{
  JAWS_FILE *mutable_this = (JAWS_FILE *) this;
  return mutable_this->mem_map (length, prot, share, addr, offset, sa);
}

ACE_Mem_Map *
JAWS_FILE::mem_map (int length,
                    int prot,
                    int share,
                    void *addr,
                    ACE_OFF_T offset,
                    LPSECURITY_ATTRIBUTES sa)
{
  if (this->map_ == 0)
    {
      ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, g ,this->lock_, 0);

      if (this->map_ == 0)
        {
          this->map_ = new ACE_Mem_Map;
          if (this->map_ != 0)
            {
              int r = this->map_->map (this->get_handle (),
                                       static_cast<size_t> (length),
                                       prot,
                                       share,
                                       addr,
                                       offset,
                                       sa);
              if (r < 0)
                {
                  delete this->map_;
                  this->map_ = 0;
                }
            }
        }
    }

  return this->map_;

}


ACE_Mem_Map *
JAWS_FILE::map (void) const
{
  return this->map_;
}
