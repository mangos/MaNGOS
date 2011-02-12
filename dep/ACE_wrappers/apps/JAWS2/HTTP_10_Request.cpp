// $Id: HTTP_10_Request.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "JAWS/JAWS.h"
#include "HTTP_10_Request.h"
#include "ace/OS_NS_pwd.h"



static int dummy;

JAWS_HTTP_10_Request::JAWS_HTTP_10_Request (void)
  : path_ (0)
{
}

JAWS_HTTP_10_Request::~JAWS_HTTP_10_Request (void)
{
  ACE_OS::free (this->path_);
  this->path_ = 0;
}

const char *
JAWS_HTTP_10_Request::method (void) const
{
  return this->request_line ()->method_str ();
}

const char *
JAWS_HTTP_10_Request::uri (void) const
{
  return this->request_line ()->url ();
}

const char *
JAWS_HTTP_10_Request::version (void) const
{
  return this->request_line ()->version ();
}

int
JAWS_HTTP_10_Request::type (void) const
{
  return this->request_line ()->method ();
}

const char *
JAWS_HTTP_10_Request::path (void) const
{
  if (this->path_ == 0)
    {
      JAWS_HTTP_10_Request *mutable_this = (JAWS_HTTP_10_Request *)this;
      mutable_this->path (this->uri ());
    }
  return this->path_;
}

void
JAWS_HTTP_10_Request::set_status (int s)
{
  HTTP_Request::set_status (s);
}

void
JAWS_HTTP_10_Request::path (const char *uri_string)
{
  char const *file_name = uri_string;
  char buf[MAXPATHLEN + 1];
  buf[0] = '\0';

  if (file_name == 0) return;

  if (*file_name == '/')
    {
      file_name++;
      if (*file_name == '~')
        {
          char *ptr = buf;

          while (*++file_name && *file_name != '/')
            *ptr++ = *file_name;

          *ptr = '\0';

          if (ptr == buf)
            ACE_OS::strcpy (buf, ACE_OS::getenv ("HOME"));
          else
            {
#if !defined (ACE_WIN32) && !defined (VXWORKS)
              char pw_buf[BUFSIZ];
              struct passwd pw_struct;
              if (ACE_OS::getpwnam_r (buf, &pw_struct, pw_buf, sizeof (pw_buf))
                  == 0)
                return;
              ACE_OS::strcpy (buf, pw_struct.pw_dir);
#endif /* NOT ACE_WIN32 AND NOT VXWORKS */
            }

          ACE_OS::strcat (buf, "/");
#if 0
          ACE_OS::strcat (buf, HTTP_Config::instance ()->user_dir ());
#else
          ACE_OS::strcat (buf, ".www-docs");
#endif
          ACE_OS::strcat (buf, file_name);
        }
      else
        {
          // With a starting '/' but no '~'
#if 0
          ACE_OS::strcat (buf, HTTP_Config::instance ()->document_root ());
#else
          ACE_OS::strcat (buf, ".");
#endif
          ACE_OS::strcat (buf, file_name - 1);
        }
    }

  if (*buf != '\0')
    this->path_ = ACE_OS::strdup (buf);
}
