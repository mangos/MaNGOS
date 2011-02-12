// $Id: File_Manager.cpp 81993 2008-06-16 20:26:16Z sowayaa $

#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_pwd.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_ctype.h"
#include "File_Manager.h"

File_Manager::File_Manager (void)
  : number_of_friends (0),
    max_key_length (0),
    buffer_ptr (0),
    current_ptr (0),
    buffer_size (0)
{
}

// Either opens the friends file (if FILENAME is not a NULL pointer)
// or opens up the password file.  In either case, the number of
// entries in the file are returned, i.e., number of friends...

int
File_Manager::open_file (const char *filename)
{
  return filename == 0
    ? this->open_passwd_file ()
    : this->open_friends_file (filename);
}

// Returns the next LOGIN_NAME and REAL_NAME from the file.

void
File_Manager::get_login_and_real_name (const char *&login_name, const char *&real_name)
{
  char *buf_ptr = this->current_ptr;

  login_name = buf_ptr;

  // Skip to the end of the login name.

  while (ACE_OS::ace_isalnum (*buf_ptr))
    buf_ptr++;

  *buf_ptr++ = '\0';

  // Now skip over white space to *start* of real name!

  while (ACE_OS::ace_isspace (*buf_ptr) || *buf_ptr == '\0')
    buf_ptr++;

  real_name = buf_ptr;

  while (*buf_ptr++ != '\n')
    continue;

  // Clear the trailing blanks and junk.

  for (char *tmp_ptr = buf_ptr - 1;
       ACE_OS::ace_isspace (*tmp_ptr);
       tmp_ptr--)
    *tmp_ptr = '\0';

  // Skip over consecutive blank lines.

  while (*buf_ptr == '\n')
    buf_ptr++;

  this->current_ptr = buf_ptr;
}

// Open up the yp passwd file and slurp all the users in!

int
File_Manager::open_passwd_file (void)
{
  const char *filename = ACE_OS::tempnam ();
  FILE *fp = ACE_OS::fopen (filename, "w");

  if (fp == 0)
    return -1;

  passwd *pwent;

  for (ACE_OS::setpwent ();
       (pwent = ACE_OS::getpwent ()) != 0; )
    if (*pwent->pw_gecos != '\0')
      {
        char *cp = ACE_OS::strchr (pwent->pw_gecos, ',');

        if (cp != 0)
          *cp = '\0';

        ACE_OS::fprintf (fp,
                         "%-8.8s %s\n",
                         pwent->pw_name,
                         pwent->pw_gecos);
        this->number_of_friends++;
      }

  ACE_OS::endpwent ();

  ACE_OS::fclose (fp);

  if (this->mmap_.map (filename) == -1)
    return -1;

  this->buffer_ptr = (char *) this->mmap_.addr ();

  this->buffer_size = this->mmap_.size ();
  this->current_ptr = this->buffer_ptr;
  return this->number_of_friends;
}

// This function opens up FILENAME and memory maps it in our address
// space.

int
File_Manager::open_friends_file (const char *filename)
{
  char directory[MAXPATHLEN];
  const char *pathname = directory;

  // See if we've got a filename or a pathname (i.e., directory/filename).

  if (ACE_OS::strrchr (filename, '/') != 0)
    // We've got a complete pathname.
    pathname = filename;
  else
    {
      directory[0] = '\0';

      const char *home = ACE_OS::getenv ("HOME");
      if (home != 0)
        {
          ACE_OS::strcat (directory, home);
          ACE_OS::strcat (directory, "/");
        }
      ACE_OS::strcat (directory, filename);
    }

  // Do the mmap'ing.

  if (this->mmap_.map (pathname) == -1)
    return -1;

  this->buffer_ptr = (char *) this->mmap_.addr ();

  this->buffer_size = this->mmap_.size ();
  this->current_ptr = this->buffer_ptr;

  // Determine how many friends there are by counting the newlines.

  for (char *cp = this->buffer_ptr + this->buffer_size;
       cp > this->buffer_ptr
       ;)
    if (*--cp == '\n')
      {
        this->number_of_friends++;

        // Skip consecutive newlines.
        while (cp[-1] == '\n')
          --cp;
      }

  return this->number_of_friends;
}

#if defined (ACE_HAS_EXPLICIT_STATIC_TEMPLATE_MEMBER_INSTANTIATION)
template ACE_Singleton<File_Manager, ACE_Null_Mutex> *
  ACE_Singleton<File_Manager, ACE_Null_Mutex>::singleton_;
#endif /* ACE_HAS_EXPLICIT_STATIC_TEMPLATE_MEMBER_INSTANTIATION */
