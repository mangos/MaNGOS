// $Id: http_base.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef HTTPU_HTTP_BASE_H
#define HTTPU_HTTP_BASE_H

#include "ace/Message_Block.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "JAWS/Parse_Headers.h"
#include "HTTPU/http_export.h"
#include "HTTPU/http_status.h"
#include "HTTPU/http_headers.h"

class HTTPU_Export HTTP_Base : public HTTP_SCode_Base
{
public:

  HTTP_Base (void);
  virtual ~HTTP_Base (void);

  virtual int receive (ACE_Message_Block &mb);
  virtual int deliver (ACE_Message_Block &mb);

  virtual int receive_payload (ACE_Message_Block &mb);
  virtual int receive_payload (ACE_Message_Block &mb, long length);

  const char * payload (void);
  unsigned long payload_size (void);

  int status (void) const;
  const char *line (void) const;
  HTTP_Headers *http_headers (void);
  JAWS_Headers *headers (void);

  int build_headers (JAWS_Headers *new_headers);
  // takes a set of new headers that will replace existing headers or
  // be added to the header list if there is no corresponding one to replace.

  void dump (void);

protected:

  virtual void parse_line (void) = 0;
  // Hook into the receive function to do specialized parsing of initial line.
  // Sets the status_ variable.

  virtual int espouse_line (void) = 0;
  // Hook into the deliver function to do specialized initial line creation.
  // Returns 0 for success and -1 for failure.

  int deliver_header_name (JAWS_Header_Data *&data);
  // Returns the next deliver state

  int deliver_header_value (JAWS_Header_Data *&data);
  // Returns the next deliver state

  virtual int extract_line (ACE_Message_Block &mb);
  // The first line of a request or a response.
  // Return 0 if more data needed.
  // Return 1 if line successfully parsed.

protected:

  int status_;
  char *line_;
  int deliver_state_;
  int no_headers_;
  HTTP_Headers info_;
  JAWS_Header_Table_Iterator iter_;
  ACE_Message_Block *mb_;
  ACE_Message_Block payload_;

};


#if defined (ACE_HAS_INLINED_OSCALLS)
#   if defined (ACE_INLINE)
#     undef ACE_INLINE
#   endif /* ACE_INLINE */
#   define ACE_INLINE inline
#   include "HTTPU/http_base.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */

#endif /* !defined (HTTPU_HTTP_BASE_H) */
