/* -*- c++ -*- */

//=============================================================================
/**
 *  @file    Parse_Headers.h
 *
 *  $Id: Parse_Headers.h 80826 2008-03-04 14:51:23Z wotte $
 *
 *  @author James Hu
 */
//=============================================================================


#ifndef PARSE_HEADERS_H
#define PARSE_HEADERS_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class Headers_Map_Item
{
friend class Headers_Map;
friend class Headers;

private:
  Headers_Map_Item (void);
  ~Headers_Map_Item (void);

  // operator const char * (void) const;
  Headers_Map_Item &operator= (char *);
  Headers_Map_Item &operator= (const char *);
  Headers_Map_Item &operator= (const Headers_Map_Item &);

public:
  const char *header (void) const;
  const char *value (void) const;

private:
  const char *header_;
  const char *value_;
};

/**
 * @class Headers_Map
 *
 * @brief Map textual headings to header values (e.g. "Subject:" maps to
 * "Re: My left foot"
 */
class Headers_Map
{
public:
  Headers_Map (void);
  ~Headers_Map (void);

  Headers_Map_Item &operator[] (const char *const header);
  const Headers_Map_Item &operator[] (const char *const header) const;

  enum
  {
    MAX_HEADERS = 100
  };

  int mapped (const char *const header) const;

private:
  Headers_Map_Item *find (const char *const header) const;
  Headers_Map_Item *place (const char *const header);
  static int compare (const void *item1, const void *item2);

private:
  Headers_Map_Item map_[MAX_HEADERS];
  Headers_Map_Item garbage_;

  int num_headers_;
};

/**
 * @class Headers
 *
 * @brief A general mechanism to parse headers of Internet text headers.
 *
 * Allow interesting headers to be inserted and later associated
 * with values.  This implementation assumes the parsing of headers
 * will be done from ACE_Message_Blocks.
 */
class Headers
{
public:
  Headers (void);
  ~Headers (void);

  void recognize (const char *const header);

  void parse_header_line (char *const header_line);

  /**
   * -1 -> end of line but not complete header line
   *  0 -> no end of line marker
   *  1 -> complete header line
   */
  int complete_header_line (char *const header_line);

  int end_of_headers (void) const;

  enum
  {
    MAX_HEADER_LINE_LENGTH = 8192
  };

  Headers_Map_Item &operator[] (const char *const header);
  const Headers_Map_Item &operator[] (const char *const header) const;

private:
  int end_of_line (char *&line, int &offset) const;

private:
  Headers_Map map_;
  int done_;
};

#endif /* PARSE_HEADERS_H */
