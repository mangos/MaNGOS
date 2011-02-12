#!/pkg/gnu/bin//perl5
#
#$Id: runbench.pl 80826 2008-03-04 14:51:23Z wotte $
#

push(@INC, "$wd/bin");
require('WebStone-common.pl');
require('flush.pl');

# force flush after every write or print
$| = 1;

html_begin("In Progress");

print CLIENT "<H3><CENTER>Running WebStone</CENTER></H3><PRE>";
show_model();
&flush(CLIENT);
&flush(STDOUT);

&start_runbench();

print CLIENT <<EOF
<TITLE>WebStone Completed</TITLE>
</PRE><CENTER>
<FORM METHOD=POST ACTION="http://localhost:$html_port$wd/bin/view-results.pl">
<INPUT TYPE="submit" VALUE="View Results">
</CENTER>
EOF
    ;

html_end();

sub start_runbench {
    $command = "cd $wd/bin; ./runbench";
    system($command);
}

# end
