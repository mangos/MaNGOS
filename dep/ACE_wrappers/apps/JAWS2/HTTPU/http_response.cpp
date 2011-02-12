// $Id: http_response.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "HTTPU/http_response.h"

void
HTTP_Response::parse_line (void)
{
  this->response_.init (this->line ());
  if (this->response_.error () != Parse_HTTP_Response::HTTPU_OK)
    this->status_ = STATUS_INTERNAL_SERVER_ERROR;
}

int
HTTP_Response::espouse_line (void)
{
  int count;
  int status;

  if (this->status_ != (int)STATUS_OK)
    status = this->status_;
  else
    status = this->response_line ()->code ();

  count = ACE_OS::sprintf (this->mb_->wr_ptr (), "%s %d %s\r\n",
                           "HTTP/1.1",
                           status,
                           (char *)(*HTTP_SCode::instance ())[status]);
  // Last arg is hard coded since we are suppose to report the
  // level of server we are, and not act like the level of the
  // client.  This information should be obtained from the config.

  if (count < 0)
    return -1;

  this->mb_->wr_ptr (count);
  return 0;
}

void
HTTP_Response::dump (void)
{
  ACE_DEBUG ((LM_DEBUG, "===== BEGIN entera_HTTP_Response::dump =====\n"));
  HTTP_Base::dump ();
  ACE_DEBUG ((LM_DEBUG, "===== END entera_HTTP_Response::dump =====\n"));
}

#if !defined (ACE_HAS_INLINED_OSCALLS)
#   include "HTTPU/http_response.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */
