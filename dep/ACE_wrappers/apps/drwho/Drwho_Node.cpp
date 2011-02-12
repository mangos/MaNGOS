// $Id: Drwho_Node.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Drwho_Node.h"

Drwho_Node::Drwho_Node (const char *h_name, Drwho_Node *n)
     : key_name1_ (h_name),
       key_name2_ (0),
       tty_name_ (0),
       idle_time_ (0),
       active_count_ (0),
       inactive_count_ (0),
       next_ (n)
{}

Drwho_Node::Drwho_Node (void)
     : key_name1_ (0),
       key_name2_ (0),
       tty_name_ (0),
       idle_time_ (0),
       active_count_ (0),
       inactive_count_ (0),
       next_ (0)
{}

const char *
Drwho_Node::get_login_name (void)
{
  return this->key_name1_;
}

const char *
Drwho_Node::set_login_name (const char *str)
{
  this->key_name1_ = str;
  return str;
}

const char *
Drwho_Node::get_real_name (void)
{
  return this->key_name2_;
}

const char *
Drwho_Node::set_real_name (const char *str)
{
  this->key_name2_ = str;
  return str;
}

const char *
Drwho_Node::get_host_name (void)
{
  return this->key_name1_;
}

const char *
Drwho_Node::set_host_name (const char *str)
{
  this->key_name1_ = str;
  return str;
}

int
Drwho_Node::get_active_count (void)
{
  return this->active_count_;
}

int
Drwho_Node::get_inactive_count (void)
{
  return this->inactive_count_;
}

int
Drwho_Node::set_active_count (int count)
{
  this->active_count_ = count;
  return count;
}

int
Drwho_Node::set_inactive_count (int count)
{
  this->inactive_count_ = count;
  return count;
}

int
Drwho_Node::set_idle_time (int idle_time)
{
  this->idle_time_ = idle_time;
  return idle_time;
}

int
Drwho_Node::get_idle_time (void)
{
  return this->idle_time_;
}
