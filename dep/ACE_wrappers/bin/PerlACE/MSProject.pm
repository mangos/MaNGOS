#! /usr/bin/perl
# $Id: MSProject.pm 91813 2010-09-17 07:52:52Z johnnyw $

package PerlACE::MSProject;

use strict;
use FileHandle;

###############################################################################

# Constructor

sub new
{
    my $proto = shift;
    my $class = ref ($proto) || $proto;
    my $self = {};

    $self->{FILENAME} = shift;
    $self->{VERSION} = undef;
    $self->{NAME} = undef;
    %{$self->{CONFIGS}} = ();

    bless ($self, $class);
    return $self;
}

###############################################################################

# Accessors

sub Filename
{
    my $self = shift;

    if (@_ != 0) {
        $self->{FILENAME} = shift;
    }

    return $self->{FILENAME};
}

sub Version ()
{
    my $self = shift;
    return $self->{VERSION};
}

sub Name ()
{
    my $self = shift;
    return $self->{NAME};
}

sub Configs ()
{
    my $self = shift;
    return keys %{$self->{CONFIGS}};
}

sub DepOutputFile ($)
{
    my $self = shift;
    my $config = shift;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    my $name = $self->OutputFile ($config);

    if ($name =~ m/\.dll$/) {
        $name = $self->LibraryFile ($config);
    }

    $name =~ s/.*\\//; # / <- For devenv
    $name =~ s/.*\///;

    return $name;
}

sub OutputFile ($)
{
    my $self = shift;
    my $config = shift;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    if (%{$self->{CONFIGS}}->{$config}->{LINK} =~ m/out\:\"([^\"]*)\"/) {
        return $1;
    }
    elsif (defined $self->Name ()) {
        my $filename = $self->Filename;
        my $ext = "";

        if (%{$self->{CONFIGS}}->{$config}->{LINK} =~ m/\/dll/) {
            $ext = ".dll";
        }
        elsif (%{$self->{CONFIGS}}->{$config}->{LINK} =~ m/\/subsystem\:/) {
            $ext = ".exe";
        }
        else {
            $ext = ".lib";
        }

        $filename =~ s/\.[^\.]*$/$ext/;
        return $filename;
    }
}


sub LibraryFile ($)
{
    my $self = shift;
    my $config = shift;
    my $dll = undef;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    if ($self->OutputFile ($config) =~ m/([^\/\\]*)\.dll$/i) {
        $dll = $1;
    }

    if (defined $dll) {
        if (%{$self->{CONFIGS}}->{$config}->{LINK} =~ m/implib\:\"([^\"]*)\"/i) {
            return $1;
        }
        else {
            $dll =~ s/.*\\//ig; # / <- Just here to fix color coding in devenv beta
            return $self->OutputDir ($config). $dll . ".lib";
        }
    }
}

sub OutputDir ($)
{
    my $self = shift;
    my $config = shift;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    return %{$self->{CONFIGS}}->{$config}->{OUTPUTDIR};
}

sub IntermidiateDir ($)
{
    my $self = shift;
    my $config = shift;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    return %{$self->{CONFIGS}}->{$config}->{INTERMEDIATEDIR};
}

sub TargetDir ($)
{
    my $self = shift;
    my $config = shift;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    return %{$self->{CONFIGS}}->{$config}->{TARGETDIR};
}

sub CPPOptions ($)
{
    my $self = shift;
    my $config = shift;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    return %{$self->{CONFIGS}}->{$config}->{CPP};
}

sub LINKOptions ($)
{
    my $self = shift;
    my $config = shift;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    return %{$self->{CONFIGS}}->{$config}->{LINK};
}

sub Libs($)
{
    my $self = shift;
    my $config = shift;

    if (!defined $config) {
        print STDERR "Error: No configuration specified\n";
        return;
    }

    return %{$self->{CONFIGS}}->{$config}->{LIBS};
}

sub UsesTAOIDL ()
{
    my $self = shift;

    return $self->{TAOIDL};
}

sub Compiler ()
{
    my $self = shift;

    return $self->{COMPILER};
}

###############################################################################

# Big methods

sub Load ()
{
    my $self = shift;
    my $config = "Unknown";

    $self->{valid} = 0;

    my $fh = new FileHandle;

    unless ($fh->open ("<" . $self->{FILENAME})) {
        print "Could not open file ", $self->{FILENAME}, ": ", $_;
        return;
    }

    while (<$fh>) {
        if (m/^\#.*Project File - Name=\"([^\"]*)\"/) {
            $self->{NAME} = $1;
        }

        if (m/^\#.*Format Version (.*)/) {
            $self->{VERSION} = $1;
        }

        # Check for configurations

        if (m/^\!.*IF  \"\$\(CFG\)\" == \".* - (.*)$\"/) {
            $config = $1;
        }
        elsif (m/^\!ENDIF$/) {
            $config = "";
        }

        # Check for directories

        if (m/\# PROP Output_Dir \"(.*)\"/) {
            %{$self->{CONFIGS}}->{$config}->{OUTPUTDIR} = $1;
        }
        elsif (m/\# PROP Intermediate_Dir \"(.*)\"/) {
            %{$self->{CONFIGS}}->{$config}->{INTERMEDIATEDIR} = $1;
        }
        elsif (m/\# PROP Target_Dir \"(.*)\"/) {
            %{$self->{CONFIGS}}->{$config}->{TARGETDIR} = $1;
        }

        # Look at CPP options

        if (m/\# ADD BASE CPP(.*)$/ || m/\# ADD CPP(.*)$/) {
            my @flags = split (/ \//, $1);

            foreach my $flag (@flags) {
                if ($flag && %{$self->{CONFIGS}}->{$config}->{CPP} !~ m/$flag/) {
                    %{$self->{CONFIGS}}->{$config}->{CPP} .= " /$flag";
                }
            }
        }
        elsif (m/\# SUBTRACT CPP(.*)$/ || m/\# SUBTRACT BASE CPP(.*)$/) {
            my @flags = split (/ \//, $1);

            foreach my $flag (@flags) {
                if ($flag && %{$self->{CONFIGS}}->{$config}->{CPP} =~ m/$flag/) {
                    %{$self->{CONFIGS}}->{$config}->{CPP} =~ s/ \/$flag//g;
                }
            }
        }

        # Look at LINK32 options

        if (m/\# ADD BASE LINK32(.*)$/ || m/\# ADD LINK32(.*)$/
            || m/\# ADD BASE LIB32(.*)$/ || m/\# ADD LIB32(.*)$/) {
            my @flags = split (/ \//, $1);

            foreach my $flag (@flags) {
                my $found = 0;
                my @libs = split (/ /, $flag);

                foreach my $lib (@libs) {
                    if ($lib =~ m/\.lib$/) {
                        if (%{$self->{CONFIGS}}->{$config}->{LIBS} !~ m/\Q$lib\E/) {
                            %{$self->{CONFIGS}}->{$config}->{LIBS} .= " $lib";
                        }
                        $found = 1;
                    }
                }

                if (!$found && $flag) {
                    my $shortflag = $flag;
                    if ($flag =~ m/^(.*)\:/) {
                        $shortflag = $1;
                    }

                    if (%{$self->{CONFIGS}}->{$config}->{LINK} !~ m/ \/$shortflag/) {
                        %{$self->{CONFIGS}}->{$config}->{LINK} .= " /$flag";
                    }
                }
            }
        }
        elsif (m/\# SUBTRACT BASE LINK32(.*)$/ || m/\# SUBTRACT LINK32(.*)$/
               || m/\# SUBTRACT BASE LIB32(.*)$/ || m/\# SUBTRACT LIB32(.*)$/) {
            my @flags = split (/ \//, $1);

            foreach my $flag (@flags) {
                my $shortflag = $flag;
                if ($flag =~ m/^(.*)\:/) {
                    $shortflag = $1;
                }

                if ($flag && %{$self->{CONFIGS}}->{$config}->{LINK} =~ m/ (\/$shortflag\:[^ ]*)/) {
                    %{$self->{CONFIGS}}->{$config}->{LINK} =~ s/ \Q$1\E//ig;
                }
            }
        }

        if (m/^\# Name \".* - (.*)\"/ && defined %{$self->{CONFIGS}}->{"Unknown"}) {
            %{$self->{CONFIGS}}->{$1} = %{$self->{CONFIGS}}->{"Unknown"};
            delete %{$self->{CONFIGS}}->{"Unknown"};
        }

        if (m/tao\_idl/ && m/\$\(InputName\)\.idl/ || m/tao\_idl/ && m/\$\(InputPath\)/) {
            $self->{TAOIDL} = 1;
        }
    }
    $fh->close ();
    $self->{valid} = 1;
}

###############################################################################

# Build functions

sub Build ($)
{
    my $self = shift;
    my ($config) = @_;

    my $command = $self->Compiler () . " " . $self->Filename ()
                  . " /USEENV"
                  . " /MAKE \"" . $self->Name ()
                  . " - " . $config . "\"";

    system $command;
}

sub Clean ($)
{
    my $self = shift;
    my ($config) = @_;

    my $command = $self->Compiler () . " " . $self->Filename ()
                  . " /USEENV"
                  . " /MAKE \"" . $self->Name ()
                  . " - " . $config . "\" /CLEAN";

    system $command;
}


1;