/* -*- c++ -*- */
// $Id: Headers.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_HEADERS_H
#define JAWS_HEADERS_H

#include "JAWS/Export.h"
#include "ace/Containers.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// Use scenario --

// Parse_Headers will parse out a header name and then will want to
// update "last header data" in Header_Info.  At this point, only the
// header name is strdup'd.

// Parse_Headers will then append additional data to the header value
// until the header value of "last header data" is done.

// Parse_Headers will notify Header_Info that "last header data" is
// done and is ready to be inserted into the Headers data structure.
// At this point, the header value is strdup'd.

class JAWS_Export JAWS_Header_Data
{
public:
  JAWS_Header_Data (const char *name, const char *value = 0, int type = 0);
  JAWS_Header_Data (const char *name, int type, const char *value = 0);
  ~JAWS_Header_Data (void);

  const char * header_name (void) const;
  const char * header_value (void) const;
  int header_type (void) const;

  void header_name (const char *name);
  void header_value (const char *value);
  void header_type (int type);

private:
  const char * header_name_;
  const char * header_value_;
  int header_type_;
};

typedef ACE_DLList<JAWS_Header_Data> JAWS_Header_Table;
typedef ACE_DLList_Iterator<JAWS_Header_Data> JAWS_Header_Table_Iterator;

class JAWS_Export JAWS_Headers : public JAWS_Header_Table
{
public:
  JAWS_Headers (void);
  ~JAWS_Headers (void);

  int insert (JAWS_Header_Data *new_data);
  // insert the new data 0 -> success, -1 -> failure

  JAWS_Header_Data * find (const char *const &header_name);
  // find always begins from the beginning of the list
  // result is NULL if not found

  JAWS_Header_Data * find_next (const char *const &header_name);
  // behaves like find, but from where that last find left off
  // result is NULL if not found

  void remove_all (const char *const &header_name);
  // remove all headers from list that match header_name

  JAWS_Header_Table_Iterator &iter (void);
  // returns an iterator to the headers container

private:
  JAWS_Header_Table_Iterator iter_;
};

#endif /* !defined (JAWS_HEADERS_H) */
