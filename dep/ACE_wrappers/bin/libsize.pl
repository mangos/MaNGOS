eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: libsize.pl 80826 2008-03-04 14:51:23Z wotte $
#
# Provides size breakdown of ACE, TAO, or orbsvcs libs.
#
# Assumes (or builds) the lib with debug=0.  Allows other make args,
# such as -j 4, to be passed on the command line.

$usage =
  "$0 [-h, for html output] [-s, for shared libs] [-v] [make arguments]\n";

####
#### Configuration parameters.
####
$build_args =
  'debug=0 optimize=1 static_libs_only=1 DEFFLAGS=-DACE_USE_RCSID=0';
$ACE_COMPONENTS =
  'OS Utils Logging Threads Demux Connection Sockets IPC Svcconf ' .
    'Streams Memory Token Other';
$TAO_COMPONENTS =
  'POA Pluggable_Protocols Default_Resources Interpretive_Marshaling ' .
    'IDL_Compiler ORB_Core Dynamic_Any';
$ORBSVCS_COMPONENTS =
  'Naming ImplRepo Time Concurrency Property Trader LifeCycle Sched ' .
    'Event CosEvent Event2 AV';


#### The following are only used for VxWorks libraries, and
#### only if the corresponding environment variable isn't set.
$default_toolenv = '386';
$default_wind_base = '/project/doc/pkg/wind';
$default_host_type = 'sun4-solaris2';

#### Use gmake if it's on the user's PATH, otherwise use make.  Use
#### sh -c to avoid warning if gmake isn't found.
$make =
  system ("sh -c \"gmake --version\" > /dev/null 2>&1")  ?  'make'  :  'gmake';

$ACE_ROOT = $ENV{'ACE_ROOT'}  ||
  die "$0: ACE_ROOT was not set!\n";


$html = $verbose = 0;
$lib_extension = 'a';

####
#### Process command line args.
####
while ($#ARGV >= $[  &&  $ARGV[0] =~ /^-/) {
  if ($ARGV[0] eq '-h') {
    $html = 1;
    chop ($sysname = `uname -s`);
    chop ($sysrev = `uname -r`);
    shift;
  } elsif ($ARGV[0] eq '-s') {
    $lib_extension = 'so';
    $build_args =~ s/ static_libs_only=1//;
    shift;
  } elsif ($ARGV[0] eq '-v') {
    $verbose = 1;
    shift;
  } elsif ($ARGV[0] eq '-?') {
    print "$usage";
    exit;
  } else {
    #### Pass remaining args to make.
  }
}

$make_args = join (' ', @ARGV) . $build_args;

chop ($pwd = `pwd`);

if ($pwd =~ m%/ace$%) {
  #### libACE
  $COMPONENTS = "$ACE_COMPONENTS";
  $LIB_COMPONENTS = 'ACE_COMPONENTS';
  $libname = 'ACE';
} elsif ($pwd =~ m%/tao$%) {
  $COMPONENTS = "$TAO_COMPONENTS";
  $LIB_COMPONENTS = 'TAO_COMPONENTS';
  $libname = 'TAO';
} elsif ($pwd =~ m%/orbsvcs/orbsvcs$%) {
  $COMPONENTS = "$ORBSVCS_COMPONENTS";
  $LIB_COMPONENTS = 'TAO_ORBSVCS';
  $libname = 'orbsvcs';
} else {
  die "$0: unsupported directory; $pwd\n";
}

$lib = "lib${libname}.$lib_extension";


####
#### Select the size command based on ACE_ROOT setting.
####
if ($ACE_ROOT =~ /vxworks/) {
  $TOOLENV = $ENV{'TOOLENV'}  ||  $default_toolenv;
  $WIND_BASE = $ENV{'WIND_BASE'}  ||  $default_wind_base;
  $WIND_HOST_TYPE = $ENV{'WIND_HOST_TYPE'}  ||  $default_host_type;
  $size = "$WIND_BASE/host/$WIND_HOST_TYPE/bin/size$TOOLENV";
} elsif ($ACE_ROOT =~ /lynx-ppc/) {
  $size = '/usr/lynx/3.0.0/ppc/cdk/sunos-xcoff-ppc/bin/size';
} elsif ($ACE_ROOT =~ /lynx/) {
  $size = '/usr/lynx/3.0.0/x86/cdk/sunos-coff-x86/bin/size';
} elsif ($ACE_ROOT =~ /chorus/) {
  $size = '/project/doc/mvme/green68k/gnu/bin/size';
} else {
  $size = 'size';
}


####
#### Measure the size of the entire library.
####
$sizeTotal = build_lib ("$LIB_COMPONENTS=\"$COMPONENTS\"");
$components = "   <th>Platform\n    <th>Component\n    <th>Total";
$componentSize = "   <th>Size, bytes\n    <td align=center>$sizeTotal";
$componentPercentage =
  "   <th>Percentage of<br>total size\n    <td align=center>100";
print "Total $sizeTotal (100)\n" unless $html;


####
#### Measure the size of each library component.
####
foreach my $i (split (' ', $COMPONENTS)) {
  $sizeLib = build_lib ("$LIB_COMPONENTS=\"$i\"");
  $components .= "\n    <th>$i";
  $componentSize .= "\n    <td align=center>$sizeLib";
  $thisPercentage = percentage ($sizeLib, $sizeTotal);
  $componentPercentage .= "\n    <td align=center>$thisPercentage";
  print "$i $sizeLib ($thisPercentage)\n" unless $html;
}

####
#### Produce HTML output, if requested.
####
if ($html) {
  print '<center><table cellpadding=4 border=4>' . "\n";
  print '  <tr>' . "\n";
  print "$echoArgs $components\n";
  print '  <tr>' . "\n";
  print "    <th rowspan=2>$sysname $sysrev $ACE_ROOT\n";
  print "$echoArgs $componentSize\n";
  print '  <tr>' . "\n";
  print "$echoArgs $componentPercentage\n";
  print '</table></center><p>' . "\n";
}


####
#### Build library with componnents specified in argument.
####
sub build_lib ()
{
  my ($lib_components) = @_;

  unlink "$lib";

  print "$make $make_args $lib_components\n" if $verbose;

  system ("$make $make_args $lib_components >> make.log 2>&1")  &&
    die "$0: command failed; $make $make_args $lib_components\n";

  my $libSize = 0;

  open (SIZE, "$size $lib |")  ||
    die "$0: unable to open $size\n";
  while (<SIZE>) {
    my (@field) = split;
    $libSize += $field[3] if $field[3] =~ /\d/;  #### Skip size header line.
  }
  close (SIZE);

  $libSize;
}


####
#### Return percentage of first argument as fraction of second.
#### Returns a string with two-decimal place precision.
####
sub percentage ()
{
  my ($size, $total) = @_;

  sprintf ("%.2f", $size * 100 / $total);
}
