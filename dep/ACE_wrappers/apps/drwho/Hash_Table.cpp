// $Id: Hash_Table.cpp 81994 2008-06-16 21:23:17Z sowayaa $

#include "Options.h"
#include "Hash_Table.h"
#include "ace/Log_Msg.h"
#include "ace/OS_Memory.h"
#include "ace/OS_NS_string.h"

Hash_Table::Hash_Table (void)
  : current_ptr (0),
    current_index (0),
    hash_table_size (HASH_TABLE_SIZE)
{
  ACE_NEW (this->hash_table,
           Protocol_Record *[this->hash_table_size]);
  ACE_OS::memset (this->hash_table,
                  0,
                  this->hash_table_size * sizeof *this->hash_table);
}

// Iterate through the hash table returning one node at a time...

Protocol_Record *
Hash_Table::get_next_entry (void)
{
  // Reset the iterator if we are starting from the beginning.

  if (this->current_index == -1)
    this->current_index = 0;

  if (this->current_ptr == 0)
    {
      for (; this->current_index < this->hash_table_size; this->current_index++)
        if (this->hash_table[this->current_index] != 0)
          {
            Protocol_Record *prp = this->hash_table[this->current_index++];
            this->current_ptr  = prp->next_;
            return prp;
          }

      this->current_index = -1;
      return 0;
    }
  else
    {
      Protocol_Record *prp = this->current_ptr;
      this->current_ptr = this->current_ptr->next_;
      return prp;
    }
}

Protocol_Record *
Hash_Table::get_each_entry (void)
{
  return this->get_next_entry ();
}

// Frees up all the dynamic memory in the hash table.

Hash_Table::~Hash_Table (void)
{
  if (Options::get_opt (Options::DEBUGGING))
    ACE_DEBUG ((LM_DEBUG,
                "disposing Hash_Table\n"));

  for (int i = 0; i < this->hash_table_size; i++)
    for (Protocol_Record *prp = this->hash_table[i];
         prp != 0; )
      {
        prp = prp->next_;
      }
}
