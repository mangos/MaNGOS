eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: msvc_mpc_auto_compile.pl 91938 2010-09-22 18:43:23Z wotte $
#   Win32 auto_compile script.

use File::Find;
use Cwd;

if (!$ENV{ACE_ROOT}) {
    $ACE_ROOT = getcwd ()."\\";
    warn "ACE_ROOT not defined, defaulting to ACE_ROOT=$ACE_ROOT";
}
else {
    $ACE_ROOT = $ENV{ACE_ROOT};
}

@directories = ();


@ace_core_dirs = ("$ACE_ROOT\\ace",
                  "$ACE_ROOT\\Kokyu",
                  "$ACE_ROOT\\ACEXML",
                  "$ACE_ROOT\\examples",
                  "$ACE_ROOT\\tests",
                  "$ACE_ROOT\\protocols");

@orbsvcs_core_dirs = ("$ACE_ROOT\\TAO\\orbsvcs\\orbsvcs");

@dance_core_dirs = ("$ACE_ROOT\\TAO\\DAnCE");

@ciao_core_dirs = ("$ACE_ROOT\\TAO\\CIAO");

$debug = 0;
$verbose = 0;
$print_status = 0;
$Ignore_errors = 0;              # By default, bail out if an error occurs.
$Build_Debug = 0;
$Build_Release = 0;
$build_all = 0;
$Build_Cmd = "/BUILD";
$use_custom_dir = 0;
$useenv = '';
$vc7 = 0;

# Build_Config takes in a string of the type "project--configuration" and
# runs msdev to build it.
# sub Build_Config ($)
#{
#    my ($arg) = @_;
#    my ($project, $config) = split /--/, $arg;
#
#    return Build ($project, $config);
#}

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

# Only builds the core libraries.
sub Build_Custom ()
{
    print STDERR "Building Custom\n";
    print "Building Custom directories specified\n";# if ($verbose == 1);

    print "Build " if ($verbose);
    print "Debug " if ($verbose) && ($Build_Debug);
    print "Release " if ($verbose) && ($Build_Release);
    print "\n" if ($verbose);

    my @custom_list = Find_Dsw (@directories);

    print "List now is @custom_list \n";
    foreach $c (@custom_list) {
        print "List now is $c \n";
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

# Build all examples and directories
sub Build_All ()
{
    push @directories, @ace_core_dirs;
    push @directories, @orbsvcs_core_dirs;
    push @directories, @dance_core_dirs;
    push @directories, @ciao_core_dirs;

    print STDERR "First pass (libraries)\n" if ($print_status == 1);
    print "\nmsvc_auto_compile: First Pass CORE (libraries)\n";

    Build_Custom ();

    my @new_directory_search = "$ACE_ROOT";

    my @configurations = Find_Dsw (@new_directory_search);

    print STDERR "Second pass (for other things)\n" if ($print_status == 1);
    print "\nmsvc_mpc_auto_compile: Second  Pass (rest of the stuff)\n";

    foreach $c (@configurations) {
        print "\nUsing $c for compilation\n";
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


# Only builds the core libraries.
sub Build_Custom_VC7 ()
{
    print STDERR "Building Custom\n";
    print "Building Custom directories specified\n";# if ($verbose == 1);

    print "Build " if ($verbose);
    print "Debug " if ($verbose) && ($Build_Debug);
    print "Release " if ($verbose) && ($Build_Release);
    print "\n" if ($verbose);

    my @custom_list = Find_Sln (@directories);

    print "List now is @custom_list \n";
    foreach $c (@custom_list) {
        print "List now is $c \n";
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

# Build all examples and directories
sub Build_All_VC7 ()
{
    push @directories, @ace_core_dirs;
    push @directories, @orbsvcs_core_dirs;
    push @directories, @dance_core_dirs;
    push @directories, @ciao_core_dirs;

    print STDERR "First pass (libraries)\n" if ($print_status == 1);
    print "\nmsvc_auto_compile: First Pass CORE (libraries)\n";

    Build_Custom_VC7 ();

    my @new_directory_search = "$ACE_ROOT";

    my @configurations = Find_Sln (@new_directory_search);

    print STDERR "Second pass (for other things)\n" if ($print_status == 1);
    print "\nmsvc_mpc_auto_compile: Second  Pass (rest of the stuff)\n";

    foreach $c (@configurations) {
        print "\nUsing $c for compilation\n";
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
    }
    elsif ($ARGV[0] =~ '-vc8') {    # Use VC8 project and solution files.
        print "Using VC8 files\n" if ( $verbose );
        $vc7 = 1; # vc8 is like vc7
    }
    elsif ($ARGV[0] =~ '-vc9') {    # Use VC9 project and solution files.
        print "Using VC9 files\n" if ( $verbose );
        $vc7 = 1; # vc9 is like vc7
    }
    elsif ($ARGV[0] =~ '-v') {          # verbose mode
        $verbose = 1;
    }
    elsif ($ARGV[0] =~ '-s') {          # status messages
        $print_status = 1;
    }
    elsif ($ARGV[0] =~ '-u') {          # USEENV
        print "Using Environment\n" if ($verbose);
        $useenv = '/USEENV';
    }
    elsif ($ARGV[0] =~ '-ACE') {# Build ACE and its tests
        print "Building ACE\n" if ( $verbose );
        $use_custom_dir = 1;
        push @directories, @ace_core_dirs;
    }
    elsif ($ARGV[0] =~ '-TAO') {# Build TAO and its tests
        print "Building TAO\n" if ( $verbose );
        $use_custom_dir = 1;
        push @directories, @ace_core_dirs;
    }
    elsif ($ARGV[0] =~ '-ORBSVCS') {# Build TAO/ORBSVCS and its tests
        print "Building ACE+TAO+orbsvcs\n" if ( $verbose );
        $use_custom_dir = 1;
        push @directories, @ace_core_dirs;
        push @directories, @orbsvcs_core_dirs;
    }
    elsif ($ARGV[0] =~ '-CIAO') {# Build the CIAO and related
                                 # libraries
        print "Building only CIAO\n" if ( $verbose );
        $use_custom_dir = 1;
        push @directories, @ace_core_dirs;
        push @directories, @orbsvcs_core_dirs;
        push @directories, @dance_core_dirs;
        push @directories, @ciao_core_dirs;
    }
    elsif ($ARGV[0] =~ '-ALL') {# Build the CIAO and related
                                 # libraries
        print "Building ALL \n" if ( $verbose );
        $build_all = 1;
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
    }
    elsif ($ARGV[0] =~ '-Release') {    # Release versions
        print "Building Release Version\n" if ( $verbose );
        $Build_Release = 1;
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
        print "-vc9       = Use MSVC 9 toolset\n";
        print "\n";
        print "-ACE       = Build ACE and its tests\n";
        print "-TAO       = Build ACE+TAO and its tests\n";
        print "-ORBSVCS   = Build ACE+TAO+ORBSVCS and its tests\n";
        print "-CIAO      = Build ACE+TAO+ORBSVCS+CIAO and its tests\n";
        print "-dir <dir> = Compile custom directories\n";
        print "\n";
        print "-rebuild   = Rebuild All\n";
        print "-clean     = Clean\n";
        print "-Debug     = Compile Debug versions\n";
        print "-Release   = Compile Release versions\n";
        exit;
    }
    else {
        warn "$0: error unknown option $ARGV[0]\n";
        die -1;
    }
    shift;
}

if (!$Build_Debug && !$Build_Release) {
    $Build_Debug = 1;
    $Build_Release = 1;
}

print "MPC version of msvc_mpc_auto_compile: Begin\n";
if ($vc7) {
    Build_All_VC7 if ($build_all && !$use_custom_dir);
    Build_Custom_VC7 if $use_custom_dir;
}
else {
    Build_All if ($build_all && !$use_custom_dir);
    Build_Custom if $use_custom_dir;
}
print "msvc_mpc_auto_compile: End\n";
print STDERR "End\n" if ($print_status == 1);
