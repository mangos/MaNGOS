#! /usr/bin/perl
# $Id: ProcessWinCE_Unix.pm 88363 2009-12-30 10:51:36Z johnnyw $

package PerlACE::ProcessVX;

use strict;
use POSIX "sys_wait_h";
use File::Basename;
use File::Spec;
use Config;
use FileHandle;
use Cwd;

eval { require Net::Telnet; };

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

# This is what GetExitCode will return if the process is still running.
my $STILL_ACTIVE = 259;

###############################################################################

### Constructor and Destructor

sub new
{
    my $proto = shift;
    my $class = ref ($proto) || $proto;
    my $self = {};

    $self->{RUNNING} = 0;
    $self->{IGNOREEXESUBDIR} = 1;
    $self->{IGNOREHOSTROOT} = 0;
    $self->{PROCESS} = undef;
    $self->{EXECUTABLE} = shift;
    $self->{ARGUMENTS} = shift;
    $self->{TARGET} = shift;
    if (!defined $PerlACE::ProcessVX::WAIT_DELAY_FACTOR) {
        $PerlACE::ProcessVX::WAIT_DELAY_FACTOR = 2;
    }
    if (!defined $PerlACE::ProcessVX::RebootCmd) {
        $PerlACE::ProcessVX::RebootCmd = "reboot 0x02";
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

    if (defined $ENV{'ACE_RUN_VX_IBOOT'} && !defined $ENV{'ACE_RUN_VX_NO_SHUTDOWN'}) {
      # Shutdown the target to save power
      $self->iboot_cycle_power(1);
    }
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

    my $cmdline;

    # Reboot the target if necessery
    $self->reboot();

    my $program = $self->Executable ();
    my $cwdrel = dirname ($program);
    my $prjroot = defined $ENV{"ACE_RUN_VX_PRJ_ROOT"} ? $ENV{"ACE_RUN_VX_PRJ_ROOT"} : $ENV{"ACE_ROOT"};
    if (length ($cwdrel) > 0) {
        $cwdrel = File::Spec->abs2rel( cwd(), $prjroot );
    }
    else {
        $cwdrel = File::Spec->abs2rel( $cwdrel, $prjroot );
    }
    $program = basename($program, $PerlACE::ProcessVX::ExeExt);

    my @cmds;
    my $cmdnr = 0;
    my $arguments = "";
    my $prompt = '';
    my $exesubdir = defined $ENV{"ACE_RUN_VX_EXE_SUBDIR"} ? $ENV{"ACE_RUN_VX_EXE_SUBDIR"} : "";

    if (defined $ENV{"ACE_RUN_VX_STARTUP_SCRIPT"}) {
      if (defined $ENV{"ACE_RUN_VX_STARTUP_SCRIPT_ROOT"}) {
        @cmds[$cmdnr++] = 'cd "' . $ENV{'ACE_RUN_VX_STARTUP_SCRIPT_ROOT'} . '"';
      }
      @cmds[$cmdnr++] = '< ' . $ENV{"ACE_RUN_VX_STARTUP_SCRIPT"};
    }

    if (defined $ENV{"ACE_RUN_VX_STARTUP_COMMAND"}) {
      @cmds[$cmdnr++] = $ENV{"ACE_RUN_VX_STARTUP_COMMAND"};
    }

        @cmds[$cmdnr++] = 'cd ' . $ENV{"ACE_RUN_VX_TGTSVR_ROOT"} . "/" . $cwdrel . "/" . $exesubdir;
        @cmds[$cmdnr++] = 'set TMPDIR=' . $ENV{"ACE_RUN_VX_TGTSVR_ROOT"} . "/" . $cwdrel;

        if (defined $ENV{'ACE_RUN_ACE_DEBUG'}) {
            @cmds[$cmdnr++] = 'set ACE_DEBUG=' . $ENV{"ACE_RUN_ACE_DEBUG"};
        }

        if (defined $ENV{'ACE_RUN_TAO_ORB_DEBUG'}) {
            @cmds[$cmdnr++] = 'set TAO_ORB_DEBUG=' . $ENV{"ACE_RUN_TAO_ORB_DEBUG"};
        }

        if (defined $ENV{'ACE_RUN_ACE_LD_SEARCH_PATH'}) {
            @cmds[$cmdnr++] = 'set ACE_LD_SEARCH_PATH=' . $ENV{"ACE_RUN_ACE_LD_SEARCH_PATH"};
        }

        my(@load_commands);
        my(@unload_commands);
        my $vxtest_file = $program . '.vxtest';
        if (handle_vxtest_file($self, $vxtest_file, \@load_commands, \@unload_commands)) {
            push @cmds, @load_commands;
            $cmdnr += scalar @load_commands;
          } else {
            print STDERR "ERROR: Cannot find <", $vxtest_file, ">\n";
          }

        if (defined $self->{ARGUMENTS}) {
            ($arguments = $self->{ARGUMENTS})=~ s/\"/\\\"/g;
            ($arguments = $self->{ARGUMENTS})=~ s/\'/\\\'/g;
        }
        $cmdline = $program . ' ' . $arguments;
        if (defined $ENV{'ACE_RUN_VX_TGTSRV_WORKINGDIR'}) {
            @cmds[$cmdnr++] = 'cd ' . $ENV{'ACE_RUN_VX_TGTSRV_WORKINGDIR'};
        } else {
            @cmds[$cmdnr++] = 'cd ' . $ENV{'ACE_RUN_VX_TGTSVR_ROOT'} . "/" . $cwdrel;
        }
        @cmds[$cmdnr++] = $cmdline;
        if (!defined $ENV{'ACE_TEST_VERBOSE'}) {
          push @cmds, @unload_commands;
          $cmdnr += scalar @unload_commands;
        }
        $prompt = '[\\\\].*>[\ ]$';

    FORK:
    {
        if ($self->{PROCESS} = fork) {
            #parent here
            bless $self;
        }
        elsif (defined $self->{PROCESS}) {
            #child here
            my $telnet_port = $ENV{'ACE_RUN_VX_TGT_TELNET_PORT'};
            my $telnet_host = $ENV{'ACE_RUN_VX_TGT_TELNET_HOST'};
            if (!defined $telnet_host)  {
              $telnet_host = $ENV{'ACE_RUN_VX_TGTHOST'};
            }
            if (!defined $telnet_port)  {
                $telnet_port = 23;
              }
            if (defined $ENV{'ACE_TEST_VERBOSE'}) {
                print "Opening telnet connection <" . $telnet_host . ":". $telnet_port . ">\n";
            }
            my $t = new Net::Telnet(Timeout => 600, Errmode => 'return', Host => $telnet_host, Port => $telnet_port);
            if (!defined $t) {
              die "ERROR: Telnet failed to <" . $telnet_host . ":". $telnet_port . ">";
            }
            my $retries = 10;
            while ($retries--) {
              if (!$t->open()) {
                if (defined $ENV{'ACE_TEST_VERBOSE'}) {
                  print "Couldn't open telnet connection; sleeping then retrying\n";
                }
                if ($retries == 0) {
                  die "ERROR: Telnet open to <" . $telnet_host . ":". $telnet_port . "> " . $t->errmsg;
                }
                sleep(5);
              } else {
                last;
              }
            }

            my $target_login = $ENV{'ACE_RUN_VX_LOGIN'};
            my $target_password = $ENV{'ACE_RUN_VX_PASSWORD'};

            if (defined $target_login)  {
              $t->waitfor('/VxWorks login: $/');
              $t->print("$target_login");
            }

            if (defined $target_password)  {
              $t->waitfor('/Password: $/');
              $t->print("$target_password");
            }

            my $buf = '';
            # wait for the prompt
            while (1) {
              my $blk = $t->get;
              print $blk;
              $buf .= $blk;
              if ($buf =~ /$prompt/) {
                last;
              }
            }
            if ($buf !~ /$prompt/) {
              die "ERROR: Didn't got prompt but got <$buf>";
            }
            my $i = 0;
            my @lines;
            while($i < $cmdnr) {
              if (defined $ENV{'ACE_TEST_VERBOSE'}) {
                print @cmds[$i]."\n";
              }
              if ($t->print (@cmds[$i++])) {
                # After each command wait for the prompt
                my $buf = '';
                while (1) {
                  my $blk = $t->get;
                  print $blk;
                  $buf .= $blk;
                  if ($buf =~ /$prompt/) {
                    last;
                  }
                }
              } else {
                print $t->errmsg;
              }
            }
            $t->close();
            sleep(2);
            exit;
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
    $self->{RUNNING} = 1;
    return 0;
}


# Terminate the process and wait for it to finish

sub TerminateWaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    if ($self->{RUNNING}) {
        print STDERR "INFO: $self->{EXECUTABLE} being killed.\n";
        kill ('TERM', $self->{PROCESS});

        $PerlACE::ProcessVX::DoVxInit = 1; # force reboot on next run
    }

    return $self->WaitKill ($timeout);
}

# really only for internal use
sub check_return_value ($)
{
    my $self = shift;
    my $rc = shift;

    my $CC_MASK = 0xff00;

    # Exit code processing
    if ($rc == 0) {
        return 0;
    }
    elsif ($rc == $CC_MASK) {
        print STDERR "ERROR: <", $self->{EXECUTABLE},
                     "> failed: $!\n";

        $PerlACE::ProcessVX::DoVxInit = 1; # force reboot on next run

        return ($rc >> 8);
    }
    elsif (($rc & 0xff) == 0) {
        $rc >>= 8;
        return $rc;
    }

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

    $PerlACE::ProcessVX::DoVxInit = 1; # force reboot on next run

    return 0;
}

sub Kill ()
{
    my $self = shift;

    if ($self->{RUNNING} && !defined $ENV{'ACE_TEST_WINDOW'}) {
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

    if ($PerlACE::Process::WAIT_DELAY_FACTOR > 0) {
      $timeout *= $PerlACE::Process::WAIT_DELAY_FACTOR;
    }

    while ($timeout-- != 0) {
        my $pid = waitpid ($self->{PROCESS}, &WNOHANG);
        if ($pid != 0 && $? != -1) {
            return $self->check_return_value ($?);
        }
        sleep 1;
    }

    $PerlACE::ProcessVX::DoVxInit = 1; # force reboot on next run

    return -1;
}

sub handle_vxtest_file
{
  my $self = shift;
  my $vxtestfile = shift;
  my $vx_ref = shift;
  my $unld_ref = shift;
  my $fh = new FileHandle;

  if (defined $self->{TARGET} && $self->{TARGET}->SystemLibs())
    {
      my @tokens = split(/;/, $self->{TARGET}->SystemLibs());
      foreach my $token (@tokens) {
        push @$vx_ref, "copy " . $ENV{"ACE_RUN_VX_TGTSVR_ROOT"} . "/lib/" . $token . " .";
        unshift @$unld_ref, "del " . $token;
      }
    }
  if (!$PerlACE::Static) {
    if (open ($fh, $vxtestfile)) {
      my $line1 = <$fh>;
      chomp $line1;
      while(<$fh>) {
        $line1 = $_;
        chomp $line1;
        push @$vx_ref, "copy " . $ENV{"ACE_RUN_VX_TGTSVR_ROOT"} . "/lib/$line1" . ".dll .";
        unshift @$unld_ref, "del $line1" . ".dll";
      }
      close $fh;
    } else {
      return 0;
    }
  }
  return 1;
}

1;
