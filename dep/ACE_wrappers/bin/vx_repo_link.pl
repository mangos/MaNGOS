eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

#
# $Id: vx_repo_link.pl 80826 2008-03-04 14:51:23Z wotte $
#


use FindBin;
use lib $FindBin::Bin;

use diagnostics;
use Time::Local;
use File::Basename;
use File::Spec;
use FileHandle;
use Cwd;

my $VXDEBUG = (defined $ENV{'VX_REPO_LINK_DEBUG'}) ? $ENV{'VX_REPO_LINK_DEBUG'} : 0;
my $ace_root = $ENV{'ACE_ROOT'};
$ace_root =~ s/\//\\/g;
my $wind_base = $ENV{'WIND_BASE'};
$wind_base =~ s/\//\\/g;
my $AR = $ENV{'AR'};
my $RANLIB = $ENV{'RANLIB'};
my @lib_arr;
my $lib;
my $curdir = cwd();
$curdir =~ s/\//\\/g;


if ($VXDEBUG > 1)
{
  print STDERR "AR = ". $AR . "\n";
  print STDERR "ACE_ROOT = " . $ace_root . "\n";
  print STDERR "WIND_BASE = " . $wind_base, "\n";
  print STDERR "curdir = " . $curdir, "\n";
}

if ($VXDEBUG > 1) {
  print STDERR "LINKLIBS are:\n";
}

while ($#ARGV >= 0)
{
    if ($ARGV[0] =~ /^-l/) {
        $lib = shift;
        $lib =~ s/^-l//;
        if ($VXDEBUG > 1) {
          print STDERR $ace_root."/lib/lib".$lib.".a\n";
        }
        push @lib_arr, $lib;
    }
}

my @lib_registry = ();

my $LINKLIBS = "";

my $libobj_dir = ".lib";
system("if not exist ".$libobj_dir." mkdir ".$libobj_dir);
foreach $lib (@lib_arr) {
  my $libname = $ace_root."\\lib\\lib".$lib.".a";
  if (-e $libname) {
    if (! -e $libobj_dir."\\lib".$lib.".a") {
      if ($VXDEBUG) {
        print STDERR "Unpacking ".$lib."...\n";
      }
      system("cmd /c if exist ".$libobj_dir."\\".$lib." del /q/f ".$libobj_dir."\\".$lib."\\*.*");
      system("cmd /c (mkdir ".$libobj_dir."\\".$lib." && cd ".$libobj_dir."\\".$lib." && ".$AR." -x ".$libname." && cd ".$curdir.")");

      if ($VXDEBUG) {
        print STDERR "Processing ".$lib."...\n";
      }

      my @rpo_arr = glob ($libobj_dir."\\".$lib."\\*.rpo");

      if ($VXDEBUG > 1) {
        print STDERR "Read #".$#rpo_arr." filenames\n";
      }

      my $line;
      my $fh;
      my $newfh;
      my $module;
      foreach $rpo (@rpo_arr) {
        if ($VXDEBUG > 2) {
          print STDERR $rpo."\n";
        }
        $fh = new FileHandle();
        $newfh = new FileHandle();
        $module = basename($rpo, ".rpo");
        open ($newfh, '>', dirname($rpo)."\\".$module.".__rpo");
        open ($fh, '<', $rpo);
        while (defined ($line = readline $fh)) {
          if ($line =~ /^M /) {
            $line =~ s/\//\\/g;
          }
          elsif ($line =~ /^D /) {
            $line =~ s/(^D[ ])(.*ACE_wrappers)(.*)/${1}$ace_root${3}/;
            $line =~ s/\//\\/g;
          }
          elsif ($line =~ /^A /) {
            $line =~ s/('-I)([^ ]*ACE_wrappers)([']|[^']+')/${1}$ace_root${3}/g;
            $line =~ s/('-I)([^ ]*Tornado[^\/\\]+)([']|[^']+')/${1}$wind_base${3}/g;
            $line =~ s/('-o' )('.*')/'-o' '$curdir\/$libobj_dir\/$lib\/$module.o'/g;
            $line =~ s/\//\\/g;
          }
          print $newfh $line;
        }
        close ($fh);
        close ($newfh);

        system("cmd /c (del /f/q ".$rpo." && rename ".dirname($rpo)."\\".$module.".__rpo ".basename($rpo).")");
      }

      $fh = new FileHandle();
      open($fh, '<', $libobj_dir."\\".$lib."\\.prelink.spec");
      my $lnkcmd = readline($fh);
      my $arcmd = readline($fh);
      my $libdeps = readline($fh);
      close($fh);

      $lib_registry[++$#lib_registry] = {
        lib => $lib,
        linkcmd => $lnkcmd,
        arcmd => $arcmd,
        libdeps => (defined $libdeps ? $libdeps : '')
      };
    }

    $LINKLIBS = $LINKLIBS." ".$libobj_dir."\\lib".$lib.".a";
  }
  else {
    $LINKLIBS = "-l".$lib." ".$LINKLIBS;
  }
}

sub _find_lib_entry
{
  my $lib = shift;
  foreach my $reg (@lib_registry) {
    if ($reg->{lib} =~ /(^| )$lib( |$)/) {
      return $reg;
    }
  }
  return undef;
}

sub _prelink_lib
{
  my $lentry = shift;
  my $stack = shift;
  my $lib = $lentry->{lib};
  my $lnkcmd = $lentry->{linkcmd};
  my $arcmd = $lentry->{arcmd};
  my @deps = split(' ', $lentry->{libdeps});

  $lnkcmd =~ s/\n//g;
  $arcmd =~ s/\n//g;

  if ($VXDEBUG) {
    print STDERR "Prelink requested for ".$lib." (dependend on ".join(' ', @deps).")...\n";
  }

  # check dependencies
  foreach my $dep (@deps) {
    if ($VXDEBUG > 1) {
      print STDERR "  Checking dependency ".$dep."(stack = ".$stack.")...\n";
    }
    if (($stack !~ /\<$dep\>/) && !(-e $libobj_dir."\\lib".$dep.".a")) {
      my $le = _find_lib_entry ($dep);
      if (defined $le) {
        _prelink_lib ($le, $stack." <".$lib.">");
      }
    }
  }

  # prelink library
  if ($VXDEBUG) {
    print STDERR "Prelinking ".$lib."...\n";
  }

  my $objs = join(' ', glob ($libobj_dir."\\".$lib."\\*.o"));
  my $libs = join(' ', glob ($libobj_dir."\\*.a"));
  if ($VXDEBUG > 1) {
    print STDERR " > cmd /c ".$lnkcmd." ".$libobj_dir."\\.prelink_lib ".$objs." ".$libs."\n";
  }
  system("cmd /c ".$lnkcmd." ".$libobj_dir."\\.prelink_lib ".$objs." ".$libs);
  system("cmd /c del /f/q ".$libobj_dir."\\.prelink_lib");

  # build prelinked library
  if ($VXDEBUG) {
    print STDERR "Building prelinked lib ".$lib."...\n";
  }

  $arcmd =~ s/lib$lib/$libobj_dir\\lib$lib/;
  #$arcmd = $AR." r ".$libobj_dir."\\lib".$lib.".a";
  $arcmd =~ s/ rv / rc /g;
  if ($VXDEBUG > 1) {
    print STDERR "cmd /c echo ".$objs." | ".$arcmd."\n";
    print STDERR "cmd /c (".$RANLIB." ".$libobj_dir."\\lib".$lib.".a && rmdir /s/q ".$libobj_dir."\\".$lib.")"."\n";
  }
  system("cmd /c echo ".$objs." | ".$arcmd);
  system("cmd /c (".$RANLIB." ".$libobj_dir."\\lib".$lib.".a && rmdir /s/q ".$libobj_dir."\\".$lib.")");
}

foreach my $libentry (@lib_registry) {
  if (! -e $libobj_dir."\\lib".$libentry->{lib}.".a") {
    _prelink_lib ($libentry, "");
  }
}

print $LINKLIBS."\n";

1;
