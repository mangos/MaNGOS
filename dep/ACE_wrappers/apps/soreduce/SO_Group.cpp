// -*- C++ -*-
// $Id: SO_Group.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// File: SO_Group.cpp

#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Log_Msg.h"
#include "ace/Process.h"
#include "ace/Pipe.h"

#include "Library.h"
#include "SO_Group.h"

SO_Group::SO_Group ()
  : undef_wrapper_ ("nothing"),
    undefs_(undef_wrapper_.imports()),
    libs_ (0),
    max_libs_ (128),
    num_libs_(0)
{
  libs_ = new Library*[max_libs_];
}

SO_Group::~SO_Group (void)
{
  for (int i = 0; i < num_libs_; delete libs_[i++])
    {
      // No action.
    }

  delete [] libs_;
}

void
SO_Group::add_executable (const char * path)
{
  ACE_Process proc;
  ACE_Process_Options opts;

  ACE_HANDLE pipe[2];
  ACE_Pipe io(pipe);

  opts.set_handles (ACE_STDIN,pipe[1]);

  int result = opts.command_line ("ldd %s",path);
  // Prevent compiler warning about "unused variable" if ACE_ASSERT is
  // an empty macro.
  ACE_UNUSED_ARG (result);
  ACE_ASSERT (result == 0);

  proc.spawn (opts);
  if (ACE_OS::close(pipe[1]) == -1)
    ACE_DEBUG ((LM_DEBUG, "%p\n", "close"));
  opts.release_handles();

  const int max_line_length = 1000;
  char line[max_line_length];

  while (true)
  {
    ACE_OS::memset (line,0,max_line_length);
    int len = 0;
    int nread = 0;
    int bogus = 0;

    // Skip initial whitespace.
    while ((nread = ACE_OS::read (pipe[0], line,1)) == 1
           && (*line == ' ' || *line == '\t'))
      {
        // No action.
      }

    if (nread != 1)
      {
        break;
      }

    // read the library name
    len = 1;

    while ((nread = ACE_OS::read (pipe[0], line + len, 1)) == 1
           && (line[len] != ' '))
      {
        if (! bogus && ++len == max_line_length)
          {
            bogus = 1;
            break;
          }
      }

    if (nread != 1 || bogus)
      {
        break;
      }

    line[len] = 0;
    char * dot = ACE_OS::strchr (line,'.');

    if (dot != 0)
      {
        *dot = 0;
      }

    char * libname = line + 3; // skip over "lib"

    // check to see if this is a new library
    int found = 0;

    for (int i = 0; !found && i < num_libs_; ++i)
      {
        found = (libs_[i]->name() == libname);
      }

    if (!found)
    {
      Library *nlib = new Library(libname);
      ACE_OS::memset (line,0,max_line_length);

      // Skip over '=> '.
      if (ACE_OS::read (pipe[0], line, 3) != 3)
        {
          break;
        }

      // get library path
      len = 0;

      while ((nread = ACE_OS::read(pipe[0],line + len,1)) == 1 &&
             (line[len] != ' '))
        {
          if (! bogus && ++len == max_line_length)
            {
              bogus = 1;
              break;
            }
        }

      if (nread != 1 || bogus)
        {
          break;
        }

      line[len] = 0;
      nlib->set_path (line);
      libs_[num_libs_++] = nlib;
      ACE_ASSERT (num_libs_ < max_libs_); // grow max libs?
    }

    // Skip the rest of the line.
    while ((nread = ACE_OS::read (pipe[0], line, 1)) == 1
           && *line != '\n')
      {
        // No action.
      }

    if (nread != 1)
      {
        break;
      }
  }
  proc.wait ();
  ACE_OS::close (pipe[0]);

  undef_wrapper_.add_source(path,1);
  // now do the ldd, iterate over the results to add new libs, etc.
}

void
SO_Group::analize (void)
{
  for (int passcount = 0; undefs_.modified (); ++passcount)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "pass %d, undef count = %d\n",
                  passcount,
                  undefs_.size ()));

      for (int i = 0; i < num_libs_; libs_[i++]->resolve (undefs_))
        {
          // No action.
        }
    }
}

void
SO_Group::write_results (void)
{
  for (int i = 0; i < num_libs_; libs_[i++]->write_export_list (1))
    {
      // No action.
    }
}

void
SO_Group::load_modules (void)
{
  for (int i = 0; i < num_libs_; libs_[i++]->load_modules ())
    {
      // No action.
    }
}

void
SO_Group::list_libs (void)
{
  ACE_DEBUG ((LM_DEBUG, "Libs subject to analysis:\n"));

  for (int i = 0; i < num_libs_; ++i)
  {
    if (libs_[i]->has_modules ())
      {
        ACE_DEBUG ((LM_DEBUG, "  %s\n", libs_[i]->name ().c_str ()));
      }
  }
}







