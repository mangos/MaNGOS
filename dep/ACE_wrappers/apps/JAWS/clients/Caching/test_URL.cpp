// $Id: test_URL.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "Locator_Request_Reply.h"

int ACE_TMAIN (int , ACE_TCHAR *[])
{
  ACE_URL_Locator_Request original, derived;

  ACE_URL_Property_Seq ouch1(3);
  ouch1[0].name ("name 1");
  ouch1[0].value ("value 1");
  ouch1[1].name ("name 2");
  ouch1[1].value ("value 2");
  ouch1[2].name ("name 3");
  ouch1[2].value ("value 3");

  original.url_query (1, ouch1, 3);

  original.dump ();
  derived.dump ();

  original.encode ();

  derived.decode ((void*) original.buffer ());

  derived.dump ();

  return 0;
}
