// -*- C++ -*-
//
// $Id: Client_Test.h 80826 2008-03-04 14:51:23Z wotte $

#include "ace/Event_Handler.h"
#include "ace/Naming_Context.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class Client_Test : public ACE_Event_Handler
{
public:
  Client_Test (void);

  int open (void);
  // Cache reactor and then register self with reactor

  int close (void);
  // Close things down and free up resources.

  virtual int handle_input (ACE_HANDLE handle);
  // Handle user entered commands

  void list_options (void);
  // Print name options

  int bind (const char *key,
            const char *value,
            const char *type = "");
  // Bind a key to a value

  int unbind (const char *key);
  // Unbind a name binding

  int rebind (const char *key,
              const char *value,
              const char *type = "");
  // Rebind a name binding

  int find (const char *key);
  // Find the value associated with a key

  int list_names (const char *pattern);
  // Find all names that match pattern

  int list_values (const char *pattern);
  // Find all values that match pattern

  int list_types (const char *pattern);
  // Find all types that match pattern

  int list_name_entries (const char *pattern);
  // Find all names that match pattern

  int list_value_entries (const char *pattern);
  // Find all values that match pattern

  int list_type_entries (const char *pattern);
  // Find all types that match pattern

private:
  ACE_Name_Options *name_options_;
  // Name Options associated with the Naming Context

  void display_menu (void);
  // Display user menu

  int set_proc_local (void);
  // Set options to use PROC_LOCAL naming context

  int set_node_local (void);
  // Set options to use NODE_LOCAL naming context

  int set_host (const char *hostname, int port);
  // Set options to use NET_LOCAL naming context specifying host name
  // and port number
};
