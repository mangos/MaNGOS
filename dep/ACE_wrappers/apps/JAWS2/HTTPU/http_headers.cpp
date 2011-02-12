// $Id: http_headers.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/RB_Tree.h"
#include "HTTPU/http_headers.h"

HTTP_Hdr_Node HTTP_HCodes::HTTP ("HTTP", "HTTP%s %s");
HTTP_Hdr_Node HTTP_HCodes::ACCEPT ("Accept", "Accept: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::ACCEPTCHARSET ("Accept-Charset",
                                          "Accept-Charset: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::ACCEPTENCODING ("Accept-Encoding",
                                           "Accept-Encoding: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::ACCEPTLANGUAGE ("Accept-Language",
                                           "Accept-Language: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::ACCEPTRANGES ("Accept-Ranges",
                                         "Accept-Ranges: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::AGE ("Age", "Age: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::ALLOW ("Allow", "Allow: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::AUTHORIZATION ("Authorization",
                                          "Authorization: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::CACHECONTROL ("Cache-Control",
                                         "Cache-Control: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::CONNECTION ("Connection", "Connection: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::CONTENTENCODING ("Content-Encoding",
                                            "Content-Encoding: %d\r\n");
HTTP_Hdr_Node HTTP_HCodes::CONTENTLENGTH ("Content-Length",
                                          "Content-Length: %d\r\n");
HTTP_Hdr_Node HTTP_HCodes::CONTENTLOCATION ("Content-Location",
                                            "Content-Location: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::CONTENTMD5 ("Content-MD5",
                                       "Content-MD5: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::CONTENTRANGE ("Content-Range",
                                         "Content-Range: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::CONTENTTYPE ("Content-Type",
                                        "Content-Type: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::DATE ("Date", "Date: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::ETAG ("ETag", "ETag: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::EXPECT ("Expect", "Expect: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::EXPIRES ("Expires", "Expires: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::FROM ("From", "From: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::HOST ("Host", "Host: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::IFMATCH ("If-Match", "If-Match: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::IFMODIFIEDSINCE ("If-Modified-Since",
                                            "If-Modified-Since: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::IFNONEMATCH ("If-None-Match",
                                        "If-None-Match: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::IFRANGE ("If-Range", "If-Range: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::IFUNMODIFIEDSINCE ("If-Unmodified-Since",
                                              "If-Unmodified-Since: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::LASTMODIFIED ("Last-Modified",
                                         "Last-Modified: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::LOCATION ("Location", "Location: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::MAXFORWARDS ("Max-Forwards",
                                        "Max-Forwards: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::PRAGMA ("Pragma", "Pragma: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::PROXYAUTHENTICATE ("Proxy-Authenticate",
                                              "Proxy-Authenticate: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::PROXYAUTHORIZATION ("Proxy-Authorization",
                                               "Proxy-Authorization: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::RANGE ("Range", "Range: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::REFERER ("Referer", "Referer: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::SERVER ("Server", "Server: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::TE ("TE", "TE: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::TRAILER ("Trailer", "Trailer: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::TRANSFERENCODING ("Transfer-Encoding",
                                             "Transfer-Encoding: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::UPGRADE ("Ugrade", "Ugrade: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::USERAGENT ("User-Agent", "User-Agent: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::VARY ("Vary", "Vary: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::VIA ("Via", "Via: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::WARNING ("Warning", "Warning: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::WWWAUTHENTICATE ("WWW-Authenticate",
                                            "WWW-Authenticate: %s\r\n");
HTTP_Hdr_Node HTTP_HCodes::GET ("GET", "GET %s HTTP/%s\r\n");
HTTP_Hdr_Node HTTP_HCodes::HEAD ("HEAD", "HEAD %s HTTP/%s\r\n");
HTTP_Hdr_Node HTTP_HCodes::POST ("POST", "POST %s HTTP/%s\r\n");
HTTP_Hdr_Node HTTP_HCodes::PUT ("PUT", "PUT %s HTTP/%s\r\n");
HTTP_Hdr_Node HTTP_HCodes::QUIT ("QUIT", "QUIT %s HTTP/%s\r\n");
HTTP_Hdr_Node HTTP_HCodes::DUNNO ("", "");
HTTP_Hdr_Node HTTP_HCodes::META ("<META", "<META %s>");
HTTP_Hdr_Node HTTP_HCodes::A ("<A", "<A %s>");
HTTP_Hdr_Node HTTP_HCodes::SCRIPT ("<SCRIPT", "<SCRIPT %s>");
HTTP_Hdr_Node HTTP_HCodes::APPLET ("<APPLET", "<APPLET %s>");


const int &HTTP_HCodes::NUM_HEADER_STRINGS
  = HTTP_Header_Nodes_Singleton::instance ()->num_header_strings_;

HTTP_Header_Nodes::HTTP_Header_Nodes (void)
  : num_header_strings_ (0)
{
}

HTTP_Hdr_Node::HTTP_Hdr_Node (const char *token, const char *format)
  : token_ (token),
    format_ (format)
{
  HTTP_Header_Nodes *header_nodes
    = HTTP_Header_Nodes_Singleton::instance ();

  this->index_ = header_nodes->num_header_strings_;
  header_nodes->insert (this->index_, this);
  header_nodes->num_header_strings_++;
}

HTTP_HCodes::HTTP_HCodes (void)
  : header_nodes_ (HTTP_Header_Nodes_Singleton::instance ())
{
}

HTTP_Headers::HTTP_Headers (void)
{
}

const char *
HTTP_Headers::header (int name) const
{
  return this->header_token (name);
}

const char *
HTTP_Headers::value (int index)
{
  this->value_reset ();
  return this->value_next (index);
}

const char *
HTTP_Headers::value_next (int index)
{
  const char *hs = 0;
  const char *hv = 0;
  JAWS_Header_Data *data;

  if (0 <= index && index < NUM_HEADER_STRINGS)
    {
      hs = this->header (index);
      data = this->table ()->find_next (hs);
      if (data != 0)
        hv = data->header_value ();
    }

  return hv;
}

void
HTTP_Headers::value_reset (void)
{
  this->table ()->iter ().first ();
}

#if !defined (ACE_HAS_INLINED_OSCALLS)
#   include "HTTPU/http_headers.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */

