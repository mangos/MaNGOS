#! /usr/bin/perl
# $Id: ProcessWinCE.pm 88363 2009-12-30 10:51:36Z johnnyw $

package PerlACE::ProcessVX;

use strict;
use Win32::Process;
use File::Basename;
use File::Spec;
use FileHandle;
use Cwd;

eval { require Net::Telnet; };

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
    $self->{TARGET} = shift;
    $self->{EXECUTABLE} = shift;
    $self->{ARGUMENTS} = shift;
    if (!defined $PerlACE::ProcessVX::WAIT_DELAY_FACTOR) {
        $PerlACE::ProcessVX::WAIT_DELAY_FACTOR = 3;
    }
    if (!defined $PerlACE::ProcessVX::RebootCmd) {
        $PerlACE::ProcessVX::RebootCmd = "reboot";
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

    if (!defined $ENV{'ACE_TEST_VERBOSE'}) {
        unlink "run_vx.pl";
    }

    if (defined $ENV{'ACE_RUN_VX_IBOOT'} && !defined $ENV{'ACE_RUN_VX_NO_SHUTDOWN'}) {
      # Shutdown the target to save power
      $self->iboot_cycle_power(1);
    }
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
    $cwdrel =~ s/\\/\//g;
    $program = basename($program, $PerlACE::ProcessVX::ExeExt);

    unlink "run_vx.pl";
    my $oh = new FileHandle();
    if (!open($oh, ">run_vx.pl")) {
        print STDERR "ERROR: Unable to write to run_vx.pl\n";
        exit -1;
    }

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
        $prompt = '\> $';

    print $oh "require Net::Telnet;\n";
    print $oh "my \@cmds;\n";
    print $oh "my \$prompt = '$prompt';\n";
    my $i = 0;
    while($i < $cmdnr) {
      print $oh "\@cmds[$i] = '" . @cmds[$i++] . "';\n";
    }
    print $oh "my \$cmdnr = $cmdnr;\n\n";

    print $oh <<'__END__';

my $telnet_port = $ENV{'ACE_RUN_VX_TGT_TELNET_PORT'};
my $telnet_host = $ENV{'ACE_RUN_VX_TGT_TELNET_HOST'};
if (!defined $telnet_host)  {
  $telnet_host = $ENV{'ACE_RUN_VX_TGTHOST'};
}
if (!defined $telnet_port)  {
  $telnet_port = 23;
}
my $t = new Net::Telnet(Timeout => 600, Errmode => 'return', Host => $telnet_host, Port => $telnet_port);
if (!defined $t) {
  die "ERROR: Telnet failed to <" . $telnet_host . ":". $telnet_port . ">";
}
$t->open();

my $ok = false;
my $buf = '';
while (1) {
  my $blk = $t->get;
  print $blk;
  $buf .= $blk;
  if ($buf =~ /$prompt/) {
    $ok = true;
    last;
  }
}
if ($ok) {
  my $i = 0;
  my @lines;
  while($i < $cmdnr) {
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print @cmds[$i]."\n";
    }
    if ($t->print (@cmds[$i++])) {
      my $buf = '';
      while (1) {
        my $blk = $t->get;
        printf $blk;
        $buf .= $blk;
        if ($buf =~ /$prompt/) {
          last;
        }
      }
    } else {
      print $t->errmsg;
    }
  }
}
else {
  die "ERROR: No prompt appeared\n";
}
$t->close();
sleep(2);
exit;
__END__

    close($oh);

    Win32::Process::Create ($self->{PROCESS},
                            "$^X",
                            "$^X run_vx.pl",
                            0,
                            0,
                            '.');

    Win32::Process::GetExitCode ($self->{PROCESS}, $status);

    if ($status != $STILL_ACTIVE) {
        print STDERR "ERROR: Spawn failed for <", "$^X run_vx.pl", ">\n";
        exit $status;
    }

    $self->{RUNNING} = 1;
    return 0;
}

# Wait for a process to exit with a timeout

sub TimedWait ($)
{
    my($self) = shift;
    my($timeout) = shift;
    return $self->Wait($timeout);
}


# Terminate the process and wait for it to finish

sub TerminateWaitKill ($)
{
    my $self = shift;
    my $timeout = shift;

    if ($self->{RUNNING}) {
        print STDERR "INFO: $self->{EXECUTABLE} being killed.\n";
        Win32::Process::Kill ($self->{PROCESS}, 0);
        $PerlACE::ProcessVX::DoVxInit = 1; # force reboot on next run
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
      $timeout = $timeout * 1000 * $PerlACE::ProcessVX::WAIT_DELAY_FACTOR;
    }

    my $result = 0;

    if ($self->{RUNNING}) {
      $result = Win32::Process::Wait ($self->{PROCESS}, $timeout);
      if ($result == 0) {
        return -1;
      }
    }
    Win32::Process::GetExitCode ($self->{PROCESS}, $result);
    if ($result != 0) {
        $PerlACE::ProcessVX::DoVxInit = 1; # force reboot on next run
    }
    return $result;
}



# Kill the process

sub Kill ()
{
    my $self = shift;

    if ($self->{RUNNING}) {
        Win32::Process::Kill ($self->{PROCESS}, -1);
    }

    $self->{RUNNING} = 0;
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
      }
    }
  if (!$PerlACE::Static) {
    if (open ($fh, $vxtestfile)) {
      my $line1 = <$fh>;
      chomp $line1;
      while(<$fh>) {
        $line1 = $_;
        chomp $line1;
        push @$vx_ref, "copy " . $ENV{"ACE_RUN_VX_TGTSVR_ROOT"} . "/lib/$line1" . "d.dll .";
        unshift @$unld_ref, "del $line1" . "d.dll";
      }
      close $fh;
    } else {
      return 0;
    }
  }
  return 1;
}


1;
