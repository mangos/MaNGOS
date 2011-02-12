#! /usr/bin/perl
eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}'
    & eval 'exec perl -w -S $0 $argv:q'
    if 0;

# ******************************************************************
#      Author: Chad Elliott
#        Date: 6/17/2002
#         $Id: mwc.pl 89398 2010-03-08 15:38:06Z mitza $
# ******************************************************************

# ******************************************************************
# Pragma Section
# ******************************************************************

require 5.006;

use strict;
use FindBin;
use File::Spec;
use File::Basename;

my($basePath) = (defined $FindBin::RealBin ? $FindBin::RealBin :
                                             File::Spec->rel2abs(dirname($0)));
if ($^O eq 'VMS') {
  $basePath = File::Spec->rel2abs(dirname($0)) if ($basePath eq '');
  $basePath = VMS::Filespec::unixify($basePath);
}
$basePath .= '/MakeProjectCreator';

my($mpcroot) = $ENV{MPC_ROOT};
my($mpcpath) = (defined $mpcroot ? $mpcroot :
                                   dirname(dirname($basePath)) . '/MPC');
unshift(@INC, $mpcpath . '/modules');

if (defined $mpcroot) {
  print STDERR "MPC_ROOT was set to $mpcroot.\n";
}

if (! -d "$mpcpath/modules") {
  print STDERR "ERROR: Unable to find the MPC modules in $mpcpath.\n";
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
# Subroutine Section
# ************************************************************

sub getBasePath {
  return $mpcpath;
}

# ************************************************************
# Main Section
# ************************************************************

my($driver) = new Driver($basePath, Driver::workspaces());
exit($driver->run(@ARGV));
