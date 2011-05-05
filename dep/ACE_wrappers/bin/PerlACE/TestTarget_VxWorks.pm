#! /usr/bin/perl
package PerlACE::TestTarget_VxWorks;

# ******************************************************************
# Description : Creates a PerlACE::ProcessVX
# Author      : Chad Elliott
# Create Date : 6/20/2008
#          $Id: TestTarget_VxWorks.pm 91813 2010-09-17 07:52:52Z johnnyw $
# ******************************************************************

# ******************************************************************
# Pragma Section
# ******************************************************************

use strict;

use PerlACE::TestTarget;
use PerlACE::ProcessVX;
use Cwd;
use English;

our @ISA = qw(PerlACE::TestTarget);

# ******************************************************************
# Subroutine Section
# ******************************************************************

sub LocalFile {
    my($self, $file) = @_;
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
        print STDERR "LocalFile is $file\n";
    }
    return $file;
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
    my $process = new PerlACE::ProcessVX (@_);
    $process->{TARGET} = $self;
    return $process;
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
    return PerlACE::waitforfile_timed ($newfile, $timeout);
}

sub KillAll ($)
{
    my $self = shift;
    my $procmask = shift;
    PerlACE::ProcessVX::kill_all ($procmask, $self);
}

1;
