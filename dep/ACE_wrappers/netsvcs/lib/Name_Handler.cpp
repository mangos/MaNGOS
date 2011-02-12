// $Id: Name_Handler.cpp 91673 2010-09-08 18:49:47Z johnnyw $

#include "ace/Containers.h"
#include "ace/Get_Opt.h"
#include "ace/Singleton.h"
#include "ace/Auto_Ptr.h"
#include "Name_Handler.h"
#include "ace/Signal.h"
#include "ace/OS_NS_string.h"

// Simple macro that does bitwise AND -- useful in table lookup
#define ACE_TABLE_MAP(INDEX, MASK) (INDEX & MASK)

// Simple macro that does bitwise AND and then right shift bits by 3
#define ACE_LIST_MAP(INDEX, MASK) (((unsigned long) (INDEX & MASK)) >> 3)

int
ACE_Name_Acceptor::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Acceptor::parse_args"));

  int service_port = ACE_DEFAULT_SERVER_PORT;

  ACE_LOG_MSG->open (ACE_TEXT ("Name Service"));

  this->naming_context()->name_options()->parse_args( argc, argv );
  service_port = this->naming_context()->name_options()->nameserver_port();

  // dont allow to connect to another name serever
  if(this->naming_context()->name_options()->context() == ACE_Naming_Context::NET_LOCAL)
        this->naming_context()->name_options()->nameserver_host(ACE_TEXT ("localhost"));

  if (this->naming_context()->open( this->naming_context()->name_options()->context() ) == -1)
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT ("%n:\n open naming context failed.\n")),
                           -1);

  this->service_addr_.set (service_port);
  return 0;
}

int
ACE_Name_Acceptor::init (int argc, ACE_TCHAR *argv[])
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Acceptor::init"));

  // Use the options hook to parse the command line arguments and set
  // options.
  if( this->parse_args (argc, argv) == -1 )
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("ACE_Name_Acceptor::parse_args failed")),
                      -1);

  // Set the acceptor endpoint into listen mode (use the Singleton
  // global Reactor...).
  if (this->open (this->service_addr_,
                  ACE_Reactor::instance (),
                  0, 0, 0,
                  &this->scheduling_strategy_,
                  ACE_TEXT ("Name Server"),
                  ACE_TEXT ("ACE naming service")) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%n: %p on port %d\n"),
                       ACE_TEXT ("acceptor::open failed"),
                       this->service_addr_.get_port_number ()),
                      -1);

  // Ignore SIGPIPE so that each <SVC_HANDLER> can handle this on its
  // own.
  ACE_Sig_Action sig ((ACE_SignalHandler) SIG_IGN, SIGPIPE);
  ACE_UNUSED_ARG (sig);

  ACE_INET_Addr server_addr;

  // Figure out what port we're really bound to.
  if (this->acceptor ().get_local_addr (server_addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_local_addr")),
                      -1);

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("starting up Name Server at port %d on handle %d\n"),
              server_addr.get_port_number (),
              this->acceptor ().get_handle ()));
  return 0;
}

// The following is a "Factory" used by the ACE_Service_Config and
// svc.conf file to dynamically initialize the state of the Naming
// Server.

ACE_SVC_FACTORY_DEFINE (ACE_Name_Acceptor)

// Default constructor.
ACE_Name_Handler::ACE_Name_Handler (ACE_Thread_Manager *tm)
  : ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> (tm)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::ACE_Name_Handler"));

  // Set up pointers to member functions for the top-level dispatching
  // of client requests.
  this->op_table_[ACE_Name_Request::BIND] = &ACE_Name_Handler::bind;
  this->op_table_[ACE_Name_Request::REBIND] = &ACE_Name_Handler::rebind;
  this->op_table_[ACE_Name_Request::RESOLVE] = &ACE_Name_Handler::resolve;
  this->op_table_[ACE_Name_Request::UNBIND] = &ACE_Name_Handler::unbind;
  this->op_table_[ACE_Name_Request::LIST_NAMES] = &ACE_Name_Handler::lists;
  this->op_table_[ACE_Name_Request::LIST_NAME_ENTRIES] = &ACE_Name_Handler::lists_entries;

  // Assign references to simplify subsequent code.
  LIST_ENTRY &list_names_ref = this->list_table_[ACE_LIST_MAP (ACE_Name_Request::LIST_NAMES,
                                                               ACE_Name_Request::LIST_OP_MASK)];
  LIST_ENTRY &list_values_ref = this->list_table_[ACE_LIST_MAP (ACE_Name_Request::LIST_VALUES,
                                                                ACE_Name_Request::LIST_OP_MASK)];
  LIST_ENTRY &list_types_ref = this->list_table_[ACE_LIST_MAP (ACE_Name_Request::LIST_TYPES,
                                                               ACE_Name_Request::LIST_OP_MASK)];

  // Set up pointers to member functions for dispatching within the
  // LIST_{NAMES,VALUES,TYPES} methods.

  list_names_ref.operation_ = &ACE_Naming_Context::list_names;
  list_names_ref.request_factory_ = &ACE_Name_Handler::name_request;
  list_names_ref.description_ = "request for LIST_NAMES\n";

  list_values_ref.operation_ = &ACE_Naming_Context::list_values;
  list_values_ref.request_factory_ = &ACE_Name_Handler::value_request;
  list_values_ref.description_ = "request for LIST_VALUES\n";

  list_types_ref.operation_ = &ACE_Naming_Context::list_types;
  list_types_ref.request_factory_ = &ACE_Name_Handler::type_request;
  list_types_ref.description_ = "request for LIST_TYPES\n";
}

// Activate this instance of the ACE_Name_Handler (called by the
// ACE_Name_Acceptor).

/* VIRTUAL */ int
ACE_Name_Handler::open (void * v)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::open"));

  // Call down to our parent to register ourselves with the Reactor.
  if (ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>::open (0) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("open")),
                      -1);

  ACE_Name_Acceptor* acceptor_ = static_cast<ACE_Name_Acceptor*>(v);
  naming_context_ = acceptor_->naming_context();

  return 0;
}

// Create and send a reply to the client.

/* VIRTUAL */ int
ACE_Name_Handler::send_reply (ACE_INT32 status,
                              ACE_UINT32 err)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::send_reply"));
  void *buf;
  this->name_reply_.msg_type (status);
  this->name_reply_.errnum (err);

  this->name_reply_.init ();
  int len = this->name_reply_.encode (buf);

  if (len == -1)
    return -1;

  ssize_t n = this->peer ().send (buf, len);

  if (n != len)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n, expected len = %d, actual len = %d"),
                       ACE_TEXT ("send failed"),
                       len,
                       n),
                      -1);
  else
    return 0;
}

/* VIRTUAL */ int
ACE_Name_Handler::send_request (ACE_Name_Request &request)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::send_request"));
  void *buffer;
  ssize_t length = request.encode (buffer);

  if (length == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("encode failed")),
                      -1);
  // Transmit request via a blocking send.

  if (this->peer ().send_n (buffer, length) != length)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("send_n failed")),
                      -1);
  return 0;
}

// Give up waiting (e.g., when a timeout occurs or a client shuts down
// unexpectedly).

/* VIRTUAL */ int
ACE_Name_Handler::abandon (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::abandon"));
  return this->send_reply (-1, errno);
}

// Enable clients to limit the amount of time they'll wait

/* VIRTUAL */ int
ACE_Name_Handler::handle_timeout (const ACE_Time_Value &, const void *)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::handle_timeout"));
  return this->abandon ();
}

// Return the underlying ACE_HANDLE.

/* VIRTUAL */ ACE_HANDLE
ACE_Name_Handler::get_handle (void) const
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::get_handle"));
  return this->peer ().get_handle ();
}

// Dispatch the appropriate operation to handle the client request.

/* VIRTUAL */ int
ACE_Name_Handler::dispatch (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::dispatch"));
  // Dispatch the appropriate request.
  int index = this->name_request_.msg_type ();

  // Invoke the appropriate member function obtained by indexing into
  // the op_table_. ACE_TABLE_MAP returns the same index (by bitwise
  // AND) for list_names, list_values, and list_types since they are
  // all handled by the same method. Similarly, it returns the same
  // index for list_name_entries, list_value_entries, and
  // list_type_entries.
  return (this->*op_table_[ACE_TABLE_MAP (index,
                                          ACE_Name_Request::OP_TABLE_MASK)]) ();
}

// Receive, frame, and decode the client's request.  Note, this method
// should use non-blocking I/O.

/* VIRTUAL */ int
ACE_Name_Handler::recv_request (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::recv_request"));
  // Read the first 4 bytes to get the length of the message This
  // implementation assumes that the first 4 bytes are the length of
  // the message.
  ssize_t n = this->peer ().recv ((void *) &this->name_request_,
                                  sizeof (ACE_UINT32));
  switch (n)
    {
    case -1:
      /* FALLTHROUGH */
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("****************** recv_request returned -1\n")));
    default:
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("%p got %d bytes, expected %d bytes\n"),
                  ACE_TEXT ("recv failed"),
                  n,
                  sizeof (ACE_UINT32)));
      /* FALLTHROUGH */
    case 0:
      // We've shutdown unexpectedly, let's abandon the connection.
      this->abandon ();
      return -1;
      /* NOTREACHED */
    case sizeof (ACE_UINT32):
      {
        // Transform the length into host byte order.
        ssize_t length = ACE_NTOHL (this->name_request_.length ());

        // Do a sanity check on the length of the message.
        if (length > (ssize_t) sizeof this->name_request_)
          {
            ACE_ERROR ((LM_ERROR,
                        ACE_TEXT ("length %d too long\n"),
                        length));
            return this->abandon ();
          }

        // Receive the rest of the request message.
        // @@ beware of blocking read!!!.
        n = this->peer ().recv ((void *) (((char *) &this->name_request_)
                                        + sizeof (ACE_UINT32)),
                                length - sizeof (ACE_UINT32));

        // Subtract off the size of the part we skipped over...
        if (n != (length - (ssize_t) sizeof (ACE_UINT32)))
          {
            ACE_ERROR ((LM_ERROR, ACE_TEXT ("%p expected %d, got %d\n"),
                       ACE_TEXT ("invalid length"), length, n));
            return this->abandon ();
          }

        // Decode the request into host byte order.
        if (this->name_request_.decode () == -1)
          {
            ACE_ERROR ((LM_ERROR,
                        ACE_TEXT ("%p\n"),
                        ACE_TEXT ("decode failed")));
            return this->abandon ();
          }
      }
    }
  return 0;
}

// Callback method invoked by the ACE_Reactor when events arrive from
// the client.

/* VIRTUAL */ int
ACE_Name_Handler::handle_input (ACE_HANDLE)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::handle_input"));

  if (this->recv_request () == -1)
    return -1;
  else
    return this->dispatch ();
}

int
ACE_Name_Handler::bind (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::bind"));
  return this->shared_bind (0);
}

int
ACE_Name_Handler::rebind (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::rebind"));
  int result = this->shared_bind (1);
  return result == 1 ? 0 : result;
}

int
ACE_Name_Handler::shared_bind (int rebind)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::shared_bind"));
  ACE_NS_WString a_name (this->name_request_.name (),
                         this->name_request_.name_len () / sizeof (ACE_WCHAR_T));
  ACE_NS_WString a_value (this->name_request_.value (),
                          this->name_request_.value_len () / sizeof (ACE_WCHAR_T));
  int result;
  if (rebind == 0)
    {
#if 0
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("request for BIND\n")));
#endif /* 0 */
      result = this->naming_context ()->bind (a_name,
                                                  a_value,
                                                  this->name_request_.type ());
    }
  else
    {
#if 0
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("request for REBIND\n")));
#endif /* 0 */
      result = this->naming_context ()->rebind (a_name,
                                                    a_value,
                                                    this->name_request_.type ());
      if (result == 1)
        result = 0;
    }
  if (result == 0)
    return this->send_reply (0);
  else
    return this->send_reply (-1);
}

int
ACE_Name_Handler::resolve (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::resolve"));
#if 0
  ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("request for RESOLVE\n")));
#endif /* 0 */
  ACE_NS_WString a_name (this->name_request_.name (),
                         this->name_request_.name_len () / sizeof (ACE_WCHAR_T));

  // The following will deliver our reply back to client we
  // pre-suppose success (indicated by type RESOLVE).

  ACE_NS_WString avalue;
  char *atype;
  if (this->naming_context ()->resolve (a_name, avalue, atype) == 0)
    {
      ACE_Auto_Basic_Array_Ptr<ACE_WCHAR_T> avalue_urep (avalue.rep ());
      ACE_Name_Request nrq (ACE_Name_Request::RESOLVE,
                            0,
                            0,
                            avalue_urep.get (),
                            avalue.length () * sizeof (ACE_WCHAR_T),
                            atype, ACE_OS::strlen (atype));
      delete[] atype;
      return this->send_request (nrq);
    }

  ACE_Name_Request nrq (ACE_Name_Request::BIND, 0, 0, 0, 0, 0, 0);
  this->send_request (nrq);
  return 0;
}

int
ACE_Name_Handler::unbind (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::unbind"));
#if 0
  ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("request for UNBIND\n")));
#endif /* 0 */
  ACE_NS_WString a_name (this->name_request_.name (),
                         this->name_request_.name_len () / sizeof (ACE_WCHAR_T));

  if (this->naming_context ()->unbind (a_name) == 0)
    return this->send_reply (0);
  else
    return this->send_reply (-1);
}

ACE_Name_Request
ACE_Name_Handler::name_request (ACE_NS_WString *one_name)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::name_request"));
  ACE_Auto_Basic_Array_Ptr<ACE_WCHAR_T> one_name_urep (one_name->rep ());
  return ACE_Name_Request (ACE_Name_Request::LIST_NAMES,
                           one_name_urep.get (),
                           one_name->length () * sizeof (ACE_WCHAR_T),
                           0, 0,
                           0, 0);
}

ACE_Name_Request
ACE_Name_Handler::value_request (ACE_NS_WString *one_value)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::value_request"));
  ACE_Auto_Basic_Array_Ptr<ACE_WCHAR_T> one_value_urep (one_value->rep ());
  return ACE_Name_Request (ACE_Name_Request::LIST_VALUES,
                           0, 0,
                           one_value_urep.get (),
                           one_value->length () * sizeof (ACE_WCHAR_T),
                           0, 0);
}

ACE_Name_Request
ACE_Name_Handler::type_request (ACE_NS_WString *one_type)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::type_request"));
  return ACE_Name_Request (ACE_Name_Request::LIST_TYPES,
                           0, 0,
                           0, 0,
                           ACE_Auto_Basic_Array_Ptr<char> (one_type->char_rep ()).get (),
                           one_type->length ());
}

int
ACE_Name_Handler::lists (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::lists"));

  ACE_PWSTRING_SET set;
  ACE_NS_WString pattern (this->name_request_.name (),
                          this->name_request_.name_len () / sizeof (ACE_WCHAR_T));

  // Get the index into the list table
  int index = ACE_LIST_MAP (this->name_request_.msg_type (),
                            ACE_Name_Request::LIST_OP_MASK);

  // Print the message type
  ACE_DEBUG ((LM_DEBUG, list_table_[index].description_));

  // Call the appropriate method
  if ((this->naming_context ()->*list_table_[index].operation_) (set, pattern) != 0)
    {
      // None found so send blank request back
      ACE_Name_Request end_rq (ACE_Name_Request::MAX_ENUM, 0, 0, 0, 0, 0, 0);

      if (this->send_request (end_rq) == -1)
        return -1;
    }
  else
    {
      ACE_NS_WString *one_entry = 0;

      for (ACE_Unbounded_Set_Iterator<ACE_NS_WString> set_iterator (set);
           set_iterator.next (one_entry) !=0;
           set_iterator.advance())
        {
          ACE_Name_Request nrq ((this->*list_table_[index].request_factory_) (one_entry));

          // Create a request by calling the appropriate method obtained
          // by accessing into the table. Then send the request across.
          if (this->send_request (nrq) == -1)
            return -1;
        }

      // Send last message indicator.
      ACE_Name_Request nrq (ACE_Name_Request::MAX_ENUM,
                            0, 0,
                            0, 0,
                            0, 0);
      return this->send_request (nrq);
    }
  return 0;
}

int
ACE_Name_Handler::lists_entries (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::lists_entries"));
  ACE_BINDING_SET set;
  ACE_NS_WString pattern (this->name_request_.name (),
                          this->name_request_.name_len () / sizeof (ACE_WCHAR_T));

  int result = -1;

  const ACE_Name_Request::Constants msg_type =
    static_cast<ACE_Name_Request::Constants> (this->name_request_.msg_type ());

  // NOTE:  This multi-branch conditional statement used to be
  // (and should be) a switch statement.  However, it caused
  // Internal compiler error 980331 with egcs 1.1 (2.91.57).
  // So, the pointer-to-member-function temporary has been removed.
  if (msg_type == ACE_Name_Request::LIST_NAME_ENTRIES)
    {
#if 0
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("request for LIST_NAME_ENTRIES\n")));
#endif /* 0 */
      result = this->naming_context ()->
        ACE_Naming_Context::list_name_entries (set, pattern);
    }
  else if (msg_type == ACE_Name_Request::LIST_VALUE_ENTRIES)
    {
#if 0
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("request for LIST_VALUE_ENTRIES\n")));
#endif /* 0 */
      result = this->naming_context ()->
        ACE_Naming_Context::list_value_entries (set, pattern);
    }
  else if (msg_type == ACE_Name_Request::LIST_TYPE_ENTRIES)
    {
#if 0
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("request for LIST_TYPE_ENTRIES\n")));
#endif /* 0 */
      result = this->naming_context ()->
        ACE_Naming_Context::list_type_entries (set, pattern);
    }
  else
    return -1;

  if (result == 0)
    {
      ACE_Name_Binding *one_entry = 0;

      for (ACE_Unbounded_Set_Iterator<ACE_Name_Binding> set_iterator (set);
           set_iterator.next (one_entry) !=0;
           set_iterator.advance())
        {
           ACE_Auto_Basic_Array_Ptr<ACE_WCHAR_T>
             name_urep (one_entry->name_.rep ());
           ACE_Auto_Basic_Array_Ptr<ACE_WCHAR_T>
             value_urep (one_entry->value_.rep ());
           ACE_Name_Request mynrq (this->name_request_.msg_type (),
                                  name_urep.get (),
                                  one_entry->name_.length () * sizeof (ACE_WCHAR_T),
                                  value_urep.get (),
                                  one_entry->value_.length () * sizeof (ACE_WCHAR_T),
                                  one_entry->type_,
                                  ACE_OS::strlen (one_entry->type_));

          if (this->send_request (mynrq) == -1)
            return -1;
        }

      // send last message indicator
      ACE_Name_Request nrq (ACE_Name_Request::MAX_ENUM, 0, 0, 0, 0, 0, 0);

      if (this->send_request (nrq) == -1)
        return -1;
    }
  else
    {
      // None found so send blank request back.
      ACE_Name_Request end_rq (ACE_Name_Request::MAX_ENUM, 0, 0, 0, 0, 0, 0);

      if (this->send_request (end_rq) == -1)
        return -1;
    }

  return 0;
}

ACE_Naming_Context *
ACE_Name_Handler::naming_context (void)
{
  return naming_context_;
}

ACE_Naming_Context *
ACE_Name_Acceptor::naming_context (void)
{
  return &naming_context_;
}

ACE_Name_Handler::~ACE_Name_Handler (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_Name_Handler::~ACE_Name_Handler"));
#if 0
  ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("closing down Handle  %d\n"),
              this->get_handle ()));
#endif /* 0 */
}
