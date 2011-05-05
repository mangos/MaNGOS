#! /usr/bin/perl
# $Id: ProcessLVRT.pm 89840 2010-04-12 09:36:32Z mcorino $
#
# ProcessLVRT - how to run ACE+TAO tests on a LabVIEW RT target.
# Tests on LabVIEW RT are not executables - LabVIEW RT can't start plain
# executables; the tests are built/packaged as DLLs and loaded and executed
# from a DLL loaded at LabVIEW RT boot time. The DLL on the target listens
# on a TCP port (8888 by default) for connections from the host. Host requests
# actions using text commands to the target.
#
# NOTE: This module requires the Net-Telnet Perl module.
#
# We can FTP files to and from the LabVIEW target, but there's no NFS or
# SMB shares.

package PerlACE::ProcessLVRT;
our @ISA = "PerlACE::Process";

use strict;
use Cwd;
use English;
use File::Basename;
use Net::FTP;
use Net::Telnet;
use POSIX qw(:time_h);

$PerlACE::ProcessLVRT::ExeSubDir = './';

### Check for -ExeSubDir commands, store the last one
my @new_argv = ();

for(my $i = 0; $i <= $#ARGV; ++$i) {
    if ($ARGV[$i] eq '-ExeSubDir') {
        if (defined $ARGV[$i + 1]) {
            $PerlACE::ProcessLVRT::ExeSubDir = $ARGV[++$i].'/';
        }
        else {
            print STDERR "You must pass a directory with ExeSubDir\n";
            exit(1);
        }
    }
    else {
        push @new_argv, $ARGV[$i];
    }
}
@ARGV = @new_argv;

### Constructor and Destructor

sub new
{
    my $proto = shift;
    my $class = ref ($proto) || $proto;
    my $self = {};

    $self->{TARGET} = shift;
    $self->{EXECUTABLE} = shift;
    $self->{ARGUMENTS} = shift;
    $self->{RUNNING} = 0;
    $self->{IGNOREEXESUBDIR} = 1;

    bless ($self, $class);
    return $self;
}

sub DESTROY
{
    my $self = shift;

    if ($self->{RUNNING} == 1) {
        print STDERR "ERROR: <", $self->{EXECUTABLE},
                     "> still running upon object destruction\n";
        $self->Kill ();
    }
    if (defined $self->{TELNET}) {
        $self->{TELNET}->close();
        $self->{TELNET} = undef;
    }
}

###############################################################################

# Adjust executable name for LabVIEW RT testing needs. These tests are DLLs.

sub Executable
{
    my $self = shift;

    if (@_ != 0) {
        $self->{EXECUTABLE} = shift;
    }

    my $executable = $self->{EXECUTABLE};

    my $basename = basename ($executable);
    my $dirname = dirname ($executable). '/';
    my $subdir = $PerlACE::ProcessLVRT::ExeSubDir;
    if (defined $self->{TARGET}) {
        $subdir = $self->{TARGET}->ExeSubDir();
    }
    $executable = $dirname.$subdir.$basename.".DLL";
    $executable =~ s/\//\\/g; # / <- # color coding issue in devenv

    return $executable;
}

sub Arguments
{
    my $self = shift;

    if (@_ != 0) {
        $self->{ARGUMENTS} = shift;
    }

    return $self->{ARGUMENTS};
}

sub CommandLine ()
{
    my $self = shift;

    my $commandline = "run " . basename($self->Executable(), ".dll");
    if (defined $self->{ARGUMENTS}) {
        $commandline .= ' '.$self->{ARGUMENTS};
    }

    return $commandline;
}

###############################################################################

# Spawn the process and continue.

sub Spawn ()
{
    my $self = shift;

    if ($self->{RUNNING} == 1) {
        print STDERR "ERROR: Cannot Spawn: <", $self->Executable (),
                     "> already running\n";
        return -1;
    }

    if (!defined $self->{EXECUTABLE}) {
        print STDERR "ERROR: Cannot Spawn: No executable specified\n";
        return -1;
    }

    if ($self->{IGNOREEXESUBDIR} == 0) {
        if (!-f $self->Executable ()) {
            print STDERR "ERROR: Cannot Spawn: <", $self->Executable (),
                         "> not found\n";
            return -1;
        }
    }

    my $status = 0;

    my $program = $self->Executable ();
    my $cwdrel = dirname ($program);
    my $target_ace_root = $self->{TARGET}->ACE_ROOT();
    if (length ($cwdrel) > 0) {
        $cwdrel = File::Spec->abs2rel(cwd(), $target_ace_root);
    }
    else {
        $cwdrel = File::Spec->abs2rel($cwdrel, $target_ace_root);
    }

    $self->{TARGET}->{FTP}->cwd($self->{TARGET}->{FSROOT});
    $self->{TARGET}->{FTP}->binary();
    $self->{TARGET}->{FTP}->put($program);

    my $targethost = $self->{TARGET}->{IPNAME};
    my $targetport = $self->{TARGET}->{CTLPORT};
    $self->{TELNET} = new Net::Telnet(Errmode => 'return');
    if (!$self->{TELNET}->open(Host => $targethost, Port => $targetport)) {
        print STDERR "ERROR: target $targethost:$targetport: ",
                      $self->{TELNET}->errmsg(), "\n";
        $self->{TELNET} = undef;
        $self->{TARGET}->NeedReboot;
        $self->{TARGET}->{FTP}->delete($program);
        return -1;
    }
    my $cmdline = $self->CommandLine();
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print "-> $cmdline\n";
    }
    $self->{TELNET}->print("$cmdline");
    my $reply;
    $reply = $self->{TELNET}->getline();
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print "<- $reply\n";
    }
    if ($reply eq "OK\n") {
        $self->{RUNNING} = 1;
        return 0;
    }
    print STDERR "ERROR: can't $cmdline: " . $reply . "\n";
    $self->{TARGET}->{FTP}->delete($program);
    # Not unless can't get the response.  $self->{TARGET}->NeedReboot;
    return -1;
}


# Wait for the process to exit or kill after a time period

sub WaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    my $status = $self->TimedWait ($timeout);

    $self->{RUNNING} = 0;

    # If the test timed out, the target is probably toast. Don't bother
    # trying to get the log file until after rebooting and resetting FTP.
    if ($status == -1) {
        print STDERR "ERROR: $self->{EXECUTABLE} timedout\n";
        $self->Kill();
    }

    # Now get the log file from the test, and delete the test from
    # the target. The FTP session should still be open.
    my $program = $self->Executable ();
    my $logname = basename($program,".dll") . ".log";
    my $target_log_path = $self->{TARGET}->{FSROOT} . "\\log\\" . $logname;
    $program = basename($program);
    $self->{TARGET}->{FTP}->delete($program);
    $self->{TARGET}->{FTP}->get($target_log_path,"log\\$logname");
    $self->{TARGET}->{FTP}->delete($target_log_path);

    return $status;
}


# Do a Spawn and immediately WaitKill

sub SpawnWaitKill ($)
{
    my $self = shift;
    my $timeout = shift;
    my $status = $self->Spawn ();
    if ($status == 0) {
        $status = $self->WaitKill ($timeout);
    }

    return $status;
}

sub TerminateWaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    if ($self->{RUNNING}) {
        print STDERR "INFO: $self->{EXECUTABLE} being killed.\n";
        $self->Kill();
    }

    return $self->WaitKill ($timeout);
}

sub Kill ()
{
    my $self = shift;

    if ($self->{RUNNING}) {
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
            print "-> kill\n";
        }
        $self->{TELNET}->print("kill");
        # Just wait for any reply; don't care what it is.
        my $reply = $self->{TELNET}->getline();
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
            print "<- $reply\n";
        }
    }

    $self->{RUNNING} = 0;
    # Trying to kill a LabVIEW RT thread and recover is probably futile. Just
    # reboot and reset the FTP connection.
    if (defined $self->{TELNET}) {
        $self->{TELNET}->close();
        $self->{TELNET} = undef;
    }
    $self->{TARGET}->RebootReset;
}

# Wait until a process exits.
# return -1 if the process is still alive.
sub Wait ($)
{
    my $self = shift;
    my $timeout = shift;
    if (!defined $timeout || $timeout < 0) {
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
            print "-> wait\n";
        }
        $self->{TELNET}->print("wait");
        my $reply = $self->{TELNET}->getline(Timeout => 300);
        $self->{RUNNING} = 0;
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
          print "<- $reply\n";
        }
        return 0+ $reply;
    } else {
        return TimedWait($self, $timeout);
    }

}

sub TimedWait ($)
{
    my $self = shift;
    my $timeout = shift;
    my $reply;
    if (!$self->{RUNNING}) {
      return -1;
    }

CHECK:
    while ($timeout > 0) {
        $self->{TELNET}->print ("status");
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
          print "-> status\n";
        }
        $reply = $self->{TELNET}->getline(Timeout => $timeout);
        if (!defined $reply) {
            last CHECK;
        }
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
          print "<- $reply\n";
        }
        if ($reply =~ /^RUNNING/) {
            sleep 2;
            $timeout -= 2;
            next CHECK;
        }
        # Have a status; return it.
        $self->{RUNNING} = 0;
        return 0+ $reply;
    }

    return -1;
}

###

sub kill_all
{
  my $procmask = shift;
  my $target = shift;
  ## NOT IMPLEMENTED YET
}

1;
