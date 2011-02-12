// $Id: Protocol_Record.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Options.h"
#include "Protocol_Record.h"
#include "ace/Log_Msg.h"

// Static initialization.

Drwho_Node Protocol_Record::drwho_node_;

Protocol_Record::~Protocol_Record (void)
{
  if (Options::get_opt (Options::DEBUGGING))
    ACE_DEBUG ((LM_DEBUG,
                "disposing Protocol_Record\n"));

  for (Drwho_Node *np = this->get_drwho_list ();
       np != 0; )
    {
      Drwho_Node *t = np;
      np = np->next_;
      delete t;
    }
}

Protocol_Record::Protocol_Record (void)
  : key_name1_ (0),
    key_name2_ (0),
    drwho_list_ (0),
    next_ (0),
    is_active_ (0)
{
}

Protocol_Record::Protocol_Record (int)
  : key_name1_ (0),
    key_name2_ (0),
    drwho_list_ (&Protocol_Record::drwho_node_),
    next_ (0),
    is_active_ (0)
{
}

Protocol_Record::Protocol_Record (const char *kn1,
                                  Protocol_Record *next)
  : key_name1_ (kn1),
    key_name2_ (0),
    drwho_list_ (0),
    next_ (next),
    is_active_ (0)
{
}

const char *
Protocol_Record::get_login (void)
{
  return this->key_name1_;
}

const char *
Protocol_Record::set_login (const char *str)
{
  this->key_name1_ = str;
  return str;
}

const char *
Protocol_Record::get_real (void)
{
  return this->key_name2_;
}

const char *
Protocol_Record::get_host (void)
{
  return this->key_name1_;
}

const char *
Protocol_Record::set_host (const char *str)
{
  this->key_name1_ = str;
  return str;
}

const char *
Protocol_Record::set_real (const char *str)
{
  this->key_name2_ = str;
  return str;
}

Drwho_Node *
Protocol_Record::get_drwho_list (void)
{
  return this->drwho_list_;
}
