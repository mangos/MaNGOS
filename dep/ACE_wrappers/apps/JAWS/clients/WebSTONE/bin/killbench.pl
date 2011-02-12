#!/pkg/gnu/bin//perl5
#
#$Id: killbench.pl 80826 2008-03-04 14:51:23Z wotte $
#

push(@INC, "$wd/bin");
require('WebStone-common.pl');

html_begin();

print CLIENT "<P>Killing WebStone processes<PRE>";
system("$wd/bin/killbench");
print CLIENT "</PRE><P>Done.";

html_end();

# end
