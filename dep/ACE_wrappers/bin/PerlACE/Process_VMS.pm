#! /usr/bin/perl
# $Id: Process_VMS.pm 89840 2010-04-12 09:36:32Z mcorino $

package PerlACE::Process;

use strict;
use POSIX "sys_wait_h";
use Cwd;
use File::Basename;
use Config;
use VmsProcess;

###############################################################################

### Chorus stuff

$PerlACE::Process::chorushostname = "localhost";
$PerlACE::Process::chorus = 0;

$PerlACE::Process::cwd = getcwd();

for(my $i = 0; $i <= $#ARGV; $i++) {
    if ($ARGV[$i] eq '-chorus') {
        if (defined $ARGV[$i + 1]) {
            $PerlACE::Process::chorus = 1;
            $PerlACE::Process::chorushostname = $ARGV[$1 + 1];
        }
        else {
            print STDERR "The -chorus option requires " .
                         "the hostname of the target\n";
            exit(1);
        }

        splice(@ARGV, $i, 2);
        # Don't break from the loop just in case there
        # is an accidental duplication of the -chorus option
    }
}

###############################################################################

###  Grab signal names

my @signame;

if (defined $Config{sig_name}) {
    my $i = 0;
    foreach my $name (split (' ', $Config{sig_name})) {
        $signame[$i] = $name;
        $i++;
    }
}
else {
    my $i;
    for ($i = 0; $i < 255; ++$i) {
        $signame[$i] = $i;
    }
}

###############################################################################

### Constructor and Destructor

sub new
{
    my $proto = shift;
    my $class = ref ($proto) || $proto;
    my $self = {};

    $self->{RUNNING} = 0;
    $self->{IGNOREEXESUBDIR} = 0;
    $self->{PROCESS} = undef;
    $self->{EXECUTABLE} = shift;
    $self->{ARGUMENTS} = shift;
    $self->{VALGRIND_CMD} = $ENV{"ACE_RUN_VALGRIND_CMD"};

    if (!defined $PerlACE::Process::WAIT_DELAY_FACTOR) {
         if (defined $self->{PURIFY_CMD}) {
           $PerlACE::Process::WAIT_DELAY_FACTOR = 10;
         }
         elsif (defined $self->{VALGRIND_CMD}) {
           $PerlACE::Process::WAIT_DELAY_FACTOR = 5;
         }
         else {
           $PerlACE::Process::WAIT_DELAY_FACTOR = 1;
        }
    }

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
}

###############################################################################

### Some Accessors

sub Executable
{
    my $self = shift;

    if (@_ != 0) {
        $self->{EXECUTABLE} = shift;
    }

    my $executable = $self->{EXECUTABLE};

    if ($self->{IGNOREEXESUBDIR}) {
        return $executable;
    }

    my $basename = basename ($executable);
    my $dirname = dirname ($executable). '/';
    if ($dirname != "") {
      $executable = $dirname.$PerlACE::Process::ExeSubDir.$basename;
    }
    else {
      $executable = $dirname.$basename;
    }

    if ( !-x $executable ) {
      if ( -x $executable.'.exe' ) {
        $executable = $executable.'.exe';
      }
    }

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

    my $commandline = $self->Executable ();

    if (defined $self->{ARGUMENTS}) {
        $commandline .= ' '.$self->{ARGUMENTS};
    }

    if ($PerlACE::Process::chorus == 1) {
        $commandline = "rsh "
                       . $PerlACE::Process::chorushostname
                       . " arun "
                       . $PerlACE::Process::cwd
                       . "/"
                       . $commandline;
    }

    return $commandline;
}

sub IgnoreExeSubDir
{
    my $self = shift;

    if (@_ != 0) {
        $self->{IGNOREEXESUBDIR} = shift;
    }

    return $self->{IGNOREEXESUBDIR};
}

###############################################################################

# Spawn the process and continue;

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

        if (!$PerlACE::Process::chorus && !-x $self->Executable ()) {
            print STDERR "ERROR: Cannot Spawn: <", $self->Executable (),
                         "> not executable\n";
            return -1;
        }
    }

    {
        $self->{PROCESS} = VmsProcess::Spawn $self->{EXECUTABLE}, $self->{ARGUMENTS};
        if ($self->{PROCESS}) {
            #parent here
            bless $self;
        }
        else {
            # weird fork error
            print STDERR "ERROR: Can't spawn <" . $self->CommandLine () . ">: $!\n";
        }
    }
    $self->{RUNNING} = 1;
    return 0;
}

sub WaitKill ($)
{
    my $self = shift;
    my $timeout = shift;
    my $status = $self->TimedWait ($timeout);

    if ($status == -1) {
        print STDERR "ERROR: $self->{EXECUTABLE} timedout\n";
        $self->Kill ();
    }

    $self->{RUNNING} = 0;

    return $status;
}


# Do a Spawn and immediately WaitKill

sub SpawnWaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    if ($self->Spawn () == -1) {
        return -1;
    }

    return $self->WaitKill ($timeout);
}

sub TerminateWaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    if ($self->{RUNNING}) {
        print STDERR "INFO: $self->{EXECUTABLE} being killed.\n";
        kill ('TERM', $self->{PROCESS});
    }

    return $self->WaitKill ($timeout);
}

# really only for internal use
sub check_return_value ($)
{
    my $self = shift;
    my $rc = shift;

    if ($rc == 0) {
        return 0;
    }
    elsif ($rc == 0xff00) {
        print STDERR "ERROR: <", $self->{EXECUTABLE},
                     "> failed: $!\n";
        return ($rc >> 8);
    }
    elsif (($rc & 0xff) == 0) {
        $rc >>= 8;
        return $rc;
    }

    my $dump = 0;

    if ($rc & 0x80) {
        $rc &= ~0x80;
        $dump = 1;
    }

    # check for ABRT, KILL or TERM
    if ($rc == 6 || $rc == 9 || $rc == 15) {
        return 0;
    }

    print STDERR "ERROR: <", $self->{EXECUTABLE},
                 "> exited with ";

    print STDERR "coredump from " if ($dump == 1);

    print STDERR "signal $rc : ", $signame[$rc], "\n";

    return 0;
}

sub Kill ()
{
    my $self = shift;

    if ($self->{RUNNING}) {
        kill ('KILL', $self->{PROCESS});
        waitpid ($self->{PROCESS}, 0);
        $self->check_return_value ($?);
    }

    $self->{RUNNING} = 0;
}

# Wait until a process exits.
# return -1 if the process is still alive.
sub Wait ($)
{
    my $self = shift;
    my $timeout = shift;
    if (!defined $timeout || $timeout < 0) {
      waitpid ($self->{PROCESS}, 0);
    } else {
      return TimedWait($self, $timeout);
    }

}

sub TimedWait ($)
{
    my $self = shift;
    my $timeout = shift;

    $timeout *= $PerlACE::Process::WAIT_DELAY_FACTOR;

    my $status;
    my $pid = VmsProcess::TimedWaitPid ($self->{PROCESS}, $timeout, $status);
    if ($pid > 0) {
      return $self->check_return_value ($status);
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
