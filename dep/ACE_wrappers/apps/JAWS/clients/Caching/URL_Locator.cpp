// $Id: URL_Locator.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#if !defined (ACE_URL_LOCATOR_C)
#define ACE_URL_LOCATOR_C

#include "URL_Locator.h"

const char * const
ACE_URL_Locator::opname[] =
// Human readable operation name
{
  "Query",
  "Export",
  "Withdraw",
  "Describe",
  "Modify",
  "Invalid Operation"
};

const char * const
ACE_URL_Locator::selection_name[] =
{
  "None",
  "Some",
  "All",
  "Invalid Selection"
};

const char * const
ACE_URL_Locator::err_name[] =
{
  "No error",
  "Offer already exist",
  "no such offer",
  "invalid argument",
  "function not implemented",
  "unknown error"
};

ACE_URL_Locator::~ACE_URL_Locator (void)
{
}

const char *
ACE_URL_Locator::error_status (void)
{
  return "Not implemented yet.";
}

#endif /* ACE_URL_LOCATOR_C */
