#!/pkg/gnu/bin//perl5
#
#$Id: move-filelist.pl 80826 2008-03-04 14:51:23Z wotte $
#

push(@INC, "$wd/bin");
require('WebStone-common.pl');

$oldfilelist = "$wd/conf/filelist";

html_begin();

if ($filelist ne $oldfilelist) {
    $backup = $oldfilelist . ".bak";
    print CLIENT "<BODY>Backing up $oldfilelist to $backup";
    rename($oldfilelist, $backup);
    print CLIENT "<P>Copying $filelist to $oldfilelist";
    link($filelist, $oldfilelist);
    print CLIENT "<P>Done.";
}
else
{
    print CLIENT "<STRONG>Can't move $filelist <P>to $filelist</STRONG>";
}

html_end();

# end
