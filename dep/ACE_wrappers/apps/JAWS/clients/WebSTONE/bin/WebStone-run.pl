#!/pkg/gnu/bin//perl5
#
#$Id: WebStone-run.pl 80826 2008-03-04 14:51:23Z wotte $
#

$testbed = "conf/testbed";

push(@INC, "$wd/bin");
require('WebStone-common.pl');

html_begin("Run");

print CLIENT <<EOF
<H3><CENTER><A HREF="WebStone-setup.pl">Edit Configuration</A>
</CENTER></H3>
EOF
    ;

&show_model();
print CLIENT "<PRE>";

open(FILEHANDLE, $testbed);
while (<FILEHANDLE>) {
    (/^\#/) || print CLIENT $_;
}
close(FILEHANDLE);

print CLIENT <<EOF
</PRE>
<CENTER>
<FORM METHOD=POST ACTION="http://localhost:$html_port$wd/bin/runbench.pl">
<INPUT TYPE="submit" VALUE="Run WebStone">
</CENTER>
EOF
    ;

html_end();

# end
