eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}'
    & eval 'exec perl -w -S $0 $argv:q'
    if 0;

# ************************************************************
# Description   : Find macros in specified config files that
#                 are not referenced in other config files,
#                 but are referenced in the rest of the source
#                 files.
# Author        : Chad Elliott
# Create Date   : 12/22/2004
#            $Id: show_unused_macros.pl 80826 2008-03-04 14:51:23Z wotte $
# ************************************************************

# ************************************************************
# Pragmas
# ************************************************************

use strict;
use Cwd;
use FileHandle;
use File::Basename;

# ************************************************************
# Data Section
# ************************************************************

my($in_comment) = undef;

# ************************************************************
# Subroutine Section
# ************************************************************

sub getline {
  my($fh)   = shift;
  my($line) = $fh->getline();

  if (defined $line) {
    ## Remove the line feed
    $line =~ s/\n//;

    ## Remove one line c comments
    $line =~ s/\/\*.*\*\///;

    ## Check for multi lined c comments
    if ($line =~ s/\/\*.*//) {
      $in_comment = 1;
    }
    elsif ($line =~ s/.*\*\///) {
      $in_comment = 0;
    }
    elsif ($in_comment) {
      $line = '';
    }
    else {
      ## Remove c++ comments
      $line =~ s/\/\/.*//;

      ## Concatenate lines
      if ($line =~ s/\\\s*$//) {
        $line .= getline($fh);
      }
    }
  }

  return $line;
}


sub findMacros {
  my($defines) = shift;
  my($macros)  = shift;
  my(@files)   = @_;
  foreach my $file (@files) {
    my($fh) = new FileHandle();

    if (open($fh, $file)) {
      $in_comment = undef;
      while(defined($_ = getline($fh))) {
        if (($defines & 1) == 1 && /^\s*#\s*define\s*([^\s]+)/) {
          my($word) = $1;
          $word =~ s/\(.*//;
          if (!defined $$macros{$word}) {
            $$macros{$word} = $file;
          }
        }
        elsif (($defines & 2) == 2 && /^\s*#\s*if/) {
          foreach my $word (split(/[^\w]/, $_)) {
            if ($word =~ /^[^\d]\w+$/ &&
                $word !~ /^if([n]?def)?$/ &&
                $word !~ /^define[d]?/ &&
                $word !~ /^els(e|if)$/ && !defined $$macros{$word}) {
              $$macros{$word} = $file;
            }
          }
        }
      }

      close($fh);
    }
    else {
      print STDERR "Unable to open $file\n";
      exit(2);
    }
  }
}


sub usageAndExit {
  my($msg) = shift;

  if (defined $msg) {
    print STDERR "ERROR: $msg\n";
  }

  print STDERR 'Usage: ', basename($0), " [-I <directory>] <config headers>\n\n",
               "This script will provide a set of macros that may possibly\n",
               "be removed from ACE.\n\n",
               "It should be run under ACE_wrappers/ace and the input should\n",
               "be the config header file or files planned for removal.\n";
  exit(1);
}


# ************************************************************
# Main Section
# ************************************************************

my(@files) = ();
my(@dirs)  = ('.', 'os_include', 'os_include/sys',
              'os_include/netinet', 'os_include/net',
              'os_include/arpa',
             );

for(my $i = 0; $i <= $#ARGV; ++$i) {
   my($arg) = $ARGV[$i];
  if ($arg =~ /^-/) {
    if ($arg eq '-h') {
      usageAndExit();
    }
    elsif ($arg eq '-I') {
      ++$i;
      if (defined $ARGV[$i]) {
        push(@dirs, $ARGV[$i]);
      }
      else {
        usageAndExit('-I requires a directory parameter');
      }
    }
    else {
      usageAndExit("$arg is an unknown option");
    }
  }
  else {
    push(@files, $arg);
  }
}

if (!defined $files[0]) {
  usageAndExit();
}

## First find all of the control macros
my(%control) = ();
findMacros(3, \%control, @files);

## Now find all of the macros from the other config files
my(@other) = grep(!/config-all\.h|config-lite\.h/, <config-*.h>);

for(my $i = 0; $i <= $#other; ++$i) {
  foreach my $file (@files) {
    if ($other[$i] eq $file) {
      splice(@other, $i, 1);
      --$i;
      last;
    }
  }
}
my(%other) = ();
findMacros(3, \%other, @other);


my(%notreferenced) = ();
foreach my $macro (keys %control) {
  if (!defined $other{$macro}) {
    $notreferenced{$macro} = $control{$macro};
  }
}


## Find all other macros
my(@all) = ();
foreach my $dir (@dirs) {
  my($orig) = getcwd();
  if (chdir($dir)) {
    my(@more) =  <*.h *.i* *.cpp>;
    if ($dir ne '.') {
      foreach my $file (@more) {
        $file = "$dir/$file";
      }
    }
    push(@all, @more);
    chdir($orig);
  }
}

for(my $i = 0; $i <= $#all; ++$i) {
  foreach my $file (@files, @other) {
    if ($all[$i] eq $file) {
      splice(@all, $i, 1);
      --$i;
      last;
    }
  }
}

my(%all) = ();
findMacros(2, \%all, @all);

foreach my $macro (sort keys %notreferenced) {
  if (defined $all{$macro}) {
    print "$macro\n";
  }
}
