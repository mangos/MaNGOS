// $Id: HTTP_Request.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "ace/Message_Block.h"
#include "HTTP_Request.h"
#include "HTTP_Helpers.h"
#include "HTTP_Config.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_pwd.h"
#include "ace/Log_Msg.h"

const char *const
HTTP_Request::static_header_strings_[HTTP_Request::NUM_HEADER_STRINGS] =
{
  "Date",
  "Pragma",
  "Authorization",
  "From",
  "If-Modified-Since",
  "Referrer",
  "User-Agent",
  "Allow",
  "Content-Encoding",
  "Content-Length",
  "Content-Type",
  "Expires",
  "Last-Modified"
};

const char *const
HTTP_Request::static_method_strings_[HTTP_Request::NUM_METHOD_STRINGS] =
{
  "GET",
  "HEAD",
  "POST",
  "PUT"
};

// For reasons of efficiency, this class expects buffer to be
// null-terminated, and buflen does NOT include the \0.

HTTP_Request::HTTP_Request (void)
  : got_request_line_ (0),
    method_ (0),
    uri_ (0),
    version_ (0),
    path_ (0),
    cgi_ (0),
    cgi_env_ (0),
    cgi_args_ (0),
    query_string_ (0),
    path_info_ (0),
    header_strings_ (HTTP_Request::static_header_strings_),
    method_strings_ (HTTP_Request::static_method_strings_)
{

  for (size_t i = 0;
       i < HTTP_Request::NUM_HEADER_STRINGS;
       i++)
    this->headers_.recognize (this->header_strings_[i]);
}

HTTP_Request::~HTTP_Request (void)
{
  ACE_OS::free (this->method_);
  ACE_OS::free (this->uri_);
  ACE_OS::free (this->version_);
  ACE_OS::free (this->path_);
  ACE_OS::free (this->query_string_);
  ACE_OS::free (this->path_info_);

  delete [] this->cgi_env_;
}

int
HTTP_Request::parse_request (ACE_Message_Block &mb)
{
  mb.wr_ptr ()[0] = '\0';

  // Note that RFC 822 does not mention the maximum length of a header
  // line.  So in theory, there is no maximum length.

  // In Apache, they assume that each header line should not exceed
  // 8K.

  int result = this->headers_.complete_header_line (mb.rd_ptr ());

  if (result != 0)
    {
      if (!this->got_request_line ())
        {
          this->parse_request_line (mb.rd_ptr ());
          while (this->headers_.complete_header_line (mb.rd_ptr ()) > 0)
            this->headers_.parse_header_line (mb.rd_ptr ());
        }
      else if (result > 0)
        do
          this->headers_.parse_header_line (mb.rd_ptr ());
        while (this->headers_.complete_header_line (mb.rd_ptr ()) > 0);
    }

  mb.wr_ptr (ACE_OS::strlen(mb.rd_ptr ()) - mb.length ());

  if (this->headers_.end_of_headers ()
      || (this->got_request_line () && this->version () == 0))
    return this->init (mb.rd_ptr (), mb.length ());
  else
    return 0;
}

void
HTTP_Request::parse_request_line (char *const request_line)
{
  char *ptr = request_line;
  char *buf = request_line;
  int offset = 1;

  this->status_ = HTTP_Status_Code::STATUS_OK;

  ptr = ACE_OS::strchr (request_line, '\n');

  if (ptr > request_line && ptr[-1] == '\r')
    ptr--, offset++;

  if (ptr == request_line)
    {
      this->status_ = HTTP_Status_Code::STATUS_BAD_REQUEST;
      return;
    }

  *ptr = '\0';
  ptr += offset;

  char *lasts = 0; // for strtok_r

  // Get the request type.
  this->got_request_line_ = 1;

  if (this->method (ACE_OS::strtok_r (buf, " \t", &lasts))
      && this->uri (ACE_OS::strtok_r (0, " \t", &lasts)))
    {
      this->type (this->method ());

      if (this->version (ACE_OS::strtok_r (0, " \t", &lasts)) == 0
          && this->type () != HTTP_Request::GET)
        this->status_ = HTTP_Status_Code::STATUS_NOT_IMPLEMENTED;

      if (this->path (this->uri ()) == 0)
        this->status_ = HTTP_Status_Code::STATUS_NOT_FOUND;
    }

  ACE_DEBUG ((LM_DEBUG, " (%t) request %s %s %s parsed\n",
              (this->method () ? this->method () : "-"),
              (this->uri () ? this->uri () : "="),
              (this->version () ? this->version () : "HTTP/0.9")));

  ACE_OS::memmove (buf, ptr, ACE_OS::strlen (ptr)+1);
}

int
HTTP_Request::init (char *const buffer,
                    int buflen)
{
  // Initialize these every time.
  content_length_ = -1;

  // Extract the data pointer.
  data_ = buffer;
  datalen_ = 0;

  // Set the datalen
  if (data_ != 0)
    datalen_ = buflen;
  else
    datalen_ = 0;

  ACE_DEBUG ((LM_DEBUG, " (%t) init has initialized\n"));

  return 1;
}

const char *
HTTP_Request::method (void) const
{
  return this->method_;
}

const char *
HTTP_Request::uri (void) const
{
  return this->uri_;
}

const char *
HTTP_Request::version (void) const
{
  return this->version_;
}

const char *
HTTP_Request::path (void) const
{
  return this->path_;
}

int
HTTP_Request::cgi (void) const
{
  return this->cgi_;
}

const char **
HTTP_Request::cgi_env (void) const
{
  return (const char **)this->cgi_env_;
}

const char *
HTTP_Request::cgi_args (void) const
{
  return this->cgi_args_;
}

const char *
HTTP_Request::query_string (void) const
{
  return this->query_string_;
}

const char *
HTTP_Request::path_info (void) const
{
  return this->path_info_;
}

int
HTTP_Request::got_request_line (void) const
{
  return this->got_request_line_;
}

int
HTTP_Request::type (void) const
{
  return type_;
}

const Headers &
HTTP_Request::headers (void) const
{
  return this->headers_;
}

const char *
HTTP_Request::header_strings (int index) const
{
  const char *hs = 0;

  if (0 <= index && index < NUM_HEADER_STRINGS)
    hs = this->header_strings_[index];

  return hs;
}

const char *
HTTP_Request::header_values (int index) const
{
  const char *hs = 0;
  const char *hv = 0;

  if (0 <= index && index < NUM_HEADER_STRINGS)
    {
      hs = this->header_strings_[index];
      hv = this->headers_[hs].value ();
    }

  return hv;
}

char *
HTTP_Request::data (void)
{
  return data_;
}

int
HTTP_Request::data_length (void)
{
  return datalen_;
}

int
HTTP_Request::content_length (void)
{
  if (this->content_length_ == -1)
    {
      const char * clv = this->headers_["Content-length"].value ();
      this->content_length_ = (clv ? ACE_OS::atoi (clv) : 0);
    }

  return this->content_length_;
}

int
HTTP_Request::status (void)
{
  return this->status_;
}

const char *
HTTP_Request::status_string (void)
{
  return HTTP_Status_Code::instance ()[this->status_];
}

void
HTTP_Request::dump (void)
{
  ACE_DEBUG ((LM_DEBUG, "%s command.\n"
              "filename is %s,"
              " length of the file is %d,"
              " data string is %s,"
              " datalen is %d,"
              " status is %d, which is %s\n\n",
              this->method () ? this->method () : "EMPTY",
              this->uri () ? this->uri () : "EMPTY",
              this->content_length (),
              this->data () ? this->data () : "EMPTY",
              this->data_length (),
              this->status (),
              this->status_string ()));
}

const char *
HTTP_Request::method (const char *method_string)
{
  if (this->method_)
    ACE_OS::free (this->method_);

  if (method_string == 0)
    {
      this->status_ = HTTP_Status_Code::STATUS_BAD_REQUEST;
      this->method_ = 0;
    }
  else
    this->method_ = ACE_OS::strdup (method_string);

  return this->method_;
}

const char *
HTTP_Request::uri (char *uri_string)
{
  if (this->uri_)
    ACE_OS::free (this->uri_);

  if (uri_string == 0)
    {
      this->status_ = HTTP_Status_Code::STATUS_BAD_REQUEST;
      this->uri_ = 0;
    }
  else
    {
      this->uri_ =  ACE_OS::strdup (uri_string);
      this->cgi (this->uri_);
      HTTP_Helper::HTTP_decode_string (this->uri_);
    }

  return this->uri_;
}

const char *
HTTP_Request::version (const char *version_string)
{
  if (this->version_)
    ACE_OS::free (this->version_);

  if (version_string)
    this->version_ = ACE_OS::strdup (version_string);
  else
    this->version_ = 0;

  return this->version_;
}

int
HTTP_Request::type (const char *type_string)
{
  this->type_ = HTTP_Request::NO_TYPE;

  if (type_string == 0)
    return this->type_;

  for (size_t i = 0;
       i < HTTP_Request::NUM_METHOD_STRINGS;
       i++)

    if (ACE_OS::strcmp (type_string, this->method_strings_[i]) == 0)
      {
        this->type_ = i;
        break;
      }

  if (this->type_ == HTTP_Request::NO_TYPE)
    this->status_ = HTTP_Status_Code::STATUS_NOT_IMPLEMENTED;

  return this->type_;
}

int
HTTP_Request::cgi (char *uri_string)
{
  this->cgi_ = 0;
  this->cgi_env_ = 0;
  this->cgi_args_ = 0;

  ACE_DEBUG ((LM_DEBUG, " (%t) HTTP_Request::cgi (%s)\n", uri_string));

  if (uri_string == 0 || ACE_OS::strlen (uri_string) == 0)
    return 0;

  // There are 2 cases where a file could be a CGI script
  //
  // (1) the file has a CGI extension.
  // (2) the file resides in a CGI bin directory.

  char *extra_path_info = 0;
  if (this->cgi_in_path (uri_string, extra_path_info)
      || this->cgi_in_extension (uri_string, extra_path_info))
    {
      cgi_args_and_env (extra_path_info);

      if (extra_path_info)
        {
          this->path_info_ = ACE_OS::strdup (extra_path_info);
          HTTP_Helper::HTTP_decode_string (this->path_info_);
          *extra_path_info = '\0';
        }
    }

  return this->cgi_;
}

int
HTTP_Request::cgi_in_path (char *uri_string, char *&extra_path_info)
{
  char *cgi_path;

  ACE_DEBUG ((LM_DEBUG, " (%t) HTTP_Request::cgi_in_path (%s)\n",
              uri_string));

  if (HTTP_Config::instance ()->cgi_path ())
    cgi_path = ACE_OS::strdup (HTTP_Config::instance ()->cgi_path ());
  else
    cgi_path = ACE_OS::strdup ("");

  // error checking considered helpful!
  if (cgi_path == 0)
    return 0;

  char *lasts = 0;
  char *cgi_path_next = ACE_OS::strtok_r (cgi_path, ":", &lasts);

  if (cgi_path_next)
    do
      {
        int len = ACE_OS::strlen (cgi_path_next);

        // match path to cgi path
        int in_cgi_path = 0;

        if (*cgi_path_next == '/')
          {
            // cgi path next points to an ``absolute'' path
            extra_path_info = uri_string;
            in_cgi_path =
              (ACE_OS::strncmp (extra_path_info, cgi_path_next, len) == 0);
          }
        else
          {
            // cgi path next points to a ``relative'' path
            extra_path_info = ACE_OS::strstr (uri_string, cgi_path_next);
            in_cgi_path = (extra_path_info != 0);
          }

        if (in_cgi_path)
          {
            if (extra_path_info[len] == '/')
              {
                this->cgi_ = 1;
                extra_path_info += len;

                // move past the executable name
                do
                  extra_path_info++;
                while (*extra_path_info != '/'
                       && *extra_path_info != '?'
                       && *extra_path_info != '\0');

                if (*extra_path_info == '\0')
                  extra_path_info = 0;

                break;
              }
          }
        extra_path_info = 0;

        cgi_path_next = ACE_OS::strtok_r (0, ":", &lasts);
      }
    while (cgi_path_next);

  ACE_OS::free (cgi_path);

  return this->cgi_;
}

int
HTTP_Request::cgi_in_extension (char *uri_string, char *&extra_path_info)
{
  extra_path_info = ACE_OS::strstr (uri_string, ".cgi");

  ACE_DEBUG ((LM_DEBUG, " (%t) HTTP_Request::cgi_in_extension (%s)\n",
              uri_string));

  while (extra_path_info != 0)
    {
      extra_path_info += 4;
      // skip past ``.cgi''

      switch (*extra_path_info)
        {
        case '\0':
          extra_path_info = 0;
          break;
        case '/':
        case '?':
          break;
        default:
          extra_path_info = ACE_OS::strstr (extra_path_info, ".cgi");
          continue;
        }
      this->cgi_ = 1;
      break;
    }

  return this->cgi_;
}

void
HTTP_Request::cgi_args_and_env (char *&extra_path_info)
{
  char *cgi_question = 0;

  if (extra_path_info)
    cgi_question = ACE_OS::strchr (extra_path_info, '?');

  if (extra_path_info == cgi_question)
    extra_path_info = 0;

  if (cgi_question)
    {
      *cgi_question++ = '\0';

      if (*cgi_question != '\0')
        {
          // We need the ``original'' QUERY_STRING for the
          // environment.  We will substitute '+'s for spaces in the
          // other copy.

          this->query_string_ = ACE_OS::strdup (cgi_question);

          char *ptr = cgi_question;
          int count = 0;
          do
            if (*ptr == '+')
              *ptr = ' ';
            else if (*ptr == '&' || *ptr == '=')
              count++;
          while (*++ptr);

          count++;

          if (ACE_OS::strchr (cgi_question, '='))
            {
              ACE_NEW (this->cgi_env_, char *[count+1]);

              int i = 0;
              ptr = cgi_question;
              do
                {
                  this->cgi_env_ [i++] = ptr;

                  while (*ptr++)
                      if (*ptr == '&' || *ptr == '=')
                        *ptr = '\0';

                  HTTP_Helper::HTTP_decode_string (this->cgi_env_[i-1]);
                }
              while (i < count);

              this->cgi_env_[count] = 0;
            }
          else
            {
              this->cgi_args_ = cgi_question;
              HTTP_Helper::HTTP_decode_string (cgi_question);
            }
        }
    }
}

const char *
HTTP_Request::path (const char *uri_string)
{
  char const *file_name = uri_string;
  char buf[MAXPATHLEN + 1];
  buf[0] = '\0';

  if (file_name == 0) return 0;

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
                return 0;
              ACE_OS::strcpy (buf, pw_struct.pw_dir);
#endif /* NOT ACE_WIN32 AND NOT VXWORKS */
            }

          ACE_OS::strcat (buf, "/");
          ACE_OS::strcat (buf, HTTP_Config::instance ()->user_dir ());
          ACE_OS::strcat (buf, file_name);
        }
      else
        {
          // With a starting '/' but no '~'
          ACE_OS::strcat (buf, HTTP_Config::instance ()->document_root ());
          ACE_OS::strcat (buf, file_name - 1);
        }
    }

  if (*buf != '\0')
    this->path_ = ACE_OS::strdup (buf);

  return this->path_;
}
