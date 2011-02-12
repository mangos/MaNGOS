#! /usr/bin/perl
# $Id: TestTarget.pm 91813 2010-09-17 07:52:52Z johnnyw $
#
# The TestTarget class is for operations that are per-target while testing.
# They can be overridden for specific needs like embedded systems, etc.

package PerlACE::TestTarget;

use strict;
use English;
use POSIX qw(:time_h);
use File::Copy;
use PerlACE::Run_Test;
use Sys::Hostname;

###############################################################################

# Create the proper kind of TestTarget based on specified test component.
# Pass the component number as the first argument. If there's no
# DOC_TEST_<component-number> environment variable, use the local machine.

sub create_target
{
    my $component = shift;
    if ($component == 0) {
        print STDERR "Warning: components should be numbers, not names\n";
    }
    my $target = undef;
    my $envname = "DOC_TEST_\U$component";
    if (!exists $ENV{$envname}) {
        $target = new PerlACE::TestTarget("default");
        return $target;
    }
    my $config_name = $ENV{$envname};
    # There's a configuration name; use it to look up the platform.
    $config_name = uc $config_name;
    $envname = $config_name.'_OS';
    if (!exists $ENV{$envname}) {
        print STDERR "$config_name requires an OS type in $envname\n";
        return undef;
    }
    my $config_os = $ENV{$envname};
    SWITCH: {
      if ($config_os =~ m/local|remote/i) {
        $target = new PerlACE::TestTarget ($config_name);
        last SWITCH;
      }
      if ($config_os =~ m/LabVIEW_RT/i) {
        require PerlACE::TestTarget_LVRT;
        $target = new PerlACE::TestTarget_LVRT ($config_name);
        last SWITCH;
      }
      if ($config_os =~ /VxWorks/i) {
        require PerlACE::TestTarget_VxWorks;
        $target = new PerlACE::TestTarget_VxWorks ($config_name);
        last SWITCH;
      }
      if ($config_os =~ /WinCE/i) {
        require PerlACE::TestTarget_WinCE;
        $target = new PerlACE::TestTarget_WinCE ($config_name);
        last SWITCH;
      }
      print STDERR "$config_os is an unknown OS type!\n";
    }
    return $target;
}

### Constructor and Destructor

sub new
{
    my $proto = shift;
    my $class = ref ($proto) || $proto;
    my $self = {};

    my $config_name = shift;
    bless ($self, $class);
    $self->GetConfigSettings($config_name);

    return $self;
}

sub DESTROY
{
    my $self = shift;
}

# If there was a config name specified, use it to look up the configure
# info. Else, use the traditional defaults.
sub GetConfigSettings ($)
{
    my $self = shift;
    my $config_name = shift;
    my $env_prefix = '';
    if (defined $config_name) {
        $env_prefix = $config_name."_";
    }
    my $env_name = $env_prefix.'ACE_ROOT';
    if (exists $ENV{$env_name}) {
        $self->{ace_root} = $ENV{$env_name};
    }
    else {
        # Fall back to naked ACE_ROOT if no config-specific one.
        $self->{ace_root} = $ENV{'ACE_ROOT'};
    }
    $env_name = $env_prefix.'TAO_ROOT';
    if (exists $ENV{$env_name}) {
        $self->{tao_root} = $ENV{$env_name};
    } else {
        $self->{tao_root} = "$self->{ace_root}/TAO";
    }
    $env_name = $env_prefix.'CIAO_ROOT';
    if (exists $ENV{$env_name}) {
        $self->{ciao_root} = $ENV{$env_name};
    } else {
        $self->{ciao_root} = "$self->{tao_root}/CIAO";
    }
    $env_name = $env_prefix.'EXE_SUBDIR';
    if (exists $ENV{$env_name}) {
        $self->{EXE_SUBDIR} = $ENV{$env_name}.'/';
    } else {
        # If no ExeSubDir given via env variable, and this is an unnamed
        # config, allow use of the subdir specified on the command line.
        # This preserves historical behavior.
        if (defined $config_name && $config_name ne 'default') {
            $self->{EXE_SUBDIR} = './';
        }
        else {
            $self->{EXE_SUBDIR} = $PerlACE::Process::ExeSubDir;
        }
    }
    $env_name = $env_prefix.'ARCH';
    if (exists $ENV{$env_name}) {
        $self->{ARCH} = $ENV{$env_name};
    } elsif ($config_name eq 'default'
             && grep(($_ eq 'ARCH'), @PerlACE::ConfigList::Configs)) {
        $self->{ARCH} = 1;
    }
    $env_name = $env_prefix.'PROCESS_START_WAIT_INTERVAL';
    if (exists $ENV{$env_name}) {
        $self->{PROCESS_START_WAIT_INTERVAL} = $ENV{$env_name};
    } else {
        $self->{PROCESS_START_WAIT_INTERVAL} = 15;
    }
    $env_name = $env_prefix.'PROCESS_STOP_WAIT_INTERVAL';
    if (exists $ENV{$env_name}) {
        $self->{PROCESS_STOP_WAIT_INTERVAL} = $ENV{$env_name};
    } else {
        $self->{PROCESS_STOP_WAIT_INTERVAL} = 10;
    }
    $env_name = $env_prefix.'HOSTNAME';
    if (exists $ENV{$env_name}) {
        $self->{HOSTNAME} = $ENV{$env_name};
    } else {
        $self->{HOSTNAME} = hostname();
    }
    $env_name = $env_prefix.'IBOOT';
    if (exists $ENV{$env_name}) {
        $self->{IBOOT} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'IBOOT_PASSWD';
    if (exists $ENV{$env_name}) {
        $self->{IBOOT_PASSWD} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'IBOOT_OUTLET';
    if (exists $ENV{$env_name}) {
        $self->{IBOOT_OUTLET} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'IBOOT_USER';
    if (exists $ENV{$env_name}) {
        $self->{IBOOT_USER} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'IBOOT_PASSWD';
    if (exists $ENV{$env_name}) {
        $self->{IBOOT_PASSWD} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'REBOOT_TIME';
    if (exists $ENV{$env_name}) {
        $self->{REBOOT_TIME} = $ENV{$env_name};
    } else {
        $self->{REBOOT_TIME} = 0;
    }
    $env_name = $env_prefix.'REBOOT';
    if (exists $ENV{$env_name}) {
        $self->{REBOOT} = $ENV{$env_name};
    } else {
        $self->{REBOOT} = 0;
    }
    $env_name = $env_prefix.'STARTUP_COMMAND';
    if (exists $ENV{$env_name}) {
        $self->{STARTUP_COMMAND} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'TELNET_HOST';
    if (exists $ENV{$env_name}) {
        $self->{TELNET_HOST} = $ENV{$env_name};
    } else {
        $self->{TELNET_HOST} = $self->{HOSTNAME};
    }
    $env_name = $env_prefix.'TELNET_PORT';
    if (exists $ENV{$env_name}) {
        $self->{TELNET_PORT} = $ENV{$env_name};
    } else {
        $self->{TELNET_PORT} = 23;
    }
    $env_name = $env_prefix.'HOST_ROOT';
    if (exists $ENV{$env_name}) {
        $self->{HOST_ROOT} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'SYSTEM_LIBS';
    if (exists $ENV{$env_name}) {
        $self->{SYSTEM_LIBS} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'REMOTE_SHELL';
    if (exists $ENV{$env_name}) {
        $self->{REMOTE_SHELL} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'LIBPATH';
    if (exists $ENV{$env_name}) {
        $self->{LIBPATH} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'REMOTE_FILETEST';
    if (exists $ENV{$env_name}) {
        $self->{REMOTE_FILETEST} = $ENV{$env_name};
    }
    $env_name = $env_prefix.'PS_CMD';
    if (exists $ENV{$env_name}) {
        $self->{PS_CMD} = $ENV{$env_name};
    }
    $self->{EXTRA_ENV} = {};
    $env_name = $env_prefix.'EXTRA_ENV';
    if (exists $ENV{$env_name}) {
        my @x_env = split (' ', $ENV{$env_name});
        foreach my $x_env_s (@x_env) {
          if ($x_env_s =~ /(\w+)=(.*)/) {
            $self->{EXTRA_ENV}->{$1} = $2;
          }
        }
    }
}

##################################################################

sub ACE_ROOT ($)
{
    my $self = shift;
    return $self->{ace_root};
}

sub TAO_ROOT ($)
{
    my $self = shift;
    return $self->{tao_root};
}

sub CIAO_ROOT ($)
{
    my $self = shift;
    return $self->{ciao_root};
}

sub HostName ($)
{
    my $self = shift;
    return $self->{HOSTNAME};
}

sub ExeSubDir ($)
{
    my $self = shift;
    my $new_val = shift;
    if (defined $new_val) {
        $self->{EXE_SUBDIR} = $new_val;
    }
    return $self->{EXE_SUBDIR};
}

sub GetArchDir
{
    my $self = shift;
    my $dir = shift;
    if (exists $self->{ARCH}) {
        return $dir . $self->{EXE_SUBDIR};
    }
    return $dir;
}


sub SystemLibs ($)
{
    my $self = shift;
    return $self->{SYSTEM_LIBS};
}

sub RandomPort ($)
{
    my $self = shift;
    return (int(rand($$)) % 22766) + 10002;
}

sub ProcessStartWaitInterval ($)
{
    my $self = shift;
    return $self->{PROCESS_START_WAIT_INTERVAL};
}

sub ProcessStopWaitInterval ($)
{
    my $self = shift;
    return $self->{PROCESS_STOP_WAIT_INTERVAL};
}

sub LocalFile ($)
{
    my $self = shift;
    my $file = shift;
    my $newfile = PerlACE::LocalFile($file);
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
        print STDERR "LocalFile for $file is $newfile\n";
    }
    return $newfile;
}

sub AddLibPath ($)
{
    my $self = shift;
    my $dir = shift;
    my $noarch = shift;

    # If we have -Config ARCH, use the -ExeSubDir setting as a sub-directory
    # of the lib path.  This is in addition to the regular LibPath.
    if (!$noarch && defined $self->{ARCH}) {
        $self->AddLibPath($dir, 1);
        $dir .= '/' . $self->{EXE_SUBDIR};
    }

    if ($self->ACE_ROOT () eq $ENV{'ACE_ROOT'}) {
        # add (relative) path without rebasing
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
            print STDERR "Adding libpath $dir\n";
        }
        $self->{LIBPATH} = PerlACE::concat_path ($self->{LIBPATH}, $dir);
    } else {
        # add rebased path
        $dir = PerlACE::rebase_path ($dir, $ENV{"ACE_ROOT"}, $self->ACE_ROOT ());
        if (defined $ENV{'ACE_TEST_VERBOSE'}) {
            print STDERR "Adding libpath $dir\n";
        }
        $self->{LIBPATH} = PerlACE::concat_path ($self->{LIBPATH}, $dir);
    }
}

sub SetEnv ($)
{
    my $self = shift;
    my $env_name = shift;
    my $env_value = shift;
    $self->{EXTRA_ENV}->{$env_name} = $env_value;
}

sub GetEnv ($)
{
    my $self = shift;
    my $env_name = shift;
    return $self->{EXTRA_ENV}->{$env_name};
}

sub DeleteFile ($)
{
    my $self = shift;
    my $file = shift;
    my $newfile = PerlACE::LocalFile($file);
    unlink ($newfile);
}

sub GetFile ($)
{
    # On local host, the file is already there.
    my $self = shift;
    my $remote_file = shift;
    my $local_file = shift;
    return 0;
}

# Put file from a to b
sub PutFile ($)
{
    my $self = shift;
    my $src = shift;
    my $dest = $self->LocalFile ($src);
    if ($src != $dest) {
        copy ($src, $dest);
    }
    return 0;
}

sub WaitForFileTimed ($)
{
    my $self = shift;
    my $file = shift;
    my $timeout = shift;
    my $newfile = $self->LocalFile($file);
    if (defined $self->{REMOTE_SHELL} && defined $self->{REMOTE_FILETEST}) {
      # If the target's config has a different ACE_ROOT, rebase the file
      # from $ACE_ROOT to the target's root.
      if ($self->ACE_ROOT () ne $ENV{'ACE_ROOT'}) {
        $file = File::Spec->rel2abs($file);
        $file = File::Spec->abs2rel($file, $ENV{"ACE_ROOT"});
        $file = $self->{TARGET}->ACE_ROOT() . "/$file";
      }
      $timeout *= $PerlACE::Process::WAIT_DELAY_FACTOR;
      my $cmd = $self->{REMOTE_SHELL};
      if ($self->{REMOTE_FILETEST} =~ /^\d*$/) {
        $cmd .= " 'test -e $newfile && test -s $newfile ; echo \$?'";
      } else {
        $cmd .= $self->{REMOTE_FILETEST} . ' ' . $file;
      }
      my $rc = 1;
      while ($timeout-- != 0) {
        $rc = int(`$cmd`);
        if ($rc == 0) {
          return 0;
        }
        sleep 1;
      }
      return -1;
    } else {
      return PerlACE::waitforfile_timed ($newfile, $timeout);
    }
}

sub CreateProcess ($)
{
    my $self = shift;
    my $process = new PerlACE::Process (@_);
    $process->Target($self);
    return $process;
}

# Don't need to do anything in most cases.
sub GetStderrLog ($)
{
    my $self = shift;
    return;
}

sub KillAll ($)
{
    my $self = shift;
    my $procmask = shift;
    PerlACE::Process::kill_all ($procmask, $self);
}

1;
