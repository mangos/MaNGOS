eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# ******************************************************************
#      Author: Chad Elliott
#        Date: 6/18/2002
#         $Id: cle.pl 89793 2010-04-07 16:28:13Z mitza $
# ******************************************************************

use strict;
use Cwd;
use File::Basename;
use Sys::Hostname;

if ( $^O eq 'VMS' ) {
  require VMS::Filespec;
  import VMS::Filespec qw(unixpath);
}

unshift(@INC, getExecutePath($0) . 'ChangeLogEditor');

require ChangeLogEdit;
require EmailTranslator;

# ************************************************************
# Subroutine Section
# ************************************************************

sub which {
  my($prog)   = shift;
  my($exec)   = $prog;
  my($part)   = '';
  if ( $^O eq 'VMS' ) {
    my($envSep) = ';';
    if (defined $ENV{'PATH'}) {
      foreach $part (split(/$envSep/, $ENV{'PATH'})) {
        $part .= "$prog";
        if ( -x $part ) {
          $exec = $part;
          last;
        }
      }
    }
  }
  else  {
    my($envSep) = ($^O eq 'MSWin32' ? ';' : ':');
    if (defined $ENV{'PATH'}) {
      foreach $part (split(/$envSep/, $ENV{'PATH'})) {
        $part .= "/$prog";
        if ( -x $part ) {
          $exec = $part;
          last;
        }
      }
    }
  }

  return $exec;
}


sub getExecutePath {
  my($prog) = shift;
  my($loc)  = '';

  if ( $^O eq 'VMS' ) {
    if ($prog ne basename($prog)) {
      my($dir) = unixpath( dirname($prog) );
      if ($prog =~ /^[\/\\]/) {
        $loc = $dir;
      }
      else {
        $loc = unixpath(getcwd()) . $dir;
      }
    }
    else {
      $loc = unixpath( dirname(which($prog)) );
    }

    if ($loc eq '.') {
      $loc = unixpath( getcwd() );
    }
  } else {
    if ($prog ne basename($prog)) {
      if ($prog =~ /^[\/\\]/ ||
          $prog =~ /^[A-Za-z]:[\/\\]?/) {
        $loc = dirname($prog);
      }
      else {
        $loc = getcwd() . '/' . dirname($prog);
      }
    }
    else {
      $loc = dirname(which($prog));
    }

    $loc =~ s/\/\.$//;

    if ($loc eq '.') {
      $loc = getcwd();
    }

    if ($loc ne '') {
      $loc .= '/';
    }
  }

  return $loc;
}

sub getDefaultDomain {
  my($domain) = undef;
  my($host)   = hostname();

  if (defined $host) {
    ## First try the hostname
    if ($host =~ /[^\.]+\.(.*)/) {
      $domain = $1;
    }
    else {
      ## Next try the hosts file
      my($hosts) = ($^O eq 'MSWin32' ?
                      "$ENV{SystemRoot}/system32/drivers/etc/hosts" :
                      '/etc/hosts');
      my($fh)    = new FileHandle();
      if (open($fh, $hosts)) {
        while(<$fh>) {
          if (/$host\.([^\s]+)/) {
            $domain = $1;
            last;
          }
        }
        close($fh);
      }

      if (!defined $domain) {
        ## Next try ipconfig on Windows
        if ($^O eq 'MSWin32') {
          if (open($fh, 'ipconfig /all |')) {
            while(<$fh>) {
              if (/Primary\s+DNS\s+Suffix[^:]+:\s+(.*)/) {
                $domain = $1;
              }
              elsif (/DNS\s+Suffix\s+Search[^:]+:\s+(.*)/) {
                $domain = $1;
              }
            }
            close($fh);
          }
        }
        else {
          ## Try /etc/resolv.conf on UNIX
          if (open($fh, '/etc/resolv.conf')) {
            while(<$fh>) {
              if (/search\s+(.*)/) {
                $domain = $1;
                last;
              }
            }
            close($fh);
          }
        }
      }
    }
  }
  return $domain;
}


sub usageAndExit {
  my($arg)  = shift;
  my($base) = basename($0);
  if (defined $arg) {
    print "$arg\n\n";
  }
  print "Usage: $base [ChangeLog File] [user name] [email address]\n" .
        "        " . (' ' x length($base)) . "[-d <dir1 dir2 ... dirN>]\n\n" .
        "       Uses cvs to determine which files are modified or added\n" .
        "       and generates a bare ChangeLog entry based on those files.\n" .
        "       This script should be run at the same directory level in\n" .
        "       which the ChangeLog exists.  The entry is prepended to the\n" .
        "       existing ChangeLog.\n" .
        "\n" .
        "       Email addresses are generated with a certain set of\n" .
        "       defaults and can be modified using various environment\n" .
        "       variables.  By default email addresses are generated\n" .
        "       using the user last name followed by an underscore and\n" .
        "       the first initial of the user first name followed by the\n" .
        "       email domain.\n" .
        "\n" .
        "       REPLYTO      If this environment variable is set, the value\n" .
        "                    is used as the email address.\n" .
        "       CL_USERNAME  This environment variable is used to override\n" .
        "                    the user name (obtained from the password file).\n" .
        "       CL_CHANGELOG_FILE  This environment variable is used as the\n".
        "                          [ChangeLog File] argument if none is given\n".
        "                          on the command line.\n".
        "\n" .
        "       The user name and email address can be passed as a parameter to\n" .
        "       this script.  If either is not passed, then the script will try\n" .
        "       to determine it automatically.\n" .
        "\n" .
        "       If -d is used, everything on the command line after it is\n" .
        "       considered a directory or file to be considered in the\n" .
        "       ChangeLog entry.\n";
  exit(0);
}


# ************************************************************
# Subroutine Section
# ************************************************************

my($file)     = undef;
my($name)     = undef;
my($email)    = undef;
my(@dirs)     = ();
my($restdirs) = 0;

foreach my $arg (@ARGV) {
  if ($restdirs) {
    push(@dirs, $arg);
  }
  elsif ($arg eq '-h') {
    usageAndExit();
  }
  elsif ($arg eq '-d') {
    $restdirs = 1;
  }
  elsif ($arg =~ /^\-/) {
    usageAndExit("Unrecognized parameter: $arg");
  }
  elsif (!defined $file) {
    $file = $arg;
  }
  elsif (!defined $name) {
    $name = $arg;
  }
  elsif (!defined $email) {
    $email = $arg;
  }
}

if (!defined $file) {
  if (defined $ENV{CL_CHANGELOG_FILE}) {
    $file = $ENV{CL_CHANGELOG_FILE};
  }
  else {
    $file = 'ChangeLog';
  }
}
if (!defined $name) {
  my(@pwd)    = ();
  if (defined $ENV{CL_USERNAME}) {
    $pwd[6] = $ENV{CL_USERNAME};
  }
  else {
    if ($^O eq 'MSWin32' || $^O eq 'cygwin') {
      $pwd[6] = 'unknown';
    }
    else {
      @pwd = getpwuid($<);
      $pwd[6] =~ s/,//g;
    }
  }
  $name = $pwd[6];
}

if (!defined $email) {
  my($trans) = new EmailTranslator(getDefaultDomain());
  $email = $trans->translate($name);
}

my($editor) = new ChangeLogEdit($name, $email);
my($status, $error, $unknown) = $editor->edit($file, @dirs);

if (defined $unknown) {
  my(@uarray) = @$unknown;
  if ($#uarray >= 0) {
    print "WARNING: The following files are unknown to the ",
          "revision control system:\n";
    foreach my $unk (@uarray) {
      print "$unk\n";
    }
    print "\n";
  }
}

if ($status) {
  print "You are now ready to edit the $file.\n";
}
else {
  print "$error\n";
}

exit($status ? 0 : 1);
