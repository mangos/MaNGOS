# $Id: pippen.pl 80826 2008-03-04 14:51:23Z wotte $

BEGIN {
    use Cwd;
    if (!$ENV{ACE_ROOT}) {
        $ACE_ROOT = getcwd ()."\\";
        print STDERR "Error: ACE_ROOT not defined\n";
        exit 1;
    }
    else {
        $ACE_ROOT = $ENV{ACE_ROOT};
    }
}
use lib "$ACE_ROOT/bin";
use PerlACE::MSProject::DSP;
use PerlACE::MSProject::VCP;
use File::DosGlob 'glob';
use DirHandle;
use strict;

################################################################################

my $extension;
my $recurse = 0;
my $list = 0;
my $verbose = 0;
my @arguments;
my @configs;
my @roots;
my $auto_compile = 0;
my $clean = 0;
my $debug = 0;

my $aceroot = 0;

################################################################################

# Parse command line arguments

while ( $#ARGV >= 0)
{
    if ($ARGV[0] =~ m/^-list/i) {
        $list = 1;
    }
    elsif ($ARGV[0] =~ m/^-evc3/i) {
        $extension = "vcp";
    }
    elsif ($ARGV[0] =~ m/^-msvc6/i) {
        $extension = "dsp";
    }
    elsif ($ARGV[0] =~ m/^-msvc7/i) {
        $extension = "vcproj";
    }
    elsif ($ARGV[0] =~ m/^-config/i) {
        push @configs, $ARGV[1];
        shift;
    }
    elsif ($ARGV[0] =~ m/^-r/i) {
        $recurse = 1;
    }
    elsif ($ARGV[0] =~ m/^-v/i) {
        $verbose = 1;
    }
    elsif ($ARGV[0] =~ m/^-auto_compile/i) {
        $auto_compile = 1;
    }
    elsif ($ARGV[0] =~ m/^-clean/i) {
        $clean = 1;
    }
    elsif ($ARGV[0] =~ m/^-useroot/i) {
        push @roots, $ARGV[1];
        shift;
    }
    elsif ($ARGV[0] =~ m/^-aceroot/i) {
        $aceroot = 1;
    }
    elsif ($ARGV[0] =~ m/^-(\?|h)/i) {     # Help information
        print "Options\n";
        print "-list          - Prints out the list of project files\n";
        print "-config <c>    - Use <c> as a configuratoin\n";
        print "-evc3          - Looks for eMbedded Visual C++ 3.0 projects\n";
        print "-msvc6         - Looks for Visual C++ 5.0/6.0 projects\n";
        print "-msvc7         - Looks for Visual C++ 7.0 projects\n";
        print "-clean         - Clean instead of building\n";
        print "-recurse       - Recurse into directories\n";
        print "-verbose       - Make some noise\n";
        print "-auto_compile  - Print out auto_compile info during build\n";
        print "-useroot <dir> - Use <dir> as a root to look for dependencies\n";
        print "-aceroot       - Use %ACE_ROOT% as a dependency root\n";
        exit;
    }
    elsif ($ARGV[0] =~ m/^-/) {
        warn "$0:  unknown option $ARGV[0]\n";
        exit 1;
    }
    else {
        push @arguments, $ARGV[0];
    }
    shift;
}

if ($#configs < 0) {
    if (defined $ENV{WINMAKE_CONFIGS}) {
        @configs = split /:/, $ENV{WINMAKE_CONFIGS};
    }
    elsif (defined $ENV{PIPPEN_CONFIGS}) {
        @configs = split /:/, $ENV{PIPPEN_CONFIGS};
    }
    else {
        print STDERR "Error: No config specified\n";
        exit 1;
    }
}

if (!defined $extension) {
    my $compiler = '';
    if (defined $ENV{WINMAKE_COMPILER}) {
        $compiler = $ENV{WINMAKE_COMPILER};
    }
    elsif (defined $ENV{PIPPEN_COMPILER}) {
        $compiler = $ENV{PIPPEN_COMPILER};
    }
    else {
        print STDERR "Error: No compiler specified\n";
        exit 1;
    }

    if ($compiler eq "evc3") {
        $extension = "vcp";
    }
    elsif ($compiler eq "msvc6") {
        $extension = "dsp";
    }
    elsif ($compiler eq "msvc7") {
        $extension = "vcproj";
    }
}

################################################################################

# I like these variables

# %projects->{$file}->{BUILD} <- Are we supposed to build this file?
#                   ->{PROJ} <- MSProject object
#                   ->{CONFIGS}->{$config}->{DEPS} <- List of dependencies
#                                         ->{DONE} <- Have we compiled it yet?

my %projects;

# %names->{$output} <- points to the $file used in the above %projects

my %names;

################################################################################

# Expand all the files/directories passed in on the command line

sub ProjectSearch ($@)
{
    my $build = shift;
    my @targets = @_;

    while ($#targets >= 0) {
        my $target = $targets[0];
        if (-d $target) {
            print "    Reading Directory $target\n" if ($verbose);
            if ($recurse) {
                my $dh = new DirHandle ($target);

                if (defined $dh) {
                    foreach my $entry ($dh->read ()) {
                        if (-d "$target/$entry" && $entry ne "." && $entry ne "..") {
                            $entry =~ s/^.\\//; # / <- fix for color coding in devenv
                            push @targets, ($target . "\\". $entry);
                        }
                    }
                }
                else {
                    print STDERR "Error: Cannot read $target: $!\n";
                }
            }

            foreach my $t (glob ($target . "\\*." . $extension)) {
                print "    Adding project $t\n" if ($verbose);
                %projects->{$t}->{BUILD} = $build;
            }
        }
        else {
            foreach my $t (glob ($target)) {
                print "    Adding project $t\n" if ($verbose);
                %projects->{$t}->{BUILD} = $build;
            }
        }
        shift @targets;
    }
}

print "=== Expanding Command line Arguments\n" if ($verbose);

if ($#arguments < 0) {
    print "    No files specified, defaulting to \".\"\n" if ($verbose);
    push @arguments, (".");
}

ProjectSearch (1, @arguments);

print "=== Expanding Root Arguments\n" if ($verbose);

ProjectSearch (0, @roots);

if ($aceroot == 1) {
    my $oldrecurse = $recurse;
    $recurse = 1;
    my @aceroots = ($ENV{ACE_ROOT}."\\ace",
                    $ENV{ACE_ROOT}."\\apps\\gperf\\src",
                    $ENV{ACE_ROOT}."\\TAO\\TAO_IDL",
                    $ENV{ACE_ROOT}."\\TAO\\tao",
                    $ENV{ACE_ROOT}."\\TAO\\orbsvcs\\orbsvcs");
    ProjectSearch (0, @aceroots);
    $recurse = $oldrecurse;
}

################################################################################

# Read each project file to gather dependency and output information

print "=== Reading Project Files\n" if ($verbose);

foreach my $project (keys %projects) {
    my $proj;

    if ($project =~ m/\.dsp$/i) {
        $proj = new PerlACE::MSProject::DSP ($project);
    }
    elsif ($project =~ m/\.vcp$/i) {
        $proj = new PerlACE::MSProject::VCP ($project);
    }
    elsif ($project =~ m/\.vcproj$/i) {
        print STDERR "Error: MSVC7 not supported yet\n";
    }
    else {
        print STDERR "Error: Unrecognized file: $project\n";
    }

    print "    Loading $project:" if ($verbose);

    $proj->Load ();

    foreach my $config (@configs) {
        foreach my $proj_config ($proj->Configs ()) {
            if ($proj_config =~ m/\Q$config\E/i) {
                print " \"$proj_config\"" if ($verbose);
                my $name = $proj->DepOutputFile ($proj_config);

                %names->{lc $name} = $project;
                my @deps = split / /, $proj->Libs ($proj_config);

                foreach my $dep (@deps) {
#                    $dep =~ s/.*[\/\\]//g;
                    push (@{%projects->{$project}->{CONFIGS}->{$proj_config}->{DEPS}}, $dep);
                }
                if ($proj->UsesTAOIDL () == 1) {
                    push @{%projects->{$project}->{CONFIGS}->{$proj_config}->{DEPS}}, ("gperf.exe", "tao_idl.exe");
                }
            }
        }
    }

    print "\n" if ($verbose);

    %projects->{$project}->{PROJ} = $proj;
}

################################################################################

# Clean out the dependency lists, we only keep the libraries which we know
# how to generate

print "=== Cleaning out Dependency Lists\n" if ($verbose);

foreach my $project (keys %projects) {
    foreach my $config (keys %{%projects->{$project}->{CONFIGS}}) {
        print "    Cleaning Dependencies: $project ($config)\n" if ($verbose);
        print "        Before:", join (" ", @{%projects->{$project}->{CONFIGS}->{$config}->{DEPS}}), "\n" if ($verbose);
        my @newdeps;
        foreach my $dep (@{%projects->{$project}->{CONFIGS}->{$config}->{DEPS}}) {
            $dep =~ s/.*[\/\\]//g;

            if (defined %names->{lc $dep}) {
                push @newdeps, $dep;
            }
        }
        print "        After:", join (" ", @newdeps), "\n" if ($verbose);
        @{%projects->{$project}->{CONFIGS}->{$config}->{DEPS}} = @newdeps;
    }
}

################################################################################

# Make sure to build any dependencies found

print "=== Walking Dependency Lists\n" if ($verbose);

my $finished = 0;

do {
    $finished = 1;
    foreach my $project (keys %projects) {
        foreach my $config (keys %{%projects->{$project}->{CONFIGS}}) {
            if (%projects->{$project}->{BUILD} == 1) {
                foreach my $dep (@{%projects->{$project}->{CONFIGS}->{$config}->{DEPS}}) {
                    if (%projects->{%names->{lc $dep}}->{BUILD} != 1) {
                        %projects->{%names->{lc $dep}}->{BUILD} = 1;
                        $finished = 0;
                    }
                }
            }
        }
    }

} while (!$finished);


################################################################################

# Output a list, if requested

if ($debug) {
    print "List of Dependencies\n";
    print "--------------------\n";
    foreach my $project (keys %projects) {
        print "=== $project\n";
        foreach my $config (keys %{%projects->{$project}->{CONFIGS}}) {
            print "    Config: $config\n";
            print "        Depends: ", join (" ", @{%projects->{$project}->{CONFIGS}->{$config}->{DEPS}}), "\n";
        }
    }

    print "\n";
    print "List of Outputs\n";
    print "---------------\n";

    foreach my $name (keys %names) {
        print "$name\n";
    }
}

################################################################################

# Loop through and

print "=== Compiling\n" if ($verbose);

my $compilations;  # Keep track of the number of compiles done during a pass
my $unfinished;
my $loop = 1;

do {
    $compilations = 0;
    $unfinished = 0;

    foreach my $project (keys %projects) {
        if (%projects->{$project}->{BUILD} == 1) {
            foreach my $config (keys %{%projects->{$project}->{CONFIGS}}) {
                if (%projects->{$project}->{CONFIGS}->{$config}->{DONE} != 1) {
                    my $depsleft = 0;
                    foreach my $dep (@{%projects->{$project}->{CONFIGS}->{$config}->{DEPS}}) {
                        if (%projects->{%names->{lc $dep}}->{CONFIGS}->{$config}->{DONE} != 1) {
                            ++$depsleft;
                        }
                    }

                    if ($depsleft == 0) {
                        ++$compilations;
                        print "Auto_compiling $project : $config\n" if ($auto_compile);

                        if ($list == 1) {
                            if ($clean == 1) {
                                print "Cleaning ";
                            }
                            else {
                                print "Compiling ";
                            }

                            print "$project : $config\n";
                        }
                        elsif ($clean == 1) {
                            %projects->{$project}->{PROJ}->Clean ($config);
                        }
                        else {
                            %projects->{$project}->{PROJ}->Build ($config);
                        }

                        %projects->{$project}->{CONFIGS}->{$config}->{DONE} = 1;
                    }
                    else {
                        ++$unfinished;
                    }
                }
            }
        }
    }

    print "    === Loop $loop: $compilations compiles, $unfinished left\n" if ($verbose);
    ++$loop;
} while ($compilations != 0);

# Loop through and see if anything wasn't compiled.  If so, this means either there is
# an error in the script or that there are circular dependencies

foreach my $project (keys %projects) {
    if (%projects->{$project}->{BUILD} == 1) {
        foreach my $config (keys %{%projects->{$project}->{CONFIGS}}) {
            if (%projects->{$project}->{CONFIGS}->{$config}->{DONE} != 1) {
                print STDERR "Error: Project not compiled: $project - $config\n",
            }
        }
    }
}
