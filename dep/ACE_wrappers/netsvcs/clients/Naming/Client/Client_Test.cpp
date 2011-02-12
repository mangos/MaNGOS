// $Id: Client_Test.cpp 91673 2010-09-08 18:49:47Z johnnyw $

#include "ace/Service_Config.h"
#include "ace/Naming_Context.h"
#include "ace/Dynamic_Service.h"
#include "ace/Thread_Manager.h"
#include "ace/Reactor.h"
#include "ace/os_include/os_ctype.h"
#include "ace/OS_NS_signal.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_ctype.h"
#include "ace/os_include/os_assert.h"

#include "Client_Test.h"

// Get the instance of Name_Service using Dynamic_Service

//inline Name_Service *
//NAME_SERVICE (void)

inline ACE_Naming_Context *
NAMING_CONTEXT (void)
{
  return ACE_Dynamic_Service<ACE_Naming_Context>::instance ("ACE_Naming_Context");
}

Client_Test::Client_Test (void)
{
  ACE_DEBUG ((LM_DEBUG,
              "Client_Test::Client_Test\n"));
}

int
Client_Test::open (void)
{
  // Cache the name options.
  this->name_options_ = NAMING_CONTEXT ()->name_options ();

  this->display_menu ();

  if (ACE_Event_Handler::register_stdin_handler (this,
                                                 ACE_Reactor::instance (),
                                                 ACE_Thread_Manager::instance ()) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p\n",
                       "register_stdin_handler"),
                      -1);
  return 0;
}


int
Client_Test::close (void)
{
  // Deregister this handler with the ACE_Reactor.
  return ACE_Reactor::instance ()->remove_handler
    (ACE_STDIN,
     ACE_Event_Handler::DONT_CALL | ACE_Event_Handler::READ_MASK);
}

int
Client_Test::handle_input (ACE_HANDLE)
{
  char option[BUFSIZ];
  char buf1[BUFSIZ];
  char buf2[BUFSIZ];
  char buf3[BUFSIZ];
  char *temp_buf;
  int port;
  char input[1024];

  if (::scanf ("%s", option) <= 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Try again!\n",
                       "Client_Test::handle_input"),
                      0);

  int result = -1;

  switch (ACE_OS::ace_isupper (option[0]) ? ACE_OS::ace_tolower (option[0]) : option[0])
    {
    case 'p' :
      result = this->set_proc_local ();
      break;
    case 'n' :
      result = this->set_node_local ();
      break;
    case 'h' :
      if (::scanf ("%s %d", buf1, &port) <= 0)
        break;
      result = this->set_host (buf1, port);
      break;
    case 'b' :
      // get the input from stdin
      ACE_OS::fgets (input, sizeof input, stdin);

      // get the key
      if (0 != (temp_buf = ACE_OS::strtok (input, " ")))
        {
          ACE_OS::strcpy (buf1, temp_buf);

          temp_buf = ACE_OS::strtok (0, " ");

          // get the value
          if (temp_buf)
            {
              ACE_OS::strcpy (buf2, temp_buf);

              temp_buf = ACE_OS::strtok (0, " ");

              // get the type (if entered).
              if (temp_buf)
                {
                  ACE_OS::strcpy (buf3, temp_buf);
                  result = this->bind (buf1, buf2, buf3);
                }
              else
                result = this->bind (buf1, buf2);
            }
          else
            ACE_ERROR ((LM_ERROR,
                        "Bind Failed! Value not entered.\n"));
        }
      else
        ACE_ERROR ((LM_ERROR,
                    "Bind Failed! Key and Value not entered.\n"));
      break;
    case 'u' :
      if (::scanf ("%s", buf1) <= 0)
        break;
      result = this->unbind (buf1);
      break;
    case 'r' :
      // get the input from stdin
      ACE_OS::fgets (input, sizeof input, stdin);

      temp_buf = ACE_OS::strtok (input, " ");
      // get the key
      if (temp_buf)
        {
          ACE_OS::strcpy (buf1, temp_buf);

          temp_buf = ACE_OS::strtok (0, " ");

          // get the value
          if (temp_buf)
            {
              ACE_OS::strcpy (buf2, temp_buf);

              temp_buf = ACE_OS::strtok (0, " ");
              // get the type (if entered)
              if (temp_buf)
                {
                  ACE_OS::strcpy (buf3, temp_buf);
                  result = this->rebind (buf1, buf2, buf3);
                }
              else
                result = this->rebind (buf1, buf2);
            }
          else
            ACE_ERROR ((LM_ERROR,
                        "Rebind Failed! Value not entered.\n"));
        }
      else
        ACE_ERROR ((LM_ERROR,
                    "Reind Failed! Key and value not entered.\n"));
      break;
    case 'f' :
      if (::scanf ("%s", buf1) <= 0)
        break;
      result = this->find (buf1);
      break;
    case 'j' :
      if (::scanf ("%s", buf1) <= 0)
        break;
      else
        result = this->list_names (buf1);
      break;
    case 'k' :
      if (::scanf ("%s", buf1) <= 0)
        break;
      else
        result = this->list_values (buf1);
      break;
    case 'l' :
      if (::scanf ("%s", buf1) <= 0)
        break;
      else
        result = this->list_types (buf1);
      break;
    case 'c' :
      if (::scanf ("%s", buf1) <= 0)
        break;
      else
        result = this->list_name_entries (buf1);
      break;
    case 'd' :
      if (::scanf ("%s", buf1) <= 0)
        break;
      else
        result = this->list_value_entries (buf1);
      break;
    case 'e' :
      if (::scanf ("%s", buf1) <= 0)
        break;
      else
        result = this->list_type_entries (buf1);
      break;
    case 'q' :
      result = -1;
      break;
    default :
      ACE_DEBUG ((LM_DEBUG,
                  "Unrecognized command.\n"));
    }

  if (result == -1)
    ACE_Reactor::instance ()->end_reactor_event_loop ();
  else
    this->display_menu ();
  return result;
}

void
Client_Test::display_menu (void)
{
  ACE_DEBUG ((LM_DEBUG, "\n"));
  this->list_options ();
  ACE_DEBUG ((LM_DEBUG, "         Name Service Main Menu\n"));
  ACE_DEBUG ((LM_DEBUG, "         ----------------------\n"));
  ACE_DEBUG ((LM_DEBUG, "<P> Use Process Local Database\n"));
  ACE_DEBUG ((LM_DEBUG, "<N> Use Node Local Database\n"));;
  ACE_DEBUG ((LM_DEBUG, "<H> Set Remote Name server <host> and <port>\n\n"));
  ACE_DEBUG ((LM_DEBUG, "<B> Bind <key> <value> [<type>]\n"));
  ACE_DEBUG ((LM_DEBUG, "<U> Unbind <key>\n"));
  ACE_DEBUG ((LM_DEBUG, "<R> Rebind <key> <value> [<type>]\n"));
  ACE_DEBUG ((LM_DEBUG, "<F> Find <key>\n"));
  ACE_DEBUG ((LM_DEBUG, "<J> Lookup keys matching <pattern>\n"));
  ACE_DEBUG ((LM_DEBUG, "<K> Lookup values matching <pattern>\n"));
  ACE_DEBUG ((LM_DEBUG, "<L> Lookup types matching <pattern>\n"));
  ACE_DEBUG ((LM_DEBUG, "<C> Complete lookup keys matching <pattern>\n"));
  ACE_DEBUG ((LM_DEBUG, "<D> Complete lookup values matching <pattern>\n"));
  ACE_DEBUG ((LM_DEBUG, "<E> Complete lookup types matching <pattern>\n"));

  ACE_DEBUG ((LM_DEBUG, "<Q> or ^C (exit)\n"));
}

void
Client_Test::list_options (void)
{
  switch (this->name_options_->context ())
    {
    case ACE_Naming_Context::PROC_LOCAL:
      ACE_DEBUG ((LM_DEBUG,
                  "  *** Using Process Local Database\n"));
      break;
    case ACE_Naming_Context::NODE_LOCAL:
      ACE_DEBUG ((LM_DEBUG,
                  "  *** Using Node Local Database\n"));
      break;
    case ACE_Naming_Context::NET_LOCAL:
      ACE_DEBUG ((LM_DEBUG,
                  "  *** Hostname: %s\n",
                  this->name_options_->nameserver_host ()));
      ACE_DEBUG ((LM_DEBUG,
                  "  *** Port Number: %d\n",
                  this->name_options_->nameserver_port ()));
      break;
    default:
      ACE_ERROR ((LM_ERROR, "ERROR: shouldn't occur!\n"));
      break;
    }
  ACE_DEBUG ((LM_DEBUG,
              "  *** Namespace directory is %s ***\n",
              this->name_options_->namespace_dir ()));
}

int
Client_Test::set_proc_local (void)
{
  // Close down original name space
  NAMING_CONTEXT ()->close ();
  this->name_options_->nameserver_host (ACE_TEXT ("localhost"));
  this->name_options_->context (ACE_Naming_Context::PROC_LOCAL);
  return NAMING_CONTEXT ()->open (ACE_Naming_Context::PROC_LOCAL);
}

int
Client_Test::set_node_local (void)
{
  // Close down original name space
  NAMING_CONTEXT ()->close ();
  this->name_options_->nameserver_host (ACE_TEXT ("localhost"));
  this->name_options_->context (ACE_Naming_Context::NODE_LOCAL);
  return NAMING_CONTEXT ()->open (ACE_Naming_Context::NODE_LOCAL);
}

int
Client_Test::set_host (const char *hostname, int port)
{
  // Close down original name space
  NAMING_CONTEXT ()->close ();

  this->name_options_->context (ACE_Naming_Context::NET_LOCAL);
  // Set Name Options
  this->name_options_->nameserver_host (ACE_TEXT_CHAR_TO_TCHAR (hostname));
  this->name_options_->nameserver_port (port);

  return NAMING_CONTEXT ()->open (ACE_Naming_Context::NET_LOCAL);
}

int
Client_Test::bind (const char *key,
                   const char *value,
                   const char *type)
{
  if (NAMING_CONTEXT ()->bind (key, value, type) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Bind failed! Key %s exists\n",
                       "Client_Test::bind",
                       key),
                      0);
  return 0;
}

int
Client_Test::unbind (const char *key)
{
  if (NAMING_CONTEXT ()->unbind (key) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Unbind failed! Key %s not found\n",
                       "Client_Test::unbind",
                       key),
                      0);
  return 0;
}

int
Client_Test::rebind (const char *key,
                     const char *value,
                     const char *type)
{
  int result = NAMING_CONTEXT ()->rebind (key, value, type );
  return result == 1 ? 0 : result;
}

int
Client_Test::list_names (const char *pattern)
{
  ACE_PWSTRING_SET set;

  if (NAMING_CONTEXT ()->list_names (set, pattern) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Pattern matching failed!\n",
                       "Client_Test::list_names"),
                      0);
  else
    {
      ACE_PWSTRING_ITERATOR set_iterator (set);

      for (ACE_NS_WString *name = 0;
           set_iterator.next (name) !=0;
           set_iterator.advance())
        ACE_DEBUG ((LM_DEBUG,
                    "%s\n",
                    name->char_rep ()));
    }
  return 0;
}

int
Client_Test::list_values (const char *pattern)
{
  ACE_PWSTRING_SET set;

  if (NAMING_CONTEXT ()->list_values (set, pattern) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Pattern matching failed!\n",
                       "Client_Test::list_values"),
                      0);
  else
    {
      ACE_PWSTRING_ITERATOR set_iterator (set);

      for (ACE_NS_WString *value = 0;
           set_iterator.next (value) !=0;
           set_iterator.advance())
        ACE_DEBUG ((LM_DEBUG,
                    "%s\n",
                    value->char_rep ()));
    }
  return 0;
}

int
Client_Test::list_types (const char *pattern)
{
  ACE_PWSTRING_SET set;

  if (NAMING_CONTEXT ()->list_types (set, pattern) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Pattern matching failed!\n",
                       "Client_Test::list_types"),
                      0);
  else
    {
      ACE_PWSTRING_ITERATOR set_iterator (set);

      for (ACE_NS_WString *type = 0;
           set_iterator.next (type) !=0;
           set_iterator.advance())
        ACE_DEBUG ((LM_DEBUG,
                    "%s\n",
                    type->char_rep ()));
    }
  return 0;
}

int
Client_Test::list_name_entries (const char *pattern)
{
  ACE_BINDING_SET set;

  if (NAMING_CONTEXT ()->list_name_entries (set, pattern) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Pattern matching failed!\n",
                       "Client_Test::list_names"),
                      0);
  else
    {
      ACE_BINDING_ITERATOR set_iterator (set);

      for (ACE_Name_Binding *entry = 0;
           set_iterator.next (entry) !=0;
           set_iterator.advance())
        {
          ACE_DEBUG ((LM_DEBUG,
                      "%s\t",
                      entry->name_.char_rep ()));
          ACE_DEBUG ((LM_DEBUG,
                      "%s\t",
                      entry->value_.char_rep ()));
          if (entry->type_)
            ACE_DEBUG ((LM_DEBUG,
                        "%s\n",
                        entry->type_));
        }
    }
  return 0;
}

int
Client_Test::list_value_entries (const char *pattern)
{
  ACE_BINDING_SET set;

  if (NAMING_CONTEXT ()->list_value_entries (set, pattern) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Pattern matching failed!\n",
                       "Client_Test::list_values"),
                      0);
  else
    {
      ACE_BINDING_ITERATOR set_iterator (set);
      for (ACE_Name_Binding *entry = 0;
           set_iterator.next (entry) !=0;
           set_iterator.advance())
        {
          ACE_DEBUG ((LM_DEBUG,
                      "%s\t",
                      entry->name_.char_rep ()));
          ACE_DEBUG ((LM_DEBUG,
                      "%s\t",
                      entry->value_.char_rep ()));
          if (entry->type_)
            ACE_DEBUG ((LM_DEBUG,
                        "%s\n",
                        entry->type_));
        }
    }
  return 0;
}

int
Client_Test::list_type_entries (const char *pattern)
{
  ACE_BINDING_SET set;

  if (NAMING_CONTEXT ()->list_type_entries (set, pattern) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Pattern matching failed!\n",
                       "Client_Test::list_types"),
                      0);
  else
    {
      ACE_BINDING_ITERATOR set_iterator (set);

      for (ACE_Name_Binding *entry = 0;
           set_iterator.next (entry) !=0;
           set_iterator.advance())
        {
          ACE_DEBUG ((LM_DEBUG,
                      "%s\t",
                      entry->name_.char_rep ()));
          ACE_DEBUG ((LM_DEBUG,
                      "%s\t",
                      entry->value_.char_rep ()));
          ACE_DEBUG ((LM_DEBUG,
                      "%s\n",
                      entry->type_));
        }
    }
  return 0;
}

int
Client_Test::find (const char *key)
{
  char *value = 0;
  char *type = 0;

  if (NAMING_CONTEXT ()->resolve (key, value, type) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "%p Find failed! Key %s not found\n",
                       "Client_Test::list_find",
                       key),
                      0);
  else
    {
      ACE_DEBUG ((LM_DEBUG,
                  "Binding for %s : value = %s\ttype = %s\n",
                  key,
                  value,
                  type));
      if (type)
        delete [] type;
      return 0;
    }
}

