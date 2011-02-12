eval '(exit $?0)' && eval 'exec perl -i -S $0 ${1+"$@"}'
    & eval 'exec perl -i -S $0 $argv:q'
    if 0;

# $Id: indent_macros.pl 80826 2008-03-04 14:51:23Z wotte $

# This perl script re-arrange the macro indentation so it's easier to
# see the layering relationship.

$lineno = 0;
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

while (<>) {
    $lineno++;
    if (/^[ \t]*\#[ \t]*((if|el|en|).*)/)
    {
        $cont = $1;
        $temp = $2;
        if ($temp =~ /if/) {
            print "#", " " x &get_indent (), $cont,"\n";
            inc_indent ();
        }
        elsif ($temp =~ /el/) {
            dec_indent ();
            print "#", " " x &get_indent (), $cont,"\n";
            inc_indent ();
        }
        elsif ($temp =~ /en/) {
            dec_indent ();
            print "#", " " x &get_indent (), $cont,"\n";
        }
        else {
            print "#", " " x &get_indent (), $cont,"\n";
        }
    }
    else {
        print $_;
    }
}

die ("$0 (EOF): Unbalanced macro pairs\n") if ($indent != 0);
