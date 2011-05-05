eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: svcconf-convert.pl 80826 2008-03-04 14:51:23Z wotte $
#
# File: svcconf-convert.pl:
# Purpose: This perl script convert classic svc.conf file into XML svc.conf file format.
# Usage: svcconf-convert.pl [-i infile] [-o outfile] [-verbose] [-nocomment]
#           -i: Specify the input classic svc.conf filename.
#               If omitted, the default input filename is "svc.conf".
#           -o: Specify the output XML svc.conf filename.
#               If this argument is omitted, the resulting XML file will be written
#               to file called <input filename>.xml.
#           -verbose: Verbose output.
#           -nocomment: Remove all comments.  Use this argument if you have comments
#                       mixing in the middle of a classic svc.conf directive.

$indent = 0;

sub inc_indent
{
    $indent += 2;
}

sub dec_indent
{
    $indent -= 2;
}

sub get_indent
{
    $retv = 0;
    print STDERR "$0 (", $lineno, "): Unbalanced macro pairs\n" if ($indent < 0);
    $retv = $indent - 1 if ($indent > 0);
    $retv;
}

sub acexml_comment
{
    my $comment = shift;
    print OUT " " x &get_indent (), "<!-- $comment -->\n";
}

sub acexml_start
{
    my $name = shift;
    my $standalone = shift;

    print OUT " " x &get_indent (), "<$name";


    while (@attnames)
    {
        print OUT " ", pop (@attnames), '="', pop (@attvalues), '"';
    }

    if ($standalone != 0)
    {
        print OUT "/>\n";
    }
    else
    {
        print OUT ">\n";
        inc_indent ();
    }
}

sub acexml_end
{
    my $name = shift;

    dec_indent ();
    print OUT " " x &get_indent (), "</$name>\n";
}

$verbose = 0;
$nocomment = 0;
@attvalues = ();
@attnames = ();
$stream = "";
$infile = "";
$outfile = "";

while ( $#ARGV >= 0)
{
    if ($ARGV[0] =~ m/^-i/i) {
        shift;
        $infile = "$ARGV[0]";
    }
    elsif ($ARGV[0] =~ m/^-o/i) {
        shift;
        $outfile = "$ARGV[0]";
    }
    elsif ($ARGV[0] =~ m/^-verbose/i) {
        $verbose = 1;
    }
    elsif ($ARGV[0] =~ m/^-nocomment/i) {
        $nocomment = 1;
    }
    elsif ($ARGV[0] =~ m/^-(\?|h|help)/i) {     # Help information
        print
" Usage: svcconf-convert.pl [-i infile] [-o outfile] [-verbose] [-nocomment]
           -i: Specify the input classic svc.conf filename.
               If omitted, the default input filename is 'svc.conf'.
           -o: Specify the output XML svc.conf filename.
               If this argument is omitted, the resulting XML file will
               be written to file called <input filename>.xml.
           -verbose: Verbose output.
           -nocomment: Remove all comments.  Use this argument if you
                       have comments mixing in the middle of a classic
                       svc.conf directive.
";
        exit;
    }
    elsif ($ARGV[0] =~ m/^-/) {
        warn "$0:  unknown option $ARGV[0]\n";
        exit 1;
    }
    else {
        die "unknow argument\n";
    }
    shift;
}

if ($infile eq "") {
    print "Using default svc.conf name\n" if ($verbose != 0);
    $infile = "svc.conf";
}


if ($outfile eq "") {
    $outfile = "$infile.xml";
}
print "OUTFILE = $outfile \n" if ($verbose);

open (OUT, "> $outfile") or die "Unable to open $outfile\n";

undef $/;
open (FH, "< $infile");
$_ = <FH>;

if ($nocomment) {
    if (s/^\#(.*)$//mg) {
        print "ts = $_\n" if ($verbose != 0);
    }
}

print "------------------------------------------------------------\n" if ($verbose != 0);

print OUT "<?xml version='1.0'?>\n";
print OUT "<!-- Converted from $infile by svcconf-convert.pl -->\n";
acexml_start ("ACE_Svc_Conf", 0);

while (length ($_) != 0) {
    s/^\s*$//mg;

    print "INPUT =\n$_\n" if ($verbose);
  PARSE: {
      if (s/^\s*\#(.*)//) {
          acexml_comment ($1);
          print "# $1\n" if ($verbose);
      }
      if (s/^\s*{//) {
          acexml_start ("module", 0);
          print "open module\n" if ($verbose);
      }

      if (s/^\s*}//) {
          acexml_end ("module");
          acexml_end ($stream);
          print "close module\n" if ($verbose);
      }

      if (s/^\s*stream\s+dynamic\s+(\w+)\s+(\w+)\s*\*\s*(\S+):(\S+)\s*\(\s*\)(\s+(active|inactive))?(\s+"([^"]*)")?//) {
          $name = $1;
          $type = $2;
          $path = $3;
          $init = $4;
          $state = $6;
          $param = $8;
          acexml_start ("streamdef");
          if ($status ne "") {
              push @attnames, ("status");
              push @attvalues, ("$state");
          }
          push @attnames, ("type");
          push @attvalues, ("$type");
          push @attnames, ("id");
          push @attvalues, ("$name");
          acexml_start ("dynamic", 0);
          if ($param ne "") {
              push @attnames, ("params");
              push @attvalues, ("$param");
          }
          push @attnames, ("init");
          push @attvalues, ("$init");
          push @attnames, ("path");
          push @attvalues, ("$path");
          acexml_start ("initializer", 1);
          acexml_end ("dynamic");
          $stream = "streamdef";
          print "stream dynamic $name $type * $init:$path \"$param\" $state\n" if ($verbose);
      }

      if (s/^\s*stream\s+static\s+(\w+)(\s+("(.*)"))?//) {
          $name = $1;
          $param = $4;
          acexml_start ("streamdef", 0);
          if ($param ne "") {
              push @attnames, ("params");
              push @attvalues, ("$param");
          }
          push @attnames, ("id");
          push @attvalues, ("$name");
          acexml_start ("static", 1);
          $stream = "streamdef";
          print "static $name \"$param\"\n" if ($verbose);
      }

      if (s/^\s*stream\s+(\w+)//) {
          $name = $1;
          push @attnames, ("id");
          push @attvalues, ("$name");
          acexml_start ("stream", 0);
          $stream = "stream";
          print "stream $name\n" if ($verbose);
      }

      if (s/^\s*dynamic\s+(\w+)\s+(\w+)\s*\*\s*(\S+):(\S+)\s*\(\s*\)(\s+(active|inactive))?(\s+"([^"]*)")?//) {
          $name = $1;
          $type = $2;
          $path = $3;
          $init = $4;
          $state = $6;
          $param = $8;
          if ($status ne "") {
              push @attnames, ("status");
              push @attvalues, ("$state");
          }
          push @attnames, ("type");
          push @attvalues, ("$type");
          push @attnames, ("id");
          push @attvalues, ("$name");
          acexml_start ("dynamic", 0);
          if ($param ne "") {
              push @attnames, ("params");
              push @attvalues, ("$param");
          }
          push @attnames, ("init");
          push @attvalues, ("$init");
          push @attnames, ("path");
          push @attvalues, ("$path");
          acexml_start ("initializer", 1);
          acexml_end ("dynamic");
          print "dynamic $name $type * $init:$path \"$param\" $state\n" if ($verbose);
      }

      if (s/^\s*static\s+(\w+)(\s+("(.*)"))?//) {
          $name = $1;
          $param = $4;
          if ($param ne "") {
              push @attnames, ("params");
              push @attvalues, ("$param");
          }
          push @attnames, ("id");
          push @attvalues, ("$name");
          acexml_start ("static", 1);
          print "static $name \"$param\"\n" if ($verbose);
      }

      if ( s/^\s*resume\s+(\w+)//) {
          $name = $1;
          push @attnames, ("id");
          push @attvalues, ("$name");
          acexml_start ("resume", 1);
          print "resume $name\n" if ($verbose);
      }

      if ( s/^\s*remove\s+(\w+)//) {
          $name = $1;
          push @attnames, ("id");
          push @attvalues, ("$name");
          acexml_start ("remove", 1);
          print "remove $name\n" if ($verbose);
      }

      if ( s/^\s*suspend\s+(\w+)//) {
          $name = $1;
          push @attnames, ("id");
          push @attvalues, ("$name");
          acexml_start ("suspend", 1);
          print "suspend $name\n" if ($verbose);
      }

      $nothing = 1;
  }
}

acexml_end ("ACE_Svc_Conf");
