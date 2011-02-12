// $Id: http_base.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "JAWS/Parse_Headers.h"
#include "HTTPU/http_base.h"
#include "HTTPU/http_headers.h"

int
HTTP_Base::receive (ACE_Message_Block &mb)
{
  if (this->line () == 0)
    {
      if (this->extract_line (mb) == 0)
        return 0;
      if (this->status () != STATUS_OK)
        return 1;

      // Call into the receive hook.
      this->parse_line ();
      if (this->status_ == STATUS_INTERNAL_SERVER_ERROR || this->no_headers_)
        return 1;
    }

  // Parse headers
  JAWS_Parse_Headers *parser = JAWS_Parse_Headers_Singleton::instance ();
  int ret = parser->parse_headers (&(this->info_), mb);

  switch (this->info_.status ())
    {
    case JAWS_Header_Info::STATUS_CODE_OK:
      break;

    case JAWS_Header_Info::STATUS_CODE_NO_MEMORY:
    case JAWS_Header_Info::STATUS_CODE_TOO_LONG:
    default:
      this->status_ = STATUS_INTERNAL_SERVER_ERROR;
      break;
    }

  return ret;
}

int
HTTP_Base::deliver (ACE_Message_Block &mb)
{
  JAWS_Header_Data *data = 0;

  // Deliver this outgoing request.
  // We do this by building the request up and writing it into the
  // message block.
  if (this->mb_ == 0)
    {
      // Make our Message Block big enough to hold a header name and
      // header value
      this->mb_ = new ACE_Message_Block (16384); // MAGIC!  2 x 8192
      if (this->mb_ == 0)
        {
          this->status_ = STATUS_INTERNAL_SERVER_ERROR;
          return -1;
        }

      // Call into the deliver hook
      int r = this->espouse_line ();
      if (r == -1)
        return -1;

      if (r == 1)
        this->deliver_state_ = 2;

      this->iter_.first ();
    }

  while (this->deliver_state_ < 3)
    {
      // Deliver whatever is currently held in this->mb_.
      size_t sz = (mb.space () < this->mb_->length ()
                   ? mb.space ()
                   : this->mb_->length ());

      if (sz > 0)
        {
          mb.copy (this->mb_->rd_ptr (), sz);
          this->mb_->rd_ptr (sz);
        }

      if (mb.space () == 0)
        return 0;

      // Arriving here means this->mb_ has been emptied.
      this->mb_->crunch ();

      switch (this->deliver_state_)
        {
        case 0: // Obtain the next header data // Deliver a header name
          this->deliver_state_ = this->deliver_header_name (data);
          break;

        case 1: // Deliver a header value
          this->deliver_state_ = this->deliver_header_value (data);
          break;

        case 2: // Finished!
          delete this->mb_;
          this->mb_ = 0;
          this->deliver_state_ = 3;
        }
    }

  return 1;
}

int
HTTP_Base::receive_payload (ACE_Message_Block &mb)
{
  int result = 0;

  if (this->payload_.space () < mb.length ())
    result = this->payload_.size (this->payload_.size () +
                                  mb.length () - this->payload_.space ());

  if (result == 0)
    {
      this->payload_.copy (mb.rd_ptr (), mb.length ());
      mb.rd_ptr (mb.wr_ptr ());
      mb.crunch ();
    }
  else
    this->status_ = STATUS_INTERNAL_SERVER_ERROR;

  return result;
}

int
HTTP_Base::receive_payload (ACE_Message_Block &mb, long length)
{
  int result = 0;

  if (length == -1)
    return this->receive_payload (mb);

  if (this->payload_.size () < (unsigned long) length)
    result = this->payload_.size (length);

  if (result == -1)
    {
      this->status_ = STATUS_INTERNAL_SERVER_ERROR;
      return -1;
    }

  if (this->payload_.space () >= mb.length ())
    {
      this->payload_.copy (mb.rd_ptr (), mb.length ());
      mb.rd_ptr (mb.wr_ptr ());
      mb.crunch ();
    }
  else
    {
      size_t space = this->payload_.space ();
      this->payload_.copy (mb.rd_ptr (), space);
      mb.rd_ptr (space);
    }

  return this->payload_.length () == (unsigned long) length;
}

const char *
HTTP_Base::payload (void)
{
  return this->payload_.rd_ptr ();
}

unsigned long
HTTP_Base::payload_size (void)
{
  return this->payload_.length ();
}

int
HTTP_Base::build_headers (JAWS_Headers *new_headers)
{
  JAWS_Header_Data *data = 0;
  JAWS_Header_Data *data2 = 0;
  JAWS_Header_Table_Iterator iter (*new_headers);

  iter.first ();
  while (! iter.done ())
    {
      data = iter.next ();
      if (data == 0)
        {
          iter.advance ();
          continue;
        }

      if (data->header_type () == HTTP_HCodes::REPLACE_HEADER)
        this->headers ()->remove_all (data->header_name ());
      else if (data->header_type () == HTTP_HCodes::INSERT_HEADER
               || data->header_type () == HTTP_HCodes::APPENDTO_HEADER)
        {
          data2 = this->headers ()->find (data->header_name ());
          if (data2 != 0)
            {
              if (data->header_type () == HTTP_HCodes::APPENDTO_HEADER)
                {
                  // Append to existing header
                  size_t len
                    = ACE_OS::strlen (data->header_value ())
                    + ACE_OS::strlen (data2->header_value ())
                    + 3; /* for comma, space, and nul */
                  char *buf = new char [len];
                  if (buf == 0)
                    {
                      this->status_ = STATUS_INTERNAL_SERVER_ERROR;
                      return -1;
                    }
                  ACE_OS::sprintf (buf, "%s, %s",
                                   data2->header_value (),
                                   data->header_value ());
                  data2->header_value (buf);
                  delete [] buf;
                }

              // Only insert if it isn't already present
              iter.advance ();
              continue;
            }
        }

      data2 = new JAWS_Header_Data (data->header_name (),
                                    data->header_value ());
      if (data2 == 0 || data2->header_name () == 0
          || data2->header_value () == 0)
        {
          this->status_ = STATUS_INTERNAL_SERVER_ERROR;
          return -1;
        }
      this->headers ()->insert (data2);

      iter.advance ();
    }

  return 0;
}

int
HTTP_Base::deliver_header_name (JAWS_Header_Data *&data)
{
  data = 0;

  for (;;)
    {
      if ((data = this->iter_.next ()) == 0)
        {
          // No more headers, deliver final "\r\n"
          this->mb_->copy ("\r\n", 2);
          return 2;
        }

      if (data->header_name ())
        break;

      this->iter_.advance ();
    }

  // Assume the following lines will always succeed.
  this->mb_->copy (data->header_name ());
  this->mb_->wr_ptr (this->mb_->wr_ptr () - 1);
  this->mb_->copy (": ", 2);

  return 1;
}

int
HTTP_Base::deliver_header_value (JAWS_Header_Data *&data)
{
  // Assume the following line will always succeed.
  if (data->header_value ())
    {
      this->mb_->copy (data->header_value ());
      this->mb_->wr_ptr (this->mb_->wr_ptr () - 1);
    }
  this->mb_->copy ("\r\n", 2);

  this->iter_.advance ();
  return 0;
}


int
HTTP_Base::extract_line (ACE_Message_Block &mb)
{
  JAWS_Parse_Headers *parser = JAWS_Parse_Headers_Singleton::instance ();
  char *p = parser->skipset ("\n", mb.rd_ptr (), mb.wr_ptr ());
  if (p == mb.wr_ptr ())
    return 0;

  this->status_ = STATUS_OK;

  *p = '\0';
  if (p[-1] == '\r')
    p[-1] = '\0';

  this->line_ = ACE_OS::strdup (mb.rd_ptr ());
  if (this->line_ == 0)
    this->status_ = STATUS_INTERNAL_SERVER_ERROR;

  mb.rd_ptr (p+1);
  this->info_.end_of_line (1);
  return 1;
}

void
HTTP_Base::dump (void)
{
  ACE_DEBUG ((LM_DEBUG, "%s\n", this->line ()));
  this->info_.dump ();
  ACE_DEBUG ((LM_DEBUG, "STATUS IS %d %s\n",
              this->status (),
              (*HTTP_SCode::instance ())[this->status ()]));
}

#if !defined (ACE_HAS_INLINED_OSCALLS)
#   include "HTTPU/http_base.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */
