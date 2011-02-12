#! /usr/bin/perl
eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}'
    & eval 'exec perl -w -S $0 $argv:q'
    if 0;

# ************************************************************
# Description   : Generate dependencies for GNU Make and NMake.
# Author        : Chad Elliott
# Create Date   : 5/06/2002
#            $Id: depgen.pl 83992 2008-12-09 04:42:49Z johnnyw $
# ************************************************************

# ************************************************************
# Pragma Section
# ************************************************************

use strict;
use FindBin;
use File::Spec;
use File::Basename;

my($basePath) = $FindBin::RealBin;
if ($^O eq 'VMS') {
  $basePath = File::Spec->rel2abs(dirname($0)) if ($basePath eq '');
  $basePath = VMS::Filespec::unixify($basePath);
}
unshift(@INC, $basePath . '/DependencyGenerator');

my($mpcroot) = $ENV{MPC_ROOT};
my($mpcpath) = (defined $mpcroot ? $mpcroot :
                                   dirname($basePath) . '/MPC');
unshift(@INC, $mpcpath . '/modules/Depgen', $mpcpath . '/modules');

if (! -d "$mpcpath/modules/Depgen") {
  print STDERR "ERROR: Unable to find the MPC DependencyGenerator ",
               "modules in $mpcpath.\n";
  if (defined $mpcroot) {
    print STDERR "Your MPC_ROOT environment variable does not point to a ",
                 "valid MPC location.\n";
  }
  else {
    print STDERR "You can set the MPC_ROOT environment variable to the ",
                 "location of MPC.\n";
  }
  exit(255);
}

require Driver;

# ************************************************************
# Main Section
# ************************************************************

my($driver) = new Driver('UNIX=gnu',
                         'automatic=ACE_ROOT,TAO_ROOT,CIAO_ROOT,' .
                         'DDS_ROOT,ACE_PLATFORM_CONFIG');
exit($driver->run(\@ARGV));
