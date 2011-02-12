/* -*- c++ -*- */
// $Id: HTTP_10.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_HTTP_10_H
#define JAWS_HTTP_10_H

#include "ace/RB_Tree.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "JAWS/Pipeline_Tasks.h"

// Forward declaration
class JAWS_HTTP_10_Request;

// Reading the initial request

class JAWS_HTTP_10_Read_Task : public JAWS_Pipeline_Handler
{
public:
  JAWS_HTTP_10_Read_Task (void);
  virtual ~JAWS_HTTP_10_Read_Task (void);

  virtual int handle_put (JAWS_Data_Block *data, ACE_Time_Value *tv);

private:
};

// Parsing the request

class JAWS_HTTP_10_Parse_Task : public JAWS_Pipeline_Handler
{
public:
  JAWS_HTTP_10_Parse_Task (void);
  virtual ~JAWS_HTTP_10_Parse_Task (void);

  virtual int handle_put (JAWS_Data_Block *data, ACE_Time_Value *tv);

};

// Write the response

class JAWS_HTTP_10_Write_Task : public JAWS_Pipeline_Handler
{
public:
  JAWS_HTTP_10_Write_Task (void);
  virtual ~JAWS_HTTP_10_Write_Task (void);

  virtual int handle_put (JAWS_Data_Block *data, ACE_Time_Value *tv);

private:
};

// Helpers

class JAWS_HTTP_10_Helper
// Static functions to enhance the lives of HTTP programmers everywhere.
{
public:

  static char *HTTP_decode_string (char *path);
  // Decode '%' escape codes in a URI

};

#endif /* !defined (JAWS_HTTP_10_H) */
