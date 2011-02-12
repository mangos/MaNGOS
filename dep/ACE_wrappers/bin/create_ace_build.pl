eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: create_ace_build.pl 80826 2008-03-04 14:51:23Z wotte $
#
# Creates an ACE build tree in directory "build/<build name>" below the $ACE_ROOT
# directory.  The build tree directory structure mirrors that of the ACE
# top level directory structure, except that instead of containing any plain
# files, it contains only links to the files in the ACE top level structure.
# Symbolic links will be used instead of hard links if available.
#
# Makefiles, projects, and other build files are not linked. This allows
# use of MPC to generate the correct project types as needed. Use the
# -nompc option to disable this.
#
# This program has a similar purpose to "clone", but in addition to
# only creating symlinks (clone creates hard links, by default), this
# script:
# 1) uses relative rather than absolute symlinks,
# 2) tries not to put junk files into the build tree,
# 3) only creates a new tree in a build/ directory below the current,
#    top level ACE directory (it's a feature :-), but it does enforce
#    consistency).
#
# This program can be re-run on a build tree at any time in order to
# update it.  It will add links for newly added files, and remove
# any that are no longer valid.
# Specifying no command line options will cause all current builds
# to be updated.
#
# If the <build name> starts with "build/", that part will be removed
# from it.

use File::Find ();
use File::Basename;
use FileHandle;
use File::stat;
use File::Copy;
use File::Path;

print "You should consider using clone_build_tree.pl found with MPC\n";

$usage = "usage: $0 -? | [-a] [-d <directory mode>] [-v] [-nompc] <build name>\n";
$directory_mode = 0777;   #### Will be modified by umask, also.
$verbose = 0;
$mpc = 1;   #### When using mpc, we don't want links created for mpc-generated files.
$update_all = 1;
$source='.';
$absolute = 0;

####
#### Check that we're in an ACE "top level" directory.
####
unless (-d 'ace' && -d 'include')  {
  die "Must be in an ACE top level (ACE_ROOT) directory!\n";
}
if (-e 'create_ace_build.links') {
  die "Must be in an ACE top level (ACE_ROOT) directory!\n";
}

$perl_version = $] + 0;
if ($perl_version >= 5) {
  #### Use an eval so that this script will compile with perl4.
  eval <<'PERL5_CWD'
  require Cwd;
  sub cwd {
    Cwd::getcwd ();
  }
PERL5_CWD
} else {
  sub cwd {
    local ($pwd);

    chop ($pwd = `pwd`);
    $pwd;
  }
}

my($starting_dir) = cwd ();
my(@nlinks)       = ();
my(@build_re)     = ();

print "Creating or updating builds in $starting_dir\n";

#### If the $linked file is newer than the real file then
#### backup the real file, and replace it with the linked
#### version.

sub backup_and_copy_changed {
  my($real, $linked) = @_;
  my($status_real) = stat($real);

  if (! $status_real) {
    die "ERROR: cannot access $real.\n";
  }

  my($status_linked) = stat($linked);
  if ($status_linked->mtime > $status_real->mtime) {
    rename($real, $real . '.bak');
    rename($linked, $real);
    return 1;
  }

  if ($status_real->mtime != $status_linked->mtime) {
    unlink($linked);
    return 1;
  }
  if ($status_real->size != $status_linked->size) {
    unlink($linked);
    return 1;
  }
  return 0;
}

sub cab_link {
  my($real,$linked,$build_regex) = @_;

  my($status) = 0;
  if ($^O eq 'MSWin32') {
    my($fixed) = $linked;
    $fixed =~ s/$build_regex//;
    push(@nlinks, $fixed);

    my($curdir) = "$starting_dir/" . dirname($linked);
    if (! -d $curdir) {
      die "ERROR: Dir not found: $curdir\n";
    }
    $status = chdir($curdir);
    if (! $status) {
      die "ERROR: cab_link() chdir " . $curdir . " failed.\n";
    }

    my($base_linked) = basename($linked);

    if (! -e $real) {
      ## If the real file "doesn't exist", then we need to change back to
      ## the starting directory and look up the short file name.
      chdir($starting_dir);
      my($short) = Win32::GetShortPathName($fixed);

      ## If we were able to find the short file name, then we need to
      ## modyfy $real.  Note, we don't need to change back to $curdir
      ## unless the short name lookup was successful.
      if (defined $short) {
        ## Replace a section of $real (the part that isn't a relative
        ## path) with the short file name.  The hard link will still have
        ## the right name, it's just pointing to the short name.
        substr($real, length($real) - length($fixed)) = $short;

        ## Get back to the right directory for when we make the hard link
        chdir($curdir);
      }
      else {
        ## This should never happen, but there appears to be a bug
        ## with the underlying win32 apis on Windows Server 2003.
        ## Long paths will cause an error which perl will ignore.
        ## Unicode versions of the apis seem to work fine.
        ## To experiment try Win32 _fullpath() and CreateHardLink with
        ## long paths.
        print "ERROR : Skipping $real.\n";
        return;
      }
    }

    if (-e $base_linked) {
      if (! backup_and_copy_changed($real, $base_linked)) {
        return;
      }
    }

    print "link $real $linked\n" if $verbose;
    $status = link ($real, $base_linked);
    if (! $status) {
      ## Once again, this happens for long paths on Win2003
      print "ERROR: Can't link $real\n";
      return;
    }
    chdir($starting_dir);
  } else {
    print "$symlink $real $linked\n" if $verbose;
    $status = symlink ($real, $linked);
  }
  if (!$status) {
    die "$0: $real -> $linked failed\n";
  }
}

####
#### Process command line args.
####
while ($#ARGV >= 0  &&  $ARGV[0] =~ /^-/) {
  if ($ARGV[0] eq '-v') {
    $verbose = 1;
  } elsif ($ARGV[0] eq '-d') {
    if ($ARGV[1] =~ /^\d+$/) {
      $directory_mode = eval ($ARGV[1]); shift;
    } else {
      warn "$0:  must provide argument for -d option\n";
      die $usage;
    }
  } elsif ($ARGV[0] eq '-a' && ! ($^O eq 'MSWin32')) {
    $source = &cwd ();
    $absolute = 1;
  } elsif ($ARGV[0] =~ /-[?hH]$/) {
    die "$usage";
  } elsif ($ARGV[0] eq '-nompc') {
    $mpc = 0;
  } else {
    warn "$0:  unknown option $ARGV[0]\n";
    die $usage;
  }
  shift;
}

@builds = ();

if ($#ARGV == 0) {
  $update_all = 0;
  $builds[0] = $ARGV[0];
  $builds[0] =~ s%^build[/\\]%%;        #### remove leading "build/", if any
  $builds[0] = "build/$builds[0]";
} else {
  @builds = glob "build/*";
}

sub create_build_regex {
  if ($^O eq 'MSWin32') {
    for ($idx = 0; $idx <= $#builds; $idx++) {
      ## Get the original build name
      $build_re[$idx] = $builds[idx];

      ## Remove any trailing slashes
      $build_re[$idx] =~ s/[\\\/]+$//;

      ## Add a single trailing slash
      $build_re[$idx] .= '/';

      ## Escape any special characters
      $build_re[$idx] =~ s/([\\\$\[\]\(\)\.])/\\$1/g;
    }
  }
}

create_build_regex();

# all builds go in ACE_wrappers\build
unless (-d "$starting_dir/build") {
  print "Creating $starting_dir/build\n";
  mkdir ("$starting_dir/build", $directory_mode);
}
foreach $build (@builds) {
  unless (-d "$starting_dir/$build") {
    print "Creating $starting_dir/$build\n";
    mkpath ("$starting_dir/$build", 0, $directory_mode);
  }
}

####
#### Get all ACE plain file and directory names.
####
@files = ();

sub wanted {
    my ($dev,$ino,$mode,$nlink,$uid,$gid);

    $matches = ! (
    /^CVS\z/s && ($File::Find::prune = 1)
    ||
    /^build\z/s && ($File::Find::prune = 1)
    ||
    /^\..*obj\z/s && ($File::Find::prune = 1)
    ||
    /^Templates\.DB\z/s && ($File::Find::prune = 1)
    ||
    /^Debug\z/s && ($File::Find::prune = 1)
    ||
    /^Release\z/s && ($File::Find::prune = 1)
    ||
    /^Static_Debug\z/s && ($File::Find::prune = 1)
    ||
    /^Static_Release\z/s && ($File::Find::prune = 1)
    ||
    /^\.svn\z/s && ($File::Find::prune = 1)
    );

    $matches = $matches &&
    (
        ($nlink || (($dev,$ino,$mode,$nlink,$uid,$gid) = lstat($_))) &&
        ! -l $_ &&
        ! /^core\z/s &&
        ! /^.*\.state\z/s &&
        ! /^.*\.so\z/s &&
        ! /^.*\.[oa]\z/s &&
        ! /^.*\.dll\z/s &&
        ! /^.*\.lib\z/s &&
        ! /^.*\.obj\z/s &&
        ! /^.*~\z/s &&
        ! /^\.\z/s &&
        ! /^\.#.*\z/s &&
        ! /^.*\.log\z/s
    );

    if ($mpc && $matches) {
      $matches =
        ($File::Find::dir =~ /include\/makeinclude*/) ||
        (
        ! /^.*\.dsp\z/s &&
        ! /^.*\.vcproj\z/s &&
        ! /^.*\.bor\z/s &&
        ! /^.*\.dsw\z/s &&
        ! /^.*\.sln\z/s &&
        ! /^.*\.vcp\z/s &&
        ! /^.*\.nmake\z/s &&
        ! /^.*\.am\z/s &&
        ! /^.*\.vcw\z/s &&
        ! /^.*\.mak\z/s &&
        ! /^.*\.bld\z/s &&
        ! /^.*\.icc\z/s &&
        ! /^.*\.icp\z/s &&
        ! /^.*\.ncb\z/s &&
        ! /^.*\.opt\z/s &&
        ! /^.*\.bak\z/s &&
        ! /^.*\.ilk\z/s &&
        ! /^.*\.pdb\z/s &&
        ! /^\.cvsignore\z/s &&
        ! /^\.disable\z/s &&
        ! /^GNUmakefile.*\z/s
      );
    }

    if ($matches) {
      push(@files, $File::Find::name);
    }
}

File::Find::find({wanted => \&wanted}, '.');

print "Found $#files files and directories.\n";

####
#### Create directories and symlinks to files.
####
foreach $file (@files) {
  $file =~ s%^./%%g;  #### excise leading ./ directory component
  my($fullname) = "$starting_dir/$file";
  for ($idx = 0; $idx <= $#builds; $idx++) {
    my($build) = $builds[$idx];
    if (-d $fullname) {
      unless (-d "$starting_dir/$build/$file") {
        print "Creating $build/$file\n" if $verbose;
        mkdir ("$starting_dir/$build/$file", $directory_mode);
      }
    } else {
      unless (($^O ne 'MSWin32') && (-e "$build/$file")) {
        if (!$absolute) {
          $up = '..';
          while ($build =~ m%/%g) {
            $up .= '/..';
          }
          while ($file =~ m%/%g) {
            $up .= '/..';
          }
          cab_link("$up/$file", "$build/$file", $build_re[$idx]);
        } else {
          $path = $source . '/' . $file;
          cab_link("$path", "$build/$file", $build_re[$idx]);
        }

      }
    }
  }
}

print "Finished creating and updating links.\n";

foreach $build (@builds) {
  ####
  #### Find all the symlinks in the build directory, and remove ones
  #### that are no longer actually linked to a file.
  ####

  if ($^O eq 'MSWin32') {
    my($lfh) = new FileHandle();
    my($links_file) = "$starting_dir/$build/create_ace_build.links";
    if (-e $links_file) {
      if (open($lfh, $links_file)) {
        while(<$lfh>) {
          my($line) = $_;
          $line =~ s/\s+$//;
          if (-e "$starting_dir/$line") {
            ## The links were already added in cab_link when they
            ## were checked for changes.
          } else {
            print "Removing $build/$line \n" if $verbose;
            unlink("$starting_dir/$build/$line") || warn "$0: unlink of $build/$line failed\n";
          }
        }
        close($lfh);
      }
      unless (unlink($links_file)) {
        die "Couldn't delete links file.\n";
      }
    }
    print "Writing $#nlinks links to link file.\n";
    if (open($lfh, ">$links_file")) {
      foreach my $lnk (@nlinks) {
        print $lfh "$lnk\n";
      }
      close($lfh);
    } else {
      die "Couldn't open links file.\n";
    }
  }
  else {
    @lfiles = ();

    sub lcheck {
      ## There's no way to know if we have hard linked back to a now
      ## non-existent file.  So, just do the normal -l on the file
      ## which will cause no files to be pushed on Windows.
      if (-l $_) {
        push(@lfiles, $File::Find::name);
      }
    }

    File::Find::find({wanted => \&lcheck}, $build);

    foreach (@lfiles) {
      local @s = stat $_;
      if ($#s == -1) {
        print "Removing $_ \n" if $verbose;
        unlink $_  ||  warn "$0: unlink of $_ failed\n";
      }
    }
  }

  ####
  #### Done: print message.
  ####
  print "\nCompleted creation of $build/.\n";

foreach $build (@builds) {
  unless (-d "$starting_dir/$build") {
    print "Creating $starting_dir/$build\n";
    mkdir ("$starting_dir/$build", $directory_mode);
  }


  if (! -e "$starting_dir/$build/ace/config.h") {
    print "Be sure to setup $build/ace/config.h";
  }

  if ($^O ne 'MSWin32' &&
      ! -e "$starting_dir/$build/include/makeinclude/platform_macros.GNU") {
    print " and\n$build/include/makeinclude/platform_macros.GNU";
  }
  print ".\n";
}

}

#### EOF
