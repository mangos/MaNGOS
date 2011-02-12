// $Id: manual.cpp 91671 2010-09-08 18:39:23Z johnnyw $

// ============================================================================
//
// = LIBRARY
//    examples
//
// = FILENAME
//    manual.cpp
//
// = DESCRIPTION
//    Allows manual operations on local and remote tokens.
//
// = AUTHOR
//    Tim Harrison
//
// ============================================================================

#include "ace/Get_Opt.h"
#include "ace/Local_Tokens.h"
#include "ace/Remote_Tokens.h"
#include "ace/Singleton.h"
#include "ace/Thread_Manager.h"
#include "ace/Token_Invariants.h"
#include "ace/Token_Collection.h"
#include "ace/Map_Manager.h"
#include "ace/Service_Config.h"

#if defined (ACE_HAS_THREADS) && defined (ACE_HAS_THREADS_LIBRARY)



typedef ACE_Token_Invariant_Manager ACE_TOKEN_INVARIANTS;

class STDIN_Token : public ACE_Event_Handler
  // = TITLE
  //     STDIN Token
  //
  // = DESCRIPTION
  //     Translates STDIN commands to ACE Token commands.
{
public:
  STDIN_Token (void);
  // Construction.

  int parse_args (int argc, ACE_TCHAR *argv[]);
  // Parse command-line arguments.

  //FUZZ: disable check_for_lack_ACE_OS
  int open (int argc, char *argv[]);
  // Register with whatever event dispatcher is needed and run.
  //FUZZ: enable check_for_lack_ACE_OS

  // = Event_Handler methods.
  int handle_input (ACE_HANDLE);
  int handle_exception (ACE_HANDLE);

  typedef ACE_CString TID;

private:

  void display_menu (void);
  // Display options.

  ACE_Token_Proxy *get_proxy (const char *tid, const char *token, char type);
  // Get or make a proxy to <token> with a <tid> client id.

  ACE_Token_Proxy *create_proxy (const char *token, char type);
  // Create a proxy to <token> with a <tid> client id.

  // = Mapping from tid to Token_Collection.
  typedef ACE_Map_Manager<TID, ACE_Token_Collection *, ACE_Null_Mutex>
    COLLECTIONS;
  // COLLECTION maintains a mapping from tid to a collection.

  typedef ACE_Map_Iterator<TID, ACE_Token_Collection *, ACE_Null_Mutex>
    COLLECTIONS_ITERATOR;
  // Allows iterations through collections_.

  typedef ACE_Map_Entry<TID, ACE_Token_Collection *>
    COLLECTIONS_ENTRY;
  // Allows iterations through collections_.

  COLLECTIONS collections_;
  // A collection for each <tid>.

  const char *server_host_;
  int server_port_;
  int ignore_deadlock_;
  int debug_;
  int remote_;
};

STDIN_Token::STDIN_Token (void)
  : server_host_ (ACE_DEFAULT_SERVER_HOST),
    server_port_ (ACE_DEFAULT_SERVER_PORT),
    ignore_deadlock_ (0),
    debug_ (0),
    remote_ (0)
{
}

int
STDIN_Token::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_LOG_MSG->open (argv[0], ACE_Log_Msg::STDERR);

  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT("h:p:diu"), 1);

  for (int c; (c = get_opt ()) != -1; )
    {
      switch (c)
      {
      case 'h':  // specify the host machine on which the server is running
        server_host_ = get_opt.opt_arg ();
        remote_ = 1;
        break;
      case 'p':  // specify the port on which the server is running
        server_port_ = ACE_OS::atoi (get_opt.opt_arg ());
        remote_ = 1;
        break;
      case 'd':
        debug_ = 1;
        break;
      case 'i':
        ignore_deadlock_ = 1;
        break;
      case 'u':
        // usage: fallthrough
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
        "%n:\n"
        "[-h <remote host>]\n"
        "[-p <remote port>]\n"
        "[-i ignore deadlock]\n"
        "[-d debug]\n", 1), -1);
      }
    }

  if (remote_)
    ACE_Remote_Mutex::set_server_address (ACE_INET_Addr (server_port_,
                                                         server_host_));

  return 0;
}

int
STDIN_Token::open (int argc, char *argv[])
{
  if (this->parse_args (argc, argv) == -1)
    return -1;

  // Register for signals.
  if (ACE_Reactor::instance ()->register_handler
      (SIGINT, this) == -1)
    ACE_DEBUG ((LM_DEBUG, "Can't register signal handler\n"));

#if defined (ACE_WIN32)

#else
  // Register for STDIN events with Reactor.
  if (ACE_Reactor::instance ()->register_handler
      (ACE_STDIN, this, ACE_Event_Handler::READ_MASK) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "Can't register signal handler\n"), 0);


#endif /* ACE_WIN32 */


  this->display_menu ();

#if defined (ACE_WIN32)

#else
  ACE_Reactor::run_event_loop ();
#endif /* ACE_WIN32 */

  ACE_OS::printf ("Exiting...\n");
  return 0;
}

int
STDIN_Token::handle_input (ACE_HANDLE fd)
{
  ACE_UNUSED_ARG (fd);

  char tid[BUFSIZ];
  char token[BUFSIZ];
  char type[16];
  char operation[16];

  if (::scanf ("%s %s %s %s", tid, token, type, operation) <= 0)
    {
      ACE_OS::printf ("Try again.\n");
      return 0;
    }

  ACE_Token_Proxy *proxy =
    this->get_proxy (tid, token, type[0]);

  if (proxy == 0)
    return -1;

  switch (operation[0])
    {
    case 'a':
    case 'A':
      if (proxy->acquire () == 0)
        {
          ACE_OS::printf ("Succeeded.\n");
          if (ACE_TOKEN_INVARIANTS::instance ()->acquired (proxy) == 0)
            ACE_OS::printf ("Violated invariant.\n");
        }
      else
        ACE_ERROR ((LM_ERROR, "%p.\n", "Acquire failed"));
      break;
    case 'n':
    case 'N':
      ACE_TOKEN_INVARIANTS::instance ()->releasing (proxy);
      if (proxy->renew () == 0)
        {
          ACE_OS::printf ("Succeeded.\n");
          if (ACE_TOKEN_INVARIANTS::instance ()->acquired (proxy) == 0)
            ACE_OS::printf ("Violated invariant.\n");
        }
      else
        ACE_ERROR ((LM_ERROR, "%p.\n", "Renew failed"));
      break;

    case 'r':
    case 'R':
      ACE_TOKEN_INVARIANTS::instance ()->releasing (proxy);
      if (proxy->release () == 0)
        ACE_OS::printf ("Succeeded.\n");
      else
        ACE_ERROR ((LM_ERROR, "%p.\n", "Release failed"));
      break;

    case 't':
    case 'T':
      if (proxy->tryacquire () == 0)
        {
          ACE_OS::printf ("Succeeded.\n");
          if (ACE_TOKEN_INVARIANTS::instance ()->acquired (proxy) == 0)
            ACE_OS::printf ("Violated invariant.\n");
        }
      else
        ACE_ERROR ((LM_ERROR, "%p.\n", "Tryacquire failed"));
      break;
    }

  this->display_menu ();
  return 0;
}

void
STDIN_Token::display_menu (void)
{
  ACE_OS::printf ("<tid> <token> <type> <operation>\n");
}

int
STDIN_Token::handle_exception (ACE_HANDLE fd)
{
  ACE_UNUSED_ARG (fd);

  ACE_Reactor::run_event_loop ();
  return -1;
}

ACE_Token_Proxy *
STDIN_Token::get_proxy (const char *_tid, const char *token, char type)
{
  ACE_Token_Collection *proxy_collection;

  TID tid (_tid);

  if (collections_.find (tid, proxy_collection) == -1)
    // We did not find a proxy_collection.
    {
      // Make one.
      proxy_collection = new ACE_Token_Collection (debug_, "no name collection");

      // Put it in the collections.
      if (collections_.bind (tid, proxy_collection) == -1)
        {
          delete proxy_collection;
          return 0;
        }
    }

  // Either way, we have a proxy_collection now.

  // See if the proxy already exists in the collection.
  ACE_Token_Proxy *proxy = proxy_collection->is_member (token);

  // If not, create one.
  if (proxy == 0)
    {
      proxy = this->create_proxy (token, type);

      // Put the new_proxy in this tid's collection.
      if (proxy_collection->insert (*proxy) == -1)
        ACE_ERROR_RETURN ((LM_ERROR, "insert failed\n"), 0);

      // Delete our copy (one was created in the collection).
      delete proxy;
      proxy = proxy_collection->is_member (token);

      if (proxy == 0)
        ACE_ERROR_RETURN ((LM_ERROR, "is_member failed\n"), 0);

      // Set the client_id (it was set to 1 since we're
      // single-threaded.
      proxy->client_id (_tid);
    }

  return proxy;
}

ACE_Token_Proxy *
STDIN_Token::create_proxy (const char *token, char type)
{
  switch (type)
    {
    case 'm':
    case 'M':
      if (remote_)
        return new ACE_Remote_Mutex (token, ignore_deadlock_, debug_);
      else
        return new ACE_Local_Mutex (token, ignore_deadlock_, debug_);

    case 'r':
    case 'R':
      if (remote_)
        return new ACE_Remote_RLock (token, ignore_deadlock_, debug_);
      else
        return new ACE_Local_RLock (token, ignore_deadlock_, debug_);

    case 'w':
    case 'W':
      if (remote_)
        return new ACE_Remote_WLock (token, ignore_deadlock_, debug_);
      else
        return new ACE_Local_WLock (token, ignore_deadlock_, debug_);
    }

  // should never get here, but this avoids a compiler warning . . .
  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  STDIN_Token st;
  return st.open (argc, argv);
}

#else
int
ACE_TMAIN(int, ACE_TCHAR *[])
{
  ACE_ERROR_RETURN ((LM_ERROR,
                     "threads or ACE_HAS_TOKENS_LIBRARY not supported on this platform\n"), -1);
}
#endif /* ACE_HAS_THREADS && ACE_HAS_TOKENS_LIBRARY */
