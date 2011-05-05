eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: generate_export_file.pl 80826 2008-03-04 14:51:23Z wotte $
# Replacement for the old trusty GenExportH.bat
# Creates the nice little *_export file which is used for
# importing and exporting of symbols in DLLs.
# (they are soooo cute!)

use Getopt::Std;

##############################################################################
# Grab the options

$flags = join (" ", @ARGV);

if (!getopts ('df:hsn') || $opt_h) {
    print STDERR
          "generate_export_file.pl [-d] [-f dependency] [-n] library_name\n",
          "\n",
          "    -d         Turn on debug mode\n",
          "    -f         Adds a dependency to another *_HAS_DLL macro\n",
          "    -n         Do not add in ACE_AS_STATIC_LIBS check\n",
          "\n",
          "generate_export_file creates the *_export files that are used\n",
          "in exporting of symbols for DLLs (and not exporting them when\n",
          "the library is static).  If library_name is something like\n",
          "\"Foo\", then the file will contain definitions for Foo_Export\n",
          "and FOO_SINGLETON_DECLARE, etc. which will be controlled by\n",
          "FOO_HAS_DLL, etc.\n";
    exit (1);
}

if (defined $opt_d) {
    print STDERR "Debugging Turned on\n";

    if (defined $opt_f) {
        print STDERR "Dependency to $opt_f\n";
    }

    if (defined $opt_n) {
        print STDERR "ACE_AS_STATIC_LIBS turned off\n";
    }
}


if ($#ARGV < 0) {
    print STDERR "No library_name specified, use -h for help\n";
    exit (1);
}

$name = shift @ARGV;
$ucname = uc $name;

##############################################################################
# Prologue

$prologue = '
// -*- C++ -*-
// ' . '$' . 'Id' . '$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl '."$flags".'
// ------------------------------'."
#ifndef -UC-_EXPORT_H
#define -UC-_EXPORT_H

#include \"ace/config-all.h\"
";


##############################################################################
# Static Stuff

if (!defined $opt_n)
{
    $static_stuff = "
#if defined (ACE_AS_STATIC_LIBS) && !defined (-UC-_HAS_DLL)
#  define -UC-_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && -UC-_HAS_DLL */
";
}

##############################################################################
# Dependencies

if (defined $opt_f)
{
    $has_dll = "
#if defined ($opt_f)
#  if !defined (-UC-_HAS_DLL)
#    define -UC-_HAS_DLL 0
#  endif /* ! -UC-_HAS_DLL */
#else
#  if !defined (-UC-_HAS_DLL)
#    define -UC-_HAS_DLL 1
#  endif /* ! -UC-_HAS_DLL */
#endif
";
}
else
{
    $has_dll = "
#if !defined (-UC-_HAS_DLL)
#  define -UC-_HAS_DLL 1
#endif /* ! -UC-_HAS_DLL */
";
}

##############################################################################
# Epilogue

$epilogue = "
#if defined (-UC-_HAS_DLL) && (-UC-_HAS_DLL == 1)
#  if defined (-UC-_BUILD_DLL)
#    define -NC-_Export ACE_Proper_Export_Flag
#    define -UC-_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define -UC-_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* -UC-_BUILD_DLL */
#    define -NC-_Export ACE_Proper_Import_Flag
#    define -UC-_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define -UC-_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* -UC-_BUILD_DLL */
#else /* -UC-_HAS_DLL == 1 */
#  define -NC-_Export
#  define -UC-_SINGLETON_DECLARATION(T)
#  define -UC-_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* -UC-_HAS_DLL == 1 */

// Set -UC-_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (-UC-_NTRACE)
#  if (ACE_NTRACE == 1)
#    define -UC-_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define -UC-_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !-UC-_NTRACE */

#if (-UC-_NTRACE == 1)
#  define -UC-_TRACE(X)
#else /* (-UC-_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define -UC-_TRACE(X) ACE_TRACE_IMPL(X)
#  include \"ace/Trace.h\"
#endif /* (-UC-_NTRACE == 1) */

#endif /* -UC-_EXPORT_H */

// End of auto generated file.
";

##############################################################################
# Print the stuff out

foreach $export ($prologue, $static_stuff, $has_dll, $epilogue)
{
## -NC- stands for normal case, the name as it is
## -UC- stands for the name all upper case
    map { s/-NC-/$name/g; s/-UC-/$ucname/g; } $export;

    print $export;
};
