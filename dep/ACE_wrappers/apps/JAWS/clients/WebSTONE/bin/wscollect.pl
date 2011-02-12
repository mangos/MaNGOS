#!/pkg/gnu/bin//perl
#
#$Id: wscollect.pl 91813 2010-09-17 07:52:52Z johnnyw $
#
# $Header$
# updated version of the old wscollect script which goes through
# webstone run directories and summarizes the output in tabular
# format.
# syc 4/25/96
#

require "find.pl";

#
# the list @runs contains the timestamps for the runs which are found
# during the traversal of the runs directory. This list is used for
# indices into the associative arrays for storing run information.
#
# $numclients{ $time }    - number of clients for the run
# $connrate{ $time }      - connection rate average
# $littlesload{ $time }   - little's load factor
# $latency{ $time }       - latency average
# $error{ $time }         - error rate
# $throughput{ $time }    - throughput

local( @runs,
       %numclients,
       %connrate,
       %littlesload,
       %latency,
       %error,
       %throughput);

# Got rid of the trick hack of the title names, someone can put it
# back in later
@title = ( "Timestamp",
	  "Total number of clients",
	  "Connection rate average (conn/s)",
	  "Little's Load Factor",
	  "Average Latency (seconds)",
	  "Error Level (%)",
	  "Throughput avg. for all connections (MBits/s)");


push( @ARGV, ".") if ( !@ARGV );

for (@ARGV) {
    &find( $_ );
}

&PrintOutput;

1;

sub wanted {
    local( $filename ) = $_;

    return unless ( $filename =~ /run/ );

    local( $instats) = 0;
    local( $runtime, $tag, $data, $cruft, @cruft );

    open( FILE, $filename ) || return; # bail if failed to open
    $runtime = `pwd`;
    @cruft = split(/\//,$runtime);
    $runtime = pop( @cruft);
    chop( $runtime);
    push( @runs, $runtime);
    while ( $line = <FILE>) {
	if (! $instats) {
	    $instats = 1 if ( $line =~ /^WEBSTONE 2\.0 results/ );
	    next;
	}
	chop( $line );
	( $tag, $data ) = split( /:?\s{2,}|\t/, $line);

	# perl hack to emulate case/switch statement
	$tag =~ /number of clients/ &&
	    ($numclients{ $runtime } = $data, next);
        $tag =~ /error rate/ &&
	    (( $error{ $runtime }) = $data =~ /([\d\.]+)/, next);
	$tag =~ /connection rate/ &&
	    (( $connrate{ $runtime }) = $data =~ /([\d\.]+)/, next);
	$tag =~ /Server thruput/ &&
	    (( $throughput{ $runtime }) = $data =~ /([\d\.]+)/, next);
	$tag =~ /Little's Load/  &&
            (( $littlesload{ $runtime}) = $data =~ /([\d\.]+)/, next); # '
        $tag =~ /Average response time/ &&
	    (( $latency{ $runtime } ) = $data =~ /([\d\.]+)/, next);
    }
    close( FILE );
    unless ( $throughput{ $runtime} )  {
	pop( @runs); # if we didn't get a throughput, then the
				# data is incomplete and just drop this run
    }
}


sub printdata {
    local ($timestamp, $num_clients, $conn_rate,
	   $load, $latency, $error, $tput) = @_;
    format STDOUT =
@<<<<<<<<<<< @###### @######.## @####.## @###.#### @####.#### @######.##
$timestamp, $num_clients, $conn_rate, $load, $latency, $error, $tput
.

    if (!$printedTitles) {
	$printedTitles = 1;
	($ttimestamp, $tnum_clients, $tconn_rate,
	       $tload, $tlatency, $terror, $ttput) = @title;
	format STDOUT_TOP =
^||||||||||| ^||||||| ^||||||||| ^||||||| ^||||||||| ^||||||||| ^|||||||||||
$ttimestamp, $tnum_clients, $tconn_rate, $tload, $tlatency, $terror, $ttput
^||||||||||| ^||||||| ^||||||||| ^||||||| ^||||||||| ^||||||||| ^|||||||||||
$ttimestamp, $tnum_clients, $tconn_rate, $tload, $tlatency, $terror, $ttput
^||||||||||| ^||||||| ^||||||||| ^||||||| ^||||||||| ^||||||||| ^|||||||||||
$ttimestamp, $tnum_clients, $tconn_rate, $tload, $tlatency, $terror, $ttput
^||||||||||| ^||||||| ^||||||||| ^||||||| ^||||||||| ^||||||||| ^|||||||||||
$ttimestamp, $tnum_clients, $tconn_rate, $tload, $tlatency, $terror, $ttput
^||||||||||| ^||||||| ^||||||||| ^||||||| ^||||||||| ^||||||||| ^|||||||||||
$ttimestamp, $tnum_clients, $tconn_rate, $tload, $tlatency, $terror, $ttput
----------------------------------------------------------------------------
.
    # write STDOUT_TOP;
    } # end if printedTitles
    write STDOUT;
}

sub PrintOutput {
    local( $runtime );

    for $runtime (sort @runs) {
	&printdata( $runtime, $numclients{ $runtime}, $connrate{ $runtime},
		  $littlesload{ $runtime}, $latency{ $runtime}, $error{ $runtime},
		  $throughput{ $runtime});
    }
}





