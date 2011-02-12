// $Id: http_status.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/Log_Msg.h"
#include "HTTPU/http_status.h"

const char *
HTTP_SCode::table_ [HTTP_SCode::SC_TABLE_SIZE];

HTTP_SCode_Node HTTP_SCode_Base::STATUS_OK (200, "OK");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_CREATED (201, "Created");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_ACCEPTED (202, "Accepted");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_NO_CONTENT (204, "No Content");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_MULTIPLE_CHOICES (300,
                                                          "Multiple Choices");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_MOVED_PERMANENTLY (301,
                                                           "Moved Permanently");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_MOVED_TEMPORARILY (302,
                                                           "Moved Temporarily");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_NOT_MODIFIED (304, "Not Modified");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_INSUFFICIENT_DATA (399,
                                                           "Insufficient Data");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_BAD_REQUEST (400, "Bad Request");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_UNAUTHORIZED (401, "Unauthorized");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_FORBIDDEN (403, "Forbidden");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_NOT_FOUND (404, "Not Found");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_INTERNAL_SERVER_ERROR (500,
                                                               "Internal Server Error");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_NOT_IMPLEMENTED (501,
                                                         "Not Implemented");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_BAD_GATEWAY (502, "Bad Gateway");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_SERVICE_UNAVAILABLE (503,
                                                             "Service Unavailable");
HTTP_SCode_Node HTTP_SCode_Base::STATUS_QUIT (599, "Quit");

HTTP_SCode_Node HTTP_SCode_Base::DUMMY (0, 0);

HTTP_SCode_Node::HTTP_SCode_Node (int code, const char *code_str)
  : code_ (code),
    code_str_ (code_str)
{
  if ((HTTP_SCode::MIN_STATUS_CODE <= code)
      && (code <= HTTP_SCode::MAX_STATUS_CODE))
    HTTP_SCode::table_[code - HTTP_SCode::MIN_STATUS_CODE] = code_str;
}

const char *
HTTP_SCode::operator[] (int i) const
{
  const char *s = "Unknown";

  if (MIN_STATUS_CODE <= i && i <= MAX_STATUS_CODE)
    s = this->table_[i - MIN_STATUS_CODE];

  return s;
}

HTTP_SCode *
HTTP_SCode::instance (void)
{
  return HTTP_SCode_Singleton::instance ();
}

void
HTTP_SCode::dump (void)
{
  for (int i = 0; i < SC_TABLE_SIZE; i++)
    ACE_DEBUG ((LM_DEBUG, "%s\n", this->table_[i]));
}

HTTP_SCode::HTTP_SCode (void)
{
  int i;
  for (i = 0; i < SC_TABLE_SIZE; i++)
    {
      if (this->table_[i] == 0)
        this->table_[i] = this->table_[(i/100) * 100];
    }
}

HTTP_SCode::~HTTP_SCode (void)
{
}

#if !defined (ACE_HAS_INLINED_OSCALLS)
#   include "HTTPU/http_status.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */

