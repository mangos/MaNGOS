// $Id: Export.h 80826 2008-03-04 14:51:23Z wotte $

// Definition for Win32 Export directives.
// This file is generated automatically by
// ${TAO_ROOT}/TAO_IDL/GenExportH.BAT
// ------------------------------
#ifndef JAWS_EXPORT_H
#define JAWS_EXPORT_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if defined (ACE_AS_STATIC_LIBS)
# if !defined (JAWS_HAS_DLL)
#   define JAWS_HAS_DLL 0
# endif /* ! ACE_HAS_DLL */
#else
# if !defined (JAWS_HAS_DLL)
#   define JAWS_HAS_DLL 1
# endif /* ! ACE_HAS_DLL */
#endif /* ACE_AS_STATIC_LIB */

#if defined (JAWS_HAS_DLL)
#  if (JAWS_HAS_DLL == 1)
#    if defined (JAWS_BUILD_DLL)
#      define JAWS_Export ACE_Proper_Export_Flag
#      define JAWS_SINGLETON_DECLARATION(T) \
              ACE_EXPORT_SINGLETON_DECLARATION (T)
#    else
#      define JAWS_Export ACE_Proper_Import_Flag
#      define JAWS_SINGLETON_DECLARATION(T) \
              ACE_IMPORT_SINGLETON_DECLARATION (T)
#    endif /* JAWS_BUILD_DLL */
#  else
#    define JAWS_Export
#    define JAWS_SINGLETON_DECLARATION(T)
#  endif   /* ! JAWS_HAS_DLL == 1 */
#else
#  define JAWS_Export
#  define JAWS_SINGLETON_DECLARATION(T)
#endif     /* JAWS_HAS_DLL */

#endif     /* JAWS_EXPORT_H */
           // End of auto generated file.

