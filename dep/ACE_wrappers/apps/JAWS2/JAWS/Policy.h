/* -*- c++ -*- */
// $Id: Policy.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_POLICY_H
#define JAWS_POLICY_H

#include "JAWS/Export.h"
#include "JAWS/Concurrency.h"

class JAWS_IO;
class JAWS_IO_Handler;
class JAWS_IO_Handler_Factory;

class JAWS_Export JAWS_Dispatch_Policy
  // = TITLE
  //     Policy mechanism for choosing different concurrency models.
  //
  // = DESCRIPTION
  //     Given some (unspecified) state, decides what the concurrency
  //     model should be.  (For now, we always return the same model.)
{
public:
  JAWS_Dispatch_Policy (void);
  virtual ~JAWS_Dispatch_Policy (void);

  virtual int ratio (void) = 0;
  virtual JAWS_IO * io (void) = 0;
  virtual JAWS_IO_Handler_Factory *ioh_factory (void) = 0;
  virtual JAWS_IO_Acceptor *acceptor (void) = 0;
  virtual JAWS_Concurrency_Base * concurrency (void) = 0;

  virtual void ratio (int r) = 0;
  virtual void io (JAWS_IO *iop) = 0;
  virtual void ioh_factory (JAWS_IO_Handler_Factory *factoryp) = 0;
  virtual void acceptor (JAWS_IO_Acceptor *acceptorp) = 0;
  virtual void concurrency (JAWS_Concurrency_Base *concp) = 0;
};

class JAWS_Export JAWS_Default_Dispatch_Policy : public JAWS_Dispatch_Policy
{
public:
  JAWS_Default_Dispatch_Policy (void);
  virtual ~JAWS_Default_Dispatch_Policy (void);

  virtual int ratio (void);
  virtual JAWS_IO *io (void);
  virtual JAWS_IO_Handler_Factory *ioh_factory (void);
  virtual JAWS_IO_Acceptor *acceptor (void);
  virtual JAWS_Concurrency_Base *concurrency (void);

  virtual void ratio (int r);
  virtual void io (JAWS_IO *iop);
  virtual void ioh_factory (JAWS_IO_Handler_Factory *factoryp);
  virtual void acceptor (JAWS_IO_Acceptor *acceptorp);
  virtual void concurrency (JAWS_Concurrency_Base *concp);

private:
  int ratio_;
  JAWS_Concurrency_Base *concurrency_;
  JAWS_IO_Handler_Factory *ioh_factory_;
  JAWS_IO_Acceptor *acceptor_;
  JAWS_IO *io_;
};

#endif /* !defined (JAWS_POLICY_H) */
