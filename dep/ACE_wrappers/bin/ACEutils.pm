# $Id: ACEutils.pm 91813 2010-09-17 07:52:52Z johnnyw $

require Process;
$EXEPREFIX = ".".$DIR_SEPARATOR;
$TARGETHOSTNAME = "localhost";

package ACE;

sub CheckForExeDir
{
  for($i = 0; $i <= $#ARGV; $i++) {
    if ($ARGV[$i] eq '-ExeSubDir') {
      if (defined $ARGV[$i + 1]) {
        $::EXEPREFIX = $ARGV[$i + 1].$::DIR_SEPARATOR;
      }
      else {
        print STDERR "You must pass a directory with ExeSubDir\n";
        exit(1);
      }
      splice(@ARGV, $i, 2);
    }
  }
}


### Check and remove, but don't actually use
sub CheckForConfig
{
  for($i = 0; $i <= $#ARGV;) {
    if ($ARGV[$i] eq '-Config') {
      if (!defined $ARGV[$i + 1]) {
        print STDERR "You must pass a configuration with Config\n";
        exit(1);
      }
      splice(@ARGV, $i, 2);
    } else {
      $i++;
    }
  }
}

sub checkForTarget
{
  my($cwd) = shift;

  for($i = 0; $i <= $#ARGV; $i++) {
    if ($ARGV[$i] eq '-chorus') {
      if (defined $ARGV[$i + 1]) {
        $::TARGETHOSTNAME = $ARGV[$i + 1];
        $::EXEPREFIX = "rsh $::TARGETHOSTNAME arun $cwd$::DIR_SEPARATOR";
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
}


# Returns a unique id, uid for unix, last digit of IP for NT
sub uniqueid
{
  if ($^O eq "MSWin32")
  {
    my $uid = 1;

    open (IPNUM, "ipconfig|") || die "Can't run ipconfig: $!\n";

    while (<IPNUM>)
    {
      if (/Address/)
      {
        $uid = (split (/: (\d+)\.(\d+)\.(\d+)\.(\d+)/))[4];
      }
    }

    close IPNUM;

    return $uid;
  }
  else
  {
    return getpwnam (getlogin ());
  }
}

# Waits until a file exists
sub waitforfile
{
  local($file) = @_;
  sleep 1 while (!(-e $file && -s $file));
}

sub waitforfile_timed
{
  my $file = shift;
  my $maxtime = shift;
  while ($maxtime-- != 0) {
    if (-e $file && -s $file) {
      return 0;
    }
    sleep 1;
  }
  return -1;
}

$sleeptime = 5;

CheckForExeDir ();
CheckForConfig ();

1;
