// -*- C++ -*-
// $Id: Obj_Module.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// File: Obj_Module.cpp

// Author: Phil Mesnier

// This file contains the implementation of the classes responsible for
// managing the contents of a single object module (.o file).

#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Process.h"
#include "ace/Pipe.h"
#include "ace/Message_Block.h"
#include "ace/Log_Msg.h"

#include "Obj_Module.h"

//----------------------------------------------------------------------------

Obj_Module::Obj_Module (const ACE_CString &name, int cap)
  : name_ (name),
    imports_(cap),
    exports_(cap),
    extrefs_(0)
{
}

ACE_CString &
Obj_Module::name()
{
  return name_;
}

Sig_List &
Obj_Module::exports()
{
  return exports_;
}

Sig_List &
Obj_Module::imports()
{
  return imports_;
}

int
Obj_Module::extref()
{
  return extrefs_;
}

void
Obj_Module::add_extref()
{
  extrefs_ ++;
}

void
Obj_Module::remove_extref()
{
  extrefs_ --;
}

int
Obj_Module::read_line (ACE_HANDLE src, ACE_Message_Block **buf)
{
  int eoln = 0;
  //  ACE_Time_Value timeout (1,0);
  if (buf == 0 || *buf == 0) {
    char dummy;
    while (!eoln && ACE_OS::read(src,&dummy,1) == 1) {
      eoln = (dummy == '\n');
    }
    return eoln;
  }
  //  while (!eoln && ACE::recv(src,buf->wr_ptr(),1,&timeout) == 1) {
  while (!eoln && ACE_OS::read(src,(*buf)->wr_ptr(),1) == 1) {
    eoln = (*(*buf)->wr_ptr() == '\n');
    (*buf)->wr_ptr(1);
    if ((*buf)->space() == 0) {
      (*buf)->cont(new ACE_Message_Block(102400));
      *buf = (*buf)->cont();
    }
  }
  return eoln;
}

void
Obj_Module::add_source(const char *p, int imports_only)
{
  ACE_Process nmproc;
  ACE_Process_Options nm_opts;
  ACE_CString path (p);

  ACE_CString::size_type pathsep = path.rfind('/');

  ACE_CString src_name;
  ACE_CString workpath;

  if (pathsep == ACE_CString::npos) {
    src_name = path;
    workpath = ".";
  } else {
    src_name = path.substr(pathsep+1);
    workpath= path.substr(0,pathsep);
  }

  ACE_HANDLE pipe[2];
  ACE_Pipe io(pipe);

  nm_opts.working_directory (workpath.c_str());
  nm_opts.set_handles (ACE_STDIN,pipe[1]);

  // Options for the command line shown here are for the GNU nm 2.9.5

  int result = nm_opts.command_line ("nm -C %s",src_name.c_str());
  // Prevent compiler warning about "unused variable" if ACE_ASSERT is
  // an empty macro.
  ACE_UNUSED_ARG (result);
  ACE_ASSERT (result == 0);

  nmproc.spawn (nm_opts);
  if (ACE_OS::close(pipe[1]) == -1)
    ACE_DEBUG ((LM_DEBUG, "%p\n", "close"));
  nm_opts.release_handles();

  int import_lines = 0;
  int export_lines = 0;
  ACE_Message_Block im_buffer (102400);
  ACE_Message_Block ex_buffer (102400);
  ACE_Message_Block *im_buf_cur = &im_buffer;
  ACE_Message_Block *ex_buf_cur = &ex_buffer;
  char dummy;
  int eoln = 1;
  //  ACE_Time_Value timeout (1,0);
  int is_import = 1;
  int is_export = 1;

  while (eoln == 1) {
    for (int i = 0; i < 10; i++) {
      if (ACE_OS::read(pipe[0],&dummy,1) != 1) {
        eoln = 2;
        break;
      }
    }
    if (eoln == 2)
      break;
    is_import = dummy == 'U';
    is_export = !imports_only && (ACE_OS::strchr("BCDRTVW",dummy) != 0);

    //    if (ACE::recv(pipe[0],&dummy,1,&timeout) != 1)
    if (ACE_OS::read(pipe[0],&dummy,1) != 1)
      break;

    eoln = this->read_line (pipe[0], is_import ? &im_buf_cur :
                            (is_export ? &ex_buf_cur : 0));
    import_lines += is_import;
    export_lines += is_export;
  }
  //  ACE_DEBUG ((LM_DEBUG, "read %d import lines and %d export lines\n",
  //            import_lines, export_lines));

  nmproc.wait ();
  ACE_OS::close (pipe[0]);

  this->populate_sig_list (imports_,import_lines,&im_buffer);
  if (!imports_only)
    this->populate_sig_list (exports_,export_lines,&ex_buffer);
}

void
Obj_Module::populate_sig_list (Sig_List &siglist,
                               int lines,
                               ACE_Message_Block *buf)
{
  char *c;
  ACE_CString temp;

  for (int i = 0; i < lines; i++)
    {
      for (c = buf->rd_ptr (); c != buf->wr_ptr () && *c != '\n'; ++c)
        {
          // No action.
        }

      temp += ACE_CString (buf->rd_ptr (), (c - buf->rd_ptr ()));
      buf->rd_ptr (c + 1);

      if (*c == '\n')
        {
          //      ACE_DEBUG ((LM_DEBUG, "%s\n",temp.c_str()));
          siglist.add (temp);
          temp.clear ();
        }
      else
        {
          buf = buf->cont ();

          if (buf == 0)
            {
              siglist.add (temp);
            }
        }
    }
}
