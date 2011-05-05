#! /usr/bin/perl
# $Id: Process.pm 83679 2008-11-12 09:31:17Z johnnyw $

package PerlACE::Process;

use strict;
use English;
use POSIX qw(:time_h);

$PerlACE::Process::ExeSubDir = './';

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

### Check for -ExeSubDir commands, store the last one
my @new_argv = ();

for(my $i = 0; $i <= $#ARGV; ++$i) {
    if ($ARGV[$i] eq '-ExeSubDir') {
        if (defined $ARGV[$i + 1]) {
            $PerlACE::Process::ExeSubDir = $ARGV[++$i].'/';
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

$PerlACE::Process::WAIT_DELAY_FACTOR = (defined $ENV{"ACE_RUNTEST_DELAY"}) ? $ENV{"ACE_RUNTEST_DELAY"} : 1;

# Set the process's target. If there's none, behavior falls back to pre-target
# behavior.
sub Target($)
{
    my $self = shift;
    $self->{TARGET} = shift;
}

if ($OSNAME eq "MSWin32") {
	require PerlACE::Process_Win32;
}
elsif ($OSNAME eq "VMS") {
        require PerlACE::Process_VMS;
}
else {
	require PerlACE::Process_Unix;
}

1;
