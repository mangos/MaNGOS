// -*- C++ -*-

/**
 * $Id: Options.cpp 91671 2010-09-08 18:39:23Z johnnyw $
 *
 * Copyright (C) 1989 Free Software Foundation, Inc.
 * written by Douglas C. Schmidt (schmidt@cs.wustl.edu)
 *
 * This file is part of GNU GPERF.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Options.h"



#if defined (ACE_HAS_GPERF)

#include "ace/Get_Opt.h"
#include "Iterator.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_stdlib.h"

/// These need to appear before the global class instantiation, since
/// they are static members with a default constructor that initializes
/// an ACE_Allocator needed in the Options class constructor.
ACE_CString Options::function_name_;
ACE_CString Options::fill_default_;
ACE_CString Options::key_name_;
ACE_CString Options::class_name_;
ACE_CString Options::hash_name_;
ACE_CString Options::delimiters_;

/// Global option coordinator for the entire program.
Options option;

/// Current program version.
extern const char *version_string;

/// Size to jump on a collision.
static const int DEFAULT_JUMP_VALUE = 5;

/// Default name for generated lookup function.
static const char *const DEFAULT_NAME = "in_word_set";

/// Default filler for keyword table.
static const char *const DEFAULT_FILL = "";

/// Default name for the key component.
static const char *const DEFAULT_KEY = "name";

/// Default name for the generated class.
static const char *const DEFAULT_CLASS_NAME = "Perfect_Hash";

/// Default name for generated hash function.
static const char *const DEFAULT_HASH_NAME = "hash";

/// Default delimiters that separate keywords from their attributes.
static const char *const DEFAULT_DELIMITERS = ",\n";

int Options::option_word_;
int Options::total_switches_;
u_int Options::total_keysig_size_;
int Options::size_;
int Options::key_pos_;
int Options::jump_;
int Options::initial_asso_value_;
int Options::argc_;
ACE_TCHAR **Options::argv_;
int Options::iterations_;
char Options::key_positions_[MAX_KEY_POS];

/// Prints program usage to standard error stream.
void
Options::usage (void)
{
  ACE_ERROR ((LM_ERROR,
              "Usage: %n [-abBcCdDef[num]gGhH<hashname>i<init>IjJ"
              "k<keys>K<keyname>lL<language>mMnN<function name>o"
              "Oprs<size>S<switches>tTvVZ<class name>].\n"
              "(type %n -h for help)\n"));
}

/// Output command-line Options.
void
Options::print_options (void)
{
  int i;

  ACE_OS::printf ("/* Command-line: ");

  for (i = 0; i < argc_; i++)
    ACE_OS::printf ("%s ",
                    argv_[i]);

  ACE_OS::printf (" */");
}

/// Sorts the key positions *IN REVERSE ORDER!!* This makes further
/// routines more efficient.  Especially when generating code.  Uses a
/// simple Insertion Sort since the set is probably ordered.  Returns 1
/// if there are no duplicates, 0 otherwise.
int
Options::key_sort (char *base, int len)
{
  int j = 0;
  int i = 0;
  for (i = 0, j = len - 1; i < j; i++)
    {
      int curr = 0;
      int tmp = 0;

      for (curr = i + 1, tmp = base[curr];
           curr > 0 && tmp >= base[curr - 1];
           curr--)
        if ((base[curr] = base[curr - 1]) == tmp)
          // Oh no, a duplicate!!!
          return 0;

      base[curr] = static_cast<char> (tmp);
    }

  return 1;
}

// Sets the default Options.

Options::Options (void)
{
  key_positions_[0] = WORD_START;
  key_positions_[1] = WORD_END;
  key_positions_[2] = EOS;
  total_keysig_size_ = 2;
  delimiters_ = DEFAULT_DELIMITERS;
  jump_ = DEFAULT_JUMP_VALUE;
  option_word_ = DEFAULTCHARS | C;
  function_name_ = DEFAULT_NAME;
  fill_default_ = DEFAULT_FILL;
  key_name_ = DEFAULT_KEY;
  hash_name_ = DEFAULT_HASH_NAME;
  class_name_ = DEFAULT_CLASS_NAME;
  total_switches_ = size_ = 1;
  initial_asso_value_ = iterations_ = 0;
}

/// Dumps option status when debug is set.
Options::~Options (void)
{
  if (ACE_BIT_ENABLED (option_word_, DEBUGGING))
    {
      char *ptr = 0;

      ACE_OS::fprintf (stderr,
                       "\ndumping Options:"
                       "\nDEBUGGING is...: %s"
                       "\nORDER is.......: %s"
                       "\nANSI is........: %s"
                       "\nTYPE is........: %s"
                       "\nINLINE is......: %s"
                       "\nRANDOM is......: %s"
                       "\nDEFAULTCHARS is: %s"
                       "\nSWITCH is......: %s"
                       "\nPOINTER is.....: %s"
                       "\nNOLENGTH is....: %s"
                       "\nLENTABLE is....: %s"
                       "\nDUP is.........: %s"
                       "\nFAST is........: %s"
                       "\nCOMP is........: %s"
                       "\nNOTYPE is......: %s"
                       "\nGLOBAL is......: %s"
                       "\nCONSTANT is....: %s"
                       "\nCPLUSPLUS is...: %s"
                       "\nC is...........: %s"
                       "\nENUM is........: %s"
                       "\nSTRCASECMP is..: %s"
                       "\nOPTIMIZE is....: %s"
                       "\nLINEARSEARCH is: %s"
                       "\nBINARYSEARCH is: %s"
                       "\niterations = %d"
                       "\nlookup function name = %C"
                       "\nfill default = %C"
                       "\nhash function name = %C"
                       "\nkey name = %C"
                       "\njump value = %d"
                       "\nmax associcated value = %d"
                       "\ninitial associated value = %d"
                       "\ndelimiters = %C"
                       "\nnumber of switch statements = %d"
                       "\n",
                       ACE_BIT_ENABLED (option_word_, DEBUGGING) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, ORDER) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, ANSI) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, TYPE) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, INLINE) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, RANDOM) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, DEFAULTCHARS) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, SWITCH) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, POINTER) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, NOLENGTH) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, LENTABLE) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, DUP) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, FAST) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, COMP) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, NOTYPE) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, GLOBAL) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, CONSTANT) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, CPLUSPLUS) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, C) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, ENUM) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, STRCASECMP) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, OPTIMIZE) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, LINEARSEARCH) ? "enabled" : "disabled",
                       ACE_BIT_ENABLED (option_word_, BINARYSEARCH) ? "enabled" : "disabled",
                       iterations_,
                       function_name_.c_str (),
                       fill_default_.c_str (),
                       hash_name_.c_str (),
                       key_name_.c_str (),
                       jump_,
                       size_ - 1,
                       initial_asso_value_,
                       delimiters_.c_str (),
                       total_switches_);
      if (ACE_BIT_ENABLED (option_word_, ALLCHARS))
        ACE_OS::fprintf (stderr,
                         "all characters are used in the hash function\n");

      ACE_OS::fprintf (stderr,
                       "maximum keysig size = %d\nkey positions are:\n",
                       total_keysig_size_);

      for (ptr = key_positions_; *ptr != EOS; ptr++)
        if (*ptr == WORD_END)
          ACE_OS::fprintf (stderr, "$\n");
        else
          ACE_OS::fprintf (stderr, "%d\n", *ptr);

      ACE_OS::fprintf (stderr, "finished dumping Options\n");
    }
}

/// Parses the command line Options and sets appropriate flags in
/// option_word_.
int
Options::parse_args (int argc, ACE_TCHAR *argv[])
{
  if (ACE_LOG_MSG->open (argv[0]) == -1)
    return -1;

  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT("abBcCdDe:Ef:F:gGhH:i:IJj:k:K:lL:mMnN:oOprs:S:tTvVZ:"));

  argc_ = argc;
  argv_ = argv;

  int option_char;
  while ((option_char = get_opt ()) != -1)
    {
      switch (option_char)
        {
          // Generated coded uses the ANSI prototype format.
        case 'a':
          {
            ACE_SET_BITS (option_word_, ANSI);
            break;
          }
          // Generate code for Linear Search.
        case 'b':
          {
            ACE_SET_BITS (option_word_, LINEARSEARCH);
            break;
          }
          // Generate code for Binary Search.
        case 'B':
          {
            ACE_SET_BITS (option_word_, BINARYSEARCH);
            break;
          }
          // Generate strncmp rather than strcmp.
        case 'c':
          {
            ACE_SET_BITS (option_word_, COMP);
            break;
          }
        // Make the generated tables readonly (const).
        case 'C':
          {
            ACE_SET_BITS (option_word_, CONSTANT);
            break;
          }
        // Enable debugging option.
        case 'd':
          {
            ACE_SET_BITS (option_word_, DEBUGGING);
            ACE_ERROR ((LM_ERROR,
                        "Starting program %n, version %s, with debugging on.\n",
                        version_string));
            break;
          }
        // Enable duplicate option.
        case 'D':
          {
            ACE_SET_BITS (option_word_, DUP);
            break;
          }
        // Allows user to provide keyword/attribute separator
        case 'e':
          {
            delimiters_ = ACE_TEXT_ALWAYS_CHAR(get_opt.opt_arg ());
            break;
          }
        case 'E':
          {
            ACE_SET_BITS (option_word_, ENUM);
            break;
          }
        // Generate the hash table ``fast.''
        case 'f':
          {
            ACE_SET_BITS (option_word_, FAST);
            iterations_ = ACE_OS::atoi (get_opt.opt_arg ());
            if (iterations_ < 0)
              {
                ACE_ERROR ((LM_ERROR, "iterations value must not be negative, assuming 0\n"));
                iterations_ = 0;
              }
            break;
          }
        // Use the ``inline'' keyword for generated sub-routines.
        case 'g':
          {
            ACE_SET_BITS (option_word_, INLINE);
            break;
          }
        // Make the keyword table a global variable.
        case 'G':
          {
            ACE_SET_BITS (option_word_, GLOBAL);
            break;
          }
        // Displays a list of helpful Options to the user.
        case 'h':
          {
            ACE_OS::fprintf (stderr,
                             "-a\tGenerate ANSI standard C output code, i.e., function prototypes.\n"
                             "-b\tGenerate code for Linear Search.\n"
                             "-B\tGenerate code for Binary Search.\n"
                             "-c\tGenerate comparison code using strncmp rather than strcmp.\n"
                             "-C\tMake the contents of generated lookup tables constant, i.e., readonly.\n"
                             "-d\tEnables the debugging option (produces verbose output to the standard\n"
                             "\terror).\n"
                             "-D\tHandle keywords that hash to duplicate values.  This is useful\n"
                             "\tfor certain highly redundant keyword sets.\n"
                             "-e\tAllow user to provide a string containing delimiters used to separate\n"
                             "\tkeywords from their attributes.  Default is \",\\n\"\n"
                             "-E\tDefine constant values using an enum local to the lookup function\n"
                             "\trather than with defines\n"
                             "-f\tGenerate the gen-perf.hash function ``fast.''  This decreases GPERF's\n"
                             "\trunning time at the cost of minimizing generated table-size.\n"
                             "\tThe numeric argument represents the number of times to iterate when\n"
                             "\tresolving a collision.  `0' means ``iterate by the number of keywords.''\n"
                             "-F\tProvided expression will be used to assign default values in keyword\n"
                             "\ttable, i.e., the fill value.  Default is \"\".\n"
                             "-g\tMake generated routines use ``inline'' to remove function overhead.\n"
                             "-G\tGenerate the static table of keywords as a static global variable,\n"
                             "\trather than hiding it inside of the lookup function (which is the\n"
                             "\tdefault behavior).\n"
                             "-h\tPrints this message.\n"
                             "-H\tAllow user to specify name of generated hash function. Default\n"
                             "\tis `hash'.\n"
                             "-i\tProvide an initial value for the associate values array.  Default is 0.\n"
                             "-I\tGenerate comparison code using case insensitive string comparison, e.g.,\n"
                             "\tstrncasecmp or strcasecmp.\n"
                             "\tSetting this value larger helps inflate the size of the final table.\n"
                             "-j\tAffects the ``jump value,'' i.e., how far to advance the associated\n"
                             "\tcharacter value upon collisions.  Must be an odd number, default is %d.\n"
                             "-J\tSkips '#include \"ace/OS_NS_string.h\"' part in the output.\n"
                             "-k\tAllows selection of the key positions used in the hash function.\n"
                             "\tThe allowable choices range between 1-%d, inclusive.  The positions\n"
                             "\tare separated by commas, ranges may be used, and key positions may\n"
                             "\toccur in any order.  Also, the meta-character '*' causes the generated\n"
                             "\thash function to consider ALL key positions, and $ indicates the\n"
                             "\t``final character'' of a key, e.g., $,1,2,4,6-10.\n"
                             "-K\tAllow use to select name of the keyword component in the keyword\n"
                             "\tstructure.\n"
                             "-l\tCompare key lengths before trying a string comparison.  This helps\n"
                             "\tcut down on the number of string comparisons made during the lookup.\n"
                             "-L\tGenerates code in the language specified by the option's argument.\n"
                             "\tLanguages handled are currently C++ and C.  The default is C.\n"
                             "-m\tAvoids the warning about identical hash values. This is valid\n"
                             "\tonly if the -D option is enabled.\n"
                             "-M\tSkips class definition in the output. This is valid only in C++ mode.\n"
                             "-n\tDo not include the length of the keyword when computing the hash\n"
                             "\tfunction.\n"
                             "-N\tAllow user to specify name of generated lookup function.  Default\n"
                             "\tname is `in_word_set.'\n"
                             "-o\tReorders input keys by frequency of occurrence of the key sets.\n"
                             "\tThis should decrease the search time dramatically.\n"
                             "-O\tOptimize the generated lookup function by assuming that all input\n"
                             "\tkeywords are members of the keyset from the keyfile.\n"
                             "-p\tChanges the return value of the generated function ``in_word_set''\n"
                             "\tfrom its default boolean value (i.e., 0 or 1), to type ``pointer\n"
                             "\tto wordlist array''  This is most useful when the -t option, allowing\n"
                             "\tuser-defined structs, is used.\n"
                             "-r\tUtilizes randomness to initialize the associated values table.\n"
                             "-s\tAffects the size of the generated hash table.  The numeric argument\n"
                             "\tfor this option indicates ``how many times larger or smaller'' the\n"
                             "\tassociated value range should be, in relationship to the number of\n"
                             "\tkeys, e.g. a value of 3 means ``allow the maximum associated value\n"
                             "\tto be about 3 times larger than the number of input keys.''\n"
                             "\tConversely, a value of -3 means ``make the maximum associated\n"
                             "\tvalue about 3 times smaller than the number of input keys. A\n"
                             "\tlarger table should decrease the time required for an unsuccessful\n"
                             "\tsearch, at the expense of extra table space.  Default value is 1.\n"
                             "-S\tCauses the generated C code to use a switch statement scheme, rather\n"
                             "\tthan an array lookup table.  This can lead to a reduction in both\n"
                             "\ttime and space requirements for some keyfiles.  The argument to\n"
                             "\tthis option determines how many switch statements are generated.\n"
                             "\tA value of 1 generates 1 switch containing all the elements, a value\n"
                             "\tof 2 generates 2 tables with 1/2 the elements in each table, etc.\n"
                             "\tThis is useful since many C compilers cannot correctly generate code\n"
                             "\tfor large switch statements.\n"
                             "-t\tAllows the user to include a structured type declaration for\n"
                             "\tgenerated code. Any text before %%%% is consider part of the type\n"
                             "\tdeclaration.  Key words and additional fields may follow this, one\n"
                             "\tgroup of fields per line.\n"
                             "-T\tPrevents the transfer of the type declaration to the output file.\n"
                             "\tUse this option if the type is already defined elsewhere.\n"
                             "-v\tPrints out the current version number and exits with a value of 0\n"
                             "-V\tExits silently with a value of 0.\n"
                             "-Z\tAllow user to specify name of generated C++ class.  Default\n"
                             "\tname is `Perfect_Hash.'\n",
                             DEFAULT_JUMP_VALUE,
                             MAX_KEY_POS - 1);
            Options::usage ();
            return -1;
          }
        // Sets the name for the hash function.
        case 'H':
          {
            hash_name_ = ACE_TEXT_ALWAYS_CHAR(get_opt.opt_arg ());
            break;
          }
        // Sets the initial value for the associated values array.
        case 'i':
          {
            initial_asso_value_ = ACE_OS::atoi (get_opt.opt_arg ());
            if (initial_asso_value_ < 0)
              ACE_ERROR ((LM_ERROR,
                          "Initial value %d should be non-zero, ignoring and continuing.\n",
                          initial_asso_value_));
            if (option[RANDOM])
              ACE_ERROR ((LM_ERROR,
                          "warning, -r option superceeds -i, ignoring -i option and continuing\n"));
            break;
          }
         case 'I':
           {
             ACE_SET_BITS (option_word_, STRCASECMP);
             break;
           }
         // Sets the jump value, must be odd for later algorithms.
        case 'j':
          {
            jump_ = ACE_OS::atoi (get_opt.opt_arg ());
            if (jump_ < 0)
              ACE_ERROR_RETURN ((LM_ERROR,
                                 "Jump value %d must be a positive number.\n%r",
                                 jump_,
                                 &Options::usage),
                                 -1);
            else if (jump_ && ACE_EVEN (jump_))
              ACE_ERROR ((LM_ERROR,
                          "Jump value %d should be odd, adding 1 and continuing...\n",
                          jump_++));
            break;
          }
        // Skip including the header file ace/OS_NS_string.h.
        case 'J':
          {
            ACE_SET_BITS (option_word_, SKIPSTRINGH);
            break;
          }
        // Sets key positions used for hash function.
        case 'k':
          {
            const int BAD_VALUE = -1;
            int value;
            Iterator expand (ACE_TEXT_ALWAYS_CHAR(get_opt.opt_arg ()),
                             1,
                             MAX_KEY_POS - 1,
                             WORD_END,
                             BAD_VALUE,
                             EOS);

            // Use all the characters for hashing!!!!
            if (*get_opt.opt_arg () == '*')
              option_word_ = (option_word_ & ~DEFAULTCHARS) | ALLCHARS;
            else
              {
                char *l_key_pos;

                for (l_key_pos = key_positions_;
                     (value = expand ()) != EOS;
                     l_key_pos++)
                  if (value == BAD_VALUE)
                    ACE_ERROR_RETURN ((LM_ERROR,
                                       "Illegal key value or range, use 1,2,3-%d,'$' or '*'.\n%r",
                                       MAX_KEY_POS - 1,
                                       usage),
                                      -1);
                  else
                    *l_key_pos = static_cast<char> (value);

                *l_key_pos = EOS;

                total_keysig_size_ = (l_key_pos - key_positions_);
                if (total_keysig_size_ == 0)
                  ACE_ERROR_RETURN ((LM_ERROR,
                                     "No keys selected.\n%r",
                                     &Options::usage),
                                    -1);
                else if (key_sort (key_positions_, total_keysig_size_) == 0)
                  ACE_ERROR_RETURN ((LM_ERROR,
                                     "Duplicate keys selected\n%r",
                                     &Options::usage),
                                    -1);
                if (total_keysig_size_ != 2
                    || (key_positions_[0] != 1
                        || key_positions_[1] != WORD_END))
                  ACE_CLR_BITS (option_word_, DEFAULTCHARS);
              }
            break;
          }
        // Make this the keyname for the keyword component field.
        case 'K':
          {
            key_name_ = ACE_TEXT_ALWAYS_CHAR(get_opt.opt_arg ());
            break;
          }
        // Create length table to avoid extra string compares.
        case 'l':
          {
            ACE_SET_BITS (option_word_, LENTABLE);
            break;
          }
        // Deal with different generated languages.
        case 'L':
          {
            option_word_ &= ~C;
            if (!ACE_OS::strcmp (get_opt.opt_arg (), ACE_TEXT("C++")))
              ACE_SET_BITS (option_word_, (CPLUSPLUS | ANSI));
            else if (!ACE_OS::strcmp (get_opt.opt_arg (), ACE_TEXT("C")))
              ACE_SET_BITS (option_word_, C);
            else
              {
                ACE_ERROR ((LM_ERROR,
                            "unsupported language option %s, defaulting to C\n",
                            get_opt.opt_arg ()));
                ACE_SET_BITS (option_word_, C);
              }
            break;
          }
        // Don't print the warnings.
        case 'm':
          {
            ACE_SET_BITS (option_word_, MUTE);
            break;
          }
        // Skip the class definition while in C++ mode.
        case 'M':
          {
            ACE_SET_BITS (option_word_, SKIPCLASS);
            break;
          }
        // Don't include the length when computing hash function.
        case 'n':
          {
            ACE_SET_BITS (option_word_, NOLENGTH);
            break;
          }
        // Make generated lookup function name be.opt_arg ()
        case 'N':
          {
            function_name_ = ACE_TEXT_ALWAYS_CHAR(get_opt.opt_arg ());
            break;
          }
        // Make fill_default be.opt_arg ()
        case 'F':
          {
            fill_default_ = ACE_TEXT_ALWAYS_CHAR(get_opt.opt_arg ());
            break;
          }
        // Order input by frequency of key set occurrence.
        case 'o':
          {
            ACE_SET_BITS (option_word_, ORDER);
            break;
          }
        case 'O':
          {
            ACE_SET_BITS (option_word_, OPTIMIZE);
            break;
          }
        // Generated lookup function now a pointer instead of int.
        case 'p':
          {
            ACE_SET_BITS (option_word_, POINTER);
            break;
          }
        // Utilize randomness to initialize the associated values
        // table.
        case 'r':
          {
            ACE_SET_BITS (option_word_, RANDOM);
            if (initial_asso_value_ != 0)
              ACE_ERROR ((LM_ERROR,
                          "warning, -r option superceeds -i, disabling -i option and continuing\n"));
            break;
          }
        // Range of associated values, determines size of final table.
        case 's':
          {
            size_ = ACE_OS::atoi (get_opt.opt_arg ());
            if (abs (size_) > 50)
              ACE_ERROR ((LM_ERROR,
                          "%d is excessive, did you really mean this?! (type %n -h for help)\n",
                          size_));
            break;
          }
        // Generate switch statement output, rather than lookup table.
        case 'S':
          {
            ACE_SET_BITS (option_word_, SWITCH);
            total_switches_ = ACE_OS::atoi (get_opt.opt_arg ());
            if (total_switches_ <= 0)
              ACE_ERROR_RETURN ((LM_ERROR,
                                 "number of switches %s must be a positive number\n%r",
                                 get_opt.opt_arg (),
                                 &Options::usage),
                                -1);
            break;
          }
        // Enable the TYPE mode, allowing arbitrary user structures.
        case 't':
          {
            ACE_SET_BITS (option_word_, TYPE);
            break;
          }
        // Don't print structure definition.
        case 'T':
          {
            ACE_SET_BITS (option_word_, NOTYPE);
            break;
          }
        // Print out the version and quit.
        case 'v':
          ACE_ERROR ((LM_ERROR,
                      "%n: version %s\n%r\n",
                      version_string,
                      &Options::usage));
          ACE_OS::exit (0);
          /* NOTREACHED */
          break;
        // Exit with value of 0 (this is useful to check if gperf exists)
        case 'V':
          ACE_OS::exit (0);
          /* NOTREACHED */
          break;
        // Set the class name.
        case 'Z':
          {
            class_name_ = ACE_TEXT_ALWAYS_CHAR(get_opt.opt_arg ());
            break;
          }
        default:
          ACE_ERROR_RETURN ((LM_ERROR,
                             "%r",
                             &Options::usage),
                            -1);
        }

    }

  if (argv[get_opt.opt_ind ()] &&
    ACE_OS::freopen (argv[get_opt.opt_ind ()],
                     ACE_TEXT("r"),
                     stdin) == 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "Cannot open keyword file %p\n%r",
                       argv[get_opt.opt_ind ()],
                       &Options::usage),
                      -1);
  if (get_opt.opt_ind () + 1 < argc)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "Extra trailing arguments to %n.\n%r",
                       usage),
                      -1);
  return 0;
}

/// True if option enable, else false.
int
Options::operator[] (Option_Type option)
{
  return ACE_BIT_ENABLED (option_word_, option);
}

/// Enables option OPT.
void
Options::operator = (enum Option_Type opt)
{
  ACE_SET_BITS (option_word_, opt);
}

/// Disables option OPT.
bool
Options::operator != (enum Option_Type opt)
{
  // @@ Why is this inequality comparison operator clearing bits?
  ACE_CLR_BITS (option_word_, opt);

  return true;
}

/// Initializes the key Iterator.
void
Options::reset (void)
{
  key_pos_ = 0;
}

/// Returns current key_position and advanced index.
int
Options::get (void)
{
  return key_positions_[key_pos_++];
}

/// Sets the size of the table size.
void
Options::asso_max (int r)
{
  size_ = r;
}

/// Returns the size of the table size.
int
Options::asso_max (void)
{
  return size_;
}

/// Returns total distinct key positions.
u_int
Options::max_keysig_size (void)
{
  return total_keysig_size_;
}

/// Sets total distinct key positions.
void
Options::keysig_size (u_int a_size)
{
  total_keysig_size_ = a_size;
}

/// Returns the jump value.
int
Options::jump (void)
{
  return jump_;
}

/// Returns the generated function name.
const char *
Options::function_name (void)
{
  return function_name_.c_str ();
}

/// Returns the fill default
const char *
Options::fill_default (void)
{
  return fill_default_.c_str ();
}

/// Returns the keyword key name.
const char *
Options::key_name (void)
{
  return key_name_.c_str ();
}

/// Returns the hash function name.
const char *
Options::hash_name (void)
{
  return hash_name_.c_str ();
}

/// Returns the generated class name.
const char *
Options::class_name (void)
{
  return class_name_.c_str ();
}

/// Returns the initial associated character value.
int
Options::initial_value (void)
{
  return initial_asso_value_;
}

/// Returns the iterations value.
int
Options::iterations (void)
{
  return iterations_;
}

/// Returns the string used to delimit keywords from other attributes.
const char *
Options::delimiter (void)
{
  return delimiters_.c_str ();
}

/// Gets the total number of switch statements to generate.
int
Options::total_switches (void)
{
  return total_switches_;
}

#endif /* ACE_HAS_GPERF */
