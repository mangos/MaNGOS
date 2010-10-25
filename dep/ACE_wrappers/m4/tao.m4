dnl -------------------------------------------------------------------------
dnl       $Id: tao.m4 88990 2010-02-15 09:20:27Z johnnyw $
dnl
dnl       tao.m4
dnl
dnl       ACE M4 include file which contains TAO specific M4 macros
dnl       for enabling/disabling certain TAO features.
dnl
dnl -------------------------------------------------------------------------

dnl  Copyright (C) 1998, 1999, 2000, 2002  Ossama Othman
dnl
dnl  All Rights Reserved
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the current ACE distribution terms.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


AC_DEFUN([TAO_ENABLE_MINIMUM_CORBA],
[AC_ARG_ENABLE([minimum-corba],
              AS_HELP_STRING([--enable-minimum-corba],
		             [build TAO with minimum corba support [[[no]]]]),
[
case "${enableval}" in
  yes)
    tao_user_enable_minimum_corba=yes
    ;;
  no)
    tao_user_enable_minimum_corba=no
    ;;
  *)
    AC_MSG_ERROR([bad value ${enableval} for --enable-minimum-corba])
    ;;
esac
],[
tao_user_enable_minimum_corba=no
])
AM_CONDITIONAL([BUILD_MINIMUM_CORBA], 
	       [test X$tao_user_enable_minimum_corba = Xyes])
])

AC_DEFUN([TAO_ENABLE_EXAMPLES],
[AC_ARG_ENABLE([tao-examples],
               AS_HELP_STRING([--enable-tao-examples],
                              [build TAO examples [[[yes]]]]),
[
case "${enableval}" in
  yes)
    tao_build_examples=yes
    ;;
  no)
    tao_build_examples=no
    ;;
  *)
    AC_MSG_ERROR([bad value ${enableval} for --enable-tao-examples])
    ;;
esac
],[
tao_build_examples=yes
])
AM_CONDITIONAL([BUILD_EXAMPLES], [test X$tao_build_examples = Xyes])
])

AC_DEFUN([TAO_ENABLE_TESTS],
[AC_ARG_ENABLE([tao-tests],
               AS_HELP_STRING([--enable-tao-tests],
			      [build TAO tests [[[yes]]]]),
[
case "${enableval}" in
  yes)
    tao_build_tests=yes
    ;;
  no)
    tao_build_tests=no
    ;;
  *)
    AC_MSG_ERROR([bad value ${enableval} for --enable-tao-tests])
    ;;
esac
],[
tao_build_tests=yes
])
AM_CONDITIONAL([BUILD_TESTS], [test X$tao_build_tests = Xyes])
])
