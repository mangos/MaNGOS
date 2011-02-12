/* $Id: mkcsregdb.cpp 84565 2009-02-23 08:20:39Z johnnyw $
 *
 * To populate the registry_db, construct a codeset registry text file based
 * on the OSF's Character and Code Set Registry. See DCE RFC 40.1 for details
 * on obtaining the full text for the current registry. Once you have composed
 * a text file containing all the desired codeset information, build and run
 * mkcsregdb. The source is in $ACE_ROOT/apps/mkcsregdb. It will generate a new
 * copy of this file, with the registry_db_ array properly initialized.
 */

// FUZZ: disable check_for_streams_include
#include "ace/streams.h"

#include "ace/Codeset_Registry.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_sys_stat.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"

class csdb_generator : public ACE_Codeset_Registry
{
public:
  csdb_generator ();
  int read_from (const char *source);
  int init_output (const char *srcfile);
  int write_entry ();
  int fini_output (const char *target);
  int read_line ();
  void write_dummy_entry();
  void fail(const char *msg);
private:
  static const char * tags_[];
  static int max_line_;
  int count_;
  int in_entry_;
  registry_entry entry_;
  int linecounter_;
  char *source_line_;
  char *line_data_;
  ifstream *inf_;
  FILE *outf_;
  char *tempfilename_;
  char *ace_src_;
};

int csdb_generator::max_line_ = 1024;
const char * csdb_generator::tags_[] = {
  "start",
  "end",
  "description ",
  "loc_name ",
  "rgy_value ",
  "char_values ",
  "max_bytes ",
  0
};

csdb_generator::csdb_generator ()
  :count_(0),
   in_entry_(0),
   linecounter_(0),
   source_line_ (new char[max_line_]),
   line_data_ (0),
   inf_ (0),
   outf_ (0)
{
  ace_src_ = ACE_OS::getenv("ACE_ROOT");
}

void
csdb_generator::fail (const char *msg)
{
  ACE_ERROR ((LM_ERROR,"Error, line %d: %s\n",linecounter_,msg));
  ACE_OS::fclose (outf_);
  ACE_OS::unlink (tempfilename_);
  ACE_OS::exit (-1);
}

int
csdb_generator::read_line()
{
  inf_->getline (source_line_,max_line_);
  line_data_ = 0;
  char *cpos = ACE_OS::strchr (source_line_,'#');
  if (cpos == 0)
    cpos = source_line_ + ACE_OS::strlen(source_line_);
  while (cpos > source_line_ && ACE_OS::strchr(" \t",*(cpos - 1))) cpos--;
  *cpos = 0;
  if (cpos == source_line_)
    return 0;
  for (int i = 0; tags_[i]; i++)
    {
      cpos = ACE_OS::strstr (source_line_,tags_[i]);
      if (cpos == 0) // not found
        continue;
      if (cpos > source_line_) // make sure it's first token
        {
          char *tpos = cpos-1;
          while (tpos > source_line_ && ACE_OS::strchr(" \t",*tpos)) tpos--;
          if (tpos > source_line_)
            continue;
        }
      if (i == 0 && in_entry_)
        fail ("\"start\" encountered twice before \"end\"");
      if (i > 0 && !in_entry_)
        {
          char *emsg = new char[100];
          ACE_OS::sprintf (emsg,"\"%s\" encountered before \"start\"",tags_[i]);
          fail (emsg);
        }
      if (i > 1)
        {
          line_data_ = cpos + ACE_OS::strlen(tags_[i]);
          while (*line_data_ && ACE_OS::strchr(" \t",(*line_data_)))
            line_data_++;
        }
      return i+1;
    }
  return -1;
}

int
csdb_generator::read_from (const char *srcfile)
{
  inf_ = new ifstream(srcfile);
  char *ptr;
  while (inf_->good() && !inf_->eof()) {
    linecounter_++;
    switch (read_line ()) {
    case -1: // bogus line
      fail ("unknown field tag");
      break;
    case 0: // comment or blank line
      break;
    case 1: // start
      entry_.desc_ = 0;
      entry_.loc_name_ = 0;
      entry_.codeset_id_ = 0;
      entry_.num_sets_ = 0;
      entry_.max_bytes_ = 0;
      in_entry_ = 1;
      break;
    case 2: // end
      if (entry_.codeset_id_ == 0)
        fail ("entry missing rgy_value");
      if (entry_.num_sets_ == 0)
        fail ("entry does not include at least one char_value");
      if (entry_.max_bytes_ == 0)
        fail ("entry does not define max_bytes");
      write_entry ();
      delete [] const_cast<char *> (entry_.desc_);
      delete [] const_cast<char *> (entry_.loc_name_);
      count_++;
      in_entry_ = 0;
      break;
    case 3: // description
      if (entry_.desc_ != 0)
        fail ("duplicate description");
      entry_.desc_ = ACE_OS::strdup(line_data_);
      break;
    case 4: // loc_name
      if (entry_.loc_name_ != 0)
        fail ("duplicate loc_name");
       entry_.loc_name_ = ACE_OS::strdup(line_data_);
       break;
    case 5: // rgy_value
      if (entry_.codeset_id_ != 0)
        fail ("duplicate rgy_value");
      entry_.codeset_id_ = ACE_OS::strtoul(line_data_,&ptr,16);
      if (*ptr != 0 || entry_.codeset_id_ == 0)
        {
          char emsg [100];
          ACE_OS::sprintf (emsg,"invalid rgy_value, '%s'",line_data_);
          fail (emsg);
        }
      break;
    case 6: // char_values
      if (entry_.num_sets_ != 0)
        fail ("duplicate char_values");
      ptr = line_data_;
      do {
        if (*ptr == ':')
          ptr++;
        ACE_CDR::UShort tmp =
          static_cast<ACE_CDR::UShort> (ACE_OS::strtoul(ptr,&ptr,16));
        if (*ptr != 0 && *ptr != ':')
          {
            char *emsg = new char [100];
            ACE_OS::sprintf (emsg,"invalid symbol \'%c\' in char_values",*ptr);
            fail (emsg);
          }
        if (entry_.num_sets_ < max_charsets_)
          entry_.char_sets_[entry_.num_sets_++] = tmp;
        else entry_.num_sets_++;
      } while (*ptr == ':');
      if (entry_.num_sets_ > max_charsets_)
        {
          char *emsg = new char [200];
          ACE_OS::sprintf (emsg,"max of %d char_values exceeded.\nIncrease ACE_Codeset_Registry::max_charsets_ to at least %d and rebuild mkcsregdb",max_charsets_,entry_.num_sets_);
          fail (emsg);
        }
      break;
    case 7: // max_bytes
      if (entry_.max_bytes_ != 0)
        fail ("duplicate max_bytes");
      entry_.max_bytes_ =
        static_cast<ACE_CDR::UShort> (ACE_OS::strtol(line_data_,&ptr,10));
      if (*ptr != 0)
        fail ("invalid max_bytes");
      break;
    }
  }
  return 0;
}

int
csdb_generator::init_output (const char *srcfile)
{
  ACE_stat buf;
  if (ACE_OS::stat (srcfile,&buf) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,"Unable to open %s\n",srcfile),-1);

  if (ace_src_ == 0)
    ACE_ERROR_RETURN ((LM_ERROR,"You must first set $ACE_ROOT\n"),-1);

  time_t now = ACE_OS::time();
  tempfilename_ = ACE_OS::tempnam (ace_src_,"csdb");
  outf_ = ACE_OS::fopen (tempfilename_,"w");
  if (outf_ == 0)
    ACE_ERROR_RETURN ((LM_ERROR, "Unable to open output file, %s\n",tempfilename_),-1);

  ACE_OS::fprintf (outf_,"// $ID: $\n");
  ACE_OS::fprintf (outf_,"/*\n * Codeset registry DB, generated %s * source: %s\n",
                   ACE_OS::asctime (ACE_OS::localtime(&now)),
                   srcfile);
  ACE_OS::fprintf (outf_," *\n * To populate the registry_db, construct a codeset registry text file based\n");
  ACE_OS::fprintf (outf_," * on the OSF's Character and Code Set Registry. See DCE RFC 40.1 for details\n");
  ACE_OS::fprintf (outf_," * on obtaining the full text for the current registry. Once you have composed\n");
  ACE_OS::fprintf (outf_," * a text file containing all the desired codeset information, build and run\n");
  ACE_OS::fprintf (outf_," * mkcsregdb. The source is in $ACE_ROOT/apps/mkcsregdb. It will generate a new\n");
  ACE_OS::fprintf (outf_," * copy of this file, with the registry_db_ array properly initialized.\n */\n");
  ACE_OS::fprintf (outf_,"\n#include \"ace/Codeset_Registry.h\"\n\n%s\n%s\n{\n",
                   "ACE_Codeset_Registry::registry_entry const",
                   "ACE_Codeset_Registry::registry_db_[] =");
  return 0;
}

int
csdb_generator::write_entry ()
{
  if (count_)
    ACE_OS::fprintf (outf_,",\n");
  ACE_OS::fprintf (outf_,"  {\"%s\",\"%s\",0x%08x,%d,{",
                   entry_.desc_,
                   entry_.loc_name_,
                   entry_.codeset_id_,
                   entry_.num_sets_);
  for (ACE_CDR::UShort j = 0; j < entry_.num_sets_; j++)
    if (j+1 < entry_.num_sets_)
      ACE_OS::fprintf (outf_,"0x%04x,",entry_.char_sets_[j]);
    else
      ACE_OS::fprintf (outf_,"0x%04x",entry_.char_sets_[j]);
  ACE_OS::fprintf (outf_,"},%d}",entry_.max_bytes_);
  return 0;
}

void
csdb_generator::write_dummy_entry()
{
  entry_.desc_ = "No codesets defined";
  entry_.loc_name_ = "NONE";
  entry_.codeset_id_ = 0;
  entry_.num_sets_ = 1;
  entry_.char_sets_[0] = 0;
  entry_.max_bytes_ = 0;
  write_entry();
}

int
csdb_generator::fini_output (const char *tgt)
{
  char *target = new char [ACE_OS::strlen(ace_src_) + ACE_OS::strlen(tgt) + 6];
  ACE_OS::sprintf (target,"%s/ace/%s",ace_src_,tgt);
  if (count_ == 0)
    write_dummy_entry();
  ACE_OS::fprintf (outf_,"\n};\n\nsize_t const ACE_Codeset_Registry::num_registry_entries_ = %d;\n\n",count_);
  ACE_OS::fclose (outf_);
  ACE_stat buf;
  if (ACE_OS::stat (target,&buf) == 0)
    {
      char fname[200];
      int result = 0;
      for (int i = 0; result == 0; i++)
        {
          ACE_OS::sprintf (fname,"%s.%03d",target,i);
          result = ACE_OS::stat(fname,&buf);
        }
      ACE_DEBUG ((LM_DEBUG,"Moving $ACE_ROOT/ace/%s to $ACE_ROOT/ace%s\n",
                  tgt,ACE_OS::strrchr(fname,'/')));
      if (ACE_OS::rename (target,fname) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,"Could not create %s\n, output stored in %s",
                           fname,tempfilename_),-1);
    }
  ACE_DEBUG ((LM_DEBUG,"writing $ACE_ROOT/ace/%s\n",tgt));
  if (ACE_OS::rename (tempfilename_,target) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,"Could not create %s\n, output stored in %s",
                       target,tempfilename_),-1);
  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  ACE_CString srcname;
  if (argc > 1)
    srcname.set(argv[1]);
  else
    ACE_ERROR_RETURN ((LM_ERROR,"Usage: %s <source_file>\nwhere source file is the full path to a code set registry text file.\n",argv[0]),-1);
  csdb_generator csdb;
  if (csdb.init_output(srcname.c_str()) == -1)
    return 0;
  if (csdb.read_from (srcname.c_str()) == 0)
    csdb.fini_output ("Codeset_Registry_db.cpp");
  return 0;
}
