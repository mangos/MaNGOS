// -*- C++ -*-
// $Id: Export.h 80826 2008-03-04 14:51:23Z wotte $
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl -s JAWS
// ------------------------------
#ifndef JAWS_EXPORT_H
#define JAWS_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (JAWS_HAS_DLL)
#  define JAWS_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && JAWS_HAS_DLL */

#if !defined (JAWS_HAS_DLL)
#  define JAWS_HAS_DLL 1
#endif /* ! JAWS_HAS_DLL */

#if defined (JAWS_HAS_DLL) && (JAWS_HAS_DLL == 1)
#  if defined (JAWS_BUILD_DLL)
#    define JAWS_Export ACE_Proper_Export_Flag
#    define JAWS_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define JAWS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* JAWS_BUILD_DLL */
#    define JAWS_Export ACE_Proper_Import_Flag
#    define JAWS_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define JAWS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* JAWS_BUILD_DLL */
#else /* JAWS_HAS_DLL == 1 */
#  define JAWS_Export
#  define JAWS_SINGLETON_DECLARATION(T)
#  define JAWS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* JAWS_HAS_DLL == 1 */

// Set JAWS_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (JAWS_NTRACE)
#  if (ACE_NTRACE == 1)
#    define JAWS_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define JAWS_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !JAWS_NTRACE */

#if (JAWS_NTRACE == 1)
#  define JAWS_TRACE(X)
#else /* (JAWS_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define JAWS_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (JAWS_NTRACE == 1) */

#endif /* JAWS_EXPORT_H */

// End of auto generated file.
