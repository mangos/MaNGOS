# $Id: clean_dsp.pl 91813 2010-09-17 07:52:52Z johnnyw $
#   DSP cleaner

$if_depth = 0;
@saved_lines = ();
$dirty = 0;
$in_dependency = 0;

die "Not enough args" if ($#ARGV < 0);

open (FILE, "<$ARGV[0]");

loop: while  (<FILE>)
{
    # Check for dependency information

    if (/^DEP/ || /^NODEP/) {
        $in_dependency = 1;
    }

    if ($in_dependency) {
        $in_dependency = 0 if (!/\\$/);
        goto loop;
    }

    # Check for empty !IF blocks

    if (/^\!IF/) {
        ++$if_depth;
    }

    push @saved_lines, $_
        if ($if_depth > 0);

    if (/^\!ENDIF/) {
        --$if_depth;
        print @saved_lines
            if ($if_depth == 0 && $dirty == 1);
        @saved_lines = ();
        $dirty = 0;
    }
    elsif ($if_depth == 0) {
        print;
    }

    $dirty = 1
        if ($if_depth > 0 && !/^\!/ && !/^\s+$/);


}

close (FILE);
