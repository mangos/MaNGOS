# $Id: cltime.pl 80826 2008-03-04 14:51:23Z wotte $
use strict;

my($name)  = shift;
my($email)  = shift;
my($entry) = scalar(gmtime());

my($tz) = 'UTC';
$entry =~ s/(:\d\d\s+)(.*)(\d\d\d\d)$/$1$tz $3/;

$entry .= "  $name <$email>";

print $entry;
