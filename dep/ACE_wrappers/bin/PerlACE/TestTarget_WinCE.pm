#! /usr/bin/perl
package PerlACE::TestTarget_WinCE;

# ******************************************************************
# Description : Creates a PerlACE::WiNCE
# Author      : Johnny Willemsen
# Create Date : 29/20/2008
#          $Id: TestTarget_WinCE.pm 91813 2010-09-17 07:52:52Z johnnyw $
# ******************************************************************

# ******************************************************************
# Pragma Section
# ******************************************************************

use strict;

use PerlACE::TestTarget;
use PerlACE::ProcessVX;
use File::Copy;
use Cwd;
use English;

our @ISA = qw(PerlACE::TestTarget);

sub new
{
    my $proto = shift;
    my $config_name = shift;
    my $class = ref ($proto) || $proto;
    my $self = {};
    bless ($self, $class);
    $self->GetConfigSettings($config_name);
    my $targethost;
    my $env_name = $config_name.'_IPNAME';
    if (exists $ENV{$env_name}) {
        $targethost = $ENV{$env_name};
    }
    else {
        print STDERR "You must define target hostname/IP with $env_name\n";
        undef $self;
        return undef;
    }

    $env_name = $config_name.'_FS_ROOT';
    my $fsroot = '\network\temp\ACE\wince6';
    if (exists $ENV{$env_name}) {
        $fsroot = $ENV{$env_name};
    }
    else {
        print STDERR "Warning: no $env_name variable; falling back ",
                     "to $fsroot\n";
    }
    $self->{FSROOT} = $fsroot;

    $self->{REBOOT_CMD} = $ENV{"ACE_REBOOT_LVRT_CMD"};
    if (!defined $self->{REBOOT_CMD}) {
        $self->{REBOOT_CMD} = 'I_Need_A_Reboot_Command';
    }
    $self->{REBOOT_TIME} = $ENV{"ACE_LVRT_REBOOT_TIME"};
    if (!defined $self->{REBOOT_TIME}) {
        $self->{REBOOT_TIME} = 200;
    }

    $self->{REBOOT_TIME} = $ENV{"ACE_RUN_LVRT_REBOOT_TIME"};
    if (!defined $self->{REBOOT_TIME}) {
        $self->{REBOOT_TIME} = 200;
    }
    $self->{REBOOT_NEEDED} = undef;

    my $telnet_port = $ENV{'ACE_RUN_VX_TGT_TELNET_PORT'};
    my $telnet_host = $ENV{'ACE_RUN_VX_TGT_TELNET_HOST'};
    if (!defined $telnet_host)  {
      $telnet_host = $ENV{'ACE_RUN_VX_TGTHOST'};
    }
    if (!defined $telnet_port)  {
      $telnet_port = 23;
    }
    if (!defined $self->{HOST_ROOT})  {
      $self->{HOST_ROOT} = $self->{FSROOT};
    }

    $PerlACE::ProcessVX::ExeExt = '.exe';

    return $self;
}

# ******************************************************************
# Subroutine Section
# ******************************************************************

sub LocalFile {
    my $self = shift;
    my $file = shift;
    my $cwdrel = $file;
    my $prjroot = defined $ENV{"ACE_RUN_VX_PRJ_ROOT"} ? $ENV{"ACE_RUN_VX_PRJ_ROOT"}  : $ENV{"ACE_ROOT"};
    if (length ($cwdrel) > 0) {
        $cwdrel = File::Spec->abs2rel( cwd(), $prjroot );
    }
    else {
        $cwdrel = File::Spec->abs2rel( $cwdrel, $prjroot );
    }
    my $newfile = $self->{FSROOT} . "/" . $cwdrel . "/" . $file;
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print STDERR "WinCE LocalFile for $file is $newfile\n";
    }
    return $newfile;
}

sub AddLibPath ($) {
    my $self = shift;
    my $dir = shift;
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
        print STDERR "Adding libpath $dir\n";
    }
    PerlACE::add_lib_path ($dir);
}

sub CreateProcess {
  my $self = shift;
if ($OSNAME eq "MSWin32") {
  my $process = new PerlACE::ProcessVX ($self, @_);  return $process;
} else {
  my $process = new PerlACE::ProcessVX (@_, $self);  return $process;
}
}

# Need a reboot when this target is destroyed.
sub NeedReboot ($)
{
    my $self = shift;
    $self->{REBOOT_NEEDED} = 1;
}

# Reboot target
sub RebootNow ($)
{
    my $self = shift;
    $self->{REBOOT_NEEDED} = undef;
    print STDERR "Attempting to reboot target...\n";
    reboot ();
}

sub WaitForFileTimed ($)
{
    my $self = shift;
    my $file = shift;
    my $timeout = shift;
    my $cwdrel = $file;
    my $prjroot = defined $ENV{"ACE_RUN_VX_PRJ_ROOT"} ? $ENV{"ACE_RUN_VX_PRJ_ROOT"}  : $ENV{"ACE_ROOT"};
    if (length ($cwdrel) > 0) {
        $cwdrel = File::Spec->abs2rel( cwd(), $prjroot );
    }
    else {
        $cwdrel = File::Spec->abs2rel( $cwdrel, $prjroot );
    }
    my $newfile = $self->{HOST_ROOT} . "/" . $cwdrel . "/" . $file;
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print STDERR "WinCE waits for $newfile timeout $timeout\n";
    }
    return PerlACE::waitforfile_timed ($newfile, $timeout);
}

# Put file from a to b
sub PutFile ($)
{
    my $self = shift;
    my $src = shift;
    return 0;
}

sub DeleteFile ($)
{
    my $self = shift;
    my $file = shift;
    my $newfile = $self->LocalFile($file);
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print STDERR "delete $newfile\n";
    }
    unlink ("$newfile");
}

sub KillAll ($)
{
    my $self = shift;
    my $procmask = shift;
    PerlACE::ProcessVX::kill_all ($procmask, $self);
}

1;
