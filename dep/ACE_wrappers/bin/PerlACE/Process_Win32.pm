#! /usr/bin/perl
# $Id: Process_Win32.pm 91813 2010-09-17 07:52:52Z johnnyw $

use PerlACE::Run_Test;

package PerlACE::Process;

use strict;
use Win32::Process;
use File::Basename;
use Cwd;

###############################################################################

# This is what GetExitCode will return if the process is still running.
my $STILL_ACTIVE = 259;

###############################################################################

### Constructor and Destructor

#
# Hack in purify support thru 2 environment variables:
#   ACE_RUN_PURIFY_CMD: complete path to purify executable
#   ACE_RUNTEST_DELAY: wait delay factor, default to 10 if
#                      ACE_RUN_PURIFY_CMD is defined, or 1 if
#                      ACE_RUN_PURIFY_CMD is not defined.
# ** Notice that when ACE_RUN_PURIFY_CMD is define, PerlACE::Process
#    reports the return status of *purify*, not the process being purified.
#
# Also hack in the ability to run the test on a WinCE device using the
# ACE_WINCE_TEST_CONTROLLER environment variable. If set, it specifies a
# controlling program to use for setting up and executing the test.
# Further setup can be specialized depending on the value of the variable.

sub new
{
    my $proto = shift;
    my $class = ref ($proto) || $proto;
    my $self = {};

    $self->{EXECUTABLE} = shift;
    $self->{ARGUMENTS} = shift;
    $self->{TARGET} = undef;
    $self->{RUNNING} = 0;
    $self->{IGNOREEXESUBDIR} = 0;
    $self->{IGNOREHOSTROOT} = 0;
    $self->{PROCESS} = undef;
    $self->{PURIFY_CMD} = $ENV{"ACE_RUN_PURIFY_CMD"};
    $self->{PURIFY_OPT} = $ENV{"ACE_RUN_PURIFY_OPT"};
    if (!defined $PerlACE::Process::WAIT_DELAY_FACTOR) {
        if (defined $self->{PURIFY_CMD}) {
            $PerlACE::Process::WAIT_DELAY_FACTOR = 10;
        }
        else {
            $PerlACE::Process::WAIT_DELAY_FACTOR = 1;
        }
    }
    $self->{WINCE_CTL} = $ENV{"ACE_WINCE_TEST_CONTROLLER"};

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

sub Normalize_Executable_Name($)
{
    my $self = shift;
    my $executable = shift;
    my $basename = basename ($executable);
    my $dirname = dirname ($executable). '/';
    my $subdir;
    if (defined $self->{TARGET}) {
        $subdir = $self->{TARGET}->ExeSubDir();
    }
    else {
        $subdir = $PerlACE::Process::ExeSubDir;
    }
    $executable = $dirname.$subdir.$basename.".EXE";

    ## Installed executables do not conform to the ExeSubDir
    if (! -x $executable && -x $dirname.$basename.'.EXE') {
      $executable = $dirname.$basename.'.EXE';
    }

    $executable =~ s/\//\\/g; # / <- # color coding issue in devenv

    return $executable;
}


sub Executable
{
    my $self = shift;

    if (@_ != 0) {
        $self->{EXECUTABLE} = shift;
    }

    my $executable = $self->{EXECUTABLE};
    # If the target's config has a different ACE_ROOT, rebase the executable
    # from $ACE_ROOT to the target's root.
    if (defined $self->{TARGET} &&
        $self->{TARGET}->ACE_ROOT() ne $ENV{"ACE_ROOT"}) {
        $executable = File::Spec->rel2abs($executable);
        $executable = File::Spec->abs2rel($executable, $ENV{"ACE_ROOT"});
        $executable = $self->{TARGET}->ACE_ROOT() . "/$executable";
    }

    # After VxWorks adopts the TARGET scheme, can do away with this block.
    if ($self->{IGNOREHOSTROOT} == 0) {
      if (PerlACE::is_vxworks_test()) {
          $executable = PerlACE::VX_HostFile ($executable);
      }
    }

    if ($self->{IGNOREEXESUBDIR} == 0) {
        $executable = $self->Normalize_Executable_Name ($executable);
    }
    else {
        if ($executable !~ m/\.(BAT|EXE)$/i) {
            $executable = $executable.".EXE";
        }
        $executable =~ s/\//\\/g; # / <- # color coding issue in devenv
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
    $commandline = '"' . $commandline . '"' if $commandline =~ /\s/;

    if (defined $self->{ARGUMENTS}) {
        $commandline .= ' '.$self->{ARGUMENTS};
    }

    return $commandline;
}

sub IgnoreExeSubDir
{
    my $self = shift;

    # If we have -Config ARCH, do not set IGNOREEXESUBDIR, since with ARCH
    # all executables (even those in $ACE_ROOT/bin, etc.) are located in the
    # architecture-specific subdirectory.
    if (@_ != 0 && !grep(($_ eq 'ARCH'), @PerlACE::ConfigList::Configs)) {
        $self->{IGNOREEXESUBDIR} = shift;
    }
    elsif (@_ != 0 && $self->{EXECUTABLE} =~ /perl$/) {
        $self->{IGNOREEXESUBDIR} = shift;
    }

    return $self->{IGNOREEXESUBDIR};
}

sub IgnoreHostRoot
{
    my $self = shift;

    if (@_ != 0) {
        $self->{IGNOREHOSTROOT} = shift;
    }

    return $self->{IGNOREHOSTROOT};
}

###############################################################################

### Spawning processes


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

        if (!-x $self->Executable ()) {
            print STDERR "ERROR: Cannot Spawn: <", $self->Executable (),
                         "> not executable\n";
            return -1;
        }
    }

    my $state = 0;
    my $cmdline = "";
    my $executable = "";

    my $chdir_needed = 0;
    my $curdir_bak;

    if (defined $self->{PURIFY_CMD}) {
        my $orig_cmdline = $self->CommandLine ();
        $executable = $self->{PURIFY_CMD};
        my $basename = basename ($self->{EXECUTABLE});

        my $PurifyOptions = $self->{PURIFY_OPT};
        if (!defined $PurifyOptions) {
            $PurifyOptions =
                "/run ".
#                "/save-data=$basename.pfy ".
                "/save-text-data=$basename.pfytxt ".
                "/AllocCallStackLength=20 ".
                "/ErrorCallStackLength=20 ".
                "/HandlesInUseAtExit ".
                "/InUseAtExit ".
                "/LeaksAtExit ";
        }
        $cmdline =
            "purify " .
            "$PurifyOptions ".
            "$orig_cmdline" ;
    }
    elsif (defined $self->{WINCE_CTL}) {
        $executable = $self->Executable ();
        $cmdline = $self->CommandLine ();

        # Generate a script to copy the test down to the device, run it,
        # copy the log file(s) back to the log directory, then delete the
        # program and log files on the remote device.
        unless (open (SCRIPT, ">start_test.cmd")) {
            print STDERR "ERROR: Cannot Spawn: <", $self->Executable (),
                         "> failed to create start_test.cmd\n";
            return -1;
        }

        my $testname = basename($executable,'.EXE');
        my $here = getcwd();
        $here =~ s/\//\\/g;
        $executable =~ s/^\.//;      # Chop leading .
        $executable = $here . $executable;   # Fully qualified name
        # Take off the test name from the start of the command line.
        # The command name is preprended in the script below.
        my @tokens = split(' ', $cmdline);
        @tokens = splice(@tokens,1);
        $cmdline = join(' ', @tokens);
        print SCRIPT "copy $executable 1:\\Windows\n";
        print SCRIPT "start /wait $testname $cmdline\n";
        print SCRIPT "copy 1:\\log\\$testname*.txt $here\\log\n";
        print SCRIPT "del 1:\\Windows\\$testname.exe\n";
        print SCRIPT "del 1:\\log\\$testname*.txt\n";
        close SCRIPT;

        $executable = $ENV{"ComSpec"};
        my $pocket_device_opts = $ENV{"ACE_PCE_DEVICE"};
        $cmdline = "cmd /C start /B /WAIT $self->{WINCE_CTL} $pocket_device_opts -m NAME=start_test.cmd;WAIT=401000; -e"
    }
    elsif (defined $ENV{'ACE_TEST_WINDOW'}) {
        $state = ($ENV{'ACE_TEST_WINDOW'} =~ /\/k/i ? CREATE_NEW_CONSOLE : DETACHED_PROCESS);
        $executable = $ENV{'ComSpec'};
        $cmdline = $ENV{'ACE_TEST_WINDOW'} . ' ' . $self->CommandLine();
    }
    else {
        $executable = $self->Executable ();
        $cmdline = $self->CommandLine ();
        if ((defined $self->{TARGET}) && ($ENV{'ACE_ROOT'} ne $self->{TARGET}->ACE_ROOT ())) {
          $chdir_needed = 1;
          $curdir_bak = cwd ();
          chdir (dirname ($executable));
        }
    }
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print "$executable $cmdline\n";
    }
    my %backup_ENV = %ENV;
    # update environment for target
    if (defined $self->{TARGET}) {
        if (!defined $self->{TARGET}->{REMOTE_SHELL}) {
            my $x_env_ref = $self->{TARGET}->{EXTRA_ENV};
            while ( my ($env_key, $env_value) = each(%$x_env_ref) ) {
                if (defined $ENV{'ACE_TEST_VERBOSE'}) {
                    print "INFO: adding target environment $env_key=$env_value\n";
                }
                $ENV{$env_key} = $env_value;
            }
        }
        if ($self->{TARGET}->{LIBPATH}) {
            if (defined $ENV{'ACE_TEST_VERBOSE'}) {
                print "INFO: adding target libpath ".$self->{TARGET}->{LIBPATH}."\n";
            }
            PerlACE::add_lib_path ($self->{TARGET}->{LIBPATH});
        }
    }
    my $status = Win32::Process::Create ($self->{PROCESS},
                                         $executable,
                                         $cmdline,
                                         ($state == 0 ? 1 : 0),
                                         $state,
                                         '.');
    %ENV = %backup_ENV;
    if ($chdir_needed) {
      chdir ($curdir_bak);
    }
    if ($status == 0) {
        print STDERR "ERROR: Spawn failed for <", $self->CommandLine (), ">\n";
        return -1;
    }

    $self->{RUNNING} = 1;
    return 0;
}


# Wait for the process to exit or kill after a time period

sub WaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    my $status = $self->TimedWait ($timeout);

    if ($status == -1) {
        print STDERR "ERROR: $self->{EXECUTABLE} timedout\n";
        $self->Kill ();
        # Don't need to Wait since we are on Win32
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


# Kill the process

sub Kill ($)
{
    my $self = shift;
    my $notused = shift; #Used in Process_Unix.pm

    if ($self->{RUNNING}  && !defined $ENV{'ACE_TEST_WINDOW'}) {
        Win32::Process::Kill ($self->{PROCESS}, -1);
    }

    $self->{RUNNING} = 0;
}


# Terminate the process and wait for it to finish

sub TerminateWaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    if ($self->{RUNNING}) {
        print STDERR "INFO: $self->{EXECUTABLE} being killed.\n";
        Win32::Process::Kill ($self->{PROCESS}, 0);
    }

    return $self->WaitKill ($timeout);
}


# Wait until a process exits.
# return -1 if the process is still alive.

sub Wait ($)
{
    my $self = shift;
    my $timeout = shift;
    if (!defined $timeout || $timeout < 0) {
      $timeout = INFINITE;
    } else {
      $timeout = $timeout * 1000 * $PerlACE::Process::WAIT_DELAY_FACTOR;
    }

    my $result = 0;

    if ($self->{RUNNING}) {
      $result = Win32::Process::Wait ($self->{PROCESS}, $timeout);
      if ($result == 0) {
        return -1;
      }
    }
    Win32::Process::GetExitCode ($self->{PROCESS}, $result);
    return $result;
}


# Wait for a process to exit with a timeout

sub TimedWait ($)
{
    my($self) = shift;
    my($timeout) = shift;
    return $self->Wait($timeout);
}

###

sub kill_all
{
  my $procmask = shift;
  my $target = shift;
  my $pid = -1;
  for my $line (`tasklist /nh /fo csv`) {
    # find matching process line
    if ($line =~ /$procmask/) {
      # find process PID
      if ($line =~ /^\"[^\"]+\",\"(\d+)\",/) {
        $pid = $1;
        Win32::Process::KillProcess ($pid, 0); # kill process
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
          print STDERR "INFO: Killed process at [$line]\n"
        }
      }
    }
  }
}

1;
