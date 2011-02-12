// $Id: parse_url.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/OS_NS_string.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/Log_Msg.h"
#include "HTTPU/parse_url.h"

HTTP_Parse_URL::HTTP_Parse_URL (const char *url)
  : url_ (0),
    scheme_ (0),
    user_ (0),
    passwd_ (0),
    host_ (0),
    port_ (-1),
    url_path_ (0),
    error_ (URL_ERROR_NONE),
    is_cgi_ (0)
{
  this->init (url);
}

HTTP_Parse_URL::~HTTP_Parse_URL (void)
{
  if (this->url_)
    ACE_OS::free (this->url_);
  this->url_ = 0;
  this->scheme_ = 0;
  this->user_ = 0;
  this->passwd_ = 0;
  this->host_ = 0;
  this->port_ = -1;
  this->url_path_ = 0;
}


void
HTTP_Parse_URL::init( const char *url )
{
   // Should really reset completely and cleanly here before
   // doing anything else!
   if ( url == 0 )
      return;

   if ( url_ )
      ACE_OS::free( url_ );

   url_ = ACE_OS::strdup( url );
   if ( url_ == 0 )
   {
      error_ = URL_ERROR_STRDUP;
      return;
   }

  if (ACE_OS::strlen (this->url_) > 3 && ACE_OS::strstr ("://", this->url_))
    {
      // Parse it out completely.  Figure out what it is later.
      parse_url();
    }
  else
    {
      this->url_path_ = this->url_;
      this->is_cgi (this->url_path_);
    }
}


void
HTTP_Parse_URL::parse_url (void)
{
  char *p = this->url_;

  char *q;
  if ((q = ACE_OS::strchr (this->url_, '\r'))
      || (q = ACE_OS::strchr (this->url_, '\n')))
    *q = '\0';

  this->parse_scheme (p);
  if (*p == '\0')
    {
      this->error_ = URL_ERROR_SCHEME;
      return;
    }

  // Parse past "//"
  if (*p != '/' || *(p+1) != '/')
    {
      this->error_ = URL_ERROR_SLASHSLASH;
      return;
    }
  p += 2;

  this->parse_host (p);

  while (*p == '/')
    p++;

  if (*p == '\0')
    return;

  this->url_path_ = p;
  this->is_cgi (this->url_path_);
}

void
HTTP_Parse_URL::parse_scheme (char *&p)
{
  // Parse the scheme.  The scheme is delimited by a ':'.
  if (*p != '\0')
    {
      this->scheme_ = p++;
      for (;;)
        {
          switch (*p)
            {
            case '\0':
              break;
            case ':':
              *p++ = '\0';
              break;
            default:
              p++;
              continue;
            }
          break;
        }
    }
}

void
HTTP_Parse_URL::parse_host (char *&p)
{
  // Parse user, password, host, port
  if (*p == '/' || *p == '\0')
    {
      this->set_port_from_scheme ();
      return;
    }

  char *at = 0;
  char *colon1 = 0;
  char *colon2 = 0;
  char *q = p;
  while (*q != '\0')
    {
      if (*q == '/')
        {
          *q = '\0';
          q++;
          break;
        }
      if (*q == ':')
        {
          if (colon1 == 0)
            {
              if (at != 0 && colon2 == 0)
                colon2 = q;
              else
                colon1 = q;
            }
          else
            {
              if (at != 0 && colon2 == 0)
                colon2 = q;
            }
        }
      if (*q == '@')
        {
          if (at == 0)
            at = q;
        }
      q++;
    }

  // no user, no port
  if (at == 0 && colon1 == 0)
    {
      if (*p != '\0' && *p != '/')
        this->host_ = p;
    }

  // no user, port
  else if (at == 0 && colon1 != 0)
    {
      if (p != colon1)
        this->host_ = p;
      *colon1++ = '\0';
      this->port_ = ACE_OS::atoi (colon1);
    }

  // user, no passwd, no port
  else if (at != 0 && colon1 == 0 && colon2 == 0)
    {
      this->user_ = p;
      *at++ = '\0';
      if (*at != '\0' && *at != '/')
        this->host_ = at;
    }

  // user, no passwd, port
  else if (at != 0 && colon1 == 0 && colon2 != 0)
    {
      this->user_ = p;
      *at++ = '\0';
      if (at != colon2)
        this->host_ = at;
      *colon2++ = '\0';
      this->port_ = ACE_OS::atoi (colon2);
    }

  // user, passwd, no port
  else if (at != 0 && colon1 != 0 && colon2 == 0)
    {
      this->user_ = p;
      *colon1++ = '\0';
      this->passwd_ = colon1;
      *at++ = '\0';
      if (*at != '\0')
        this->host_ = at;
    }

  // user, passwd, and port
  else if (at != 0 && colon1 != 0 && colon2 != 0)
    {
      this->user_ = p;
      *colon1++ = '\0';
      this->passwd_ = colon1;
      *at++ = '\0';
      if (at != colon2)
        this->host_ = at;
      *colon2++ = '\0';
      this->port_ = ACE_OS::atoi (colon2);
    }

  // impossible!
  else
    {
      ACE_ERROR ((LM_ERROR, "uh oh!\n"));
      p = q;
      return;
    }

  this->set_port_from_scheme ();
  p = q;
}

void
HTTP_Parse_URL::set_port_from_scheme (void)
{
  if (ACE_OS::strcmp (this->scheme_, "ftp") == 0)
    {
      if (this->port_ == -1)
        this->port_ = 21;
      if (this->user_ == 0)
        {
          this->user_ = "anonymous";

          // *** need something better here
          this->passwd_ = "a@b.c";
        }
    }
  else if (ACE_OS::strcmp (this->scheme_, "http") == 0)
    {
      if (this->port_ == -1)
        this->port_ = 80;
    }
}

const char *
HTTP_Parse_URL::scheme (void) const
{
  return this->scheme_;
}

const char *
HTTP_Parse_URL::user (void) const
{
  return this->user_;
}

const char *
HTTP_Parse_URL::passwd (void) const
{
  return this->passwd_;
}

const char *
HTTP_Parse_URL::host (void) const
{
  return this->host_;
}

int
HTTP_Parse_URL::port (void) const
{
  return this->port_;
}

const char *
HTTP_Parse_URL::url_path (void) const
{
  return this->url_path_ ? this->url_path_ : "";
}

void
HTTP_Parse_URL::is_cgi (const char *path)
{
  int yes;

  yes = (ACE_OS::strchr (path, '?') != 0);
  if (!yes && (ACE_OS::strlen (path) >= 3))
    yes = (ACE_OS::strstr (path, "cgi") != 0);
  if (!yes)
    yes = (ACE_OS::strstr (path, "asp") != 0);

  this->is_cgi_ = yes;
}

int
HTTP_Parse_URL::is_cgi (void) const
{
  return this->is_cgi_;
}
