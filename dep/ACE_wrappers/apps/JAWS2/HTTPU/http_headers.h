// $Id: http_headers.h 80826 2008-03-04 14:51:23Z wotte $

// by James Hu
// Borrowed from HTTP_Headers.*, which appears to be irrelevent now anyway.

#ifndef HTTPU_HTTP_HEADERS_H
#define HTTPU_HTTP_HEADERS_H

#include "ace/RB_Tree.h"
#include "ace/Null_Mutex.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Singleton.h"

#include "JAWS/Parse_Headers.h"
#include "HTTPU/http_export.h"

// A header file on HP-UX defines SERVER
#if defined (SERVER)
#undef SERVER
#endif /* SERVER */

class HTTP_Headers;

class HTTPU_Export HTTP_Hdr_Node
{
  // Constructor should be passed literal strings.
  friend class HTTP_HCodes;

public:
  operator int (void) const;
  operator const char * (void) const;
  const char * format (void) const;

private:
  HTTP_Hdr_Node (const char *token, const char *format);

private:
  int index_;
  const char *token_;
  const char *format_;
};


class HTTP_HCodes;

class HTTPU_Export HTTP_Header_Nodes : public ACE_RB_Tree<int, const HTTP_Hdr_Node *, ACE_Less_Than<int>, ACE_Null_Mutex>
{
  friend class HTTP_HCodes;
  friend class HTTP_Hdr_Node;

public:
  HTTP_Header_Nodes (void);

private:
  int num_header_strings_;
};

typedef ACE_Singleton<HTTP_Header_Nodes, ACE_SYNCH_MUTEX>
        HTTP_Header_Nodes_Singleton;

class HTTPU_Export HTTP_HCodes
{
public:
  HTTP_HCodes (void);

  enum {
    REPLACE_HEADER = 1,  // Remove any existing header that matches first
    APPEND_HEADER = 2,   // Unconditionally append the header
    INSERT_HEADER = 4,   // Insert header if one does not already exist
    APPENDTO_HEADER = 8  // Concatenate data to existing header value
  };

  static HTTP_Hdr_Node HTTP;
  static HTTP_Hdr_Node ACCEPT;
  static HTTP_Hdr_Node ACCEPTCHARSET;
  static HTTP_Hdr_Node ACCEPTENCODING;
  static HTTP_Hdr_Node ACCEPTLANGUAGE;
  static HTTP_Hdr_Node ACCEPTRANGES;
  static HTTP_Hdr_Node AGE;
  static HTTP_Hdr_Node ALLOW;
  static HTTP_Hdr_Node AUTHORIZATION;
  static HTTP_Hdr_Node CACHECONTROL;
  static HTTP_Hdr_Node CONNECTION;
  static HTTP_Hdr_Node CONTENTENCODING;
  static HTTP_Hdr_Node CONTENTLENGTH;
  static HTTP_Hdr_Node CONTENTLOCATION;
  static HTTP_Hdr_Node CONTENTMD5;
  static HTTP_Hdr_Node CONTENTRANGE;
  static HTTP_Hdr_Node CONTENTTYPE;
  static HTTP_Hdr_Node DATE;
  static HTTP_Hdr_Node ETAG;
  static HTTP_Hdr_Node EXPECT;
  static HTTP_Hdr_Node EXPIRES;
  static HTTP_Hdr_Node FROM;
  static HTTP_Hdr_Node HOST;
  static HTTP_Hdr_Node IFMATCH;
  static HTTP_Hdr_Node IFMODIFIEDSINCE;
  static HTTP_Hdr_Node IFNONEMATCH;
  static HTTP_Hdr_Node IFRANGE;
  static HTTP_Hdr_Node IFUNMODIFIEDSINCE;
  static HTTP_Hdr_Node LASTMODIFIED;
  static HTTP_Hdr_Node LOCATION;
  static HTTP_Hdr_Node MAXFORWARDS;
  static HTTP_Hdr_Node PRAGMA;
  static HTTP_Hdr_Node PROXYAUTHENTICATE;
  static HTTP_Hdr_Node PROXYAUTHORIZATION;
  static HTTP_Hdr_Node RANGE;
  static HTTP_Hdr_Node REFERER;
  static HTTP_Hdr_Node RETRYAFTER;
  static HTTP_Hdr_Node SERVER;
  static HTTP_Hdr_Node TE;
  static HTTP_Hdr_Node TRAILER;
  static HTTP_Hdr_Node TRANSFERENCODING;
  static HTTP_Hdr_Node UPGRADE;
  static HTTP_Hdr_Node USERAGENT;
  static HTTP_Hdr_Node VARY;
  static HTTP_Hdr_Node VIA;
  static HTTP_Hdr_Node WARNING;
  static HTTP_Hdr_Node WWWAUTHENTICATE;
  static HTTP_Hdr_Node GET;
  static HTTP_Hdr_Node HEAD;
  static HTTP_Hdr_Node POST;
  static HTTP_Hdr_Node PUT;
  static HTTP_Hdr_Node QUIT;
  static HTTP_Hdr_Node DUNNO;
  static HTTP_Hdr_Node META;
  static HTTP_Hdr_Node A;
  static HTTP_Hdr_Node SCRIPT;
  static HTTP_Hdr_Node APPLET;

  static const int &NUM_HEADER_STRINGS;

protected:

  const HTTP_Hdr_Node &hcode (int type) const;

protected:

  HTTP_Header_Nodes *header_nodes_;
};

class HTTPU_Export HTTP_Headers : public JAWS_Header_Info, public HTTP_HCodes
{
public:
  const char *header( int name ) const;
  const char *value( int name );
  const char *value_next( int name );
  void value_reset ( void );

public:
  HTTP_Headers (void);

  const char *header_token (int name) const;
  const char *header_strings (int name) const;

};


#if defined (ACE_HAS_INLINED_OSCALLS)
#   if defined (ACE_INLINE)
#     undef ACE_INLINE
#   endif /* ACE_INLINE */
#   define ACE_INLINE inline
#   include "HTTPU/http_headers.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */

#endif /* !defined (HTTPU_HTTP_HEADERS_HPP) */
