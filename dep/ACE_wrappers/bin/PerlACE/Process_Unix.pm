#! /usr/bin/perl
# $Id: Process_Unix.pm 91052 2010-07-09 08:36:01Z mcorino $

package PerlACE::Process;

use strict;
use POSIX "sys_wait_h";
use Cwd;
use File::Basename;
use Config;

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
    $self->{IGNOREHOSTROOT} = 0;
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

    if (defined $self->{SCRIPTFILE}) {
      unlink $self->{SCRIPTFILE};
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

    # If the target's config has a different ACE_ROOT, rebase the executable
    # from $ACE_ROOT to the target's root.
    if (defined $self->{TARGET} &&
          $self->{TARGET}->ACE_ROOT() ne $ENV{"ACE_ROOT"}) {
        $executable = PerlACE::rebase_path ($executable,
                                            $ENV{"ACE_ROOT"},
                                            $self->{TARGET}->ACE_ROOT());
    }

    if ($self->{IGNOREHOSTROOT} == 0) {
      if (PerlACE::is_vxworks_test()) {
          $executable = PerlACE::VX_HostFile ($executable);
      }
    }

    if ($self->{IGNOREEXESUBDIR}) {
        return $executable;
    }

    my $basename = basename ($executable);
    my $dirname = dirname ($executable).'/';

    my $subdir = $PerlACE::Process::ExeSubDir;
    if (defined $self->{TARGET} && defined $self->{TARGET}->{EXE_SUBDIR}) {
        $subdir = $self->{TARGET}->{EXE_SUBDIR};
    }

    $executable = $dirname . $subdir . $basename;

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

    my $exe = $self->Executable ();
    my $commandline = $exe;

    if (defined $self->{REMOTEINFO}) {
      my($method)   = $self->{REMOTEINFO}->{method};
      my($username) = $self->{REMOTEINFO}->{username};
      my($remote)   = $self->{REMOTEINFO}->{hostname};
      my($exepath)  = $self->{REMOTEINFO}->{exepath};
      my($libpath)  = $self->{REMOTEINFO}->{libpath};
      my($exe)      = (defined $exepath ?
                        "$exepath/" . basename($commandline) : $commandline);
      $commandline  = "$method -l $username $remote \"";
      if (defined $libpath) {
        my($csh) = (defined $self->{REMOTEINFO}->{shell} &&
                            $self->{REMOTEINFO}->{shell} =~ /csh/);
        foreach my $pvar ('DYLD_LIBRARY_PATH', 'LD_LIBRARY_PATH',
                          'LIBPATH', 'SHLIB_PATH') {
          if ($csh) {
            $commandline .= "if (! \\\$?$pvar) setenv $pvar; " .
                            "setenv $pvar $libpath:\\\$$pvar; ";
          }
          else {
            $commandline .= "$pvar=$libpath:\\\$$pvar; export $pvar; ";
          }
        }
        my($env) = $self->{REMOTEINFO}->{env};
        if (defined $env) {
          foreach my $pvar (keys %$env) {
            if ($csh) {
              $commandline .= "setenv $pvar $$env{$pvar}; ";
            }
            else {
              $commandline .= "$pvar=$$env{$pvar}; export $pvar; ";
            }
          }
        }
      }
      $commandline .= $exe;
    }

    if (defined $self->{ARGUMENTS}) {
        $commandline .= ' '.$self->{ARGUMENTS};
    }

    # Avoid modifying TAO/tests run_test.pl scripts by using the
    # ACE_RUNTEST_ARGS environment variable to append command line
    # arguments.
    if ($^O eq "nonstop_kernel") {
        my $global_args = $ENV{"ACE_RUNTEST_ARGS"};
        if ((length($global_args) > 0)
            && ($commandline !~ /tao_idl/)) {
            $commandline = $commandline
                           . ' '
                           . $global_args;
        }
    }

    if (defined $self->{REMOTEINFO}) {
      $commandline .= '"';
    } elsif (defined $self->{TARGET} && defined $self->{TARGET}->{REMOTE_SHELL}) {
      my($shell)     = $self->{TARGET}->{REMOTE_SHELL};
      my $x_env_ref  = $self->{TARGET}->{EXTRA_ENV};
      my($root)      = $self->{TARGET}->ACE_ROOT();
      if (!defined $root) {
        $root = $ENV{'ACE_ROOT'};
      }
      my($exedir)    = dirname ($exe);
      my($local_xdir)= File::Spec->rel2abs($self->{EXECUTABLE});
      if ($exedir == '.' || $exedir == './' || $exedir == '') {
        $exedir = cwd ();
        $local_xdir = $exedir;
      }
      if (!defined $self->{PIDFILE}) {
        $self->{PIDFILE} = "/tmp/.acerun/ace-".rand(time).".pid";
      }
      if (!defined $self->{SCRIPTFILE}) {
        $self->{SCRIPTFILE} = "$local_xdir/run-".rand(time).".sh";
      }
      ## create scriptfile
      my $libpath = "$root/lib";
      if (defined $self->{TARGET}->{LIBPATH}) {
        $libpath = PerlACE::concat_path ($libpath, $self->{TARGET}->{LIBPATH});
      }
      my $run_script =
        "if [ ! -e /tmp/.acerun ]; then mkdir /tmp/.acerun; fi\n".
        "cd $exedir\n".
        "export LD_LIBRARY_PATH=$libpath:\$LD_LIBRARY_PATH\n".
        "export DYLD_LIBRARY_PATH=$libpath:\$DYLD_LIBRARY_PATH\n".
        "export LIBPATH=$libpath:\$LIBPATH\n".
        "export SHLIB_PATH=$libpath:\$SHLIB_PATH\n".
        "export PATH=\$PATH:$root/bin:$root/lib:$libpath\n";
      while ( my ($env_key, $env_value) = each(%$x_env_ref) ) {
        $run_script .=
        "export $env_key=$env_value\n";
      }
      $run_script .=
        "$commandline &\n";
      $run_script .=
        "MY_PID=\$!\n".
        "echo \$MY_PID > ".$self->{PIDFILE}."\n";
      $run_script .=
        "wait \$MY_PID\n";

      unless (open (RUN_SCRIPT, ">".$self->{SCRIPTFILE})) {
          print STDERR "ERROR: Cannot Spawn: <", $self->Executable (),
                        "> failed to create ",$self->{SCRIPTFILE},"\n";
          return -1;
      }
      print RUN_SCRIPT $run_script;
      close RUN_SCRIPT;

      if (defined $ENV{'ACE_TEST_VERBOSE'}) {
        print STDERR "INFO: created run script [",$self->{SCRIPTFILE},"]\n", $run_script;
      }

      $commandline = "$shell \"source $exedir/".basename ($self->{SCRIPTFILE})."\"";
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

sub RemoteInformation
{
  my($self)   = shift;
  my(%params) = @_;

  ## Valid keys for %params
  ##  hostname - The remote hostname
  ##  method   - either rsh or ssh
  ##  username - The remote user name
  ##  exepath  - The remote path to the executable
  ##  shell    - The shell of the remote user
  ##  libpath  - A library path for libraries required by the executable
  ##  env      - A hash reference of name value pairs to be set in the
  ##             environment prior to executing the executable.
  ##
  ## At a minimum, the user must provide the remote hostname.

  if (defined $params{'hostname'}) {
    my(@pwd) = getpwuid($<);
    $self->{REMOTEINFO} = \%params;
    if (!defined $self->{REMOTEINFO}->{'method'}) {
      $self->{REMOTEINFO}->{'method'} = 'ssh';
    }
    if (!defined $self->{REMOTEINFO}->{'username'}) {
      $self->{REMOTEINFO}->{'username'} = $pwd[0] ||
                                          $ENV{LOGNAME} || $ENV{USERNAME};
    }
    if (!defined $self->{REMOTEINFO}->{'shell'}) {
      $self->{REMOTEINFO}->{'shell'} = basename($pwd[8]);
    }
  }
}

###############################################################################

# Spawn the process and continue;

sub Normalize_Executable_Name
{
    my $executable = shift;

    my $basename = basename ($executable);
    my $dirname = dirname ($executable). '/';

    $executable = $dirname.$PerlACE::Process::ExeSubDir.$basename;

    return $executable;
}

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
        if (!defined $self->{REMOTEINFO} && !-f $self->Executable ()) {
            print STDERR "ERROR: Cannot Spawn: <", $self->Executable (),
                         "> not found\n";
            return -1;
        }
    }

    my $cmdline = "";
    my $executable = "";

    if (defined $self->{VALGRIND_CMD}) {
        my $orig_cmdline = $self->CommandLine();
        $executable = $self->{VALGRIND_CMD};
        my $basename = basename ($self->{EXECUTABLE});

        $cmdline = "$executable $orig_cmdline";
    }
    elsif (defined $ENV{'ACE_TEST_WINDOW'}) {
        $cmdline = $ENV{'ACE_TEST_WINDOW'} . ' ' . $self->CommandLine();
    }
    else {
        $executable = $self->Executable();
        $cmdline = $self->CommandLine();
    }

    FORK:
    {
        if ($self->{PROCESS} = fork) {
            #parent here
            bless $self;
        }
        elsif (defined $self->{PROCESS}) {
            #child here
            my @cmdlist = $self->parse_command_line($cmdline);
            if (defined $ENV{'ACE_TEST_VERBOSE'}) {
                print "INFO: $cmdline\n";
                foreach my $arg (@cmdlist) {
                    print "INFO: argument - '$arg'\n";
                }
            }
            # update environment for target
            if (defined $self->{TARGET}) {
                if (!(defined $self->{TARGET}->{REMOTE_SHELL} || defined $self->{REMOTEINFO})) {
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
            if (!(defined $self->{VALGRIND_CMD} || defined $ENV{'ACE_TEST_WINDOW'}) &&
                  (defined $self->{TARGET}) && ($ENV{'ACE_ROOT'} ne $self->{TARGET}->ACE_ROOT ())) {
                my $x_dir = dirname ($executable);
                chdir ($x_dir);
            }
            exec @cmdlist;
            die "ERROR: exec failed for <" . $cmdline . ">\n";
        }
        elsif ($! =~ /No more process/) {
            #EAGAIN, supposedly recoverable fork error
            sleep 5;
            redo FORK;
        }
        else {
            # weird fork error
            print STDERR "ERROR: Can't fork <" . $cmdline . ">: $!\n";
        }
    }

    if (defined $self->{TARGET} && defined $self->{TARGET}->{REMOTE_SHELL}) {
      my $shell = $self->{TARGET}->{REMOTE_SHELL};
      my $pidfile = $self->{PIDFILE};
      ## wait max 5 sec for pid file to appear
      my $timeout = 5;
      my $rc = 1;
      while ($timeout-- != 0) {
        $rc = int(`$shell 'test -e $pidfile && test -s $pidfile ; echo \$?'`);
        if ($rc == 0) {
          $timeout = 0;
        } else {
          sleep 1;
        }
      }
      if ($rc != 0) {
        print STDERR "ERROR: Remote command failed <" . $cmdline . ">: $! No PID found.\n";
        return -1;
      }
      $self->{REMOTE_PID} = `$shell cat $pidfile`;
      $self->{REMOTE_PID} =~ s/\s+//g;
      system("$shell rm -f $pidfile 2>&1 >/dev/null");
      if (defined $ENV{'ACE_TEST_VERBOSE'}) {
        print STDERR "INFO: Process started remote with pid [",$self->{REMOTE_PID},"]\n";
      }
    }

    $self->{RUNNING} = 1;
    return 0;
}

sub WaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    if ($self->{RUNNING} == 0) {
        return 0;
    }

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

    # NSK OSS has a 32-bit waitpid() status
    my $is_NSK = ($^O eq "nonstop_kernel");
    my $CC_MASK = $is_NSK ? 0xffff00 : 0xff00;

    # Exit code processing
    if ($rc == 0) {
        return 0;
    }
    elsif ($rc == $CC_MASK) {
        print STDERR "ERROR: <", $self->{EXECUTABLE},
                     "> failed: $!\n";
        return ($rc >> 8);
    }
    elsif (($rc & 0xff) == 0) {
        $rc >>= 8;
        return $rc;
    }

    # Ignore NSK 16-bit completion code
    $rc &= 0xff if $is_NSK;

    # Remember Core dump flag
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

    return 255;
}

# for internal use
sub parse_command_line ($)
{
    my $self = shift;
    my $cmdline = shift;
    $cmdline =~ s/^\s+//;

    my @cmdlist = ();
    while ($cmdline ne '') {
        if ($cmdline =~ /^\"([^\"\\]*(?:\\.[^\"\\]*)*)\"(.*)/) {
            my $unquoted = $1;
            $cmdline = $2;
            $unquoted =~ s/\\\"/\"/g;
            push @cmdlist, $unquoted;
        }
        elsif ($cmdline =~ /^\'([^\'\\]*(?:\\.[^\'\\]*)*)\'(.*)/) {
            my $unquoted = $1;
            $cmdline = $2;
            $unquoted =~ s/\\\'/\'/g;
            push @cmdlist, $unquoted;
        }
        elsif ($cmdline =~ /^([^\s]*)(.*)/) {
            push @cmdlist, $1;
            $cmdline = $2;
        }
        else {
            # this must be some kind of error
            push @cmdlist, $cmdline;
        }
        $cmdline =~ s/^\s+//;
    }

    return @cmdlist;
}

sub Kill ($)
{
    my $self = shift;
    my $ignore_return_value = shift;

    if ($self->{RUNNING} && !defined $ENV{'ACE_TEST_WINDOW'}) {
        if (defined $self->{TARGET} && defined $self->{TARGET}->{REMOTE_SHELL}) {
          my $cmd = $self->{TARGET}->{REMOTE_SHELL}." kill -s KILL ".$self->{REMOTE_PID};
          if (defined $ENV{'ACE_TEST_VERBOSE'}) {
            print STDERR "INFO: Killing remote process <", $cmd, ">\n";
          }
          $cmd = `$cmd 2>&1`;
        } else {
          kill ('KILL', $self->{PROCESS});
        }
        for(my $i = 0; $i < 10; $i++) {
          my $pid = waitpid ($self->{PROCESS}, WNOHANG);
          if ($pid > 0) {
            if (! $ignore_return_value) {
              $self->check_return_value ($?);
            }
            last;
          }
          else {
            select(undef, undef, undef, .5);
          }
        }
    }

    $self->{RUNNING} = 0;
}

# Wait until a process exits.
# return -1 if the process is still alive.
sub Wait ($)
{
    my $self = shift;
    my $timeout = shift;
    if (!defined $self->{PROCESS}) {
      return 0;
    }
    if (!defined $timeout || $timeout < 0) {
      return waitpid ($self->{PROCESS}, 0);
    } else {
      return TimedWait($self, $timeout);
    }

}

sub TimedWait ($)
{
    my $self = shift;
    my $timeout = shift;

    if (!defined $self->{PROCESS}) {
        return 0;
    }

    $timeout *= $PerlACE::Process::WAIT_DELAY_FACTOR;

    while ($timeout-- != 0) {
        my $pid = waitpid ($self->{PROCESS}, &WNOHANG);
        if ($pid != 0 && $? != -1) {
            return $self->check_return_value ($?);
        }
        sleep 1;
    }

    return -1;
}

###

sub kill_all
{
  my $procmask = shift;
  my $target = shift;
  my $pid = -1;
  my $cmd;
  my $valgrind_cmd = $ENV{"ACE_RUN_VALGRIND_CMD"};
  my $ps_cmd = 'ps -ef';
  my $ps_pid_field = 1;
  my $ps_cmd_field = 7;
  my $ps_skip_first = 1;
  my @ps_fields = 0;
  if (defined $target && defined $target->{PS_CMD}) {
    ## in case a special command is required
    ## format: <cmd>:<pid field index>:<cmd field index>[:<skip headers flag>]
    $ps_cmd_field = -1;
    @ps_fields = split (/:/, $target->{PS_CMD});
    $ps_cmd = @ps_fields[0];
    if (@ps_fields > 1) {
      $ps_pid_field = @ps_fields[1];
      if (@ps_fields > 2) {
        $ps_cmd_field = @ps_fields[2];
        if (@ps_fields > 3) {
          $ps_skip_first = (@ps_fields[3] == '1' ? 1 : 0);
        }
      }
    } else {
      print STDERR "ERROR: Missing field index for PID in [PS_CMD=".$target->{PS_CMD}."]\n";
      return 0;
    }
    @ps_fields = 0;
  } elsif (! (defined $target && defined $target->{REMOTE_SHELL}) ) {
    my $ps_file = `which ps`;
    $ps_file =~ s/^\s+//;
    $ps_file =~ s/\s+$//;
    if ((-l $ps_file) and (readlink ($ps_file)) =~ /busybox/) {
      ## some embedded targets use BusyBox for base tools
      ## with different arguments
      $ps_cmd = 'ps w';
      $ps_pid_field = 0;
      $ps_cmd_field = 4;
    }
  }
  if (defined $target && defined $target->{REMOTE_SHELL}) {
    $ps_cmd = $target->{REMOTE_SHELL}.' '.$ps_cmd;
  }
  for my $line (`$ps_cmd`) {
    if ($ps_skip_first) {
      # skip first line (headers)
      $ps_skip_first = 0;
    } else {
      # split line
      @ps_fields = split (/\s+/, $line);
      if (@ps_fields > $ps_pid_field && @ps_fields > $ps_cmd_field) {

        $pid = @ps_fields[$ps_pid_field]; # process PID
        # take care of valgrind runs
        if (defined $valgrind_cmd) {
          my $pos = index ($line, $valgrind_cmd);
          if ($pos >= 0) {
            $cmd = substr ($line, $pos + length ($valgrind_cmd));
            $cmd =~ s/^\s+//; # strip leading ws
            @ps_fields = split (/\s+/, $cmd);
            $cmd = @ps_fields[0];
          } else {
            $cmd = $line;
          }
        } else {
          if ($ps_cmd_field >= 0) {
            $cmd = @ps_fields[$ps_cmd_field]; # process cmd / executable
          } else {
            $cmd = $line;
          }
        }

        # match process cmd
        if ($cmd =~ /$procmask/) {
          if (defined $target && defined $target->{REMOTE_SHELL}) {
            my $kill_cmd = $target->{REMOTE_SHELL}." kill -s KILL $pid";
            $kill_cmd =  `$kill_cmd`;
          } else {
            kill ('KILL', $pid); # kill process
          }
          if (defined $ENV{'ACE_TEST_VERBOSE'}) {
            print STDERR "INFO: Killed process at [$line]\n";
          }
        }
      }
    }
  }
}

1;
