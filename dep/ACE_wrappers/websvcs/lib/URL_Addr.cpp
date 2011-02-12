// $Id: URL_Addr.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "URL_Addr.h"

#if !defined (__ACE_INLINE__)
#include "URL_Addr.inl"
#endif /* __ACE_INLINE__ */

#include "ace/OS_Memory.h"
#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_stdio.h"



static ACE_TCHAR http[] = ACE_TEXT ("http:");
static size_t http_size = sizeof(http)/sizeof(http[0]) - 1;
static ACE_TCHAR ftp[] = ACE_TEXT ("ftp:");
static size_t ftp_size = sizeof(ftp)/sizeof(ftp[0]) - 1;
static ACE_TCHAR mailto[] = ACE_TEXT ("mailto:");
static size_t mailto_size = sizeof(mailto)/sizeof(mailto[0]) - 1;

static ACE_TCHAR file[] = ACE_TEXT ("file:");
static size_t file_size = sizeof(file)/sizeof(file[0]) - 1;
static ACE_TCHAR afs[] = ACE_TEXT ("afs:");
static size_t afs_size = sizeof(afs)/sizeof(afs[0]) - 1;
static ACE_TCHAR news[] = ACE_TEXT ("news:");
static size_t news_size = sizeof(news)/sizeof(news[0]) - 1;
static ACE_TCHAR nntp[] = ACE_TEXT ("nntp:");
static size_t nntp_size = sizeof(nntp)/sizeof(nntp[0]) - 1;
static ACE_TCHAR cid[] = ACE_TEXT ("cid:");
static size_t cid_size = sizeof(cid)/sizeof(cid[0]) - 1;
static ACE_TCHAR mid[] = ACE_TEXT ("mid:");
static size_t mid_size = sizeof(mid)/sizeof(mid[0]) - 1;
static ACE_TCHAR wais[] = ACE_TEXT ("wais:");
static size_t wais_size = sizeof(wais)/sizeof(wais[0]) - 1;
static ACE_TCHAR prospero[] = ACE_TEXT ("prospero:");
static size_t prospero_size = sizeof(prospero)/sizeof(prospero[0]) - 1;
static ACE_TCHAR telnet[] = ACE_TEXT ("telnet:");
static size_t telnet_size = sizeof(telnet)/sizeof(telnet[0]) - 1;
static ACE_TCHAR rlogin[] = ACE_TEXT ("rlogin:");
static size_t rlogin_size = sizeof(rlogin)/sizeof(rlogin[0]) - 1;
static ACE_TCHAR tn3270[] = ACE_TEXT ("tn3270:");
static size_t tn3270_size = sizeof(tn3270)/sizeof(tn3270[0]) - 1;
static ACE_TCHAR gopher[] = ACE_TEXT ("gopher:");
static size_t gopher_size = sizeof(gopher)/sizeof(gopher[0]) - 1;

// When assembling URLs, sprintf() is often used. The format specifier for
// a string changes depending on the platform and char width being fed to it.
// Since we use ACE_TCHAR for strings, and it changes with the char width,
// the printf specifier needs to change with the platform.
#if defined (ACE_WIN32) || !defined (ACE_USES_WCHAR)
#  define ACE_PRINTF_S ACE_TEXT ("%s")
#else
#  define ACE_PRINTF_S ACE_TEXT ("%ls")
#endif

ACE_URL_Addr::ACE_URL_Addr (void)
  : url_ (0)
{
}

ACE_URL_Addr::~ACE_URL_Addr ()
{
  ACE_OS::free (this->url_);
  this->url_ = 0;
}

int
ACE_URL_Addr::set (const ACE_URL_Addr& address)
{
  return this->ACE_URL_Addr::string_to_addr (address.url_);
}

int
ACE_URL_Addr::string_to_addr (const ACE_TCHAR *address)
{
  if (this->url_ != 0)
    ACE_OS::free (this->url_);
  if (address == 0)
    return -1;
  ACE_ALLOCATOR_RETURN (this->url_, ACE_OS::strdup (address), -1);
  return 0;
}

int
ACE_URL_Addr::addr_to_string (ACE_TCHAR *s,
                              size_t size,
                              int) const
{
  if (size < ACE_OS::strlen (this->url_))
    return -1;
  ACE_OS::strcpy (s, this->url_);
  return 0;
}

int
ACE_URL_Addr::accept (ACE_URL_Addr_Visitor* visitor)
{
  return visitor->visit (this);
}

// The factory method to create URL addresses.
ACE_URL_Addr*
ACE_URL_Addr::create_address (const ACE_TCHAR *url)
{
  ACE_URL_Addr* addr = 0;
  if (ACE_OS::strncasecmp (http, url, http_size) == 0)
    ACE_NEW_RETURN (addr, ACE_HTTP_Addr (), 0);
  else if (ACE_OS::strncasecmp (ftp, url, ftp_size) == 0)
    ACE_NEW_RETURN (addr, ACE_FTP_Addr (), 0);
  else if (ACE_OS::strncasecmp (mailto, url, mailto_size) == 0)
    ACE_NEW_RETURN (addr, ACE_Mailto_Addr (), 0);
  else
    ACE_NEW_RETURN (addr, ACE_URL_Addr (), 0);

  if (addr->string_to_addr (url) != 0)
    {
      delete addr;
      addr = 0;
    }
  return addr;
}

int
ACE_URL_Addr::known_scheme (const ACE_TCHAR *url)
{
  if (ACE_OS::strncasecmp (http, url, http_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (ftp, url, ftp_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (mailto, url, mailto_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (file, url, file_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (afs, url, afs_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (news, url, news_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (nntp, url, nntp_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (cid, url, cid_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (mid, url, mid_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (wais, url, wais_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (prospero, url, prospero_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (telnet, url, telnet_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (rlogin, url, rlogin_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (tn3270, url, tn3270_size) == 0)
    return 1;
  else if (ACE_OS::strncasecmp (gopher, url, gopher_size) == 0)
    return 1;

  return 0;
}

// ****************************************************************

ACE_URL_Addr_Visitor::~ACE_URL_Addr_Visitor (void)
{
}

int
ACE_URL_Addr_Visitor::visit (ACE_URL_Addr*)
{
  return 0;
}

int
ACE_URL_Addr_Visitor::visit (ACE_HTTP_Addr*)
{
  return 0;
}

int
ACE_URL_Addr_Visitor::visit (ACE_FTP_Addr*)
{
  return 0;
}

int
ACE_URL_Addr_Visitor::visit (ACE_Mailto_Addr*)
{
  return 0;
}

// ****************************************************************

ACE_HTTP_Addr::ACE_HTTP_Addr (void)
  :  hostname_ (0),
     port_number_ (0),
     path_ (0),
     query_ (0)
{
}

ACE_HTTP_Addr::ACE_HTTP_Addr (const ACE_TCHAR *host_name,
                              const ACE_TCHAR *path,
                              const ACE_TCHAR *query,
                              u_short port)
  :  hostname_ (0),
     port_number_ (0),
     path_ (0),
     query_ (0)
{
  this->set (host_name, path, query, port);
}

ACE_HTTP_Addr::ACE_HTTP_Addr (const ACE_HTTP_Addr &addr)
  :  ACE_URL_Addr (),
     hostname_ (0),
     port_number_ (0),
     path_ (0),
     query_ (0)
{
  this->set (addr);
}

ACE_HTTP_Addr::~ACE_HTTP_Addr (void)
{
  this->clear ();
  this->hostname_ = 0;
  this->path_ = 0;
  this->query_ = 0;
}

int
ACE_HTTP_Addr::set (const ACE_TCHAR *host_name,
                    const ACE_TCHAR *path,
                    const ACE_TCHAR *query,
                    u_short port)
{
  if (host_name == 0 || path == 0)
    return -1;

  this->clear ();
  ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (host_name), -1);
  this->port_number_ = port;
  ACE_ALLOCATOR_RETURN (this->path_, ACE_OS::strdup (path), -1);
  if (query != 0)
    ACE_ALLOCATOR_RETURN (this->query_, ACE_OS::strdup (query), -1);
  else
    this->query_ = 0;

  size_t size = this->url_size (1);

  ACE_TCHAR *buffer;
  ACE_ALLOCATOR_RETURN (buffer,
                        reinterpret_cast<ACE_TCHAR *> (ACE_OS::malloc (size)),
                        -1);
  if (this->addr_to_string (buffer, size, 1) == -1)
    return -1;
  this->set_url (buffer);
  return 0;
}

int
ACE_HTTP_Addr::set (const ACE_HTTP_Addr &addr)
{
  if (this->ACE_URL_Addr::set (addr) != 0)
    return -1;
  this->clear ();
  if (addr.hostname_ != 0)
    ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (addr.hostname_), -1);
  else
    ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (ACE_TEXT ("")), -1);
  if (addr.path_ != 0)
    ACE_ALLOCATOR_RETURN (this->path_, ACE_OS::strdup (addr.path_), -1);
  else
    ACE_ALLOCATOR_RETURN (this->path_, ACE_OS::strdup (ACE_TEXT ("")), -1);
  this->port_number_ = addr.port_number_;
  if (addr.query_ != 0)
    ACE_ALLOCATOR_RETURN (this->query_, ACE_OS::strdup (addr.query_), -1);
  else
    this->query_ = 0;
  return 0;
}

void
ACE_HTTP_Addr::clear (void)
{
  if (this->hostname_ != 0)
    ACE_OS::free (this->hostname_);
  if (this->path_ != 0)
    ACE_OS::free (this->path_);
  if (this->query_ != 0)
    ACE_OS::free (this->query_);
}

size_t
ACE_HTTP_Addr::url_size (int flags) const
{
  // Notice that we cannot hard-code the value because the size in
  // wchar's may be different.
  size_t size =
    + sizeof (ACE_TEXT ("http://"))
    + sizeof (ACE_TEXT ("/:?")); // separators

  size_t chars =
    + (this->path_?ACE_OS::strlen (this->path_):0)
    + (this->query_?ACE_OS::strlen (this->query_):0);


  if (flags == 0)
    {
      size += sizeof (ACE_TEXT("255.255.255.255"));
    }
  else
    {
      chars += ACE_OS::strlen (this->hostname_);
    }

  if (this->port_number_ != ACE_DEFAULT_HTTP_PORT)
    {
      size += sizeof (ACE_TEXT(":65335"));
    }

  size += chars * sizeof(ACE_TCHAR);

  return size;
}

inline int
path_copy (const ACE_TCHAR *begin,
           const ACE_TCHAR * /* end */,
           ACE_TCHAR *& target,
           const ACE_TCHAR *src)
{
  // Copy one character at a time, if we find a /../ we go back to the
  // previous '/'
  for (; *src != 0; ++src)
    {
      ACE_TCHAR c = *src;

      switch (c)
        {
        case '/':
          if (src[1] == '.' && src[2] == '.' && src[3] == '/')
            {
              while (target != begin && *(--target) != '/')
                {
                  // No action.
                }

              src += 3;
            }
          else if (src[1] == '.' && src[2] == '/')
            {
              src += 2;
            }
          else
            {
              *target = c;
              ++target;
            }
          break;
        default:
          *target = c;
          ++target;
        }
    }
  return 0;
}

ACE_URL_Addr*
ACE_HTTP_Addr::create_relative_address (const ACE_TCHAR *url) const
{
  if (ACE_URL_Addr::known_scheme (url))
    return ACE_URL_Addr::create_address (url);

  ACE_HTTP_Addr* addr = 0;
  if (url[0] == '/')
    {
      ACE_NEW_RETURN (addr, ACE_HTTP_Addr (this->get_hostname (),
                                           url + 1,
                                           0,
                                           this->get_port_number ()),
                      0);
    }
  else
    {
      const ACE_TCHAR *path = this->get_path ();
      ACE_TCHAR *buf;
      size_t n = ACE_OS::strlen (url)
        + ACE_OS::strlen (path)
        + 2;
      ACE_NEW_RETURN (buf,
                      ACE_TCHAR [n],
                      0);

      // We copy the contens of <path> into <buf>; but simplifying the
      // path, to avoid infinite loop like:
      // "foo/../foo/../foo/../foo/../foo/index.html"
      //
      ACE_TCHAR *target = buf;

      // Copy the path
      path_copy (buf, buf + n, target, path);

      if (url[0] == '#')
        {
          // Remove any # from the path
          ACE_TCHAR *p = target;

          while (p != buf && *(--p) != '#')
            {
              // No action.
            }

          if (p != buf)
            target = p;
        }
      else
        {
          // Go back to the last / to remove the basename.
          while (target != buf && *(--target) != '/')
            {
              // No action.
            }

          // Go back if we begin with '../'
          while ((url[0] == '.' && url[1] == '.' && url[2] == '/')
                 || (url[0] == '.' && url[1] == '/'))
            {
              if (url[1] == '.')
                {
                  // A ../ go back
                  while (target != buf && *(--target) != '/')
                    {
                      // No action.
                    }

                  url += 3;
                }
              else
                {
                  // A ./ remove
                  url += 2;
                }
            }

          *target = '/'; ++target;
        }
      // Copy the url
      path_copy (buf, buf + n, target, url);
      // null terminate.
      *target = 0;
      ACE_NEW_RETURN (addr, ACE_HTTP_Addr (this->get_hostname (),
                                           buf,
                                           0,
                                           this->get_port_number ()),
                      0);
      delete[] buf;
    }

  return addr;
}

int
ACE_HTTP_Addr::string_to_addr (const ACE_TCHAR *address)
{
  if (address == 0)
    return -1;

  if (ACE_OS::strncasecmp (http, address, http_size) != 0)
    return -1;

  this->clear ();
  this->hostname_ = 0;
  this->path_ = 0;
  this->query_ = 0;

  // Save the original URL....
  if (this->ACE_URL_Addr::string_to_addr (address) != 0)
    return -1;

  const ACE_TCHAR *string = address;
  string += http_size;
  string += 2; // == strlen ("//");

  // Make a copy of the string to manipulate it.
  ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (string), -1);

  ACE_TCHAR *path_start = ACE_OS::strchr (this->hostname_, '/');
  if (path_start != 0)
    {
      // terminate the host:port substring
      path_start[0] = '\0';
      path_start++;
      ACE_TCHAR *query_start = ACE_OS::strchr (path_start, '?');
      if (query_start != 0)
        {
          query_start[0] = '\0';
          query_start++;
          ACE_ALLOCATOR_RETURN (this->query_,
                                ACE_OS::strdup (query_start),
                                -1);
        }
      ACE_ALLOCATOR_RETURN (this->path_, ACE_OS::strdup (path_start), -1);
    }

  // By now t is null terminated at the start of the path, find the
  // port (if present).
  ACE_TCHAR *port_start = ACE_OS::strchr(this->hostname_, ':');
  this->port_number_ = ACE_DEFAULT_HTTP_PORT;
  if (port_start != 0)
    {
      // terminate the ipaddr.
      port_start[0] = '\0';
      port_start++;
      this->port_number_ = ACE_OS::atoi (port_start);
    }

  return 0;
}

int
ACE_HTTP_Addr::addr_to_string (ACE_TCHAR *buffer,
                               size_t size,
                               int flags) const
{
  if (size < this->url_size (flags))
    return -1;

  if (this->hostname_ == 0)
    return -1;

  size_t n = ACE_OS::sprintf (buffer,
                              ACE_TEXT ("http://"));
  if (flags == 0)
    {
      ACE_INET_Addr inet = this->get_inet_address ();
      n += ACE_OS::sprintf (buffer + n, ACE_PRINTF_S, inet.get_host_addr ());
    }
  else
    {
     n += ACE_OS::sprintf (buffer + n, ACE_PRINTF_S, this->hostname_);
    }

  if (this->port_number_ != ACE_DEFAULT_HTTP_PORT)
    {
      n += ACE_OS::sprintf (buffer + n, ACE_TEXT (":%d"), this->port_number_);
    }
  if (this->path_ != 0)
    {
      n += ACE_OS::sprintf (buffer + n,
                            ACE_TEXT ("/") ACE_PRINTF_S,
                            this->path_);
    }
  if (this->query_ != 0)
    {
      n += ACE_OS::sprintf (buffer + n,
                            ACE_TEXT ("?") ACE_PRINTF_S,
                            this->query_);
    }
  return 0;
}

int
ACE_HTTP_Addr::accept (ACE_URL_Addr_Visitor *visitor)
{
  return visitor->visit (this);
}

// ****************************************************************

ACE_FTP_Addr::ACE_FTP_Addr (void)
  :  user_ (0),
     password_ (0),
     hostname_ (0),
     path_ (0)
{
}

ACE_FTP_Addr::ACE_FTP_Addr (const ACE_TCHAR *host_name,
                            const ACE_TCHAR *path,
                            const ACE_TCHAR *user,
                            const ACE_TCHAR *password)
  :  user_ (0),
     password_ (0),
     hostname_ (0),
     path_ (0)
{
  this->set (host_name, path, user, password);
}

ACE_FTP_Addr::ACE_FTP_Addr (const ACE_FTP_Addr& addr)
  :  ACE_URL_Addr (),
     user_ (0),
     password_ (0),
     hostname_ (0),
     path_ (0)
{
  this->set (addr);
}

ACE_FTP_Addr::~ACE_FTP_Addr (void)
{
  this->clear ();
}

int
ACE_FTP_Addr::set (const ACE_TCHAR *host_name,
                   const ACE_TCHAR *path,
                   const ACE_TCHAR *user,
                   const ACE_TCHAR *password)
{
  if (host_name == 0 || path == 0)
    return -1;
  this->clear ();
  ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (host_name), -1);
  ACE_ALLOCATOR_RETURN (this->path_, ACE_OS::strdup (path), -1);
  if (user != 0)
    ACE_ALLOCATOR_RETURN (this->user_, ACE_OS::strdup (user), -1);
  else
    this->user_ = 0;
  if (this->password_ != 0)
    ACE_ALLOCATOR_RETURN (this->password_, ACE_OS::strdup (password), -1);
  else
    this->password_ = 0;

  size_t size = this->url_size (1);

  ACE_TCHAR *buffer;
  ACE_ALLOCATOR_RETURN (buffer,
                        reinterpret_cast<ACE_TCHAR *> (ACE_OS::malloc (size)),
                        -1);
  if (this->addr_to_string (buffer, size, 1) == -1)
    return -1;
  this->set_url (buffer);
  return 0;
}

int
ACE_FTP_Addr::set (const ACE_FTP_Addr& addr)
{
  if (this->ACE_URL_Addr::set (addr) != 0)
    return -1;
  this->clear ();
  ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (addr.hostname_), -1);
  ACE_ALLOCATOR_RETURN (this->path_, ACE_OS::strdup (addr.path_), -1);
  if (addr.user_ != 0)
    ACE_ALLOCATOR_RETURN (this->user_, ACE_OS::strdup (addr.user_), -1);
  else
    this->user_ = 0;
  if (addr.password_ != 0)
    ACE_ALLOCATOR_RETURN (this->password_, ACE_OS::strdup (addr.password_), -1);
  else
    this->password_ = 0;
  return 0;
}

void
ACE_FTP_Addr::clear (void)
{
  if (this->hostname_ != 0)
    ACE_OS::free (this->hostname_);
  if (this->path_ != 0)
    ACE_OS::free (this->path_);
  if (this->user_ != 0)
    ACE_OS::free (this->user_);
  if (this->password_ != 0)
    ACE_OS::free (this->password_);
}

size_t
ACE_FTP_Addr::url_size (int flags) const
{
  // Notice that we cannot hard-code the value because the size in
  // wchar's may be different.
  size_t size =
    + sizeof (ACE_TEXT ("ftp://"))
    + sizeof (ACE_TEXT ("@:/")); // separators

  size_t chars =
    + (this->user_?ACE_OS::strlen (this->path_):0)
    + (this->password_?ACE_OS::strlen (this->password_):0)
    + (this->path_?ACE_OS::strlen (this->path_):0);

  if (flags == 0)
    {
      size += sizeof (ACE_TEXT("255.255.255.255"));
    }
  else
    {
      chars += ACE_OS::strlen (this->hostname_);
    }

  size += chars * sizeof(ACE_TCHAR);
  return size;
}

int
ACE_FTP_Addr::addr_to_string (ACE_TCHAR *buffer,
                              size_t size,
                              int flags) const
{
  if (size < this->url_size (flags))
    return -1;

  size_t n = ACE_OS::sprintf (buffer,
                              ACE_TEXT ("ftp://"));

  if (this->user_ != 0)
    {
      n += ACE_OS::sprintf (buffer + n, ACE_PRINTF_S, this->user_);
    }
  if (this->password_ != 0)
    {
      n += ACE_OS::sprintf (buffer + n,
                            ACE_TEXT (":") ACE_PRINTF_S,
                            this->password_);
    }

  if (this->user_ != 0)
    {
      n += ACE_OS::sprintf (buffer + n, ACE_TEXT ("@"));
    }

  if (flags == 0)
    {
      ACE_INET_Addr inet = this->get_inet_address ();
      n += ACE_OS::sprintf (buffer + n, ACE_PRINTF_S, inet.get_host_addr ());
    }
  else
    {
      n += ACE_OS::sprintf (buffer + n, ACE_PRINTF_S, this->hostname_);
    }
  if (this->path_ != 0)
    {
      n += ACE_OS::sprintf (buffer + n,
                            ACE_TEXT ("/") ACE_PRINTF_S,
                            this->path_);
    }
  return 0;
}

int
ACE_FTP_Addr::string_to_addr (const ACE_TCHAR *address)
{
  if (address == 0)
    return -1;
  if (ACE_OS::strncasecmp (ftp, address, ftp_size) != 0)
    return -1;

  this->clear ();
  this->hostname_ = 0;
  this->user_ = 0;
  this->password_ = 0;
  this->path_ = 0;

  // Save the original URL....
  this->ACE_URL_Addr::string_to_addr (address);

  const ACE_TCHAR *string = address;
  string += ftp_size;
  string += 2; // == strlen ("//");

  // Make a copy of the string to manipulate it.
  ACE_TCHAR *t;
  ACE_ALLOCATOR_RETURN (t, ACE_OS::strdup (string), -1);

  ACE_TCHAR *path_start = ACE_OS::strchr (t, '/');
  if (path_start != 0)
    {
      // terminate the host:port substring
      path_start[0] = '\0';
      path_start++;
      ACE_ALLOCATOR_RETURN (this->path_, ACE_OS::strdup (path_start), -1);
    }

  ACE_TCHAR *host_start = ACE_OS::strchr (t, '@');
  if (host_start != 0)
    {
      host_start[0] = '\0';
      host_start++;
      ACE_ALLOCATOR_RETURN (this->hostname_,
                            ACE_OS::strdup (host_start),
                            -1);
      ACE_TCHAR *pass_start = ACE_OS::strchr (t, ':');
      if (pass_start != 0)
        {
          pass_start[0] = '\0';
          pass_start++;
          ACE_ALLOCATOR_RETURN (this->password_,
                                ACE_OS::strdup (pass_start),
                                -1);
        }
      this->user_ = t;
    }
  else
    {
      this->hostname_ = t;
    }

  return 0;
}

int
ACE_FTP_Addr::accept (ACE_URL_Addr_Visitor* visitor)
{
  return visitor->visit (this);
}

// ****************************************************************

ACE_Mailto_Addr::ACE_Mailto_Addr (void)
  :  user_ (0),
     hostname_ (0),
     headers_ (0)
{
}

ACE_Mailto_Addr::ACE_Mailto_Addr (const ACE_TCHAR *user,
                                  const ACE_TCHAR *hostname,
                                  const ACE_TCHAR *headers)
  :  user_ (0),
     hostname_ (0),
     headers_ (0)
{
  this->set (user, hostname, headers);
}

ACE_Mailto_Addr::ACE_Mailto_Addr (const ACE_Mailto_Addr &addr)
  :  ACE_URL_Addr (addr),
     user_ (0),
     hostname_ (0),
     headers_ (0)
{
  this->set (addr);
}

ACE_Mailto_Addr::~ACE_Mailto_Addr (void)
{
  this->clear ();
}

int
ACE_Mailto_Addr::set (const ACE_TCHAR *user,
                      const ACE_TCHAR *hostname,
                      const ACE_TCHAR *headers)
{
  if (user == 0 || hostname == 0)
    return -1;
  this->clear ();
  ACE_ALLOCATOR_RETURN (this->user_, ACE_OS::strdup (user), -1);
  ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (hostname), -1);
  if (headers != 0)
    ACE_ALLOCATOR_RETURN (this->headers_, ACE_OS::strdup (headers), -1);
  else
    this->headers_ = 0;
  size_t size = this->url_size (1);
  ACE_TCHAR *buffer;
  ACE_ALLOCATOR_RETURN (buffer,
                        reinterpret_cast<ACE_TCHAR *> (ACE_OS::malloc (size)),
                        -1);
  if (this->addr_to_string (buffer, size, 1) == -1)
    return -1;
  this->set_url (buffer);
  return 0;
}

int
ACE_Mailto_Addr::set (const ACE_Mailto_Addr &addr)
{
  if (this->ACE_URL_Addr::set (addr) != 0)
    return -1;
  this->clear ();
  ACE_ALLOCATOR_RETURN (this->user_, ACE_OS::strdup (addr.user_), -1);
  ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (addr.hostname_), -1);
  if (addr.headers_ != 0)
    ACE_ALLOCATOR_RETURN (this->headers_, ACE_OS::strdup (addr.headers_), -1);
  else
    this->headers_ = 0;
  return 0;
}

void
ACE_Mailto_Addr::clear (void)
{
  if (this->user_ != 0)
    ACE_OS::free (this->user_);
  if (this->hostname_ != 0)
    ACE_OS::free (this->hostname_);
  if (this->headers_ != 0)
    ACE_OS::free (this->headers_);
}

size_t
ACE_Mailto_Addr::url_size (int) const
{
  // Notice that we cannot hard-code the value because the size in
  // wchar's may be different.
  size_t size = sizeof (ACE_TEXT ("mailto:"))
    + sizeof (ACE_TEXT ("@?")); // separators

  size_t chars =
    + (this->user_?ACE_OS::strlen (this->user_):0)
    + (this->hostname_?ACE_OS::strlen (this->hostname_):0)
    + (this->headers_?ACE_OS::strlen (this->headers_):0);
  size += chars * sizeof (ACE_TCHAR);

  return size;
}

int
ACE_Mailto_Addr::addr_to_string (ACE_TCHAR *buffer,
                                 size_t size,
                                 int flags) const
{
  if (size < this->url_size (flags))
    return -1;
  if (this->user_ == 0 || this->hostname_ == 0)
    return -1;

  size_t n = ACE_OS::sprintf (buffer,
                              ACE_TEXT ("mailto:") ACE_PRINTF_S ACE_TEXT
                              ("@") ACE_PRINTF_S,
                              this->user_, this->hostname_);
  if (this->headers_ != 0)
    {
      n += ACE_OS::sprintf (buffer + n, ACE_TEXT ("?") ACE_PRINTF_S,
                            this->headers_);
    }

  return 0;
}

int
ACE_Mailto_Addr::string_to_addr (const ACE_TCHAR *address)
{
  if (ACE_OS::strncasecmp (mailto, address, mailto_size) != 0)
    return -1;

  this->clear ();
  this->user_ = 0;
  this->hostname_ = 0;
  this->headers_ = 0;

  // Save the original URL....
  if (this->ACE_URL_Addr::string_to_addr (address) != 0)
    return -1;

  const ACE_TCHAR *string = address;
  string += mailto_size;

  // Make a copy of the string to manipulate it.
  ACE_TCHAR *t;
  ACE_ALLOCATOR_RETURN (t, ACE_OS::strdup (string), -1);

  ACE_TCHAR *host_start = ACE_OS::strchr (t, '@');
  if (host_start != 0)
    {
      // terminate the host:port substring
      host_start[0] = '\0';
      host_start++;
      ACE_TCHAR *headers_start = ACE_OS::strchr (host_start, '?');
      if (headers_start != 0)
        {
          headers_start[0] = '\0';
          headers_start++;
          ACE_ALLOCATOR_RETURN (this->headers_,
                                ACE_OS::strdup (headers_start),
                                -1);
        }
      ACE_ALLOCATOR_RETURN (this->hostname_, ACE_OS::strdup (host_start), -1);
    }
  else
    {
      ACE_OS::free (t);
      return -1;
    }
  this->user_ = t;

  return 0;
}

int
ACE_Mailto_Addr::accept (ACE_URL_Addr_Visitor* visitor)
{
  return visitor->visit (this);
}
