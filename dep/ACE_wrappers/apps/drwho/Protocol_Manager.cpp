// $Id: Protocol_Manager.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/config-all.h"
#include "ace/Log_Msg.h"
#include "ace/OS_Memory.h"
#include "ace/OS_NS_string.h"

#include "Options.h"
#include "Protocol_Manager.h"

// Returns a pointer to the Drwho_Node associated with HOST_NAME (if
// it exists, otherwise a new node is created.  Note that if a
// Drwho_Node is found it is moved to the front of the list so that
// subsequent finds are faster (i.e., self-organizing!)

Drwho_Node *
Protocol_Manager::get_drwho_node (char *key_name, Drwho_Node *&head)
{
  Drwho_Node **temp = &head;
  for (; *temp != 0; temp = &(*temp)->next_)
    if (ACE_OS::strcmp (key_name,
                        (*temp)->get_login_name ()) == 0)
      break;

  if (*temp == 0)
    ACE_NEW_RETURN (head,
                    Drwho_Node (key_name, head),
                    0);
  else
    {
      Drwho_Node *t = *temp;

      *temp = (*temp)->next_;
      t->next_ = head;

      head = t;
    }

  return head;
}

Protocol_Manager::Protocol_Manager (void)
  : total_users (0)
{
}

Protocol_Manager::~Protocol_Manager (void)
{
  if (Options::get_opt (Options::DEBUGGING))
    ACE_DEBUG ((LM_DEBUG,
                "disposing Protocol_Manager\n"));
}

// Returns the next friend in the sequence of sorted friends.

Protocol_Record *
Protocol_Manager::get_next_friend (void)
{
  return this->ss->get_next_entry ();
}

Protocol_Record *
Protocol_Manager::get_each_friend (void)
{
  return this->ss->get_each_entry ();
}

// Returns the number of friends.

int
Protocol_Manager::friend_count (void)
{
  return this->ss->n_elems ();
}

// Returns total number of users logged in throughout the system.

int
Protocol_Manager::get_total_users (void)
{
  return Protocol_Manager::total_users;
}

void
Protocol_Manager::increment_total_users (int remote_users)
{
  Protocol_Manager::total_users += remote_users;
}
