#!/pkg/gnu/bin//perl5
#
#$Id: WebStone-common.pl 91813 2010-09-17 07:52:52Z johnnyw $
#

1;

sub show_model {
    $model = `head -1 $wd/conf/filelist`;
    $model =~ s/\#//;
    ( $model ) = split(':', $model);

    print CLIENT <<EOF
<P><STRONG>
<A HREF=\"$wd/bin/WebStone-setup.pl\">$model
</A></STRONG>
EOF
    ;
}

sub html_begin {

    ( $title ) = @_;

    close(STDOUT);
    open(STDOUT, ">&CLIENT");
    close(STDERR);
    open(STDERR, ">&CLIENT");

    print CLIENT <<EOF
<HTML>
<HEAD>
<TITLE>WebStone $title</TITLE>
<A HREF="$wd/doc/WebStone.html">
<IMG SRC="$wd/doc/webstone.gif" ALT="WebStone" BORDER=0 ></A>
<H1>World Wide Web Server Benchmarking</H1>
<DL>
<DT><EM>If you have any questions, please read the
<A HREF="$wd/doc/FAQ-webstone.html">WebStone FAQ</A>.</EM>
<HR>
EOF
    ;

}

sub html_end {

    print CLIENT <<EOF
<HR>
<ADDRESS><A HREF="$wd/doc/LICENSE.html">copyright 1995 Silicon Graphics</A>
</ADDRESS>
</BODY>
</HTML>
EOF
    ;

    close(STDERR);
    close(STDOUT);
    open(STDOUT);
    open(STDERR);
}

# end
