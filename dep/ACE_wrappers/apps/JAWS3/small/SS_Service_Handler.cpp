// $Id: SS_Service_Handler.cpp 91730 2010-09-13 09:31:11Z johnnyw $

#define ACE_BUILD_SVC_DLL

#include "ace/Get_Opt.h"

#include "jaws3/Concurrency.h"

#include "SS_Service_Handler.h"
#include "SS_State_READ.h"
#include "SS_Data.h"

TeraSS_Service_Handler::TeraSS_Service_Handler (void)
  : JAWS_Protocol_Handler (TeraSS_State_READ::instance (), & this->data_)
  , data_ (this)
{
}

int
TeraSS_Service_Handler::open (void *)
{
  int result = JAWS_Concurrency::instance ()->putq (this);
  if (result < 0)
    return -1;

  return 0;
}

int
TeraSS_Service_Handler::close (unsigned long)
{
  delete this;
  return 0;
}

int
TeraSS_Acceptor::init (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt opt (argc, argv, ACE_TEXT("p:"));

  unsigned short p = 0;

  int c;
  while ((c = opt ()) != -1)
    switch (c)
      {
      case 'p':
        p = (unsigned short) ACE_OS::atoi (opt.optarg);
        break;
      default:
        break;
      }

  if (p == 0)
    p = 5555;

  if (this->open (ACE_INET_Addr (p)) == -1)
    {
      ACE_DEBUG ((LM_DEBUG, "%p\n", "ACE_Acceptor::open"));
      return -1;
    }

  return 0;
}

ACE_SVC_FACTORY_DEFINE (TeraSS_Acceptor)
