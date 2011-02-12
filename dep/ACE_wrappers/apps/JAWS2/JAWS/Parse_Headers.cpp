// $Id: Parse_Headers.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "JAWS/Parse_Headers.h"
#include "ace/OS_NS_string.h"
#include "ace/Log_Msg.h"

#define ACCESSOR(T,C,x) \
T C :: x (void) const { return this-> x##_; }\
void C :: x (T t) { this-> x##_ = t; }

int
JAWS_Parse_Headers::parse_headers (JAWS_Header_Info *info,
                                   ACE_Message_Block &mb)
{
  for (;;)
    {
      if (mb.rd_ptr () == mb.wr_ptr ())
          break;

      char *p = mb.rd_ptr ();

      if (info->end_of_line ()
          && (*p != ' ' && *p != '\t'))
        {
          int r = this->parse_header_name (info, mb);
          if (r == 1)
            return info->end_of_headers ();
          continue;
        }
      else
        {
          int r = this->parse_header_value (info, mb);
          if (r == 1)
            {
              if (info->end_of_headers ())
                return 1;
              break;
            }
          continue;
        }
    }

  // If we arrive here, it means either there is nothing more to read,
  // or parse_header_value ran into difficulties (like maybe the
  // header value was too long).

  if (mb.rd_ptr () != mb.base ())
    {
      mb.crunch ();
      return 0;
    }
  else if (mb.length () < mb.size ())
    {
      return 0;
    }
  else if (mb.length () == mb.size ())
    {
      // This is one of those cases that should rarely ever happen.
      // If we get here, the header type name is over 8K long.  We
      // flag this as a bad thing.

      // In HTTP/1.1, I have to remember that a bad request means the
      // connection needs to be closed and the client has to
      // reinitiate the connection.

      info->status (JAWS_Header_Info::STATUS_CODE_TOO_LONG);
      return 1;
    }
  else if (mb.length () > mb.size ())
    {
      ACE_DEBUG ((LM_DEBUG, "JAWS_Parse_Headers: buffer overrun!!\n"));
      info->status (JAWS_Header_Info::STATUS_CODE_TOO_LONG);
      return 1;
    }

  ACE_DEBUG ((LM_DEBUG, "JAWS_Parse_Headers -- shouldn't be here!\n"));
  return 1;
}

char *
JAWS_Parse_Headers::skipset (const char *set, char *start, char *end)
{
  char *p = start;
  while (p < end)
    {
      if (ACE_OS::strchr (set, *p) != 0)
        break;
      p++;
    }
  return p;
}

char *
JAWS_Parse_Headers::skipcset (const char *set, char *start, char *end)
{
  char *p = start;
  while (p < end)
    {
      if (ACE_OS::strchr (set, *p) == 0)
        break;
      p++;
    }
  return p;
}

int
JAWS_Parse_Headers::parse_header_name (JAWS_Header_Info *info,
                                       ACE_Message_Block &mb)
{
  char *p = mb.rd_ptr ();
  char *q;

  q = this->skipset (":\n", p, mb.wr_ptr ());
  if (q == mb.wr_ptr ())
    {
      // no more progress can be made until we find a ':'
      return 1;
    }
  if (*q != '\n' && q == p)
    {
      // Ignore empty header type names
      info->finish_last_header_value ();
      info->create_next_header_value (0);
      info->end_of_line (0);
      mb.rd_ptr (q+1);
      return 0;
    }
  if (*q == '\n')
    {
      // ignore this line
      mb.rd_ptr (q+1);
      if (q == p || ((q-1) == p && q[-1] == '\r'))
        {
          // blank line means end of headers
          info->finish_last_header_value ();
          info->create_next_header_value (0);
          info->end_of_headers (1);
          if (mb.rd_ptr () == mb.wr_ptr ())
            mb.crunch ();
          return 1;
        }

      // not a blank line, but no ':', so ignore it
      info->finish_last_header_value ();
      info->create_next_header_value (0);
      return 0;
    }

  // otherwise, we have a header type name!
  *q = '\0';
  info->create_next_header_value (p);
  info->end_of_line (0);

  mb.rd_ptr (q+1);
  return 0;
}

int
JAWS_Parse_Headers::parse_header_value (JAWS_Header_Info *info,
                                        ACE_Message_Block &mb)
{
  // break --> return 1;
  // continue --> return 0;

  char *q = mb.rd_ptr ();

  if (info->last_header_data () == 0)
    {
      // Ignoring this header (it is too long or something).

      q = this->skipset ("\n", mb.rd_ptr (), mb.wr_ptr ());
      if (q == mb.wr_ptr ())
        {
          info->end_of_line (0);
          mb.rd_ptr (q);

          // Move the rd_ptr back one character if the last thing we
          // see is a carriage return.  Assert: wr_ptr > rd_ptr.
          if (q[-1] == '\r')
            mb.rd_ptr (q-1);

          return 1;
        }

      if (*q == '\0')
        {
          // We are in the middle of binary data.  Get out!
          mb.rd_ptr (q);
          info->end_of_line (1);
          info->end_of_headers (1);
          return 1;
        }

      // Move past the newline, set the end of line flag
      if (*q == '\n')
        {
          info->end_of_line (1);
          q++;
        }
      mb.rd_ptr (q);

      return 0;
    }
  else
    {
      if (info->end_of_line ())
        {
          // Skip over leading linear white space
          q = this->skipcset (" \t", mb.rd_ptr (), mb.wr_ptr ());
          if (q == mb.wr_ptr ())
            {
              // need more input
              info->end_of_line (1);
              mb.rd_ptr (q-1);
              return 1;
            }

          if (*q != '\n')
            info->append_last_header_value (' ');
        }

      // Append to last header value character by character
      while (q < mb.wr_ptr ())
        {
          if (*q == '\n')
            break;
          info->append_last_header_value (*q);
          q++;
        }

      // Need more input
      if (q == mb.wr_ptr ())
        {
          mb.rd_ptr (q);
          info->end_of_line (0);
          return 1;
        }

      // Reached a newline
      if (*q == '\n')
        {
          // Reduce by one character if line discipline is "\r\n"
          if (info->append_last_header_value () == '\r')
            info->reduce_last_header_value ();

          // Move past newline, set end of line flag
          mb.rd_ptr (q+1);
          info->end_of_line (1);

          return 0;
        }
    }

  // NOT REACHED
  return 1;
}


JAWS_Header_Info::JAWS_Header_Info (void)
  : end_of_headers_ (0),
    end_of_line_ (1),
    last_header_data_ (0),
    last_header_length_ (0),
    status_ (0)
{
}

JAWS_Header_Info::~JAWS_Header_Info (void)
{
  JAWS_Header_Table_Iterator iter (this->table_);
  JAWS_Header_Data *data_ptr;

  for (iter.first (); !iter.done (); iter.advance ())
    {
      data_ptr = iter.next ();
      if (data_ptr)
        delete data_ptr;
    }
}

void
JAWS_Header_Info::dump (void)
{
  JAWS_Header_Table_Iterator iter (this->table_);

  ACE_DEBUG ((LM_DEBUG, "== BEGIN HEADER INFO DUMP ==\n"));
  for (iter.first (); ! iter.done (); iter.advance ())
    {
      JAWS_Header_Data *data;
      data = iter.next ();
      if (data != 0)
        ACE_DEBUG ((LM_DEBUG,
                    "%s -- %s\n",
                    data->header_name (), data->header_value ()));
      else
        ACE_DEBUG ((LM_DEBUG, "NULL ENTRY\n"));
    }
  ACE_DEBUG ((LM_DEBUG, "== END HEADER INFO DUMP ==\n"));
}

JAWS_Headers *
JAWS_Header_Info::table (void)
{
  return &(this->table_);
}

void
JAWS_Header_Info::append_last_header_value (char c)
{
  if (this->last_header_data_ == 0)
    return;

  if (this->last_header_length_ == 0 && (c == ' ' || c == '\t'))
    return;

  if (this->last_header_length_ < MAX_HEADER_LENGTH-1)
    {
      this->header_buf_[this->last_header_length_] = c;
      this->last_header_length_++;
      this->header_buf_[this->last_header_length_] = '\0';
    }

}

int
JAWS_Header_Info::append_last_header_value (void)
{
  if (this->last_header_data_ == 0 || this->last_header_length_ == 0)
    return -1;

  return this->header_buf_[this->last_header_length_-1];
}

void
JAWS_Header_Info::append_last_header_value (const char *begin, const char *end)
{
  if (this->last_header_data_ == 0)
    return;

  while (this->last_header_length_ < MAX_HEADER_LENGTH-1)
    {
      if (begin == end)
        break;

      this->header_buf_[this->last_header_length_] = *begin;
      this->last_header_length_++;
      begin++;
    }

  this->header_buf_[this->last_header_length_] = '\0';
}

void
JAWS_Header_Info::reduce_last_header_value (void)
{
  if (this->last_header_data_ == 0) return;

  if (this->last_header_length_ > 0)
    {
      this->last_header_length_--;
      this->header_buf_[this->last_header_length_] = '\0';
    }
}

void
JAWS_Header_Info::create_next_header_value (char *ht)
{
  if (ht == 0)
    {
      // discard last header data

      delete this->last_header_data_;
      this->last_header_data_ = 0;
      this->last_header_length (0);
      return;
    }

  this->finish_last_header_value ();

  if (this->status () == JAWS_Header_Info::STATUS_CODE_OK)
    {
      // create a new last_header_data_ node

      this->last_header_data_ = new JAWS_Header_Data (ht, 0);
      // The above performs a strdup.

      if (this->last_header_data_ == 0 || this->last_header_name () == 0)
        {
          this->status (JAWS_Header_Info::STATUS_CODE_NO_MEMORY);
          delete this->last_header_data_;
          this->last_header_data_ = 0;
        }
      this->last_header_length (0);
      this->header_buf_[0] = '\0';
    }
}

void
JAWS_Header_Info::finish_last_header_value (void)
{
  if (this->last_header_data_ != 0)
    {
      // prepare to insert last header data into the table.

      this->last_header_data_->header_value (this->header_buf ());
      // The above performs a strdup.

      if (this->status () == JAWS_Header_Info::STATUS_CODE_OK)
        this->table_.insert (this->last_header_data_);
      else
        delete this->last_header_data_;
      this->last_header_data_ = 0;
    }
}

char *
JAWS_Header_Info::header_buf (void)
{
  return this->header_buf_;
}

const char *
JAWS_Header_Info::last_header_name (void) const
{
  return this->last_header_data_ ? this->last_header_data_->header_name () : 0;
}

const JAWS_Header_Data *
JAWS_Header_Info::last_header_data (void) const
{
  return this->last_header_data_;
}

ACCESSOR(int,JAWS_Header_Info,last_header_length)
ACCESSOR(int,JAWS_Header_Info,end_of_line)
ACCESSOR(int,JAWS_Header_Info,end_of_headers)
ACCESSOR(int,JAWS_Header_Info,status)

