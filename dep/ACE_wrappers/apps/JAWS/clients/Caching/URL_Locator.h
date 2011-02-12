/* -*- C++ -*- */

// $Id: URL_Locator.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    none
//
// = FILENAME
//    URL_Locator.h
//
// = AUTHOR
//    Nanbor Wang
//
// ============================================================================

#ifndef ACE_URL_LOCATOR_H
#define ACE_URL_LOCATOR_H

#include "URL_Properties.h"

class ACE_Svc_Export ACE_URL_Locator
  // = TITLE
  //     Abstract Base class designates what interfaces a URL_Locator
  //     should provide.
  //
  // = DESCRIPTION
  //     This class defines the basic URL_Locator APIs.
  //     An URL locator provides services for URL clients to
  //     query specific URL location that has certain properties
  //     and URL providers to export their services and a set of
  //     APIs to maintain their offers.
{
public:
  // Request type
  enum ACE_URL_Locator_Op_Type
  {
    QUERY = 0,
    EXPORT,
    WITHDRAW,
    DESCRIBE,
    MODIFY,
    INVALID_OPERATION           // LAST
  };

  static const char * const opname[];
  // Human Readable operation name.

  // = Specify how to select offers.
  enum ACE_Selection_Criteria
  {
    NONE = 0,                   // URL that contains none of the properties.
    SOME,                       // URL that contains some of the properties.
    ALL,                        // URL that contains all of the properties.
    INVALID_SELECTION           // Invalid.
  };

  static const char * const selection_name[];

  enum ACE_URL_Locator_Error
    // errno will set to one of these value.
  {
    OK,                         // Everything is fine.
    OFFER_EXIST,                // trying to register an offer.
                                // that is already exist in repository.
    NO_SUCH_OFFER,              // No such offer in the repository.
    INVALID_ARGUMENT,           // Invalid argument encountered.
    UNIMPLEMENTED,              // function not implemented.
    UNKNOWN,                    // Unknown error.
    MAX_URL_ERROR
  };
  // Possible error code of URL_Locator.

  static const char * const err_name[];
  // Human readable error status.

  virtual ~ACE_URL_Locator (void) = 0;
  // Default destructor.

  virtual int url_query (const ACE_Selection_Criteria how,
                         const ACE_URL_Property_Seq *pseq,
                         const size_t how_many,
                         size_t &num_query,
                         ACE_URL_Offer_Seq *offer) = 0;
  // Query the locator for HTTP with designate properties (none, some,
  // or all).  The locator being queried will return a sequence of
  // offers with <how_many> offers in it.  This interface allocates
  // <offer> so users must deallocate it after use.

  virtual int export_offer (ACE_URL_Offer *offer,
                            ACE_WString &offer_id) = 0;
  // Export an offer to the locator.

  virtual int withdraw_offer (const ACE_WString &offer_id) = 0;
  // Withdraw an offer.  return 0 if succeed, -1 otherwise.

  virtual int describe_offer (const ACE_WString &offer_id,
                              ACE_URL_Offer *offer) = 0;
  // Query a specific offer.

  virtual int modify_offer (const ACE_WString &offer_id,
                            const ACE_WString *url = 0,
                            const ACE_URL_Property_Seq *del = 0,
                            const ACE_URL_Property_Seq *modify = 0) = 0;
  // Modify a previously registered offer.

  virtual const char *error_status (void);
  // Provide a human readable error status.
};

#endif /* ACE_WEB_LOCATOR_H */
