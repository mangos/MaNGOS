/* -*- C++ -*- */

// $Id: Locator_Request_Reply.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    none
//
// = FILENAME
//    Locator_Request_Response.h
//
// = AUTHOR
//    Nanbor Wang
//
// ============================================================================

#ifndef ACE_LOCATOR_REQUEST_REPLY_H
#define ACE_LOCATOR_REQUEST_REPLY_H

#include "URL_Properties.h"

class ACE_Svc_Export ACE_URL_Locator_Request
  // = TITLE
  //     A URL request message formater/wrapper.
  //
  // = DESCRIPTION
  //     This class defines a URL request data.  It is used
  //     to transform requests to an object so that we can
  //     ship them across network.
{
public:
  ACE_URL_Locator_Request (void);
  // Default ctor.

  ~ACE_URL_Locator_Request (void);
  // Default dtor.

  int url_query (const int how,
                 const ACE_URL_Property_Seq &pseq,
                 const int how_many);
  // Query the locator for HTTP with designate properties (none, some,
  // or all).  The locator being queried will return a sequence of
  // offers with <how_many> offers in it.  This interface allocates
  // <offer> so users must deallocate it after use.

  int export_offer (const ACE_URL_Offer &offer);
  // Export an offer to the locator.

  int withdraw_offer (const ACE_WString &offer_id);
  // Withdraw an offer.  return 0 if succeed, -1 otherwise.

  int describe_offer (const ACE_WString &offer_id);
  // Query a specific offer.

  int modify_offer (const ACE_WString &offer_id,
                    const char *url = 0,
                    const ACE_URL_Property_Seq &del = 0,
                    const ACE_URL_Property_Seq &modify = 0);
  // Modify a previously registered offer.

  int modify_offer (const ACE_WString &offer_id,
                    const ACE_WString *url = 0,
                    const ACE_URL_Property_Seq &del = 0,
                    const ACE_URL_Property_Seq &modify = 0);
  // Modify a previously registered offer.

  size_t encode (void);
  // Encode request for network communication.  If succeed,
  // returns the size of the buffer, otherwise, return 0.

  size_t decode (void *buffer);
  // Restore from network data.  Returns size of the buffer
  // if succeed, 0 otherwise.  When passing in a buffer,
  // caller must take the responsibility to delete the buffer
  // afterward, if so needed.

  const int how (void) const;
  const int how_many (void) const;
  const u_int opcode (void) const;
  const ACE_URL_Property_Seq *seq (void) const;
  const ACE_URL_Property_Seq *del (void) const;
  const ACE_URL_Property_Seq *modify (void) const;
  const ACE_URL_Offer *offer (void) const;
  const ACE_WString &id (void) const;
  const ACE_WString &url (void) const;
  const char *buffer (void) const;
  // A bunch of methods to access internal data.

  void dump (void) const;
  // Print out this object.

protected:
  size_t size (void);
  // Return the size of the buffer required to encode
  // this request.

  enum {
    VALID_SEQ1 = 0x1,
    VALID_SEQ2 = 0X2,
    VALID_OFFER = 0X4
  };
  // These constants used to indicate which pointers are valid.

  u_int code_;
  // Request type code.

  int how_;
  // Query method (if code_ == QUERY.)

  int how_many_;
  // How many offers are we interested in in this query.

  int valid_ptr_;
  // Bit flag to mark valid pointers within this object.

  ACE_URL_Property_Seq *seq1_;
  // For query or del in modify_offer.

  ACE_URL_Property_Seq *seq2_;
  // For modify seq. in modify_offer.

  ACE_URL_Offer *offer_;
  // Offer to export.

  ACE_WString id_;
  // Offer ID.

  ACE_WString url_;
  // URL of this offer.

  char *buffer_;
  // Buffer to store encoded data.
};

class ACE_Svc_Export ACE_URL_Locator_Reply
  // = TITLE
  //     A URL reply message formater/wrapper.
  //
  // = DESCRIPTION
  //     This class defines a URL reply data.  It is used
  //     to transform reply messages to an object so that we can
  //     ship them across network.
{
public:
  ACE_URL_Locator_Reply (void);
  // Default ctor.

  ~ACE_URL_Locator_Reply (void);
  // Default dtor.

  int status_reply (u_int op, int result);
  // Setup a reply message for EXPORT, WITHDRAW, or MODIFY operations.

  int query_reply (int result, size_t num,
                   const ACE_URL_Offer_Seq &offers);
  // Setup a reply for QUERY operation.

  int describe_reply (int result,
                      const ACE_URL_Offer &offer);
  // Construct a reply for DESCRIBE operation.

  size_t encode (void);
  // Encode request for network communication.  If succeed,
  // returns the size of the buffer, otherwise, return 0.

  size_t decode (void *buffer);
  // Restore from network data.  Returns size of the buffer
  // if succeed, 0 otherwise.  When passing in a buffer,
  // caller must take the responsibility to delete the buffer
  // afterward, if so needed.

  // Accessor function.
  const size_t num_offers (void) const;
  const ACE_URL_Offer *offer (void) const;
  const ACE_URL_Offer_Seq *offers (void) const;
  const u_int opcode (void) const;
  const u_int status (void) const;
  const char *buffer (void) const ;

  void dump (void) const ;
  // Print out this object.

protected:
  size_t size (void);
  // Return the size of the buffer required to encode
  // this request.

  enum {
    VALID_OFFER = 0x1,
    VALID_OFFERS = 0x2
  };
  // Valid pointer masks.

  u_int code_;
  // Holds the original op code.

  int status_;
  // Holds the result of an operation from the Location Server.

  size_t num_offers_;
  // Holds the number of valid offers in the offers_ sequence.

  int valid_ptr_;
  // Flag that marks valid internal pointers.

  ACE_URL_Offer *offer_;
  // Holds a single offer.  Used in query offer property.

  ACE_URL_Offer_Seq *offers_;
  // Holds the replying offer sequence from a Locator.

  char *buffer_;
  // Buffer to store encoded data.
};
#if defined (__ACE_INLINE__)
#include "Locator_Request_Reply.inl"
#endif /* __ACE_INLINE__ */

#endif /* ACE_LOCATOR_REQUEST_REPLY_H */
