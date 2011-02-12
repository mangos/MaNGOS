// $Id: HTTP_Response.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_ctype.h"
#include "ace/Process.h"
#include "ace/Mem_Map.h"
#include "ace/Log_Msg.h"

#include "HTTP_Response.h"
#include "HTTP_Request.h"
#include "HTTP_Helpers.h"
#include "HTTP_Config.h"
#include "JAWS_IO.h"

#if defined (ACE_JAWS_BASELINE)
static char * const EMPTY_HEADER = "";
#else
static const char * const EMPTY_HEADER = "";
#endif /* ACE_JAWS_BASELINE */

HTTP_Response::HTTP_Response (JAWS_IO &io, HTTP_Request &request)
  : io_(io), request_(request)
{
}

HTTP_Response::HTTP_Response (HTTP_Request &request, JAWS_IO &io)
  : io_(io), request_(request)
{
}

HTTP_Response::~HTTP_Response (void)
{
#if defined (ACE_JAWS_BASELINE)
  if (this->HTTP_HEADER != EMPTY_HEADER)
    delete [] this->HTTP_HEADER;
  // The [] is important.  Without it, there was a huge memory leak!
#endif /* ACE_JAWS_BASELINE */
}

void
HTTP_Response::process_request(HTTP_Response &response)
{
  response.process_request();
}

void
HTTP_Response::process_request (void)
{
  ACE_DEBUG ((LM_DEBUG, "  (%t) processing request: %s\n",
              this->request_.status_string ()));

 switch (this->request_.status ())
    {
    case HTTP_Status_Code::STATUS_OK :

      if (this->request_.cgi ())
        {
          this->cgi_response ();
        }
      else
        {
          this->normal_response ();
        }

      break;

    default:
      this->error_response (this->request_.status (),
                            this->request_.status_string ());
    }
}

void
HTTP_Response::error_response (int status_code, const char *log_message)
{
  ACE_DEBUG ((LM_DEBUG, "(%t) [%s %s %s] %s\n",
              this->request_.method () ? this->request_.method () : "-",
              this->request_.uri () ? this->request_.uri () : "-",
              this->request_.version() ? this->request_.version () : "-",
              log_message ? log_message : "-"));

  static char const error_header1[] =
    "%s %d %s\r\n"
    "Server: JAWS/1.0prebeta\r\n"
    "Content-type: text/html\r\n"
    "Content-length: %d\r\n"
    "\r\n"
    "%s"
    ;

  static char const error_header2[] =
    "%s %d %s\r\n"
    "Server: JAWS/1.0prebeta\r\n"
    "WWW-Authenticate: Basic realm=\"JAWS_authorization\"\r\n"
    "Content-type: text/html\r\n"
    "Content-length: %d\r\n"
    "\r\n"
    "%s"
    ;

  static char const error_message[] =
    "<html>\n"
    "<head><title>Server error message</title></head>\n"
    "<body>\n"
    "<h1>Error %d: %s</h1>\n"
    "The request could not be completed because:\n %s\n"
    "</body>\n"
    "</html>\n"
    ;


  char *buf = 0;
  char buf1[4 * BUFSIZ];
  char buf2[BUFSIZ];

  int length;
  const char *error_header = error_header1;

  if (status_code == HTTP_Status_Code::STATUS_UNAUTHORIZED)
    error_header = error_header2;

  length =
    ACE_OS::sprintf (buf2, error_message,
                     status_code, HTTP_Status_Code::instance ()[status_code],
                     log_message);

  if (this->request_.version () == 0
      || ACE_OS::strcmp ("HTTP/0.9", this->request_.version ()) == 0)
    buf = buf2;
  else
    {
      length =
        ACE_OS::sprintf (buf1, error_header,
                         this->request_.version(), status_code,
                         HTTP_Status_Code::instance ()[status_code],
                         length,
                         buf2);
      buf = buf1;
    }

  this->io_.send_error_message (buf, length);
}

void
HTTP_Response::normal_response (void)
{
  const char *hv = 0;;

  ACE_DEBUG ((LM_DEBUG, " (%t) %s request for %s [%s], version %s\n",
              request_.method (), request_.uri (), request_.path (),
              (request_.version () ? request_.version () : "HTTP/0.9")));

  switch (this->request_.type ())
    {
    case HTTP_Request::GET :

      this->build_headers ();
      this->io_.transmit_file (this->request_.path (),
                               this->HTTP_HEADER,
                               this->HTTP_HEADER_LENGTH,
                               this->HTTP_TRAILER,
                               this->HTTP_TRAILER_LENGTH);
      break;

    case HTTP_Request::HEAD :
      this->build_headers ();
      this->io_.send_confirmation_message (this->HTTP_HEADER,
                                           this->HTTP_HEADER_LENGTH);
      break;

    case HTTP_Request::POST :
      // What to do here?
      // Standard says this is implementation dependent.
      // Examples: annotations, page updates, etc.
      // This may be a good place to stick CORBA stuff,
      // and mobile code.
      this->error_response (HTTP_Status_Code::STATUS_NOT_IMPLEMENTED,
                            "Requested method is not implemented.");
      break;

    case HTTP_Request::PUT :
      // Only commit to this if we can authenticate it

      // if there is no Authentication: header on the incoming request,
      // deny it
      hv = this->request_.headers ()["Authorization"].value ();
      if (hv == 0 || *hv == '\0')
        this->error_response (HTTP_Status_Code::STATUS_UNAUTHORIZED,
                              "Unauthorized to use PUT method");
      else if (ACE_OS::strncmp (hv, "Basic ", 6) != 0)
        // ``6'' is the length of the string "Basic "
        this->error_response (HTTP_Status_Code::STATUS_UNAUTHORIZED,
                              "Unknown authorization method");
      else
        {
          ACE_Mem_Map mmapfile;
          const char *hvv = hv + 6;
          // Skip past the string "Basic "
          char *buf = new char [ACE_OS::strlen (hv)];
          char *auth
            = HTTP_Helper::HTTP_decode_base64 (ACE_OS::strcpy (buf, hvv));

          if (mmapfile.map (ACE_TEXT ("jaws.auth")) != -1
              && auth != 0
              && ACE_OS::strstr((const char *) mmapfile.addr (), auth) != 0)
            this->io_.receive_file (this->request_.path (),
                                    this->request_.data (),
                                    this->request_.data_length (),
                                    this->request_.content_length ());
          else
            this->error_response (HTTP_Status_Code::STATUS_UNAUTHORIZED,
                                  "Invalid authorization attempt");
          delete [] buf;
        }
      break;

    default :
      this->error_response (HTTP_Status_Code::STATUS_NOT_IMPLEMENTED,
                            "Requested method is not implemented.");
    }
}


void
HTTP_Response::cgi_response (void)
{
  ACE_Process_Options cgi_options;

  if (this->request_.cgi_args ())
    cgi_options.command_line ("%s %s",
                              this->request_.path (),
                              this->request_.cgi_args ());
  else
    cgi_options.command_line ("%s", this->request_.path ());

  // Build environment variables
  cgi_options.setenv (ACE_TEXT ("SERVER_SOFTWARE"), ACE_TEXT ("%s"), ACE_TEXT ("JAWS/1.0"));
  cgi_options.setenv (ACE_TEXT ("SERVER_NAME"), ACE_TEXT ("%s"), ACE_TEXT ("localhost"));
  cgi_options.setenv (ACE_TEXT ("GATEWAY_INTERFACE"), ACE_TEXT ("%s"), ACE_TEXT ("CGI/1.1"));

  cgi_options.setenv (ACE_TEXT ("SERVER_PROTOCOL"), ACE_TEXT ("%s"),
                      this->request_.version ()
                      ? this->request_.version ()
                      : "HTTP/0.9");
  cgi_options.setenv (ACE_TEXT ("SERVER_PORT"), ACE_TEXT ("%d"), 5432);

  cgi_options.setenv (ACE_TEXT ("REQUEST_METHOD"), ACE_TEXT ("%s"), this->request_.method ());

  if (this->request_.path_info ())
    {
      cgi_options.setenv (ACE_TEXT ("PATH_INFO"), ACE_TEXT ("%s"),
                          this->request_.path_info ());
      cgi_options.setenv (ACE_TEXT ("PATH_TRANSLATED"),
                          ACE_TEXT ("%s/%s"),
                          HTTP_Config::instance ()->document_root (),
                          this->request_.path_info ());
    }

  cgi_options.setenv (ACE_TEXT ("SCRIPT_NAME"),
                      ACE_TEXT ("%s"),
                      this->request_.uri ());

  if (this->request_.query_string ())
    cgi_options.setenv (ACE_TEXT ("QUERY_STRING"),
                        ACE_TEXT ("%s"),
                        this->request_.query_string ());

  if (this->request_.cgi_env ())
    for (size_t i = 0; this->request_.cgi_env ()[i]; i += 2)
      cgi_options.setenv (ACE_TEXT_CHAR_TO_TCHAR (this->request_.cgi_env ()[i]),
                          ACE_TEXT ("%s"),
                          ACE_TEXT_CHAR_TO_TCHAR (this->request_.cgi_env ()[i+1]));

  ACE_TCHAR buf[BUFSIZ];
  ACE_TCHAR *p = 0, *q = 0;
  ACE_OS::strcpy (buf, ACE_TEXT ("HTTP_"));
  p = q = buf + ACE_OS::strlen (buf);

  for (size_t i = 0; i < HTTP_Request::NUM_HEADER_STRINGS; i++)
    {
      int j = 0;

      for (char c; (c = this->request_.header_strings (i)[j++]) != '\0'; )
        if (ACE_OS::ace_isalpha (c))
          *q++ = ACE_OS::ace_toupper (c);
        else if (c == '-')
          *q++ = '_';
        else
          *q++ = c;

      *q = '\0';

      const char *hv = this->request_.header_values (i);

      if (hv && *hv)
        cgi_options.setenv (buf, "%s", hv);
      q = p;
    }

  cgi_options.set_handles (this->io_.handle (),
                           this->io_.handle (),
                           this->io_.handle ());

  this->build_headers ();
  this->io_.send_confirmation_message (this->HTTP_HEADER,
                                       this->HTTP_HEADER_LENGTH);
  //  ACE::send (this->io_.handle (),
  //  this->HTTP_HEADER, this->HTTP_HEADER_LENGTH);

  // Exec the CGI program.
  ACE_Process cgi_process;
  cgi_process.spawn (cgi_options);
  //  cgi_process.wait ();
}

void
HTTP_Response::build_headers (void)
{
  // At this point, we should really determine the type of request
  // this is, and build the appropriate header.

  // Let's assume this is HTML for now.  Unless the request is CGI,
  // then do not include content-* headers.

  if (this->request_.version () == 0
      || ACE_OS::strcmp ("HTTP/0.9", this->request_.version ()) == 0)
    {
      HTTP_HEADER = EMPTY_HEADER;
      HTTP_HEADER_LENGTH = 0;
    }
  else
    {
#if defined (ACE_JAWS_BASELINE)
      HTTP_HEADER = new char[BUFSIZ * 4];

      // We assume that at this point everything is OK
      HTTP_HEADER_LENGTH =
        ACE_OS::sprintf (HTTP_HEADER, "%s", "HTTP/1.0 200 OK\r\n");

      char date_ptr [40];
      // 40 bytes is the maximum length needed to store the date

      if (HTTP_Helper::HTTP_date (date_ptr) != 0)
        HTTP_HEADER_LENGTH +=
          ACE_OS::sprintf (HTTP_HEADER+HTTP_HEADER_LENGTH,
                           "Date: %s\r\n", date_ptr);

      if (! this->request_.cgi ()) {
        HTTP_HEADER_LENGTH +=
          ACE_OS::sprintf (HTTP_HEADER+HTTP_HEADER_LENGTH,
                           "Content-type: %s\r\n",
                           "text/html");

        struct stat file_stat;
        // If possible, add the Content-length field to the header.
        // @@ Note that using 'ACE_OS::stat' is a hack.  Normally, a
        // web browser will have a 'virtual' file system. In a VFS,
        // 'stat' might not reference the correct location.
        if ((this->request_.type () == HTTP_Request::GET) &&
            (ACE_OS::stat (this->request_.path (), &file_stat) == 0))
        {
          HTTP_HEADER_LENGTH +=
            ACE_OS::sprintf (HTTP_HEADER+HTTP_HEADER_LENGTH,
                             "Content-length: %u\r\n", file_stat.st_size);
        }

        // Complete header with empty line and adjust header length.
        HTTP_HEADER[HTTP_HEADER_LENGTH++] = '\r';
        HTTP_HEADER[HTTP_HEADER_LENGTH++] = '\n';
      }
#else
      if (! this->request_.cgi ())
        HTTP_HEADER = "HTTP/1.0 200 OK\r\n"
          "Content-type: text/html\r\n\r\n";
      else
        HTTP_HEADER = "HTTP/1.0 200 OK\r\n";

      HTTP_HEADER_LENGTH = ACE_OS::strlen (HTTP_HEADER);

#endif /* ACE_JAWS_BASELINE */
    }

  HTTP_TRAILER = "";
  HTTP_TRAILER_LENGTH = 0;
}
