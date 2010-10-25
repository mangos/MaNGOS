# ACE_FUNC_STRCASECMP
# + Defines ACE_LACKS_STRCASECMP to 1 if platform lacks strcasecmp()
# + Defines ACE_STRCASECMP_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
# + Defines ACE_LACKS_STRCASECMP_PROTOTYPE to 1 if platform lacks
#   declaration for strcasecmp().
#---------------------------------------------------------------------------
AC_DEFUN([ACE_FUNC_STRCASECMP],
[ACE_CHECK_LACKS_FUNCS(strcasecmp)
if test "$ac_cv_func_strcasecmp" = yes; then
    AC_CHECK_DECL([strcasecmp],
		  [],
		  [AC_DEFINE([ACE_LACKS_STRCASECMP_PROTOTYPE], 1,
			     [Define to 1 if platform lacks a declaration for strcasecmp()])],
		  [
#if !defined(ACE_LACKS_STRINGS_H)
#include <strings.h>
#endif
#if !defined(ACE_LACKS_STRING_H)
#include <string.h>
#endif
		  ])
else
    AC_CHECK_FUNC(stricmp)
    if test "$ac_cv_func_stricmp" = yes; then
        AC_DEFINE(ACE_STRCASECMP_EQUIVALENT, [::stricmp],
		  [Define to function that is equivalent to strcasecmp()])
    else
	AC_CHECK_FUNC(_stricmp)
	if test "$ac_cv_func__stricmp" = yes; then
	    AC_DEFINE(ACE_STRCASECMP_EQUIVALENT, [::_stricmp])
	fi
    fi
fi
])

# ACE_FUNC_STRNCASECMP
# + Defines ACE_LACKS_STRNCASECMP to 1 if platform lacks strncasecmp()
# + Defines ACE_STRNCASECMP_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
# + Defines ACE_LACKS_STRNCASECMP_PROTOTYPE to 1 if platform lacks
#   declaration for strncasecmp().
#---------------------------------------------------------------------------
AC_DEFUN([ACE_FUNC_STRNCASECMP],
[ACE_CHECK_LACKS_FUNCS(strncasecmp)
if test "$ac_cv_func_strncasecmp" = yes; then
    AC_CHECK_DECL([strncasecmp],
		  [],
		  [AC_DEFINE([ACE_LACKS_STRNCASECMP_PROTOTYPE], 1,
			     [Define to 1 if platform lacks a declaration for strncasecmp()])],
		  [
#if !defined(ACE_LACKS_STRINGS_H)
#include <strings.h>
#endif
#if !defined(ACE_LACKS_STRING_H)
#include <string.h>
#endif
		  ])
else
    AC_CHECK_FUNC(strnicmp)
    if test "$ac_cv_func_strnicmp" = yes; then
        AC_DEFINE(ACE_STRNCASECMP_EQUIVALENT, [::strnicmp],
		  [Define to function that is equivalent to strncasecmp()])
    else
	AC_CHECK_FUNC(_strnicmp)
	if test "$ac_cv_func__strnicmp" = yes; then
	    AC_DEFINE(ACE_STRNCASECMP_EQUIVALENT, [::_strnicmp])
	fi
    fi
fi
])

# ACE_FUNC_STRDUP
# + Defines ACE_LACKS_STRDUP to 1 if platform lacks strdup()
# + Defines ACE_STRDUP_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
# + Defines ACE_HAS_NONCONST_STRDUP if argument is char*. (TODO)
#---------------------------------------------------------------------------
AC_DEFUN([ACE_FUNC_STRDUP],
[ACE_CHECK_LACKS_FUNCS(strdup)
if test "$ac_cv_func_strdup" = no; then
    AC_CHECK_FUNC(_strdup)
    if test "$ac_cv_func__strdup" = yes; then
        AC_DEFINE(ACE_STRDUP_EQUIVALENT, [::_strdup],
		  [Define to function that is equivalent to strdup()])
    fi
fi
])

# ACE_FUNC_STRTOLL
# + Defines ACE_LACKS_STRTOLL to 1 if platform lacks strtoll()
# + Defines ACE_STRTOLL_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
# + Defines ACE_LACKS_STRTOLL_PROTOTYPE to 1 if platform lacks
#   declaration for strtoll().
AC_DEFUN([ACE_FUNC_STRTOLL],
[ACE_CHECK_LACKS_FUNCS(strtoll)
if test $ac_cv_func_strtoll = "no"; then
    AC_CHECK_FUNC(__strtoll)
    if test $ac_cv_func___strtoll = "yes"; then
        AC_DEFINE([ACE_STRTOLL_EQUIVALENT], [::__strtoll], 
                  [Define to function that is equivalent to strtoll()])
    else
        AC_CHECK_FUNC(_strtoi64)
        if test $ac_cv_func__strtoi64 = "yes"; then
	    AC_DEFINE([ACE_STRTOLL_EQUIVALENT], [::_strtoi64])
        fi
    fi 
else
    AC_CHECK_DECL([strtoll], 
                  [], 
		  [AC_DEFINE([ACE_LACKS_STRTOLL_PROTOTYPE], 1,
			     [Define to 1 if platform lacks a declaration for strtoll()])],
                  [#include <stdlib.h>])
fi
])

# ACE_FUNC_STRTOULL
# + Defines ACE_LACKS_STRTOULL to 1 if platform lacks strtoull()
# + Defines ACE_STRTOULL_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
# + Defines ACE_LACKS_STRTOULL_PROTOTYPE to 1 if platform lacks
#   declaration for strtoull().
#---------------------------------------------------------------------------
AC_DEFUN([ACE_FUNC_STRTOULL],
[ACE_CHECK_LACKS_FUNCS(strtoull)
if test $ac_cv_func_strtoull = "no"; then
    AC_CHECK_FUNC(__strtoull)
    if test $ac_cv_func___strtoull = "yes"; then
        AC_DEFINE([ACE_STRTOULL_EQUIVALENT], [::__strtoull], 
                  [Define to function that is equivalent to strtoull()])
    else
        AC_CHECK_FUNC(_strtoui64)
        if test $ac_cv_func__strtoui64 = "yes"; then
	    AC_DEFINE([ACE_STRTOULL_EQUIVALENT], [::_strtoui64])
        fi
    fi
else
    AC_CHECK_DECL([strtoull], 
                  [], 
		  [AC_DEFINE([ACE_LACKS_STRTOULL_PROTOTYPE], 1,
			     [Define to 1 if platform lacks a declaration for strtoull()])],
                  [#include <stdlib.h>])
fi
])

# ACE_FUNC_WCSCASECMP
# + Defines ACE_LACKS_WCSCASECMP to 1 if platform lacks wcscasecmp()
# + Defines ACE_WCSCASECMP_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
#---------------------------------------------------------------------------
AC_DEFUN([ACE_FUNC_WCSCASECMP],
[ACE_CHECK_LACKS_FUNCS(wcscasecmp)
if test "$ac_cv_func_wcscasecmp" = no; then
    AC_CHECK_FUNC(wcsicmp)
    if test "$ac_cv_func_wcsicmp" = yes; then
        AC_DEFINE(ACE_WCSCASECMP_EQUIVALENT, [::wcsicmp],
		  [Define to function that is equivalent to wcscasecmp()])
    else
	AC_CHECK_FUNC(_wcsicmp)
	if test "$ac_cv_func__wcsicmp" = yes; then
	    AC_DEFINE(ACE_WCSCASECMP_EQUIVALENT, [::_wcsicmp])
	fi
    fi
fi
])

# ACE_FUNC_WCSNCASECMP
# + Defines ACE_LACKS_WCSNCASECMP to 1 if platform lacks wcsncasecmp()
# + Defines ACE_WCSNCASECMP_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
#---------------------------------------------------------------------------
AC_DEFUN([ACE_FUNC_WCSNCASECMP],
[ACE_CHECK_LACKS_FUNCS(wcsncasecmp)
if test "$ac_cv_func_wcsncasecmp" = no; then
    AC_CHECK_FUNC(wcsnicmp)
    if test "$ac_cv_func_wcsnicmp" = yes; then
        AC_DEFINE(ACE_WCSNCASECMP_EQUIVALENT, [::wcsnicmp],
		  [Define to function that is equivalent to wcsncasecmp()])
    else
	AC_CHECK_FUNC(_wcsnicmp)
	if test "$ac_cv_func__wcsnicmp" = yes; then
	    AC_DEFINE(ACE_WCSNCASECMP_EQUIVALENT, [::_wcsnicmp])
	fi
    fi
fi
])

# ACE_FUNC_WCSDUP
# + Defines ACE_LACKS_WCSDUP to 1 if platform lacks wcsdup()
# + Defines ACE_WCSDUP_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
# + Defines ACE_HAS_NONCONST_WCSDUP if argument is char*. (TODO)
#---------------------------------------------------------------------------
AC_DEFUN([ACE_FUNC_WCSDUP],
[ACE_CHECK_LACKS_FUNCS(wcsdup)
if test "$ac_cv_func_wcsdup" = no; then
    AC_CHECK_FUNC(_wcsdup)
    if test "$ac_cv_func__wcsdup" = yes; then
        AC_DEFINE(ACE_WCSDUP_EQUIVALENT, [::_wcsdup],
		  [Define to function that is equivalent to wcsdup()])
    fi
fi
])

# ACE_FUNC_WCSTOLL
# + Defines ACE_LACKS_WCSTOLL to 1 if platform lacks wcstoll()
# + Defines ACE_WCSTOLL_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
# + Defines ACE_LACKS_WCSTOLL_PROTOTYPE to 1 if platform lacks
#   declaration for wcstoll().
AC_DEFUN([ACE_FUNC_WCSTOLL],
[ACE_CHECK_LACKS_FUNCS(wcstoll)
if test $ac_cv_func_wcstoll = "no"; then
    AC_CHECK_FUNC(__wcstoll)
    if test $ac_cv_func___wcstoll = "yes"; then
        AC_DEFINE([ACE_WCSTOLL_EQUIVALENT], [::__wcstoll], 
                  [Define to function that is equivalent to wcstoll()])
    else
	AC_CHECK_FUNC(_wcstoi64)
	if test $ac_cv_func__wcstoi64 = "yes"; then
	    AC_DEFINE([ACE_WCSTOLL_EQUIVALENT], [::_wcstoi64],
		      [Define to function that is equivalent to wcstoll()])
	fi
    fi
else
    AC_CHECK_DECL([wcstoll], 
	          [], 
		  [AC_DEFINE([ACE_LACKS_WCSTOLL_PROTOTYPE], 1,
			     [Define to 1 if platform lacks a declaration for wcstoll()])],
                  [#include <stdlib.h>
#include <wchar.h>])
fi
])

# ACE_FUNC_WCSTOULL
# + Defines ACE_LACKS_WCSTOULL to 1 if platform lacks wcstoull()
# + Defines ACE_WCSTOULL_EQUIVALENT to identifier name if platform
#   has a equivalent function that differs in name only.
# + Defines ACE_LACKS_WCSTOULL_PROTOTYPE to 1 if platform lacks
#   declaration for wcstoull().
AC_DEFUN([ACE_FUNC_WCSTOULL],
[ACE_CHECK_LACKS_FUNCS(wcstoull)
if test $ac_cv_func_wcstoull = "no"; then
    AC_CHECK_FUNC(__wcstoull)
    if test $ac_cv_func___wcstoull = "yes"; then
        AC_DEFINE([ACE_WCSTOULL_EQUIVALENT], [::__wcstoull], 
                  [Define to function that is equivalent to wcstoull()])
    else
	AC_CHECK_FUNC(_wcstoui64)
	if test $ac_cv_func__wcstoui64 = "yes"; then
	    AC_DEFINE([ACE_WCSTOULL_EQUIVALENT], [::_wcstoui64],
		      [Define to function that is equivalent to wcstoull()])
	fi
    fi
else
    AC_CHECK_DECL([wcstoull], 
                  [], 
		  [AC_DEFINE([ACE_LACKS_WCSTOULL_PROTOTYPE], 1,
			     [Define to 1 if platform lacks a declaration for wcstoull()])],
                  [#include <stdlib.h>
#include <wchar.h>])
fi
])

# ACE_CHECK_SYSINFO
#
# HP/UX, SVR4/POSIX and Linux have completely independent
# implementations of the # sysinfo() system / library call.
#
# The HP/UX syscall is undocumented.
#
# The SVR4 signature is:
#   #include <sys/systeminfo.h>
#   long sysinfo (int command, char *buf, long count)
#
# While the Linux signature is:
#   #include <sys/sysinfo.h>
#   int sysinfo (struct sysinfo* info);
#
# SVR4 (or at least Solaris) also has a sys/sysinfo.h header, so that
# cannot be used to distinguish between the two varients. As far as I
# know, Linux does not have a sys/systeminfo.h header, so that can.
# To further avoid false positives, small programs that use the two
# APIs are compiled as part of the feature tests.
#
# ACE uses the ACE_HAS_SYSV_SYSINFO feature test macro for the first
# and ACE_HAS_LINUX_SYSINFO for the second.
#
AC_DEFUN([ACE_CHECK_FUNC_SYSINFO],[
ACE_CHECK_HAS_HEADERS(sys/sysinfo.h sys/systeminfo.h)
AC_CHECK_FUNC(sysinfo)
if test "$ac_cv_func_sysinfo" = yes; then
  if test "$ac_cv_header_sys_systeminfo_h" = yes; then
     AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
		[#include <sys/systeminfo.h>],
		[char buf[256];
		sysinfo (SI_SYSNAME, buf, sizeof(buf));
		return 0;])],
	[AC_DEFINE([ACE_HAS_SYSV_SYSINFO], 1,
		[Define to 1 if system has SysV version of sysinfo().])])

  elif test "$ac_cv_header_sys_sysinfo_h" = yes; then
     AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[#include <sys/sysinfo.h>],
		[struct sysinfo s;
		sysinfo (&s);
		return 0;])],
        [AC_DEFINE([ACE_HAS_LINUX_SYSINFO], 1,
		[Define to 1 if system has Linux version of sysinfo().])])
  fi
fi
])
