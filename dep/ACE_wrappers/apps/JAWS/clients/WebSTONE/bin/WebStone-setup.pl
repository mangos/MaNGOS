#!/pkg/gnu/bin//perl5
#
#$Id: WebStone-setup.pl 91813 2010-09-17 07:52:52Z johnnyw $
#

push(@INC, "$wd/bin");
require('WebStone-common.pl');

@keylist = ();

html_begin("Setup");

show_model();

print CLIENT <<EOF
<FORM METHOD="POST" ACTION="$wd/bin/write-testbed.pl">
EOF
    ;

&gettestbed();
&getconfig();

print CLIENT <<EOF
</DL>
<P><INPUT TYPE="SUBMIT" VALUE="Write Configuration">
</FORM>

<HR>
<FORM METHOD="POST" ACTION="$wd/bin/move-filelist.pl">
<H3>Choose a Web site model:</H3>
<DL>
EOF
    ;

for $key (sort(keys %filelist)) {
    print CLIENT "<DD><INPUT TYPE=RADIO NAME=filelist ";
    if ($key eq "filelist") {
	print CLIENT " CHECKED ";
    }
    print CLIENT " VALUE=\"$wd/conf/$key\"> $key: $filelist{$key}";
}

print CLIENT <<EOF
</DL>
<INPUT TYPE="SUBMIT" VALUE="Set Workload">

EOF
    ;

html_end();

# end of main program

sub gettestbed {
    open(TESTBED, "$wd/conf/testbed");
    while (<TESTBED>) {
	if (/^\#|^(\w)*$/) { # do nothing
	}
	else {
	    ( $textvalue, $thevalue ) = split( '=', $_ );
	    ( $thevalue ) = split( '#', $thevalue);
	    $testbed{$textvalue} = $thevalue;
	    push(@keylist, $textvalue);
	}
    }
    close(TESTBED);

    open(HELPFILE, "$wd/doc/testbed.help");
    while (<HELPFILE>) {
	( $key, $textvalue ) = split( ':', $_ );
	$helptext{$key} = $textvalue;
    }
    close(HELPFILE);

    foreach $key (@keylist) {
	print CLIENT "<P><DT>$helptext{$key}";
	$thesize = length($testbed{$key}) + 5;
	print CLIENT "<DD>$key <INPUT TYPE=TEXT NAME=$key ";
	print CLIENT "SIZE=$thesize VALUE=$testbed{$key}>\n";
    }
}

sub getconfig {
    opendir(CONF, "$wd/conf") || die "open $wd/conf: $!";
    %filelist = "";
    foreach $file (sort readdir(CONF)) {
	if ( $file =~ /^filelist.*/ ) {
	    $headtext = `head -1 $wd/conf/$file`;
	    $headtext =~ s/\#//;
	    ( $headtext ) = split(':', $headtext);
	    $filelist{$file} = $headtext;
	}
    }
    closedir(CONF);
}

# end
