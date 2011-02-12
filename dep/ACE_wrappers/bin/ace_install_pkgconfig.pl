eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}'
    & eval 'exec perl -w -S $0 $argv:q'
    if 0;
# ********************************************************************
# $Id: ace_install_pkgconfig.pl 91974 2010-09-23 16:17:42Z mitza $
# ace_install_pkgconfig.pl - Creates *.pc files for pkg-config in the
#                            installed location, based on the *.pc.in
#                            files from the source tree, with @foo@
#                            variables replaced with their values.
#                            Called from the MPC-generated makefiles.
# ********************************************************************

use strict;
use Getopt::Long;

my ($prefix, $libdir, $libs, $version, %custom);
GetOptions('prefix=s' => \$prefix, 'libdir=s' => \$libdir, 'libs=s' => \$libs,
           'version=s' => \$version, 'custom=s' => \%custom);

my %subs = ('LIBS' => $libs, 'VERSION' => $version, 'exec_prefix' => $prefix,
            'prefix' => $prefix, 'includedir' => "$prefix/include",
            'libdir' => "$prefix/$libdir");

for my $k (keys %custom) {
  $subs{$k} = $custom{$k};
}

my $pcdir = "$prefix/$libdir/pkgconfig";
if (scalar @ARGV && ! -d $pcdir) {
  mkdir($pcdir, 0755);
}

for my $file (@ARGV) {
  open IN, $file;
  my $pcfile = $file;
  $pcfile =~ s/\.in$//;
  open OUT, ">$pcdir/$pcfile";
  while (<IN>) {
    s/@(\w+)@/exists $subs{$1} ? $subs{$1} : $&/ge;
    print OUT $_;
  }
  close OUT;
  close IN;
}

