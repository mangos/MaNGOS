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
         invariant

invariant_SOURCES = invariant.cpp
invariant_LDADD = \
  $(top_builddir)/netsvcs/lib/libnetsvcs.la \
  $(top_builddir)/ace/$(ACE_BUILDDIR)/ace/libACE.la

