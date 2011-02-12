##----------------------------------------------------------------------------
##       $Id: Makefile.am 80826 2008-03-04 14:51:23Z wotte $
##
##       Makefile for repeating token client application
##----------------------------------------------------------------------------

##
##  Process this file with automake to create Makefile.in
##

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)

noinst_PROGRAMS = \
  test_mutex

test_mutex_SOURCES = test_mutex.cpp
test_mutex_LDADD = \
  $(top_builddir)/netsvcs/lib/libnetsvcs.la \
  $(top_builddir)/ace/$(ACE_BUILDDIR)/ace/libACE.la


