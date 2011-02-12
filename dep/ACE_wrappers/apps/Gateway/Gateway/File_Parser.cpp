// $Id: File_Parser.cpp 80826 2008-03-04 14:51:23Z wotte $

#ifndef FILE_PARSER_C
#define FILE_PARSER_C

#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_stdlib.h"

#include "File_Parser.h"

// This fixes a nasty bug with cfront-based compilers (like
// Centerline).
typedef FPRT::Return_Type FP_RETURN_TYPE;

// File_Parser stuff.

template <class ENTRY>
File_Parser<ENTRY>::~File_Parser (void)
{
}

template <class ENTRY> int
File_Parser<ENTRY>::open (const ACE_TCHAR filename[])
{
  this->infile_ = ACE_OS::fopen (filename, ACE_TEXT ("r"));
  if (this->infile_ == 0)
    return -1;
  else
    return 0;
}

template <class ENTRY> int
File_Parser<ENTRY>::close (void)
{
  return ACE_OS::fclose (this->infile_);
}

template <class ENTRY> FP_RETURN_TYPE
File_Parser<ENTRY>::getword (char buf[])
{
  return this->readword (buf);
}

// Get the next string from the file via this->readword()
// Check make sure the string forms a valid number.

template <class ENTRY> FP_RETURN_TYPE
File_Parser<ENTRY>::getint (ACE_INT32 &value)
{
  char buf[BUFSIZ];
  FP_RETURN_TYPE read_result = this->readword (buf);

  if (read_result == FPRT::RT_SUCCESS)
    {
      // Check to see if this is the "use the default value" symbol?
      if (buf[0] == '*')
        return FPRT::RT_DEFAULT;
      else
        {
          // ptr is used for error checking with ACE_OS::strtol.
          char *ptr = 0;

          // try to convert the buf to a decimal number
          value = ACE_OS::strtol (buf, &ptr, 10);

          // check if the buf is a decimal or not
          if (value == 0 && ptr == buf)
            return FPRT::RT_PARSE_ERROR;
          else
            return FPRT::RT_SUCCESS;
        }
    }
  else
    return read_result;
}


template <class ENTRY> FP_RETURN_TYPE
File_Parser<ENTRY>::readword (char buf[])
{
  int wordlength = 0;
  int c;

  // Skip over leading delimiters and get word.

  while ((c = ACE_OS::getc (this->infile_)) != EOF && c != '\n')
    if (this->delimiter (c))
      {
        // We've reached the end of a "word".
        if (wordlength > 0)
          break;
      }
    else
      buf[wordlength++] = c;

  buf[wordlength] = '\0';

  if (c == EOF) {
    // If EOF is just a delimiter, don't return EOF so that the word
    // gets processed.
    if (wordlength > 0)
      {
        ACE_OS::ungetc (c, this->infile_);
        return FPRT::RT_SUCCESS;
      }
    else
      // else return EOF so that read loops stop
      return FPRT::RT_EOFILE;
  }
  else if (c == '\n')
    {
      // if the EOLINE is just a delimiter, don't return EOLINE
      // so that the word gets processed
      if (wordlength > 0)
        ACE_OS::ungetc (c, this->infile_);
      else
        return FPRT::RT_EOLINE;
    }

  // Skip comments.
  if (this->comments (buf[0]))
    {
      if (this->skipline () == EOF)
        return FPRT::RT_EOFILE;
      else
        return FPRT::RT_COMMENT;
    }
  else
    return FPRT::RT_SUCCESS;
}

template <class ENTRY> int
File_Parser<ENTRY>::delimiter (char ch)
{
  return ch == ' ' || ch == ',' || ch == '\t';
}

template <class ENTRY> int
File_Parser<ENTRY>::comments (char ch)
{
  return ch == '#';
}

template <class ENTRY> int
File_Parser<ENTRY>::skipline (void)
{
  // Skip the remainder of the line.

  int c;

  while ((c = ACE_OS::getc (this->infile_)) != '\n' && c != EOF)
    continue;

  return c;
}

#endif /* _FILE_PARSER_C */
