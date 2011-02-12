#!/pkg/gnu/bin//perl5
#
#$Id: webstone-gui.pl 91813 2010-09-17 07:52:52Z johnnyw $

require 'conf/paths.pl';

#$debug = 1;
$HELPME="http://reality.sgi.com/employees/mblakele_engr/WebStone/";
$| =  1; # set pipes to hot

&html();

sub html {
        local($helper, $wd);

        &start_html_server();
        # These strings are used in, among others, PERL-to-HTML scripts.
        #
        $wd = `pwd`;
        chop $wd;

	print "$html_port\n" if $debug;

        $HTML_STARTPAGE = "http://localhost:$html_port$wd/doc/WebStone.html";

        #
        # Fork off the HTML client, and fork off a server process that
        # handles requests from that client. The parent process waits
        # until the client exits and terminates the server.
        #
        print "Starting $MOSAIC...\n" if $debug;

        if (($client = fork()) == 0) {
                foreach (keys %ENV) {
                        delete $ENV{$_} if (/proxy/i && !/no_proxy/i);
                }
                exec($MOSAIC, "$HTML_STARTPAGE")
                        || die "cannot exec $MOSAIC: $!";
        }

        if (($server = fork()) == 0) {
                if (($helper = fork()) == 0) {
                        alarm 3600;
                        &patience();
                }
                kill 'TERM',$helper;
                $SIG{'PIPE'} = 'IGNORE';
                for (;;) {
                        accept(CLIENT, SOCK) || die "accept: $!";
                        select((select(CLIENT), $| = 1)[0]);
                        &process_html_request();
                        close(CLIENT);
                }
        }

        #
        # Wait until the client terminates, then terminate the server.
        #
        close(SOCK);
        waitpid($client, 0);
        kill('TERM', $server);
        exit;
}

#
# Set up a listener on an arbitrary port. There is no good reason to
# listen on a well-known port number.
#
sub start_html_server {
        local($sockaddr, $proto, $junk);

        $AF_INET = 2;
        $SOCK_STREAM = 2;
        $PORT = 0; #1024;

        $sockaddr = 'S n a4 x8';
        $this = pack($sockaddr, $AF_INET, $PORT, "\0\0\0\0");
        ($junk, $junk, $proto) = getprotobyname('tcp');
        socket(SOCK, $AF_INET, $SOCK_STREAM, $proto) || die "socket: $!";
        setsockopt(SOCK, 0xffff, 0x0004, 1) || die "setsockopt: $!";
        bind(SOCK, $this) || die "bind: $!";
        listen(SOCK, 1) || die "listen: $!";
        ($junk, $html_port) = unpack($sockaddr, getsockname(SOCK));
}


#
# Process one client request.  We expect the client to send stuff that
# begins with:
#
#       command /password/perl_script junk
#
# Where perl_script is the name of a perl file that is executed via
# do "perl_script";
#
# In case of a POST command the values in the client's attribute-value
# list are assigned to the corresponding global PERL variables.
#
sub process_html_request {
        local($request, $command, $script, $magic, $url, $peer);
        local(%args);

        #
        # Parse the command and URL. Update the default file prefix.
        #
        $request = <CLIENT>;
        print $request if $debug;
        ($command, $url) = split(/\s+/, $request);
        if ($command eq "" || $command eq "QUIT") {
                return;
        }

        #($junk, $script) = split(/\//, $url, 2);
        #($script, $html_script_args) = split(',', $script, 2);
        #($HTML_CWD = "file:$script") =~ s/\/[^\/]*$//;
        $script = $url;

        while (<CLIENT>) {
                last if (/^\s+$/);
        }

        if ($command eq "GET") {
		if (-d $script) {
			get_dir($script);
		}
		elsif ($script =~ /\.pl\b/) {
			perl_html_script($script);
		}
		else {
		get_file($script);
		}
        } elsif ($command eq "POST") {

        	print $request if $debug;
		flush;
                #
                # Process the attribute-value list.
                #
                if ($_ = <CLIENT>) {
        		print "Hi $_" if $debug;
			flush;
                        s/\s+$//;
                        s/^/\n/;
                        s/&/\n/g;
                        $html_post_attributes = '';
                        $* = 1;
                        for (split(/(%[0-9][0-9A-Z])/, $_)) {
                                $html_post_attributes .= (/%([0-9][0-9A-Z])/) ?
                                        pack('c',hex($1)) : $_;
                        }
                        %args = ('_junk_', split(/\n([^=]+)=/, $html_post_attributes));
                        delete $args{'_junk_'};
                        for (keys %args) {
                                print "\$$_ = $args{$_}\n" if $debug;
                                ${$_} = $args{$_};
                        }
                        perl_html_script($script);
                } else {
                        &bad_html_form($script);
                }
        } else {
                &bad_html_command($request);
        }
}


#
# Unexpected HTML command.
#
sub bad_html_command {
        local($request) = @_;

        print CLIENT <<EOF
<HTML>
<HEAD>
<TITLE>Unknown command</TITLE>
<LINK REV="made" HREF=$HELPME>
</HEAD>
<BODY>
<H1>Unknown command</H1>
The command <TT>$request<TT> was not recognized.
</BODY>
</HTML>
EOF
;
}

#
# Execute PERL script
#
sub perl_html_script {
        local($script) = @_;

        if (! -e $script) {
                print CLIENT <<EOF
<HTML>
<HEAD>
<TITLE>File not found</TITLE>
<LINK REV="made" HREF=$HELPME>
</HEAD>
<BODY>
<H1>File not found</H1>
The file <TT>$script</TT> does not exist or is not accessible.
</BODY>
</HTML>
EOF
;               return;
        }
        do $script;
        if ($@ && ($@ ne "\n")) {
                print CLIENT <<EOF
<HTML>
<HEAD>
<TITLE>Command failed</TITLE>
<LINK REV="made" HREF=$HELPME>
</HEAD>
<BODY>
<H1>Command failed</H1>
$@
</BODY>
</HTML>
EOF
;
        }
}

#
# Missing attribute list
#
sub bad_html_form {
        local($script) = @_;

        print CLIENT <<EOF
<HTML>
<HEAD>
<TITLE>No attribute list</TITLE>
<LINK REV="made" HREF=$HELPME>
</HEAD>
<BODY>
<H1>No attribute list</H1>

No attribute list was found.
</BODY>
</HTML>
EOF
;
}

#
# Give them something to read while the server is initializing.
#
sub patience {
        for (;;) {
                accept(CLIENT, SOCK) || die "accept: $!";
                <CLIENT>;
                print CLIENT <<EOF
<HTML>
<HEAD>
<TITLE>Initializing...</TITLE>
<LINK REV="made" HREF=$HELPME>
</HEAD>
<BODY>
<H1>Initializing...</H1>
WebStone is initializing...
</BODY>
</HTML>
EOF
;
                close(CLIENT);
        }
}

sub get_file {
	local($file) = @_;

	print CLIENT <<EOF
<HTML>
<HEAD>
<TITLE>$file</TITLE>
</HEAD>
<H1>$file</H1>
<BODY><PRE>
EOF
   	unless ($file =~ /(html|htm|gif|jpeg|jpg)\b/);

	open(FILE, $file);
	while (<FILE>) {
		print CLIENT $_;
	}
	close(FILE);

	print CLIENT <<EOF
</PRE>
</HTML>
EOF
	unless ($file =~ /(html|htm|gif|jpeg|jpg)\b/);
}

sub get_dir {
	local($dir) = @_;
	opendir(DIRECTORY, $dir);
	@listing = readdir(DIRECTORY);
	closedir(DIRECTORY);
	print CLIENT <<EOF
<HTML>
<HEAD>
<TITLE>$dir</TITLE>
</HEAD>
<H1>$dir</H1>
<BODY>
EOF
   ;

	while (<@listing>) {
		print CLIENT "<P><A HREF=$dir/$_>$_</A>";
	}
	print CLIENT "</HTML>";
}
