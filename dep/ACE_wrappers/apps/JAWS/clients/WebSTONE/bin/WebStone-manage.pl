#!/pkg/gnu/bin//perl5
#
#$Id: WebStone-manage.pl 80826 2008-03-04 14:51:23Z wotte $
#

push(@INC, "$wd/bin");
require('WebStone-common.pl');

html_begin("Administration");

$runsdir = "$wd/bin/runs";
$thelength = length($runsdir) + 10;
$oldrunsdir = $runsdir;
$oldfilelist = "$wd/conf/filelist";

print CLIENT <<EOF
<FORM METHOD="POST" ACTION="$wd/bin/killbench.pl">
<H3>Clean up stray WebStone processes</H3>
<INPUT TYPE="SUBMIT" VALUE="Kill">
</FORM>

<HR>
<FORM METHOD="POST" ACTION="$wd/bin/move-runs.pl">
<H3>Move Results Directory to:</H3>
<INPUT TYPE=TEXT NAME=runsdir SIZE=$thelength VALUE=$runsdir>
<INPUT TYPE="SUBMIT" VALUE="Move Directory">
</FORM>
EOF
    ;

html_end();

# end
