// $Id: http_request.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "HTTPU/http_request.h"
#include "HTTPU/parse_http_request.h"

void
HTTP_Request::parse_line (void)
{
  this->status_ = STATUS_OK;

  this->request_.init (this->line ());
  if (this->request_.error () != Parse_HTTP_Request::HTTPU_OK)
    {
      this->status_ = STATUS_INTERNAL_SERVER_ERROR;
      return;
    }
  if (this->request_.major_version () == 0)
    {
      this->no_headers_ = 1;
      return;
    }

  this->url_.init (this->request_.url ());
  if (this->url_.error () != 0)
    this->status_ = STATUS_INTERNAL_SERVER_ERROR;
}

int
HTTP_Request::espouse_line (void)
{
  int count;

  if (this->request_.major_version () == 0)
    {
      count = ACE_OS::sprintf (this->mb_->wr_ptr (), "%s /%s\r\n\r\n",
                               this->request_.method_str (),
                               this->url_.url_path ());

      if (count < 0)
        return -1;

      this->mb_->wr_ptr (count);

      return 1;
    }

  count = ACE_OS::sprintf (this->mb_->wr_ptr (), "%s /%s %s\r\n",
                           this->request_.method_str (),
                           this->url_.url_path (),
                           this->request_.version ());

  if (count < 0)
    return -1;

  this->mb_->wr_ptr (count);

  if (this->url_.host () != 0)
    {
      JAWS_Header_Data *hd = this->headers ()->find ("Host");
      if (hd == 0)
        {
          count = ACE_OS::sprintf (this->mb_->wr_ptr (), "Host: %s\r\n",
                                   this->url_.host ());

          if (count < 0)
            return -1;

          this->mb_->wr_ptr (count);
        }
    }

  return 0;
}

void
HTTP_Request::dump (void)
{
  ACE_DEBUG ((LM_DEBUG, "===== BEGIN entera_HTTP_Request::dump =====\n"));
  HTTP_Base::dump ();
  this->request_.dump ();
  ACE_DEBUG ((LM_DEBUG, "===== END entera_HTTP_Request::dump =====\n"));
}

#if !defined (ACE_HAS_INLINED_OSCALLS)
#   include "HTTPU/http_request.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */
