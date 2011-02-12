// $Id: Config_File.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_string.h"
#include "ace/FILE_Connector.h"
#include "ace/Message_Block.h"
#include "ace/Singleton.h"
#include "ace/Unbounded_Queue.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Config_File.h"
#include "jaws3/Symbol_Table.h"

// = Helper class to manage "constant" strings.

class JAWS_strings
{
public:

  ~JAWS_strings (void)
  {
    void *p;
    while (this->queue_.dequeue_head (p) != -1)
      ACE_OS::free (p);
  }

  const ACE_TCHAR * duplicate (const ACE_TCHAR *s)
  {
    void **x;
    const ACE_TCHAR *d = 0;
    ACE_Unbounded_Queue_Iterator<void *> iter (this->queue_);

    while (iter.next (x))
      {
        d = (const ACE_TCHAR *) *x;
        if (ACE_OS::strcmp (d, s) == 0)
          break;
        d = 0;
        iter.advance ();
      }

    if (d == 0)
      {
        d = ACE_OS::strdup (s);
        this->queue_.enqueue_tail ((void *) d);
      }

    return d;
  }

private:

  ACE_Unbounded_Queue<void *> queue_;

};


// = Underlying implementation class.


class JAWS_Config_File_Impl
{
public:

  JAWS_Config_File_Impl (const ACE_TCHAR *config_file);
  ~JAWS_Config_File_Impl (void);
  int find (const ACE_TCHAR *key, const ACE_TCHAR *&value);

  void parse_file (void);
  void reset (void);
  void dump (void);

  enum { JAWS_CONFIG_FILE_SYMBOL_TABLE_SIZE = 211 };

private:

  ACE_FILE_Addr faddr_;
  JAWS_strings *strings_;
  JAWS_Symbol_Table *symbols_;

};

JAWS_Config_File_Impl::JAWS_Config_File_Impl (const ACE_TCHAR *config_file)
  : faddr_ (config_file)
  , strings_ (0)
  , symbols_ (0)
{
  this->strings_ = new JAWS_strings;
  this->symbols_ = new JAWS_Symbol_Table (JAWS_CONFIG_FILE_SYMBOL_TABLE_SIZE);
  this->parse_file ();
}

JAWS_Config_File_Impl::~JAWS_Config_File_Impl (void)
{
  delete this->symbols_;
  this->symbols_ = 0;
  delete this->strings_;
  this->strings_ = 0;
}

int
JAWS_Config_File_Impl::find (const ACE_TCHAR *key, const ACE_TCHAR *&value)
{
  return this->symbols_->find (key, value);
}

void
JAWS_Config_File_Impl::parse_file (void)
{
  ACE_FILE_Connector fconnector;
  ACE_FILE_IO fio;

  if (fconnector.connect ( fio
                         , this->faddr_
                         , 0
                         , ACE_Addr::sap_any
                         , 0
                         , O_RDONLY
                         ) == -1)
    return;

  ACE_Message_Block buffer (8192);
  ACE_Message_Block line (4096);
  ssize_t count = 0;
  const ACE_TCHAR *sym_name;
  const ACE_TCHAR *sym_value;
  int last_line_was_read = 0;
  ACE_TCHAR *end_of_current_line = 0;
  ACE_TCHAR *p = 0;

  while (last_line_was_read
         || (count = fio.recv (buffer.wr_ptr (), buffer.space () - 2)) >= 0)
    {
      end_of_current_line = 0;

      // Make sure input is newline terminated if it is the last line,
      // and always null terminated.
      if (! last_line_was_read)
        {
          if (count > 0)
            {
              buffer.wr_ptr (count);
              // Scan forward for at least one newline character
              p = buffer.rd_ptr ();
              while (p != buffer.wr_ptr ())
                {
                  if (*p == '\n')
                    break;
                  p++;
                }

              if (p == buffer.wr_ptr ())
                continue;

              end_of_current_line = p;
            }
          else
            {
              if (buffer.wr_ptr ()[-1] != '\n')
                {
                  buffer.wr_ptr ()[0] = '\n';
                  buffer.wr_ptr (1);
                }

              last_line_was_read = 1;
            }

          buffer.wr_ptr ()[0] = '\0';
        }

      if (end_of_current_line == 0)
        {
          end_of_current_line = buffer.rd_ptr ();
          while (*end_of_current_line != '\n')
            end_of_current_line++;
        }

      // If buffer is not pointing to a continuation line, or there is
      // no more input, then can commit the scanned configuration
      // line.
      if (line.length () != 0
          && ((last_line_was_read && buffer.length () == 0)
              || (buffer.rd_ptr ()[0] != ' '
                  && buffer.rd_ptr ()[0] != '\t')))
        {
          ACE_TCHAR *name = 0;
          ACE_TCHAR *value = 0;

          name = line.rd_ptr ();
          for (p = name; *p != '\0'; p++)
            {
              if (*p == '=')
                {
                  line.rd_ptr (p+1);
                  while (p != name && (p[-1] == ' ' || p[-1] == '\t'))
                    p--;
                  *p = '\0';
                }
            }

          if (*name)
            {
              value = line.rd_ptr ();
              while (*value == ' ' || *value == '\t')
                value++;
              p = line.wr_ptr ();
              while (p != value && (p[-1] == ' ' || p[-1] == '\t'))
                p--;
              *p = '\0';

              sym_name = this->strings_->duplicate (name);
              sym_value = this->strings_->duplicate (value);
              this->symbols_->rebind (sym_name, sym_value);
            }

          line.reset ();
        }

      // If we are done, we are done!
      if (last_line_was_read && buffer.length () == 0)
        break;

      // If the buffer is pointing at a comment line, ignore it.
      if (buffer.rd_ptr ()[0] == '#'
          || buffer.rd_ptr ()[0] == '\n'
          || (buffer.rd_ptr ()[0] == '\r' && buffer.rd_ptr ()[1] == '\n'))
        {
          buffer.rd_ptr (end_of_current_line + 1);
          buffer.crunch ();
          continue;
        }

      // Whatever is left is either the start of a name-value-pair or a
      // continuation of one.
      line.copy (buffer.rd_ptr (),
                 end_of_current_line - buffer.rd_ptr ());
      p = line.wr_ptr ();
      while (p != line.rd_ptr () && (p[-1] == ' ' || p[-1] == '\t'))
        p--;
      line.wr_ptr (p);
      line.wr_ptr ()[0] = '\0';
      buffer.rd_ptr (end_of_current_line + 1);
      buffer.crunch ();
    }

  fio.close ();
}

void
JAWS_Config_File_Impl::reset (void)
{
  delete this->symbols_;
  delete this->strings_;
  this->strings_ = new JAWS_strings;
  this->symbols_ = new JAWS_Symbol_Table (JAWS_CONFIG_FILE_SYMBOL_TABLE_SIZE);
  this->parse_file ();
}

void
JAWS_Config_File_Impl::dump (void)
{
  JAWS_SYMBOL_TABLE_ITERATOR iter (*this->symbols_);
  JAWS_SYMBOL_TABLE_ENTRY *entry = 0;

  while (iter.next (entry))
    {
      ACE_DEBUG ((LM_DEBUG, "[%D|%t] %s=%s\n",
                  entry->ext_id_,
                  entry->int_id_));
      iter.advance ();
    }
}

JAWS_Config_File::JAWS_Config_File (const ACE_TCHAR *config_file,
                                    const ACE_TCHAR *config_dir)
{
  ACE_TCHAR filename[MAXPATHLEN];
  ACE_OS::strcpy (filename, config_dir);
  ACE_OS::strcat (filename, config_file);

  this->impl_ = new JAWS_Config_File_Impl (filename);
}

int
JAWS_Config_File::find (const ACE_TCHAR *key, const ACE_TCHAR *&value)
{
  return this->impl_->find (key, value);
}

void
JAWS_Config_File::reset (void)
{
  this->impl_->reset ();
}

void
JAWS_Config_File::dump (void)
{
  this->impl_->dump ();
}

