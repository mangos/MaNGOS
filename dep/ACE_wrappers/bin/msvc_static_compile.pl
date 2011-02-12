# $Id: msvc_static_compile.pl 80826 2008-03-04 14:51:23Z wotte $
#   Win32 auto_compile script.
if (!$ENV{ACE_ROOT}) {
    $ACE_ROOT = getcwd ()."\\";
    warn "ACE_ROOT not defined, defaulting to ACE_ROOT=$ACE_ROOT";
}
else {
    $ACE_ROOT = $ENV{ACE_ROOT};
}

use lib "$ENV{ACE_ROOT}/bin";

use File::Find;
use PerlACE::Run_Test;
use Cwd;

@directories = ();

@ace_dirs = ("$ACE_ROOT\\ace",
             "$ACE_ROOT\\ACEXML\\common",
             "$ACE_ROOT\\ACEXML\\parser",
             "$ACE_ROOT\\ACEXML\\apps",
             "$ACE_ROOT\\ACEXML\\tests",
             "$ACE_ROOT\\ACEXML\\examples",
             "$ACE_ROOT\\apps",
             "$ACE_ROOT\\ASNMP",
             "$ACE_ROOT\\examples",
             "$ACE_ROOT\\netsvcs",
             "$ACE_ROOT\\performance-tests",
             "$ACE_ROOT\\protocols",
             "$ACE_ROOT\\tests",
             "$ACE_ROOT\\websvcs");

$debug = 0;
$verbose = 0;
$print_status = 0;
$Ignore_errors = 0;              # By default, bail out if an error occurs.
$Build_LIB = 0;
$Build_Debug = 0;
$Build_Release = 0;
$Build_All = 1;
$build_core_only = 0;
$Build_Cmd = "/BUILD";
$use_custom_dir = 0;
$useenv = '';
$vc7 = 0;
$name_mod = '';
$mod_name = 0;
$proj_ext = '.dsp';

# Build
sub Build ($$)
{
  my ($project, $config) = @_;

  if ($debug == 1) {
    print "$project\n";
    return 0;
  }
  else {
    print "Auto_compiling $project : $config\n";

    print "Building $project $config\n" if $verbose;

    return system ("msdev.com $project /MAKE \"$config\" $Build_Cmd $useenv");
  }
}

# Build
sub Build_VC7 ($$)
{
  my ($project, $config) = @_;

  if ($debug == 1) {
    print "$project\n";
    return 0;
  }
  else {
    print "Auto_compiling $project : $config\n";

    print "Building $project $config\n" if $verbose;

    return system ("devenv.com $project $Build_Cmd $config $useenv");
  }
}

sub Find_Dsw (@)
{
    my (@dir) = @_;
    @array = ();

    sub wanted_dsw {
        $array[++$#array] =
            $File::Find::name if ($File::Find::name =~ /\.dsw$/i);
    }

    find (\&wanted_dsw, @dir);

    print "List of dsw's \n" if ($verbose == 1);
    return @array;
}

sub Find_Sln (@)
{
    my (@dir) = @_;
    @array = ();

    sub wanted_sln {
        $array[++$#array] =
            $File::Find::name if ($File::Find::name =~ /\.sln$/i);
    }

    find (\&wanted_sln, @dir);

    print "List of sln's \n" if ($verbose == 1);
    return @array;
}

sub Rename_Files ($$)
{
  my ($target) = shift;
  my ($newext) = shift;
  my (@array)  = ();

  sub wanted_file {
    my ($text) = shift;
    my ($next) = shift;
    if ($File::Find::name =~ /^(.*)$text$/i) {
      my ($newname) = $1 . $next;
      rename ($File::Find::name, $newname);
    }
  }

  find (sub { wanted_file ($target, $newext) }, $ACE_ROOT);
}

# Only builds the core libraries.
sub Build_Core ()
{
    print STDERR "Building Core of ACE/TAO\n" if ($print_status == 1);
    print "\nmsvc_static_compile: Building Core of ACE/TAO\n";

    print "Build \n" if ($verbose);
    print "Debug " if ($verbose) && ($Build_Debug);
    print "Release " if ($verbose) && ($Build_Release);
    print "LIB " if ($verbose) && ($Build_LIB);
    print "\n" if ($verbose);

    my @core_list = ();

    if ($Build_LIB) {
      push (@file_list, "/bin/msvc_static_order.lst");

      foreach my$test_lst (@file_list) {
        my $config_list = new PerlACE::ConfigList;
        $config_list->load ($ACE_ROOT.$test_lst);

        foreach $test ($config_list->valid_entries ()) {
          if ($mod_name) {
            @plist = split (/\//, $test);
            $fname = pop @plist;
            $fname_mod = $name_mod;
            $fname_mod =~ s/\*/$fname/;
	    push @plist,($fname_mod);
            push (@core_list, join('/', @plist) . $proj_ext);
          }
          else {
            push (@core_list, $test . $proj_ext);
          }
        }
      }


      if ( $vc7 ) {
          foreach $c (@core_list) {
              if ($Build_Debug) {
                  $Status = Build_VC7 ($c, "debug");
                  return if $Status != 0 && !$Ignore_errors;
              }
              if ($Build_Release) {
                  $Status = Build_VC7 ($c, "release");
                  return if $Status != 0 && !$Ignore_errors;
              }
          }
      }
      else {
          foreach $c (@core_list) {
              if ($Build_Debug) {
                  $Status = Build ($c, "ALL - Win32 Debug");
                  return if $Status != 0 && !$Ignore_errors;
              }
              if ($Build_Release) {
                  $Status = Build ($c, "ALL - Win32 Release");
                  return if $Status != 0 && !$Ignore_errors;
              }
          }
      }
    }
}

sub Build_All ()
{
    my @configurations = Find_Dsw (@directories);

    print STDERR "Building selected projects\n" if ($print_status == 1);
    print "\nmsvc_static_compile: Building selected projects\n";

    $count = 0;
    foreach $c (@configurations) {
        print STDERR "Configuration ".$count++." of ".$#configurations."\n" if ($print_status == 1);
        if ($Build_Debug) {
            $Status = Build ($c, "ALL - Win32 Debug");
            return if $Status != 0 && !$Ignore_errors;
        }
        if ($Build_Release) {
            $Status = Build ($c, "ALL - Win32 Release");
            return if $Status != 0 && !$Ignore_errors;
        }
    }
}

sub Build_All_VC7 ()
{
    my @configurations = Find_Sln (@directories);

    print STDERR "Building selected projects\n" if ($print_status == 1);
    print "\nmsvc_static_compile: Building selected projects\n";

    $count = 0;
    foreach $c (@configurations) {
        print STDERR "Configuration ".$count++." of ".$#configurations."\n" if ($print_status == 1);
        if ($Build_Debug) {
            $Status = Build_VC7 ($c, "debug");
            return if $Status != 0 && !$Ignore_errors;
        }
        if ($Build_Release) {
            $Status = Build_VC7 ($c, "release");
            return if $Status != 0 && !$Ignore_errors;
        }
    }
}


## Parse command line argument
while ( $#ARGV >= 0  &&  $ARGV[0] =~ /^(-|\/)/ )
{
    if ($ARGV[0] =~ '-k') {             # Ignore errors
        print "Ignore errors\n" if ( $verbose );
        $Ignore_errors = 1;
    }
    elsif ($ARGV[0] =~ /^-d$/i) {       # debug
        $debug = 1;
    }
    elsif ($ARGV[0] =~ '-vc7') {    # Use VC7 project and solution files.
        print "Using VC7 files\n" if ( $verbose );
        $vc7 = 1;
        $proj_ext = '.vcproj';
    }
    elsif ($ARGV[0] =~ '-vc8') {    # Use VC8 project and solution files.
        print "Using VC8 files\n" if ( $verbose );
        $vc7 = 1; # VC8 is like VC7
        $proj_ext = '.vcproj';
    }
    elsif ($ARGV[0] =~ '-vc9') {    # Use VC9 project and solution files.
        print "Using VC9 files\n" if ( $verbose );
        $vc7 = 1; # VC9 is like VC7
        $proj_ext = '.vcproj';
    }
    elsif ($ARGV[0] =~ '-v') {          # verbose mode
        $verbose = 1;
    }
    elsif ($ARGV[0] =~ '-name_modifier') {          # use MPC name_modifier for project
        shift;
        print "Setting name_modifier $ARGV[0]\n" if ( $verbose );
        $name_mod = $ARGV[0];
        $mod_name = 1;
    }
    elsif ($ARGV[0] =~ '-s') {          # status messages
        $print_status = 1;
    }
    elsif ($ARGV[0] =~ '-u') {          # USEENV
        print "Using Environment\n" if ($verbose);
        $useenv = '/USEENV';
    }
    elsif ($ARGV[0] =~ '-CORE') {       # Build the core of ace/tao
        print "Building only Core\n" if ( $verbose );
        $build_core_only = 1;
    }
    elsif ($ARGV[0] =~ '-ACE') {       # Build ACE and its programs
        print "Building ACE\n" if ( $verbose );
        $use_custom_dir = 1;
	push @directories, @ace_dirs;
    }
    elsif ($ARGV[0] =~ '-TAO') {       # Build TAO and its programs
        print "Building TAO\n" if ( $verbose );
        $use_custom_dir = 1;
	# Other tests depend on the lib in this dir so we need to force it
	# to the front of the build list. This is pretty ugly.
	push @directories, ("$ACE_ROOT\\TAO\\orbsvcs\\tests\\Notify\\lib");
	push @directories, ("$ACE_ROOT\\TAO");
    }
    elsif ($ARGV[0] =~ '-dir') {        # Compile only a specific directory
        shift;
        print "Adding directory $ARGV[0]\n" if ( $verbose );
        $use_custom_dir = 1;
        push @directories, $ARGV[0];
    }
    elsif ($ARGV[0] =~ '-rebuild') {    # Rebuild all
        print "Rebuild all\n" if ( $verbose );
        $Build_Cmd = "/REBUILD";
    }
    elsif ($ARGV[0] =~ '-clean') {      # Clean
        print "Cleaning all\n" if ( $verbose );
        $Build_Cmd = "/CLEAN";
    }
    elsif ($ARGV[0] =~ '-Debug') {      # Debug versions
        print "Building Debug Version\n" if ( $verbose );
        $Build_Debug = 1;
        $Build_All = 0;
    }
    elsif ($ARGV[0] =~ '-Release') {    # Release versions
        print "Building Release Version\n" if ( $verbose );
        $Build_Release = 1;
        $Build_All = 0;
    }
    elsif ($ARGV[0] =~ '-LIB') {        # Build LIB only
        print "Build LIB only\n" if ( $verbose );
        $Build_LIB = 1;
        $Build_All = 0;
    }
    elsif ($ARGV[0] =~ '-(\?|h)') {     # Help information
        print "Options\n";
        print "-d         = Debug (only print out projects)\n";
        print "-k         = Ignore Errors\n";
        print "-v         = Script verbose Mode\n";
        print "-s         = Print status messages to STDERR\n";
        print "-u         = Tell MSVC to use the environment\n";
        print "-vc7       = Use MSVC 7 toolset\n";
        print "-vc8       = Use MSVC 8 toolset\n";
        print "-name_modifier <mod> = Use MPC name_modifier to match projects\n";
        print "\n";
        print "-CORE      = Build the Core libraries\n";
        print "-ACE       = Build ACE and its programs\n";
        print "-TAO       = Build TAO and its programs\n";
        print "-dir <dir> = Compile custom directories\n";
        print "\n";
        print "-rebuild   = Rebuild All\n";
        print "-clean     = Clean\n";
        print "-Debug     = Compile Debug versions\n";
        print "-Release   = Compile Release versions\n";
        print "-LIB       = Comple LIB Configurations\n";
        exit;
    }
    else {
        warn "$0: error unknown option $ARGV[0]\n";
        die -1;
    }
    shift;
}

if (!$Build_DLL && !$Build_LIB) {
    $Build_DLL = 1;
    $Build_LIB = 1;
}

if (!$Build_Debug && !$Build_Release) {
    $Build_Debug = 1;
    $Build_Release = 1;
}

if ($#directories < 0) {
    @directories = ($ACE_ROOT);
}

print "msvc_static_compile: Begin\n";
print STDERR "Beginning Core Build\n" if ($print_status == 1);
if (!$use_custom_dir || $build_core_only) {
  if ($vc7) {
    ## devenv is too smart for it's own good.  When a .vcproj is specified,
    ## as is done when building the CORE, it will find the solution to which
    ## the .vcproj belongs and begin to build additional portions of the
    ## solution.  This is not what we want as dependencies are not set up
    ## between library projects.
    my($sln) = '.sln';
    my($core_sln) = $sln . '.build_core';

    Rename_Files ($sln, $core_sln);

    foreach my $sig ('INT', 'TERM') {
      $SIG{$sig} = sub { print STDERR "Renaming solution files, please be patient...\n";
                         Rename_Files ($core_sln, $sln);
                         exit(1); };
    }

    Build_Core ();

    Rename_Files ($core_sln, $sln);

    foreach my $sig ('INT', 'TERM') {
      $SIG{$sig} = 'DEFAULT';
    }
  }
  else {
    Build_Core ();
  }
}
print STDERR "Beginning Full Build\n" if ($print_status == 1);
if ( $vc7 ) {
    Build_All_VC7 if !$build_core_only;
}
else {
    Build_All if !$build_core_only;
}

print "msvc_static_compile: End\n";
print STDERR "End\n" if ($print_status == 1);
