#!/pkg/gnu/bin//perl5
#
#$Id: view-results.pl 80826 2008-03-04 14:51:23Z wotte $
#
push(@INC, "$wd/bin");
require('WebStone-common.pl');

require('wscollect.pl');

$debug = 0;

$printedTitles = 0;

html_begin("Results");

print CLIENT "<H3>WebStone Results</H3>";

($debug) && print STDERR "opening table\n";
print CLIENT "<BODY><TABLE BORDER=1>\r";

@directories = ("$wd/bin/runs");
directory: for (@directories) {
    &find($_);
}

($debug) && print STDERR "closing table\n";
print CLIENT "</TABLE></BODY>\r";

html_end();

# end main

sub printcustom {
    if (!$printedTitles) {
	$printedTitles = 1;
	print CLIENT "<TR>";
	for $title (@title) {
	    print CLIENT "<TH>$title</TH>\r";
	} # end for title
	print CLIENT "</TR>\r";
    }
    print CLIENT "<TR>";
    $first_column = 1;
    for $data (@data) {
	if ($first_column) {
	    $first_column = 0;
	    print CLIENT "<TD><A HREF=$wd/bin/runs/$data>$data</A></TD>\r";
	} else {
	    print CLIENT "<TD ALIGN=RIGHT>$data</TD>\r";
	}
    } # end for data
    print CLIENT "</TR>\r";
}

# end
