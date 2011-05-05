eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: regenerate_exports.pl 91813 2010-09-17 07:52:52Z johnnyw $
# Goes through ACE/TAO and replaces

use File::Find;
use Cwd;

$args = '';
$root = cwd.'/';

while ($#ARGV >= 0) {
  $args = $args . ' ' . shift @ARGV;
}

# wanted is only used for the File::Find
sub wanted
{
    my $file = $File::Find::name;

    $file =~ s/\.\//$root/;

    if ($File::Find::name =~ /\_export\.h$/i) {
        my $flag = 0;
        my $name = '';

        if (!open (FILE, $file)) {
            print STDERR "Error: Could not open $file\n";
        }

        while (<FILE>) {
            $flag = 1 if ((/generate_export/ || /GenExportH/) && $flag == 0);
            $name = $1 if (/define (\w*)_Export/);
        }

        if ($flag == 1) {
            print "Regenerating: $file\n";
            if ($OSNAME eq 'MSWIn32') {
                $file =~ s/\//\\/g;
            }
            system ("perl -S generate_export_file.pl $args $name > $file");
        }
        else {
            print "Skipping:     $file\n";
        }
        close FILE;
    }
}


find (\&wanted, ".");

