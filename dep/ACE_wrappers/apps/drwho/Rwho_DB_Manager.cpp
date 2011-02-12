// $Id: Rwho_DB_Manager.cpp 81993 2008-06-16 20:26:16Z sowayaa $
#include "global.h"
#include "Options.h"
#include "Rwho_DB_Manager.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_fcntl.h"

// Change to the RWHO directory to speed up and simplify later
// processing.  This requires opening the directory for reading with
// the directory iterator abstraction and then skipping the first two
// files in the directory, which are assumed to be "." and ".." (this
// function needs to be changed if this assumption does not hold!)

RWho_DB_Manager::RWho_DB_Manager (void)
  : number_of_users (0),
    current_user (0),
    WHOD_HEADER_SIZE (sizeof host_data - sizeof host_data.wd_we),
    rwho_dir_name (RWHODIR)
{
  if (ACE_OS::getcwd (this->original_pathname, MAXPATHLEN + 1) == 0)
    ACE_ERROR ((LM_ERROR,
                "%p\n%a",
                Options::program_name,
                1));

  if (ACE_OS::chdir (this->rwho_dir_name) < 0)
    ACE_ERROR ((LM_ERROR,
                "%p\n%a",
                this->rwho_dir_name,
                1));

  this->rwho_dir.open (this->rwho_dir_name);

#if 0
  // Skip "." and ".."
  this->rwho_dir.read ();
  this->rwho_dir.read ();
#endif
}

// The destructor cleans up the RWHOD_DIR handle.

RWho_DB_Manager::~RWho_DB_Manager (void)
{
  if (ACE_OS::chdir (this->original_pathname) < 0)
    ACE_ERROR ((LM_ERROR,
                "%p\n%a",
                Options::program_name,
                1));

  if (Options::get_opt (Options::DEBUGGING))
    ACE_DEBUG ((LM_DEBUG,
                "disposing the RWho_DB_Manager\n"));
}

// This procedure looks through the rwhod directory until it finds the next
// valid user file.
//
// The requirements for user files are:
//   1) The file is at least MIN_HOST_DATA_SIZE bytes long
//   2) It was received within the last MAX_HOST_TIMEOUT seconds
// Return:
//  Are there any more hosts? */

int
RWho_DB_Manager::get_next_host (void)
{
  time_t current_time;

  ACE_OS::time (&current_time);

  // Go through each file in the directory looking for valid entries.

  for (dirent *dir_ptr = this->rwho_dir.read ();
       dir_ptr != 0;
       dir_ptr = this->rwho_dir.read ())
    {
      ACE_HANDLE user_file =
        ACE_OS::open (dir_ptr->d_name, O_RDONLY);

      if (user_file < 0)
        return -1;

      int host_data_length =
        ACE_OS::read (user_file,
                      (char *) &this->host_data,
                      sizeof this->host_data);

      if (host_data_length > WHOD_HEADER_SIZE
          && current_time - this->host_data.wd_recvtime < MAX_HOST_TIMEOUT)
        {
          this->current_user = 0;
          this->number_of_users = (host_data_length - WHOD_HEADER_SIZE) / sizeof *this->host_data.wd_we;
          ACE_OS::close (user_file);
          return 1; // We found a good host, so return it.
        }
      else
        ACE_OS::close (user_file);
    }

  // There are no more hosts, so return False.
  return 0;
}

// Returns the next user's information.  Note that for efficiency only
// pointers are copied, i.e., this info must be used before we call
// this function again.

int
RWho_DB_Manager::get_next_user (Protocol_Record &protocol_record)
{
  // Get the next host file if necessary
  if (this->current_user >= this->number_of_users
      && this->get_next_host () == 0)
    return 0;

  protocol_record.set_login (this->host_data.wd_we[current_user].we_utmp.out_name);
  Drwho_Node *current_node = protocol_record.get_drwho_list ();
  current_node->set_host_name (this->host_data.wd_hostname);
  current_node->set_idle_time (this->host_data.wd_we[current_user].we_idle);
  this->current_user++;

  return 1;
}
