#! /usr/bin/perl
# $Id: Run_Test.pm 91813 2010-09-17 07:52:52Z johnnyw $

# This module contains a few miscellanous functions and some
# startup ARGV processing that is used by all tests.

use PerlACE::Process;
use PerlACE::ConfigList;

package PerlACE;
use File::Spec;
use Cwd;

$PerlACE::ACE_ROOT = $ENV{ACE_ROOT};
if(exists $ENV{TAO_ROOT}) {
    $PerlACE::TAO_ROOT = $ENV{TAO_ROOT};
} else {
    $PerlACE::TAO_ROOT = "$PerlACE::ACE_ROOT/TAO";
}
if(exists $ENV{CIAO_ROOT}) {
    $PerlACE::CIAO_ROOT = $ENV{CIAO_ROOT};
} else {
    $PerlACE::CIAO_ROOT = "$PerlACE::TAO_ROOT/CIAO";
}

my $config = new PerlACE::ConfigList;
$PerlACE::TestConfig = $config;

# load VxWorks Process helpers in case this is a VxWorks target build
$PerlACE::Static = $config->check_config("STATIC");
$PerlACE::VxWorks_Test = $config->check_config("VxWorks");
$PerlACE::VxWorks_RTP_Test = $config->check_config("VxWorks_RTP");
if ($PerlACE::VxWorks_Test or $PerlACE::VxWorks_RTP_Test) {
    require PerlACE::ProcessVX;
}

# Load LabVIEW RT Process helpers in case this is a LabVIEW RT target build.
$PerlACE::LabVIEW_RT_Test = $config->check_config("LabVIEW_RT");
if ($PerlACE::LabVIEW_RT_Test) {
    require PerlACE::ProcessLVRT;
}

$PerlACE::WinCE_Test = $config->check_config("WINCE");
if ($PerlACE::WinCE_Test) {
if ($OSNAME eq "MSWin32") {
    require PerlACE::ProcessWinCE;
} else {
    require PerlACE::ProcessWinCE_Unix;
}
}

# Figure out the svc.conf extension
$svcconf_ext = $ENV{"ACE_RUNTEST_SVCCONF_EXT"};
if (!defined $svcconf_ext) {
    $svcconf_ext = ".conf";
}

# Default timeout.  NSCORBA needs more time for process start up.
$wait_interval_for_process_creation = (($PerlACE::VxWorks_Test or $PerlACE::VxWorks_RTP_Test) ? 60 : 15);
if ($^O eq 'VMS') {
  $wait_interval_for_process_creation *= 3;
}
elsif ($^O eq 'nto') {
  ## QNX can be slow to start processes
  $wait_interval_for_process_creation += 5;
}

$wait_interval_for_process_shutdown = (($PerlACE::VxWorks_Test or $PerlACE::VxWorks_RTP_Test) ? 30 : 10);

# Turn on autoflush
$| = 1;

sub LocalFile ($)
{
    my $file = shift;

    my $newfile = getcwd () . '/' . $file;

    if ($^O eq "MSWin32") {
        $newfile =~ s/\//\\/g;
    }
    elsif ($^O eq 'cygwin') {
        chop($newfile = `/usr/bin/cygpath -w $newfile`);
        $newfile =~ s/\\/\\\\/g;
    }

    return $newfile;
}

sub rebase_path {
    my $path = shift;
    my $cur_root = shift;
    my $new_root = shift;
    $path = File::Spec->rel2abs ($path);
    $path = File::Spec->abs2rel ($path, $cur_root);
    return $new_root."/".$path;
}

sub VX_HostFile($)
{
    my $file = shift;
    return rebase_path ($file, $ENV{"ACE_ROOT"}, $ENV{"HOST_ROOT"});
}

# Returns a random port within the range of 10002 - 32767
sub random_port {
  return (int(rand($$)) % 22766) + 10002;
}

# Returns a unique id, uid for unix, last digit of IP for NT
sub uniqueid
{
  if ($^O eq "MSWin32")
  {
    my $uid = 1;

    open (IPNUM, "ipconfig|") || die "Can't run ipconfig: $!\n";

    while (<IPNUM>)
    {
      if (/Address/)
      {
        $uid = (split (/: (\d+)\.(\d+)\.(\d+)\.(\d+)/))[4];
      }
    }

    close IPNUM;

    return $uid;
  }
  else
  {
    return $>;
  }
}

# Waits until a file exists
sub waitforfile
{
  local($file) = @_;
  sleep 1 while (!(-e $file && -s $file));
}

sub waitforfile_timed
{
  my $file = shift;
  my $maxtime = shift;
  $maxtime *= (($PerlACE::VxWorks_Test || $PerlACE::VxWorks_RTP_Test) ? $PerlACE::ProcessVX::WAIT_DELAY_FACTOR : $PerlACE::Process::WAIT_DELAY_FACTOR);

  while ($maxtime-- != 0) {
    if (-e $file && -s $file) {
      return 0;
    }
    sleep 1;
  }
  return -1;
}

sub check_n_cleanup_files
{
  my $file = shift;
  my @flist = glob ($file);

  my $cntr = 0;
  my $nfile = scalar(@flist);

  if ($nfile != 0) {
    for (; $cntr < $nfile; $cntr++) {
      print STDERR "File <$flist[$cntr]> exists but should be cleaned up\n";
    }
    unlink @flist;
  }
}

sub generate_test_file
{
  my $file = shift;
  my $size = shift;

  while ( -e $file ) {
    $file = $file."X";
  }

  my $data = "abcdefghijklmnopqrstuvwxyz";
  $data = $data.uc($data)."0123456789";

  open( INPUT, "> $file" ) || die( "can't create input file: $file" );
  for($i=62; $i < $size ; $i += 62 ) {
    print INPUT $data;
  }
  $i -= 62;
  if ($i < $size) {
    print INPUT substr($data, 0, $size-$i);
  }
  close(INPUT);

  return $file;
}

sub is_labview_rt_test()
{
    return ($PerlACE::LabVIEW_RT_Test);
}

sub is_vxworks_test()
{
    return ($PerlACE::VxWorks_Test || $PerlACE::VxWorks_RTP_Test);
}

sub is_vxworks_rtp_test()
{
    return ($PerlACE::VxWorks_RTP_Test);
}

sub concat_path {
    my $pathlist   = shift;
    my $path       = shift;
    if ((!defined $pathlist) || $pathlist =~ /^\s*$/) {
        return $path;
    } else {
        return $pathlist . ($^O eq 'MSWin32' ? ';' : ':') . $path;
    }
}

sub add_path {
    my $name   = shift;
    my $value  = shift;
    $ENV{$name} = concat_path ($ENV{$name}, $value);
}

sub add_lib_path {
    my($value) = shift;

    # Set the library path supporting various platforms.
    foreach my $env ('PATH', 'DYLD_LIBRARY_PATH', 'LD_LIBRARY_PATH',
                     'SHLIB_PATH') {
      add_path($env, $value);
      if (grep(($_ eq 'ARCH'), @PerlACE::ConfigList::Configs)) {
        add_path($env, $value . '/' . $PerlACE::Process::ExeSubDir);
      }
    }

    if (defined $ENV{"HOST_ROOT"}) {
        add_path('PATH', VX_HostFile ($value));
        add_path('LD_LIBRARY_PATH', VX_HostFile ($value));
        add_path('LIBPATH', VX_HostFile ($value));
        add_path('SHLIB_PATH', VX_HostFile ($value));
    }
}

sub check_privilege_group {
  if ($^O eq 'hpux') {
    my($access) = 'RTSCHED';
    my($status) = 0;
    my($getprivgrp) = '/bin/getprivgrp';

    if (-x $getprivgrp) {
      if (open(GPG, "$getprivgrp |")) {
        while(<GPG>) {
          if (index($_, $access) >= 0) {
            $status = 1;
          }
        }
        close(GPG);
      }
    }

    if (!$status) {
      print STDERR "WARNING: You must have $access privileges to run this test.\n",
                   "         Run \"man 1m setprivgrp\" for more information.\n";
      exit(0);
    }
  }
}

# waits until it finds a matching regular expression in a file
#  escape metacharacters in the text to wait for
sub waitforfileoutput {
  my $file = shift;
  my $waittext = shift;

  if (-e $file && -s $file) {
    open (DATA, $file);
    while (my $line = <DATA>) {
      if ($line =~ /($waittext)/) {
        close(DATA);
        return 0;
      }
    }
    close(DATA);
  }
  sleep 1;
}

sub waitforfileoutput_timed {
  my $file = shift;
  my $waittext = shift;
  my $maxtime = shift;

  $maxtime *= (($PerlACE::VxWorks_Test || $PerlACE::VxWorks_RTP_Test) ? $PerlACE::ProcessVX::WAIT_DELAY_FACTOR : $PerlACE::Process::WAIT_DELAY_FACTOR);

  while ($maxtime-- != 0) {
    if (-e $file && -s $file) {
      open (DATA, $file);
      while (my $line = <DATA>) {
        if ($line =~ /($waittext)/) {
          close(DATA);
          return 0;
        }
      }
      close(DATA);
    }
    sleep 1;
  }
  return -1;
}

sub GetArchDir {
  my $dir = shift;
  if (grep(($_ eq 'ARCH'), @PerlACE::ConfigList::Configs)) {
    return $dir . $PerlACE::Process::ExeSubDir;
  }
  return $dir;
}

# Add PWD to the load library path
add_lib_path ('.');

$sleeptime = 5;

1;
