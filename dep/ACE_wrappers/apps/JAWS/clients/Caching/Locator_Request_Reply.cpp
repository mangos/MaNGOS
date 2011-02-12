// $Id: Locator_Request_Reply.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#if !defined (ACE_LOCATOR_REQUEST_REPLY_C)
#define ACE_LOCATOR_REQUEST_REPLY_C

#include "Locator_Request_Reply.h"

#if !defined (__ACE_INLINE__)
#include "Locator_Request_Reply.inl"
#endif

#include "ace/Auto_Ptr.h"
#include "URL_Properties.h"
#include "URL_Array_Helper.h"
#include "URL_Locator.h"

int
ACE_URL_Locator_Request::url_query (const int how,
                                    const ACE_URL_Property_Seq &pseq,
                                    const int how_many)
{
  ACE_TRACE ("ACE_URL_Locator_Request::url_query");

  if (how >= ACE_URL_Locator::INVALID_SELECTION)
    return -1;
  ACE_NEW_RETURN (this->seq1_, ACE_URL_Property_Seq (pseq), -1);
  this->how_ = how;
  this->how_many_ = how_many;
  this->code_ = ACE_URL_Locator::QUERY;
  return 0;
}

int
ACE_URL_Locator_Request::export_offer (const ACE_URL_Offer &offer)
{
  ACE_TRACE ("ACE_URL_Locator_Request::export_offer");

  ACE_NEW_RETURN (this->offer_, ACE_URL_Offer (offer), -1);
  this->code_ = ACE_URL_Locator::EXPORT;
  return 0;
}

int
ACE_URL_Locator_Request::withdraw_offer (const ACE_WString &offer_id)
{
  ACE_TRACE ("ACE_URL_Locator_Request::withdraw_offer");

  this->id_ = offer_id;
  this->code_ = ACE_URL_Locator::WITHDRAW;
  return 0;
}

int
ACE_URL_Locator_Request::describe_offer (const ACE_WString &offer_id)
{
  ACE_TRACE ("ACE_URL_Locator_Request::describe_offer");

  this->id_ = offer_id;
  this->code_ = ACE_URL_Locator::DESCRIBE;
  return 0;
}

int
ACE_URL_Locator_Request::modify_offer (const ACE_WString &offer_id,
                                       const ACE_WString *url,
                                       const ACE_URL_Property_Seq &del,
                                       const ACE_URL_Property_Seq &modify)
{
  ACE_TRACE ("ACE_URL_Locator_Request::modify_offer");

  ACE_NEW_RETURN (this->seq1_, ACE_URL_Property_Seq (del), -1);
  ACE_NEW_RETURN (this->seq2_, ACE_URL_Property_Seq (modify), -1);

  if (url != 0)
    this->url_ = *url;

  this->id_ = offer_id;
  this->code_ = ACE_URL_Locator::MODIFY;
  return 0;
}

#define ENCODE_UINT32(ADDR,LEN,V) \
   * (ACE_UINT32 *) (ADDR+LEN) = htonl (V); \
   LEN += sizeof (ACE_UINT32);

#define DECODE_UINT32(ADDR,LEN,V) \
   V = ntohl (* (ACE_UINT32 *) (ADDR+LEN)); \
   LEN += sizeof (ACE_UINT32);

size_t
ACE_URL_Locator_Request::encode (void)
{
  ACE_TRACE ("ACE_URL_Locator_Request::encode");

  size_t buf_size = this->size ();
  size_t total_length = 0;

  ACE_NEW_RETURN (this->buffer_, char [buf_size], 0);

  ENCODE_UINT32 (this->buffer_, total_length, buf_size);
  // Encode buffer size.

  ENCODE_UINT32 (this->buffer_, total_length, this->code_);
  // Encode Op code.

  ENCODE_UINT32 (this->buffer_, total_length, this->how_);
  // Encode selection criteria.

  ENCODE_UINT32 (this->buffer_, total_length, this->how_many_);
  // Encode number of offers interested.

  ENCODE_UINT32 (this->buffer_, total_length, this->valid_ptr_);
  // Encode valide pointer flag.

  if (this->seq1_ != 0)
    {
      ENCODE_UINT32 (this->buffer_, total_length, this->seq1_->size ());
      total_length += ace_array_encode (this->buffer_ + total_length, *this->seq1_);
    }
  if (this->seq2_ != 0)
    {
      ENCODE_UINT32 (this->buffer_, total_length, this->seq2_->size ());
      total_length += ace_array_encode (this->buffer_ + total_length, *this->seq2_);
    }
  if (this->offer_ != 0)
    total_length += this->offer_->encode (this->buffer_ + total_length);

  total_length += ACE_WString_Helper::encode (this->buffer_ + total_length,
                                              this->id_);
  total_length += ACE_WString_Helper::encode (this->buffer_ + total_length,
                                              this->url_);

  ACE_ASSERT (total_length == buf_size);
  return total_length;
}

size_t
ACE_URL_Locator_Request::decode (void *buffer)
{
  ACE_TRACE ("ACE_URL_Locator_Request::decode");

  if (buffer == 0)
    return 0;
  // Check if we have a valid buffer available.

  char *cbuffer = (char *) buffer;

  size_t buf_size;
  size_t total_length = 0;

  DECODE_UINT32 (cbuffer, total_length, buf_size);
  // Decode length of buffer size first.

  DECODE_UINT32 (cbuffer, total_length, this->code_);
  // Get the operation code.

  DECODE_UINT32 (cbuffer, total_length, this->how_);
  // Decode selection criteria.

  DECODE_UINT32 (cbuffer, total_length, this->how_many_);
  // Decode number of offers interested.

  DECODE_UINT32 (cbuffer, total_length, this->valid_ptr_);
  // Decode valide pointer flag.

  if ((this->valid_ptr_ & VALID_SEQ1) != 0)
    {
      size_t n;
      DECODE_UINT32 (cbuffer, total_length, n);
      ACE_NEW_RETURN (this->seq1_, ACE_URL_Property_Seq (n), 0);
      total_length += ace_array_decode (cbuffer + total_length, *this->seq1_);
    }
  if ((this->valid_ptr_ & VALID_SEQ2) != 0)
    {
      size_t n;
      DECODE_UINT32 (cbuffer, total_length, n);
      ACE_NEW_RETURN (this->seq2_, ACE_URL_Property_Seq (n), 0);
      total_length += ace_array_decode (cbuffer + total_length, *this->seq2_);
    }
  if ((this->valid_ptr_ & VALID_OFFER) != 0)
    {
      ACE_NEW_RETURN (this->offer_, ACE_URL_Offer, 0);
      total_length += this->offer_->decode (cbuffer + total_length);
    }

  this->id_ = ACE_WString ((ACE_USHORT16 *) (cbuffer + total_length));
  total_length += ACE_WString_Helper::decode (cbuffer + total_length);
  this->url_ = ACE_WString ((ACE_USHORT16 *) (cbuffer + total_length));
  total_length += ACE_WString_Helper::decode (cbuffer + total_length);

  ACE_ASSERT (total_length == buf_size);
  return total_length;
}


size_t
ACE_URL_Locator_Request::size (void)
{
  ACE_TRACE ("ACE_URL_Locator_Request::size");

  size_t total_length = 5 * sizeof (ACE_UINT32);
  // There are 5 UINT32 variables at the beginning
  // of the buffer.  <buffer size>, <code>, <how>,
  // <how_many>, <valid_ptr>.

  this->valid_ptr_ = 0;
  // Check valid pointers and mark corresponding flag in <valid_prt>.

  if (this->seq1_ != 0)
    {
      this->valid_ptr_ |= VALID_SEQ1;
      total_length += ace_array_size (*this->seq1_);
    }
  if (this->seq2_ != 0)
    {
      this->valid_ptr_ |= VALID_SEQ2;
      total_length += ace_array_size (*this->seq2_);
    }
  if (this->offer_ != 0)
    {
      this->valid_ptr_ |= VALID_OFFER;
      total_length += this->offer_->size ();
    }

  total_length += ACE_WString_Helper::size (this->id_);
  total_length += ACE_WString_Helper::size (this->url_);

  return total_length;
}

void
ACE_URL_Locator_Request::dump (void) const
{
  //ACE_TRACE ("ACE_URL_Locator_Request::dump");

  size_t i;

  ACE_DEBUG ((LM_DEBUG, ACE_BEGIN_DUMP, this));

  if (this->code_ < ACE_URL_Locator::INVALID_OPERATION)
    ACE_DEBUG ((LM_DEBUG, "%s Request:\n", ACE_URL_Locator::opname[this->code_]));
  else
    ACE_DEBUG ((LM_DEBUG, "Invalid Operation: %d\n", this->code_));

  if (this->how_ < ACE_URL_Locator::INVALID_SELECTION)
    ACE_DEBUG ((LM_DEBUG, "Select: %s\n", ACE_URL_Locator::selection_name[this->how_]));
  else
    ACE_DEBUG ((LM_DEBUG, "Invalid selection method: %d\n", this->how_));

  ACE_DEBUG ((LM_DEBUG, "At most %d reply.\n", this->how_many_));

  ACE_DEBUG ((LM_DEBUG, "Valid pointer pattern: %x\n", this->valid_ptr_));

  ACE_DEBUG ((LM_DEBUG, "Property sequence 1: %x\n", this->seq1_));
  if (this->seq1_ != 0)
    for (i = 0; i < this->seq1_->size (); i++)
      (*this->seq1_)[i].dump ();

  ACE_DEBUG ((LM_DEBUG, "Property sequence 2: %x\n", this->seq2_));
  if (this->seq2_ != 0)
    for (i = 0; i < this->seq2_->size (); i++)
      (*this->seq2_)[i].dump();

  ACE_DEBUG ((LM_DEBUG, "Offer: %x\n", this->offer_));
  if (this->offer_ != 0)
    this->offer_->dump ();

  if (this->id_.length () > 0)
    ACE_DEBUG ((LM_DEBUG, "Offer ID: %s\n",
                ACE_Auto_Basic_Array_Ptr<char> (this->id_.char_rep ()).get ()));
  else
    ACE_DEBUG ((LM_DEBUG, "Offer ID: \"\"\n"));

  if (this->url_.length () > 0)
    ACE_DEBUG ((LM_DEBUG, "URL: %s\n",
                ACE_Auto_Basic_Array_Ptr<char> (this->url_.char_rep ()).get ()));
  else
    ACE_DEBUG ((LM_DEBUG, "URL: \"\"\n"));

  ACE_DEBUG ((LM_DEBUG, ACE_END_DUMP));
}

int
ACE_URL_Locator_Reply::status_reply (u_int op, int result)
{
  ACE_TRACE ("ACE_URL_Locator_Reply::status_reply");

  this->code_ = op;
  this->status_ = result;
  return 0;
}

int
ACE_URL_Locator_Reply::query_reply (int result, size_t num,
                                    const ACE_URL_Offer_Seq &offers)
{
  ACE_TRACE ("ACE_URL_Locator_Reply::query_reply");

  this->code_ = ACE_URL_Locator::QUERY;
  this->status_ = result;
  ACE_NEW_RETURN (this->offers_, ACE_URL_Offer_Seq (offers), -1);
  return 0;
}

int
ACE_URL_Locator_Reply::describe_reply (int result,
                                       const ACE_URL_Offer &offer)
{
  ACE_TRACE ("ACE_URL_Locator_Reply::describe_reply");

  this->code_ = ACE_URL_Locator::DESCRIBE;
  this->status_ = result;
  ACE_NEW_RETURN (this->offer_, ACE_URL_Offer (offer), -1);
  return 0;
}

size_t
ACE_URL_Locator_Reply::encode (void)
{
  ACE_TRACE ("ACE_URL_Locator_Reply::encode");

  size_t buf_size = this->size ();
  size_t total_length = 0;

  ACE_NEW_RETURN (this->buffer_, char [buf_size], 0);

  ENCODE_UINT32 (this->buffer_, total_length, buf_size);
  // Encode buffer size.

  ENCODE_UINT32 (this->buffer_, total_length, this->code_);
  // Encode Op code.

  ENCODE_UINT32 (this->buffer_, total_length, this->status_);
  // Encode Op result status.

  ENCODE_UINT32 (this->buffer_, total_length, this->num_offers_);
  // Encode number of offers in this->offers_.

  ENCODE_UINT32 (this->buffer_, total_length, this->valid_ptr_);
  // Encode valid pointers mask.

  if (this->offer_ != 0)
    total_length += this->offer_->encode (this->buffer_ + total_length);

  if (this->offers_ != 0)
    {
      ENCODE_UINT32 (this->buffer_, total_length, this->offers_->size ());
      total_length += ace_array_encode (this->buffer_ + total_length, *this->offers_);
    }

  ACE_ASSERT (total_length == buf_size);
  return 0;
}

size_t
ACE_URL_Locator_Reply::decode (void *buffer)
{
  ACE_TRACE ("ACE_URL_Locator_Reply::decode");

  if (buffer == 0)
    return 0;
  // Check if we have a buffer available.

  char *cbuffer = (char *) buffer;

  size_t buf_size;
  size_t total_length = 0;

  DECODE_UINT32 (cbuffer, total_length, buf_size);
  // Get the length of the buffer first.

  DECODE_UINT32 (cbuffer, total_length, this->code_);
  // Decode Op code.

  DECODE_UINT32 (cbuffer, total_length, this->status_);
  // Decode Op result status.

  DECODE_UINT32 (cbuffer, total_length, this->num_offers_);
  // Decode number of offers in this->offers_.

  DECODE_UINT32 (cbuffer, total_length, this->valid_ptr_);
  // Decode valid pointers mask.

  if ((this->valid_ptr_ & VALID_OFFER) != 0)
    {
      ACE_NEW_RETURN (this->offer_, ACE_URL_Offer, 0);
      total_length += this->offer_->decode (cbuffer + total_length);
    }

  if ((this->valid_ptr_ & VALID_OFFERS) != 0)
    {
      size_t n;
      DECODE_UINT32 (cbuffer, total_length, n);
      ACE_NEW_RETURN (this->offers_, ACE_URL_Offer_Seq (n), 0);
      total_length += ace_array_decode (cbuffer + total_length, *this->offers_);
    }

  ACE_ASSERT (total_length ==buf_size);
  return 0;
}

size_t
ACE_URL_Locator_Reply::size (void)
{
  ACE_TRACE ("ACE_URL_Locator_Reply:size");

  size_t total_length = 5 * sizeof (ACE_UINT32);
  // size for 5 ACE_UINT32 objects: <buffer size>, <code_>,
  // <status_>, <num_offers_>, and <valid_ptr_>.

  this->valid_ptr_ = 0;
  if (this->offer_ != 0)
    {
      this->valid_ptr_ |= VALID_OFFER;
      total_length += this->offer_->size ();
    }
  if (this->offers_ != 0)
    {
      this->valid_ptr_ |= VALID_OFFERS;
      total_length += ace_array_size (*this->offers_);
    }
  return total_length;
}

void
ACE_URL_Locator_Reply::dump (void) const
{
  //ACE_TRACE ("ACE_URL_Locator_Reply::dump");

  ACE_DEBUG ((LM_DEBUG, ACE_BEGIN_DUMP, this));

  if (this->code_ < ACE_URL_Locator::INVALID_OPERATION)
    ACE_DEBUG ((LM_DEBUG, "Original request: %s\n", ACE_URL_Locator::opname[this->code_]));
  else
    ACE_DEBUG ((LM_DEBUG, "Invalid Original Request: %d\n", this->code_));

  if (this->status_ < ACE_URL_Locator::MAX_URL_ERROR)
    ACE_DEBUG ((LM_DEBUG, "Reply status: %s\n", ACE_URL_Locator::err_name[this->status_]));
  else
    ACE_DEBUG ((LM_DEBUG, "Invalid reply status: %d\n", this->status_));

  ACE_DEBUG ((LM_DEBUG, "Number of offers: %d\n", this->num_offers_));

  ACE_DEBUG ((LM_DEBUG, "Valid pointer pattern: %x\n", this->valid_ptr_));

  ACE_DEBUG ((LM_DEBUG, "Offer: %x\n", this->offer_));
  if (this->offer_ != 0)
    this->offer_->dump ();

  ACE_DEBUG ((LM_DEBUG, "Offer sequence: %x\n", this->offers_));
  if (this->offers_ != 0)
    for (size_t i = 0; i < this->offers_->size (); i++)
      (*this->offers_)[i].dump();

  ACE_DEBUG ((LM_DEBUG, ACE_END_DUMP));
}
#endif /* ACE_LOCATOR_REQUEST_REPLY_C */
