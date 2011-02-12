eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-
# $Id: generate_doxygen.pl 91755 2010-09-14 11:35:53Z johnnyw $
#

require POSIX;
require File::Path;

use Cwd;
use File::Spec;
use Env qw(ACE_ROOT TAO_ROOT CIAO_ROOT DDS_ROOT);

# Configuration and default values

if (!defined $TAO_ROOT) {
    $TAO_ROOT = "$ACE_ROOT/TAO";
}
if (!defined $CIAO_ROOT) {
    $CIAO_ROOT = "$TAO_ROOT/CIAO";
}
if (!defined $DANCE_ROOT) {
    $DANCE_ROOT = "$TAO_ROOT/DAnCE";
}

$is_release = 0;
$exclude_ace = 0;
$exclude_tao = !-r "$TAO_ROOT/VERSION";
$exclude_ciao = !-r "$CIAO_ROOT/VERSION";
$exclude_dance = !-r "$DANCE_ROOT/VERSION";
$verbose = 0;
$perl_path = '/usr/bin/perl';
$html_output_dir = '.';

$dds = 0;
if (defined $DDS_ROOT && -r "$DDS_ROOT/VERSION") {
    $dds_path = Cwd::abs_path($DDS_ROOT);
    $cwd_path = Cwd::abs_path(getcwd());
    if ($dds_path eq $cwd_path) {
        $dds = $exclude_ace = $exclude_tao = $exclude_ciao = 1;
    }
}

@ACE_DOCS = ('ace',
             'ace_man',
             'ace_rmcast',
             'ace_ssl',
             'ace_qos',
             'ace_inet',
             'acexml');
@TAO_DOCS = ('tao'
             ,'tao_anytypecode'
             ,'tao_portableserver'
             ,'tao_pi'
             ,'tao_pi_server'
             ,'tao_rtportableserver'
             ,'tao_compression'
             ,'tao_transportcurrent'
             ,'tao_rtcorba'
             ,'tao_dynamicany'
             ,'tao_dynamicinterface'
             ,'tao_iormanip'
             ,'tao_iortable'
             ,'tao_ziop'
             ,'tao_esf'
             ,'tao_rtevent'
             ,'tao_cosevent'
             ,'tao_cosnotification'
             ,'tao_implrepo'
             ,'tao_strategies'
             ,'tao_smartproxies'
             ,'tao_av'
             ,'tao_security'
             ,'tao_ssliop'
             ,'tao_cosnaming'
             ,'tao_costime'
             ,'tao_costrader'
             ,'tao_portablegroup'
             ,'tao_ifr');
@CIAO_DOCS = ('ciao_dds4ccm'
             ,'ciao');
@DANCE_DOCS = ('DAnCE');
@DDS_DOCS = ('dds');

# Modify defaults using the command line arguments
&parse_args ();

$wrote_configh = 0;
if (!-r "$ACE_ROOT/ace/config.h") {
    open(CONFIG_H, ">$ACE_ROOT/ace/config.h")
        || die "Cannot create config file\n";
    print CONFIG_H "#include \"ace/config-doxygen.h\"\n";
    close(CONFIG_H);
    $wrote_configh = 1;
}

&generate_doxy_files ('ACE',  " $ACE_ROOT", " $ACE_ROOT/VERSION", @ACE_DOCS) if (!$exclude_ace);
&generate_doxy_files ('TAO',  " $TAO_ROOT", " $TAO_ROOT/VERSION", @TAO_DOCS) if (!$exclude_tao);
&generate_doxy_files ('CIAO', " $CIAO_ROOT", " $CIAO_ROOT/VERSION", @CIAO_DOCS) if (!$exclude_ciao);
&generate_doxy_files ('DANCE'," $DANCE_ROOT", " $DANCE_ROOT/VERSION", @DANCE_DOCS) if (!$exclude_dance);
&generate_doxy_files ('DDS',   "$DDS_ROOT", " $DDS_ROOT/VERSION", @DDS_DOCS) if $dds;

unlink "$ACE_ROOT/ace/config.h" if $wrote_configh;

exit 0;

sub parse_args {
  my @ARGS = ();
  while ($#ARGV >= 0) {
    if (!($ARGV[0] =~ m/^-/)) {
      push @ARGS, $ARGV[0];
    } elsif ($ARGV[0] eq "-is_release") {
      $is_release = 1;
    } elsif ($ARGV[0] eq "-exclude_ace") {
      $exclude_ace = 1;
    } elsif ($ARGV[0] eq "-exclude_tao") {
      $exclude_tao = 1;
    } elsif ($ARGV[0] eq "-exclude_ciao") {
      $exclude_ciao = 1;
    } elsif ($ARGV[0] eq "-exclude_dance") {
      $exclude_dance = 1;
    } elsif ($ARGV[0] eq "-include_dds") {
      $dds = 1;
    } elsif ($ARGV[0] eq "-verbose") {
      $verbose = 1;
    } elsif ($ARGV[0] eq "-perl_path" && $#ARGV >= 1) {
      $perl_path = $ARGV[1];
      shift;
    } elsif ($ARGV[0] eq "-html_output" && $#ARGV >= 1) {
      $html_output_dir = $ARGV[1];
      shift;
    } else {
      print "Ignoring option $ARGV[0]\n";
    }
    shift @ARGV;
  }
  @ARGV = @ARGS;
}

#is $arg1 the same path as "$arg2/$arg3"?
sub same_dir {
  my $lhs = shift;
  my $rhs_base = shift;
  my $rhs_dir = shift;
  my $rhs = File::Spec->catdir($rhs_base, $rhs_dir);
  return File::Spec->canonpath($lhs) eq File::Spec->canonpath($rhs);
}

sub generate_doxy_files {

  my $KIT = shift;
  my $ROOT_DIR = shift;
  my $VERSION_FILE= shift;
  my @DOCS = @_;

  my $VERSION = 'Snapshot ('.
    POSIX::strftime("%Y/%m/%d-%H:%M", localtime)
      .')';

  my $KIT_path = ($KIT eq 'CIAO') ? 'TAO/CIAO' : $KIT;
  my $translate_paths =
    ($KIT eq 'TAO' && !same_dir($TAO_ROOT, $ACE_ROOT, 'TAO')) ||
    ($KIT eq 'CIAO' && !same_dir($CIAO_ROOT, $TAO_ROOT, 'CIAO'));

  foreach my $i (@DOCS) {
    if ($is_release) {
      my ($major, $minor, $beta) = &get_versions ($KIT, $VERSION_FILE);
      $VERSION = $major.'.'.$minor.'.'.$beta;
    }

    my $input = "$ROOT_DIR/etc/".$i.".doxygen";
    my $output = "/tmp/".$i.".".$$.".doxygen";

    open(DOXYINPUT, $input)
      || die "Cannot open doxygen input file $input\n";
    open(DOXYOUTPUT, ">$output")
      || die "Cannot open doxygen output file $output\n";

    my $generate_man  = 0;
    my $generate_html = 0;
    my @output_dirs = ();
    while (<DOXYINPUT>) {
      chomp;
      if (/^PROJECT_NUMBER/) {
        print DOXYOUTPUT "PROJECT_NUMBER        = ", $VERSION, "\n";
        next;
      } elsif (/^PERL_PATH /) {
	print DOXYOUTPUT "PERL_PATH = $perl_path\n";
	next;
      } elsif (/^QUIET / && $verbose) {
	print DOXYOUTPUT "QUIET = NO\n";
	next;
      } elsif (/^INLINE_SOURCES/ && $is_release) {
	print DOXYOUTPUT "INLINE_SOURCES = NO\n";
	next;
      } elsif (/^SOURCE_BROWSER/ && $is_release) {
	print DOXYOUTPUT "SOURCE_BROWSER = NO\n";
	next;
      } elsif (/^VERBATIM_HEADERS/ && $is_release) {
	print DOXYOUTPUT "VERBATIM_HEADERS = NO\n";
	next;
      } elsif (/^GENERATE_MAN/ && /= YES/) {
        $generate_man = 1;
      } elsif (/^GENERATE_HTML/ && /= YES/) {
        $generate_html = 1;
      } elsif ($generate_html && /^HTML_OUTPUT/) {
        my @field = split(' = ');
        if ($#field >= 1) {
          my $html_out_dir = "$html_output_dir/$field[1]";
          push @output_dirs, $html_out_dir;
	  print DOXYOUTPUT "HTML_OUTPUT = $html_out_dir\n";
	  next;
        }
      } elsif ($generate_html && /^GENERATE_TAGFILE/) {
        my @field = split(' = ');
        if ($#field >= 1) {
          my $html_out_dir = "$html_output_dir/$field[1]";
	  print DOXYOUTPUT "GENERATE_TAGFILE = $html_out_dir\n";
	  next;
        }
      } elsif ($generate_html && /^TAGFILES\s*=\s*(.*)$/) {
        my $value = $1;
        while ($value =~ /\\$/) {
          chop $value; #removes trailing \
          my $line = <DOXYINPUT>;
          chomp $line;
          $value .= ' ' . $line;
	}
	my @values = split(' ', $value);
	map {$_ = $html_output_dir . '/' . $_; } @values;
	print DOXYOUTPUT 'TAGFILES = ' . join(' ', @values) . "\n";
	next;
      } elsif ($generate_man && /^MAN_OUTPUT/) {
        my @field = split(' = ');
        if ($#field >= 1) {
          push @output_dirs, $field[1];
        }
      } elsif ($translate_paths && /^(INPUT|INCLUDE_PATH)\s*=\s*(.*)$/) {
	my $keyword = $1;
        my $value = $2;
        while ($value =~ /\\$/) {
          chop $value; #removes trailing \
          my $line = <DOXYINPUT>;
          chomp $line;
          $value .= ' ' . $line;
        }
        $value =~ s/$KIT_path/${"${KIT}_ROOT"}/g;
        print DOXYOUTPUT "$keyword = $value\n";
        next;
      }

      print DOXYOUTPUT $_, "\n";
    }
    close (DOXYOUTPUT);
    close (DOXYINPUT);

    foreach my $i (@output_dirs) {
      File::Path::mkpath($i, 0, 0755);
    }

    &run_doxy ($output);

    unlink $output;
  }

  if ($generate_man) {
    open(FIND, "find man -type f -print |") or die "Can't run find\n";
    while (<FIND>) {
      chomp;
      my $name_with_whitespace = $_;
      next unless ($name_with_whitespace =~ /\s/);
      my $name_without_whitespace = $name_with_whitespace;
      $name_without_whitespace =~ s/\s+//g;
      rename $name_with_whitespace, $name_without_whitespace;
    }
    close FIND;
  }
}

sub run_doxy {
  my $config = shift;
  open(DOX,"doxygen $config 2>&1 |")
    || die "cannot start ACE doxygen process\n";
  while (<DOX>) {
    print $_;
  }
  close (DOX)
    || die "error while running doxygen on $config\n";
}

########
######## Retrieve version information from VERSION file(s).
########
sub get_versions () {
  my $KIT = shift;
  my $VERSION_FILE = shift;
  my ($major_version, $minor_version, $beta_version);

  open (VERSION, '<'.$VERSION_FILE)  ||
    die "$0: unable to open $VERSION_FILE\n";
  while (<VERSION>) {
    chomp;
    if (/$KIT version (\d+)\.(\d+)\.(\d+)/) {
      $major_version = $1;
      $minor_version = $2;
      $beta_version = $3;
      last;
    } elsif (/$KIT version (\d+)\.(\d+)[^\.]/) {
      #### Previous release was a minor.
      $major_version = $1;
      $minor_version = $2;
      $beta_version  = '0';
      last;
    } elsif (/$KIT version (\d+)[^\.]/) {
      #### Previous release was a major.
      $major_version = $1;
      $minor_version = '0';
      $beta_version  = '0';
      last;
    }
  }
  close VERSION;

  return ($major_version, $minor_version, $beta_version);
}
