#! /usr/bin/perl
# $Id: ProcessVX.pm 89840 2010-04-12 09:36:32Z mcorino $

package PerlACE::ProcessVX;

use strict;
use English;
use POSIX qw(:time_h);

$PerlACE::ProcessVX::ExeSubDir = './';
$PerlACE::ProcessVX::DoVxInit = (defined $ENV{"ACE_RUN_VX_NO_INITIAL_REBOOT"}) ? 0 : 1;
$PerlACE::ProcessVX::VxDefGw = 1;
$PerlACE::ProcessVX::RebootTime = (defined $ENV{"ACE_RUN_VX_REBOOT_TIME"}) ? $ENV{"ACE_RUN_VX_REBOOT_TIME"} : 90;
$PerlACE::ProcessVX::ExeExt = ($PerlACE::VxWorks_RTP_Test) ? '.vxe' : '.out';
$PerlACE::ProcessVX::RebootCmd = $ENV{"ACE_RUN_VX_REBOOT_CMD"};

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

        $PerlACE::ProcessVX::DoVxInit = 1; # force reboot on next run
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


###############################################################################

### Some Accessors

sub Normalize_Executable_Name
{
    my $self = shift;
    my $executable = shift;

    my $basename = basename ($executable);
    my $dirname = dirname ($executable). '/';

    $executable = $dirname.$PerlACE::ProcessVX::ExeSubDir.$basename.$PerlACE::ProcessVX::ExeExt;

    ## Installed executables do not conform to the ExeSubDir
    if (! -e $executable && -e $dirname.$basename.$PerlACE::ProcessVX::ExeExt) {
      $executable = $dirname.$basename.$PerlACE::ProcessVX::ExeExt;
    }

    return $executable;
}


sub Executable
{
    my $self = shift;

    if (@_ != 0) {
        $self->{EXECUTABLE} = shift;
    }

    my $executable = $self->{EXECUTABLE};

    if ($self->{IGNOREEXESUBDIR} == 0) {
      $executable = $self->Normalize_Executable_Name ($executable);
    }
    else {
      $executable = $executable.$PerlACE::ProcessVX::ExeExt;
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

sub IgnoreHostRoot
{
    my $self = shift;

    if (@_ != 0) {
        $self->{IGNOREHOSTROOT} = shift;
    }

    return $self->{IGNOREHOSTROOT};
}

sub delay_factor {
  my($lps)    = 128;
  my($factor) = 1;

  ## Keep increasing the loops per second until the amount of time
  ## exceeds the number of clocks per second.  The original code
  ## did not multiply $ticks by 8 but, for faster machines, it doesn't
  ## seem to return false values.  The multiplication is done to minimize
  ## the amount of time it takes to determine the correct factor.
  while(($lps <<= 1)) {
    my($ticks) = clock();
    for(my $i = $lps; $i >= 0; $i--) {
    }
    $ticks = clock() - $ticks;
    if ($ticks * 8 >= CLOCKS_PER_SEC) {
      $factor = 500000 / (($lps / $ticks) * CLOCKS_PER_SEC);
      last;
    }
  }

  return $factor;
}

sub iboot_cycle_power {
  my $self = shift;
  my $mode = shift;

  # mode 0 is reboot, mode 1 is just shutdown

  my ($iboot_host,
      $iboot_outlet,
      $iboot_user,
      $iboot_passwd ) = ($ENV{'ACE_RUN_VX_IBOOT'},
                         $ENV{'ACE_RUN_VX_IBOOT_OUTLET'},
                         $ENV{'ACE_RUN_VX_IBOOT_USER'},
                         $ENV{'ACE_RUN_VX_IBOOT_PASSWORD'});

  my $v = $ENV{'ACE_TEST_VERBOSE'};

  if ($v) {
    print "Using iBoot: $iboot_host\n";
    if (defined $iboot_outlet) {
      print "Using iBoot Outlet #: $iboot_outlet\n";
    }
  }

  # There are three cases to handle here:
  # 1. using a single-outlet iBoot
  # 2. using a multi-outlet iBootBar with custom firmware
  # 3. using a multi-outlet iBootBar with standard firmware
  #
  # In cases 1 & 2, we use the iPAL protocol; in case 3 we
  # use a telnet connection and the command-line syntax.
  #
  # We determine that it's case #3 by the concurrent presence
  # of an outlet number, an iboot username, and an iboot password
  # in the environment.
  #

  if (defined($iboot_outlet) && defined($iboot_user) && defined($iboot_passwd)) {
    # We perform case #3
    # This case doesn't support shutdown
    return if $mode == 1;

    my $t = new Net::Telnet();

    $t->prompt('/iBootBar \> /');
    my $savedmode = $t->errmode();
    $t->errmode("return");

    my $retries = 5;
    my $is_open = 0;

    while ($retries--) {
      my $r = $t->open($iboot_host);
      if ($r == 1) {
        $is_open = 1;
        last;
      }
    }
    continue {
      print "Couldn't open connection; sleeping then retrying\n" if ($v);
      sleep(5);
    }

    if (! $is_open) {
      print "Unable to open $iboot_host.\n" if ($v);
      return 0;
    }

    $t->errmode($savedmode);

    # Simple login b/c Net::Telnet::login hardcodes the prompts
    $t->waitfor('/User Name:\s*$/i');
    $t->print($iboot_user);
    $t->waitfor('/password:\s*/i');
    $t->print($iboot_passwd);

    $t->waitfor($t->prompt);

    print "successfully logged in to $iboot_host\n" if ($v);

    my $output = $t->cmd("set outlet $iboot_outlet cycle");

    print "successfully cycled power on outlet $iboot_outlet\n" if ($v);

    $t->close();
  }
  else {
    # Perform cases 1 & 2
    my $iboot;
    my $text;
    if (!defined($iboot_passwd)) {
      $iboot_passwd = "PASS";
    }

    my $ipal_command_series;
    if (defined $iboot_outlet) {
      $ipal_command_series = ($mode == 0 ? ['E', 'D'] : ['E']);
    } else {
      $ipal_command_series = ($mode == 0 ? ['f', 'n'] : ['f']);
    }

    foreach my $ipal_cmd (@$ipal_command_series) {
      my $retries = 3;
      my $is_open = 0;
      while ($retries--) {
        $iboot = IO::Socket::INET->new ("$iboot_host");
        if ($iboot) {
          # if ACE_RUN_VX_IBOOT_OUTLET is defined, we're using
          # the iBootBar, and we're using the iPAL Protocol
          # to communicate with the iBootBar
          if (defined $iboot_outlet) {
            $iboot->send ("\e".$iboot_passwd."\e".$iboot_outlet.$ipal_cmd);
          }
          else  {
            $iboot->send ("\e".$iboot_passwd."\e$ipal_cmd\r");
          }
          $iboot->recv ($text,128);
          print "iBoot is currently: $text\n" if ($v);
          $iboot->close();
          if (defined $iboot_outlet) {
            $text = substr $text, $iboot_outlet - 1, 1;
            if ($text eq "0" || $text eq "1") {
              $is_open = 1;
              last;
            }
            else {
              print "iBoot is $text; sleeping then retrying\n" if ($v);
              sleep(5);
            }
          }
          else  {
            if ($text eq "OFF" || $text eq "ON") {
              $is_open = 1;
              last;
            }
            else {
              print "iBoot is $text; sleeping then retrying\n" if ($v);
              sleep(5);
            }
          }
        }
        else {
          print "ERROR: FAILED to execute 'reboot' command!\n";
        }
      }
      if (!$is_open) {
        print "Unable to reboot using $iboot_host.\n" if ($v);
        return 0;
      }
    }
  }
}

sub reboot {
  my $self = shift;
  my $iboot;
  my $text;
  my $t;
  my $ok;

  my $target_login = $ENV{'ACE_RUN_VX_LOGIN'};
  my $target_password = $ENV{'ACE_RUN_VX_PASSWORD'};

  ##
  ## initialize VxWorks kernel (reboot!) if needed
  if ($PerlACE::ProcessVX::DoVxInit || $ENV{'ACE_RUN_VX_TGT_REBOOT'}) {
      if (defined $ENV{'ACE_RUN_VX_REBOOT_TOOL'}) {
          if (defined $ENV{'ACE_TEST_VERBOSE'}) {
              print "Calling: $ENV{'ACE_RUN_VX_REBOOT_TOOL'}\n";
          }
          system ($ENV{'ACE_RUN_VX_REBOOT_TOOL'});
      }
      else {
        if (defined $ENV{'ACE_RUN_VX_IBOOT'}) {
          $self->iboot_cycle_power(0);
        }
        else {
          if (defined $ENV{'ACE_TEST_VERBOSE'}) {
            print "Executing 'reboot' command over Telnet to ".$ENV{'ACE_RUN_VX_TGTHOST'}.".\n";
          }
          $t = new Net::Telnet(Timeout => 10,
                               Prompt => '/-> $/',
                               Errmode => 'return');
          $t->open($ENV{'ACE_RUN_VX_TGTHOST'});
          $t->print("");
          if (defined $target_login)  {
            $t->waitfor('/VxWorks login: $/');
            $t->print("$target_login");
          }
          if (defined $target_password)  {
            $t->waitfor('/Password: $/');
            $t->print("$target_password");
          }
          $ok = $t->waitfor('/-> $/');
          if ($ok) {
            $t->print($PerlACE::ProcessVX::RebootCmd);
          }
          else {
            print "ERROR: FAILED to execute 'reboot' command!\n";
          }
          $t->close();
        }
      }
      $PerlACE::ProcessVX::VxDefGw = 1;
      $PerlACE::ProcessVX::DoVxInit = 0;
      sleep($PerlACE::ProcessVX::RebootTime);
  }
}

# Helper for spawning with list of kernel modules in a .vxtest file
sub handle_vxtest_file
{
  my $self = shift;
  my $vxtestfile = shift;
  my $vx_ref = shift;
  my $unld_ref = shift;
  my $fh = new FileHandle;
  if (open ($fh, $vxtestfile)) {
    my $line1 = <$fh>;
    chomp $line1;
    while(<$fh>) {
      $line1 = $_;
      chomp $line1;
      push @$vx_ref, "ld < lib$line1" . ".so";
      unshift @$unld_ref, "unld \"lib$line1" . ".so\"";
    }
    close $fh;
  } else {
    return 0;
  }
  return 1;
}

# Load a file that is used as startup script. This script has to be
# located on the host system
sub handle_startup_script
{
  my $script = shift;
  my $cmds = shift;
  my $fh = new FileHandle;
  if (open ($fh, $script)) {
    while(<$fh>) {
      my $line1 = $_;
      chomp $line1;
      push @$cmds, "$line1";
    }
    close $fh;
  } else {
    return 0;
  }
  return 1;
}

### Check for -ExeSubDir commands, store the last one
my @new_argv = ();

for(my $i = 0; $i <= $#ARGV; ++$i) {
    if ($ARGV[$i] eq '-ExeSubDir') {
        if (defined $ARGV[$i + 1]) {
            $PerlACE::ProcessVX::ExeSubDir = $ARGV[++$i].'/';
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

$PerlACE::ProcessVX::WAIT_DELAY_FACTOR = $ENV{"ACE_RUNTEST_DELAY"};

if (defined $ENV{'ACE_TEST_WINCE'}) {
  if ($OSNAME eq "MSWin32") {
      require PerlACE::ProcessWinCE;
  } else {
      require PerlACE::ProcessWinCE_Unix;
  }
} else {
  if ($OSNAME eq "MSWin32") {
      require PerlACE::ProcessVX_Win32;
  }
  else {
      require PerlACE::ProcessVX_Unix;
  }
}

###

sub kill_all
{
  my $procmask = shift;
  my $target = shift;
  ## NOT IMPLEMENTED YET
}

1;
