// $Id: Local_Locator.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#if !defined (ACE_LOCAL_LOCATOR_C)
#define ACE_LOCAL_LOCATOR_C

#include "Local_Locator.h"

#if !defined (__ACE_INLINE__)
#include "Local_Locator.inl"
#endif /* __ACE_INLINE__ */

int
ACE_URL_Local_Locator::url_query (const ACE_URL_Locator::ACE_Selection_Criteria how,
                                  const ACE_URL_Property_Seq *pseq,
                                  const size_t how_many,
                                  size_t &num_query,
                                  ACE_URL_Offer_Seq *offer)
{
  ACE_URL_Record *item = 0;

  ACE_NEW_RETURN (offer, ACE_URL_Offer_Seq (how_many), -1);

  if (how >= ACE_URL_Locator::INVALID_SELECTION)
    {
      errno = ACE_URL_Locator::INVALID_ARGUMENT;
      return -1;
    }

  num_query = 0;
  for (ACE_Unbounded_Set_Iterator<ACE_URL_Record> iter (this->repository_);
       iter.next (item) != 0;
       iter.advance ())
    {
      size_t i_query;
      size_t i_db;
      int found = 0;

      // Now this is a stupid implementation.  Perhaps we can
      // implement this using Hash_Map.  Better yet, I think we should
      // put this in a database and put SQL query here.
      for (i_query = 0; found == 0 && i_query < pseq->size (); i_query++)
        for (i_db = 0; i_db < item->offer_->url_properties ().size (); i_db++)
          {
            if ((*pseq)[i_query].name () == item->offer_->url_properties ()[i_db].name ())
              if (how == ACE_URL_Locator::SOME)
                ;

            // if match and Some, copy to <offer>, inc <num_query>, advance iterator

            // else if All, advance iterator

            // else if None, check next property in <pseq>.

            if (all properties checked and found and ALL)
              copy to <offer>; inc <num_query>;
            else if (all properties checked and not found and NONE)
              copy to <offer>; inc <num_query>;
            else
              shouldn't happen, internal error

      if (num_query == how_many)
        break;
    }

  return 0;
}

int
ACE_URL_Local_Locator::export_offer (ACE_URL_Offer *offer,
                                     ACE_WString &offer_id)
{
  ACE_URL_Record *item = 0;

  // First check if we have registered this URL already.
  for (ACE_Unbounded_Set_Iterator<ACE_URL_Record> iter (this->repository_);
       iter.next (item) != 0;
       iter.advance ())
    if (*item->offer_->url () == *offer->url ())
      {
        errno = ACE_URL_Locator::OFFER_EXIST;
        return -1;
      }

  ACE_URL_Record *new_offer;

  // Offer is not in repository, we can add new one in safely.
  ACE_NEW_RETURN (new_offer, ACE_URL_Record (offer),
                  ACE_URL_Locator::NOMEM);

  this->repository_.push (*new_offer);

  offer_id = *new_offer->id_;
  return 0;
}

int
ACE_URL_Local_Locator::withdraw_offer (const ACE_WString &offer_id)
{
  ACE_URL_Record *item = 0;

  // Iterate thru repository and remove offer with <offer_id>.
  for (ACE_Unbounded_Set_Iterator<ACE_URL_Record> iter (this->repository_);
       iter.next (item) != 0;
       iter.advance ())
      if (offer_id == *item->id_)
        {
          if (this->repository_.remove (*item) == 0)
            return 0
          else
            {
              errno = ACE_URL_Locator::UNKNOWN;
              return -1;
            }
        }

  errno = ACE_URL_Locator::NO_SUCH_OFFER;
  return 0;
}

int
ACE_URL_Local_Locator::describe_offer (const ACE_WString &offer_id,
                                       ACE_URL_Offer *offer)
{
  ACE_URL_Record *item = 0;

  // Iterate thru the repository and produce a copy of offer's
  // description.
  for (ACE_Unbounded_Set_Iterator<ACE_URL_Record> iter (this->repository_);
       iter.next (item) != 0;
       iter.advance ())
    if (offer_id == *item->id_)
      {
        ACE_NEW_RETURN (offer, ACE_URL_Offer (*item->offer_), -1);
        return 0;
      }

  errno = ACE_URL_Locator::NO_SUCH_OFFER;
  return -1;
}

int
ACE_URL_Local_Locator::modify_offer (const ACE_WString &offer_id,
                                     const ACE_WString *url,
                                     const ACE_URL_Property_Seq *del,
                                     const ACE_URL_Property_Seq *modify)
{
  ACE_Unbounded_Set_Iterator<ACE_URL_Record> iter (this->repository_);
  ACE_URL_Record *item = 0;
  ACE_URL_Record *target = 0;

  // Errno Checking

  for (; iter.next (item) != 0; iter.advance ())
    {
      if (url != 0 && *url == item->offer_->url ())
        {
          errno = ACE_URL_Locator::OFFER_EXIST;
          return -1;
        }
      if (offer_id == *item->id_)
        target = item;
    }

  if (target != 0)  // Aha, we found a target to work on
    {
      if (del != 0 && modify != 0)
        {
          // We need to make a copy of the original property sequence
          // so if any error occurs, we can revert our change easily.

          // First we need to calculate the maximum number of perperties.
          int psize = target->offer_->url_properties ().size ();
          if (del != 0)
            if ((psize -= del->size ()) < 0)
              {
                // If you try to delete more properties than we have,
                // you are doomed.  No need to proceed.
                errno = ACE_URL_Locator::INVALID_ARGUMENT;
                return -1;
              }
          if (modify != 0)
            // In the worst case, all properties in <modify> will be added.
            psize += modify->size ();

          // Now, create a temporary work space.
          ACE_URL_Property_Seq working (psize);
          size_t sz = 0;
          for (; sz < item->offer_->url_properties ().size ())
            working[sz] = item->offer_->url_properties() [sz];

          if (del != 0)
            {
              // Argh, this is really a stupid design.
              // Go thru every property we want to delete
              for (size_t i = 0; i < del->size () && sz > 0; i++)
                // For earch, go thru our property sequence and
                // search for the property.
                for (size_t j = 0; j < sz; j++)
                  if ((*del)[i].name () == working[j].name ())
                    {
                      sz -= 1;
                      working[j] = working[sz]; // pack the array.
                      break;
                    }
              // Doesn't generate error when we want to delete an
              // imaginary property.  Is this appropriate?
            }

          if (modify != 0)
            {
              // This is also stupid.
              // Go thru every property we want to modify/add
              for (size_t i = 0; i < modify->size () && sz > 0; i++)
                {
                  // For each property, go thru our property list
                  // and search for the matching property
                  for (size_t j = 0; j < sz; j++)
                    if ((*modify)[i].name () == working[j].name ())
                      {
                        // A match found.
                        working[j].value ((*modify)[i].value ().fast_rep ());
                        break;
                      }

                  // No matching property name were found,
                  // We want to add this property into the list.
                  if (j == sz)
                    working[sz++] = (*modify)[i];
                }
            }
        }

      // Yes, all operations passed.  We can now copy the working version back.
      item->offer_->url_properties (ACE_URL_Property_Seq (sz));
      for (size_t i = 0; i < sz; i ++)
        item->offer_->url_properties ()[i] = working[i];

      if (url != 0)
        item->offer_->url (url->fast_rep ()); // replace URL location.
      return 0;
    }
  errno = ACE_URL_Locator::NO_SUCH_OFFER;
  return -1;
}

#endif /* ACE_LOCAL_LOCATOR_C */
