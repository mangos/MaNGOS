#!/pkg/gnu/bin//perl5
#
#$Id: write-testbed.pl 91813 2010-09-17 07:52:52Z johnnyw $
#
# write new values from form to $wd/conf/testbed, and run WebStone
#

push(@INC, "$wd/bin");
require('WebStone-common.pl');

html_begin("Current Configuration");

&show_model();
&write_data();

print CLIENT <<EOF
<HR>
<FORM METHOD="POST" ACTION="$wd/bin/runbench.pl">
<P><INPUT TYPE="SUBMIT" VALUE="Run WebStone">
</FORM>
</DL>

EOF
    ;

html_end();

# end main

sub write_data {
    rename("$wd/conf/testbed", "$wd/conf/testbed.bak") ||
	die "rename testbed: $!\n";
    open(TESTBED, ">>$wd/conf/testbed") || die "open testbed: $!\n";

    print CLIENT "<PRE>";

    foreach $key (@keylist) {
	$$key =~ s/\+/ /g;
	$newvalue = "$key=\"$$key\"\n";
	print CLIENT $newvalue;
	print TESTBED $newvalue;
    }

    print CLIENT "</PRE>";

    close(TESTBED);
}

# end
