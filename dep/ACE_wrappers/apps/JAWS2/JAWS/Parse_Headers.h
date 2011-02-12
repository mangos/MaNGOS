/* -*- c++ -*- */
// $Id: Parse_Headers.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_PARSE_HEADERS_H
#define JAWS_PARSE_HEADERS_H

#include "JAWS/Export.h"
#include "ace/Singleton.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Message_Block.h"
#include "ace/Synch_Traits.h"

#include "Headers.h"

class JAWS_Export JAWS_Header_Info
{
public:
  JAWS_Header_Info (void);
  ~JAWS_Header_Info (void);

  int end_of_line (void) const;
  void end_of_line (int flag);

  const char *last_header_name (void) const;

  int last_header_length (void) const;
  void last_header_length (int len);

  const JAWS_Header_Data * last_header_data (void) const;

  char *header_buf (void);

  void append_last_header_value (char c);
  int append_last_header_value (void);
  void append_last_header_value (const char *begin, const char *end);
  void reduce_last_header_value (void);

  void create_next_header_value (char *ht);
  // This will insert last_header_data into the table if it is not
  // null.  Then, it will create a new header_data node and populate
  // it.  If ht is null, last_header_data is not inserted.

  void finish_last_header_value (void);
  // This will insert last_header_data into the table if it is not
  // null.

  int end_of_headers (void) const;
  void end_of_headers (int flag);

  int status (void) const;
  void status (int s);

  JAWS_Headers *table (void);

  enum STATUS_CODE
  {
    STATUS_CODE_OK = 0,
    STATUS_CODE_NO_MEMORY,
    STATUS_CODE_TOO_LONG
  };

  enum
  {
    MAX_HEADER_LENGTH = 8192
  };
  // Note that RFC 822 does not mention the maximum length of a header
  // line.  So in theory, there is no maximum length.
  // In Apache, they assume that each header line should not exceed
  // 8K.  Who am I to disagree?

  void dump (void);

private:
  int end_of_headers_;
  int end_of_line_;

  JAWS_Header_Data *last_header_data_;

  int last_header_length_;
  int status_;

  char header_buf_[MAX_HEADER_LENGTH];
  JAWS_Headers table_;
};

class JAWS_Export JAWS_Parse_Headers
{
public:

  int parse_headers (JAWS_Header_Info *info, ACE_Message_Block &mb);
  // Return 0 means need more data, and call it again.
  // Return 1 means all done or error.

  int parse_header_name (JAWS_Header_Info *info, ACE_Message_Block &mb);
  // Return 0 means reiterate on remaining input.
  // Return 1 means input has ended (either because it ended
  // prematurely, or that there are no more headers).

  int parse_header_value (JAWS_Header_Info *info, ACE_Message_Block &mb);
  // Return 0 means reiterate on remaining input.
  // Return 1 means input has ended or that an error has occurred.

  char * skipset (const char *set, char *start, char *end);
  // Scans from start to end for characters that match skip set.
  // Returns pointer to first location between start and end of a
  // character that is in the skip set.

  char * skipcset (const char *set, char *start, char *end);
  // Scans from start to end for characters that match skip set.
  // Returns pointer to first location between start and end of a
  // character that is *not* in the skip set.

};

typedef ACE_Singleton<JAWS_Parse_Headers, ACE_SYNCH_MUTEX>
        JAWS_Parse_Headers_Singleton;


#endif /* !defined (JAWS_PARSE_HEADERS_H) */
