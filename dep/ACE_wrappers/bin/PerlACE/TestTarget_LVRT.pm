#! /usr/bin/perl
# $Id: TestTarget_LVRT.pm 89840 2010-04-12 09:36:32Z mcorino $
#
# TestTarget_LVRT - how to manage the test environment on a LabVIEW RT target.
#
# We can FTP files to and from the LabVIEW target, but there's no NFS or
# SMB shares.
# Most information about the target itself is specified via environment
# variables. Environment variables with settings are named using the target's
# config name with a specific suffix. The current environment variables are:
#   <config-name>_IPNAME - the host name/IP of the target.
#   <config-name>_CTLPORT- the TCP port number to connect to for the test
#                          controller. If this is not set, port 8888 is used.
#   <config-name>_FSROOT - the root of the filesystem on the target where
#                          ACE files will be created from (cwd, if you will).
#                          If this is not set, "\ni-rt" is used as the root.
#
# Each of these settings are stored in a member variable of the same name in
# each object. The process objects can access them using, e.g.,
# $self->{TARGET}->{IPNAME}.
#
# This class also makes an FTP object available to process objects that are
# created. FTP is set up before creating a process object and can be used to
# transfer files to and from the LVRT target.

package PerlACE::TestTarget_LVRT;
our @ISA = "PerlACE::TestTarget";

### Constructor and Destructor

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

    $env_name = $config_name.'_CTLPORT';
    if (exists $ENV{$env_name}) {
        $self->{CTLPORT} = $ENV{$env_name};
    }
    else {
        print STDERR "Warning: no $env_name variable; falling back to ",
                     "port 8888\n";
        $self->{CTLPORT} = 8888;
    }

    $env_name = $config_name.'_FSROOT';
    my $fsroot = '\\ni-rt\\system';
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

    $self->{FTP} = new Net::FTP ($targethost);
    $self->{IPNAME} = $targethost;
    if (!defined $self->{FTP}) {
        print STDERR "Error opening FTP to $targethost: $@\n";
        $self->{REBOOT_NEEDED} = 1;
        undef $self;
        return undef;
    }
    $self->{FTP}->login("","");

    return $self;
}

sub DESTROY
{
    my $self = shift;

    # Reboot if needed; set up clean for the next test.
    if (defined $self->{REBOOT_NEEDED} && $self->{REBOOT_CMD}) {
        $self->RebootNow;
    }

    # See if there's a log; should be able to retrieve it from rebooted target.
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print STDERR "LVRT target checking for remaining log...\n";
    }
    $self->GetStderrLog();
    if (defined $self->{FTP}) {
        $self->{FTP}->close;
        $self->{FTP} = undef;
    }
}

##################################################################

sub LocalFile ($)
{
    my $self = shift;
    my $file = shift;
    my $newfile = $self->{FSROOT} . '\\' . $file;
    print STDERR "LVRT LocalFile for $file is $newfile\n";
    return $newfile;
}

sub DeleteFile ($)
{
    my $self = shift;
    $self->{FTP}->login("","");
    foreach my $file (@_) {
      my $newfile = $self->LocalFile($file);
      $self->{FTP}->delete($newfile);
    }
}

sub GetFile ($)
{
    # Use FTP to retrieve the file from the target; should still be open.
    # If only one name is given, use it for both local and remote (after
    # properly LocalFile-ing it). If both names are given, assume the caller
    # knows what he wants and don't adjust the paths.
    my $self = shift;
    my $remote_file = shift;
    my $local_file = shift;
    if (!defined $local_file) {
        $local_file = $remote_file;
        $remote_file = $self->LocalFile($local_file);
    }
    $self->{FTP}->ascii();
    if ($self->{FTP}->get($remote_file, $local_file)) {
        return 0;
    }
    return -1;
}

sub WaitForFileTimed ($)
{
    my $self = shift;
    my $file = shift;
    my $timeout = shift;
    my $newfile = $self->LocalFile($file);
    my $targetport = $self->{CTLPORT};
    my $target = new Net::Telnet(Errmode => 'return');
    if (!$target->open(Host => $self->{IPNAME}, Port => $targetport)) {
        print STDERR "ERROR: target $self->{IPNAME}:$targetport: ",
                      $target->errmsg(), "\n";
        return -1;
    }
    my $cmdline = "waitforfile $newfile $timeout";
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print "-> $cmdline\n";
    }
    $target->print("$cmdline");
    my $reply;
    # Add a small comms delay factor to the timeout
    $timeout = $timeout + 2;
    $reply = $target->getline(Timeout => $timeout);
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print "<- $reply\n";
    }
    $target->close();
    if ($reply eq "OK\n") {
        return 0;
    }
    return -1;
}

sub CreateProcess ($)
{
    my $self = shift;
    my $process = new PerlACE::ProcessLVRT ($self, @_);
    return $process;
}

sub GetStderrLog ($)
{
    my $self = shift;
    # Tell the target to snapshot the stderr log; if there is one, copy
    # it up here and put it out to our stderr.
    my $targetport = $self->{CTLPORT};
    my $target = new Net::Telnet(Errmode => 'return');
    if (!$target->open(Host => $self->{IPNAME}, Port => $targetport)) {
        print STDERR "ERROR: target $self->{IPNAME}:$targetport: ",
                      $target->errmsg(), "\n";
        return;
    }
    my $cmdline = "snaplog";
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
        print "-> $cmdline\n";
    }
    $target->print("$cmdline");
    my $reply;
    $reply = $target->getline();
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
        print "<- $reply\n";
    }
    $target->close();
    if ($reply eq "NONE\n") {
        return;
    }
    chomp $reply;
    if (undef $self->{FTP}) {
        $self->{FTP} = new Net::FTP ($self->{IPNAME});
        if (!defined $self->{FTP}) {
            print STDERR "$@\n";
            return -1;
        }
        $self->{FTP}->login("","");
    }
    $self->{FTP}->ascii();
    if ($self->{FTP}->get($reply, "stderr.txt")) {
        $self->{FTP}->delete($reply);
        open(LOG, "stderr.txt");
        while (<LOG>) {
            print STDERR;
        }
        close LOG;
        unlink "stderr.txt";
    }
    return;
}

# Copy a file to the target. Adjust for different types (DLL, EXE, TEXT)
# and debug/non (for DLLs). Additionally, a file can be removed when this
# object is deleted, or left in place.
sub NeedFile ($)
{
    my $self = shift;
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
    if (defined $self->{FTP}) {
        $self->{FTP}->close;
        $self->{FTP} = undef;
    }
    system ($self->{REBOOT_CMD});
    sleep ($self->{REBOOT_TIME});
}

# Reboot now then try to restore the FTP connection.
sub RebootReset ($)
{
    my $self = shift;
    $self->RebootNow;
    my $targethost = $self->{IPNAME};
    $self->{FTP} = new Net::FTP ($targethost);
    if (!defined $self->{FTP}) {
        print STDERR "Error reestablishing FTP to $targethost: $@\n";
    }
    else {
        $self->{FTP}->login("","");
    }
}

sub KillAll ($)
{
    my $self = shift;
    my $procmask = shift;
    PerlACE::ProcessLVRT::kill_all ($procmask, $self);
}

1;
