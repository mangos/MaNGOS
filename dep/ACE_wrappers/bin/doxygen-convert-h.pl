eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: doxygen-convert-h.pl 91813 2010-09-17 07:52:52Z johnnyw $
#   doxygen-convert-h.pl is a script that would be used to convert old
#   documented style ACE/TAO header files to doxygen style.

# TODO List:
#   (Search for @todo in this script)

use File::Copy;
use FileHandle;
use Getopt::Std;

##############################################################################
# Parse the options

if (!getopts ('dDhsu') || $opt_h) {
    print "doxygen-convert-h.pl [-dDhsu] filenames or directories\n";
    print "\n";
    print "    -d         debug script\n";
    print "    -D         really verbose debug\n";
    print "    -h         display this help\n";
    print "    -s         print result to stdout\n";
    print "    -u         turn off file verification\n";
    exit (1);
}

## if verbose debug, also regular debug
$opt_d = 1 if (defined $opt_D);

##############################################################################
# Find the files

@files = ();

sub recursive_find {
  my($file) = shift;
  my(@rfiles) = ();
  my($fh) = new FileHandle();

  if (opendir($fh, $file)) {
    foreach my $f (grep(!/^\.\.?$/, readdir($fh))) {
      if ($f ne '.svn') {
        my($full) = "$file/$f";
        if (-d $full) {
          push(@rfiles, recursive_find($full));
        }
        else {
          push(@rfiles, $full)
          if ($f =~ /\.(h|hxx|hpp|hh|inl|cpp|cxx|cc|c|C)$/)
          ;
        }
      }
    }
    closedir($fh);
  }

  return @rfiles;
}

foreach $arg (@ARGV) {
    my @results = glob $arg;
    if ($#results < 0) {
        print STDERR "File not Found: $arg\n"
    }
    else {
        foreach my $result (@results) {
            if (-d $result) {
                push(@files, recursive_find($result));
            }
            else {
                push(@files, $result);
            }
        }
    }
}

##############################################################################
# Subroutines used to change the file.

$fail = 0;

sub verify (@)
{
    my (@contents) = @_;

    print "Verification\n" if (defined $opt_d);

    my $found_id = 0;
    my $found_filename = 0;

    foreach $line (@contents) {
        $found_id = 1 if ($line =~ /\$Id\:/);
        $found_filename = 1 if ($line =~ /\= FILENAME/);
    }

    return 0 if ($found_id == 1 && $found_filename == 1);

    # failed
    return 1;
}

sub format_description (@)
{
    my (@description) = @_;
    my @after = ();

    my $line;

    if ($#description < 1) {
        foreach $line (@description) {
            $line =~ s/\/\// \*  \@brief /;
            push @after, $line;
        }
    }
    else {
        foreach $line (@description) {
            $line =~ s/\/\// \*/;
            $line =~ s/\*   /\* /;
            push @after, $line;
        }
    }

    return @after;
}

sub fix_file_header (@)
{
    my (@before) = @_;
    my @after = ();
    my @description = ();
    my $id = "\$Id\$\n";
    my $authors = "";

    my $state = 'before';
    ## state = before, filename, description, author, after, done

    print "Fixing File Header\n" if (defined $opt_d);

    LOOP: foreach $line (@before) {
        printf ("%10s %s", $state, $line) if (defined $opt_D);

        if ($state eq 'done') {
            push @after, $line;
            next LOOP;
        }

        if ($state eq 'before') {
            if ($line =~ /\-\*\- C\+\+ \-\*\-/) {
                push @after, $line;
            }
            elsif ($line =~ /\$Id\:(.*)\n/) {
                $id = "\$Id\:$1";
            }
            elsif ($line =~ /===================/) {
                push @after, "//========================================".
                             "=====================================\n";
                push @after, "/**\n";
            }
            elsif ($line =~ /\= FILENAME/) {
                $state = 'filename';
                next LOOP;
            }
            elsif ($line !~ /^\s*\/\//) {
                push @after, $line;
            }
        }

        if ($state eq 'filename') {
            if ($line =~ /\/\/   (.+)/) {
                push @after, " *  \@file   $1\n";
                push @after, " *\n";
                push @after, " *  $id\n";
                push @after, " *\n";
            }
            elsif ($line =~ /\= DESCRIPTION/) {
                $state = 'description';
                next LOOP;
            }
            elsif ($line =~ /\= AUTHOR/) {
                $state = 'author';
                next LOOP;
            }
            elsif ($line =~ /===================/) {
                $state = 'after';
                ### Fall through so the after can put the ending in
            }
        }

        if ($state eq 'description') {
            if ($line =~ /\= AUTHOR/) {
                push @after, format_description (@description);
                @description = ();
                push @after, " *\n";
                $state = 'author';
                next LOOP;
            }
            elsif ($line =~ /===================/) {
                push @after, format_description (@description);
                @description = ();
                push @after, " *\n";
                $state = 'after';
                ### Fall through
            }
            push @description, $line;
        }
        if ($state eq 'author') {
            if ($line =~ /\/\/   (.+)\n/) {
                $authors .= $1;
            }
            elsif ($line =~ /===================/
                   || $line =~ /\= DESCRIPTION/) {
                ## print the authors

                if ($authors ne "") {
                    @authors = split /\,/, $authors;

                    foreach $author (@authors) {
                        if ($author =~ /^ (.*)/) {
                            $author = $1;
                        }
                        push @after, " *  \@author $author\n";
                    }
                }

                if ($line =~ /\= DESCRIPTION/) {
                    push @after, " *\n";
                    $state = 'description';
                    next LOOP;
                }
                else {
                    $state = 'after';
                    ## Fall through
                }
            }
        }

        if ($state eq 'after') {
            if ($line =~ /===================/) {
                ## print the rest
                push @after, " */\n";
                push @after, "//========================================".
                             "=====================================\n";
                push @after, "\n";
                $state = 'done';
            }

            next LOOP;
        }
    }

    return @after;
}


sub fix_class_headers (@)
{
    my (@before) = @_;
    my @after = ();
    my @store = ();
    my $classname = "";

    my $state = 'outside';
    ## state =
    ##  outside   = not in class
    ##  template  = stored template line
    ##  class     = started collecting lines, in case of a class
    ##  header    = after a class foo, but before any methods

    print "Fixing class headers\n" if (defined $opt_d);

    LOOP: foreach $line (@before) {
        printf ("%10s %s", $state, $line) if (defined $opt_D);

        if ($state eq 'outside') {
            if ($line =~ /^\s*template/) {
                push @store, $line;
                $state = 'template';
                next LOOP;
            }
            elsif ($line =~ /^\s*class/) {
                $state = 'class';
                ## Fall through
            }
            else {
                push @after, $line;
            }

        }

        if ($state eq 'template') {
            if ($line =~ /^\s*class/) {
                $state = 'class';
                ## Fall through
            }
            else {
                push @after, @store;
                @store = ();
                push @after, $line;
                $state = 'outside';
                next LOOP;
            }
        }

        if ($state eq 'class') {
            if ($line =~ /^\s*class(.*)\n/) {
                push @store, $line;
                my @s = split / /, $1;
                if ($s[1] =~ /export$/i) {
                    $classname = $s[2];
                }
                else {
                    $classname = $s[1];
                }
            }
            elsif ($line =~ /^\s*\{/) {
                push @store, $line;
            }
            elsif ($line =~ /^\s*\/\//) {
                $state = 'header';
                ### Fall through
            }
            else {
                push @after, @store;
                @store = ();
                push @after, $line;
                $state = 'outside';
                next LOOP;
            }
        }

        if ($state eq 'header') {
            if ($line =~ /^\s*\/\//) {
                push @headers, $line;
            }
            else {
                my $prefix = '';

                $line =~ /^(\s*)[\w\/]/;  ### used to get indent
                my $indent = $1;
                push @after, "$indent/**\n";
                push @after, "$indent * \@class $classname\n";

                foreach $header (@headers) {
                    if ($header =~ /\= TITLE/) {
                        push @after, "$indent *\n";
                        $prefix = "$indent * \@brief";
                    }
                    elsif ($header =~ /\= DESCRIPTION/) {
                        push @after, "$indent *\n";
                        $prefix = "$indent *";
                    }
                    elsif ($header !~ /\/\/\s*\n/) {
                        my $myline = $header;
                        $myline =~ s/\s*\/\/\s*/$prefix /;
                        push @after, $myline;
                        $prefix = "$indent *";

                    }
                }
                push @after, "$indent */\n";
                @headers = ();

                push @after, @store;
                push @after, $line;
                @store = ();
                $state = 'outside';
                next LOOP;
            }
        }
    }

    return @after;
}


sub format_comment (@)
{
    my (@comments) = @_;
    my @after = ();

    my $line;

    if ($#comments < 2) {
        foreach $line (@comments) {
            $line =~ s/\/\//\/\/\//;
            push @after, $line;
        }
    }
    else {
        my $line = $comments[0];
        $line =~ /^(\s*)\//;
        my $indent = $1;

        push @after, "$indent/**\n";
        foreach $line (@comments) {
            $line =~ s/\/\// */;
            push @after, $line;
        }
        push @after, "$indent */\n";
    }

    return @after;
}

sub fix_class_members (@)
{
    my (@before) = @_;
    my @after = ();
    my @method = ();
    my @comment = ();

    my $classfound = 0;
    my $classlevel = 0;
    my $level = 0;

    print "Fixing class methods\n" if (defined $opt_d);

    LOOP: foreach $line (@before) {
        if ($line =~ /\{/ && $line !~ /^\s*\/\//) {
            $level++;
        }

        if ($line =~ /^\s*class/
            && $line !~ /\;/
            && $level == $classlevel)
        {
            $classlevel++;
        }

        if ($line =~ /\}/ && $line !~ /^\s*\/\//) {
            if ($classlevel == $level) {
                $classlevel--;
            }
            $level--;
        }

        printf ("%2d%2d", $level, $classlevel) if (defined $opt_D);

        if ($level == $classlevel && $level > 0) {
            if ($line =~ /^\s*public/
                || $line =~ /^\s*private/
                || $line =~ /\s*protected/
                || $line =~ /^\s*\n$/
                || $line =~ /^\s*\{/
                || $line =~ /^\s*\}/
                || $line =~ /^\s*\#/)
            {
                push @after, format_comment (@comment);
                push @after, @method;
                @comment = ();
                @method = ();

                print "  $line" if (defined $opt_D);
                push @after, $line;
            }
            elsif ($line =~ /^\s*\/\//) {
                print "C $line" if (defined $opt_D);

                if ($#method >= 0) {
                    push @comment, $line;
                }
                else {
                    push @after, $line;
                }
            }
            else {
                print "M $line" if (defined $opt_D);
                push @method, $line;
            }

        }
        else {
            push @after, format_comment (@comment);
            push @after, @method;
            @comment = ();
            @method = ();

            print "  $line" if (defined $opt_D);
            push @after, $line;
        }
    }

    if ($level > 0 || $classlevel > 0) {
        $fail = 1;
        $failmessage = "Brace level recognition failed"
    }

    return @after;
}

##############################################################################
# Read in the files.

FILELOOP: foreach $file (@files) {
    print "\n" if (defined $opt_d);
    print "$file\n";
    print "\n" if (defined $opt_d);

    $fail = 0;

    my @contents = ();

    ### Read file into @contents
    print "Reading\n" if (defined $opt_d);

    unless (open (FILE, "<$file")) {
        print STDERR "$file: $!\n";
        next FILELOOP;
    }

    @contents = <FILE>;

    close (FILE);

    ### Verify file
    print "Verifying file\n" if (defined $opt_d);

    if (!defined $opt_u) {
        if (verify (@contents) == 1) {
            print "$file did not pass verification\n";
            next FILELOOP;
        }
        elsif (defined $opt_d) {
            print "Passed verification\n";
        }
    }

    ### Fix up parts of it
    print "Fixing file\n" if (defined $opt_d);

    @contents = fix_file_header (@contents);
    @contents = fix_class_headers (@contents);
    @contents = fix_class_members (@contents);

    if ($fail != 0) {
        print "$file: $failmessage\n";
    }
    else {
        if (defined $opt_s) {
            print @contents;
        }
        elsif (!defined $opt_D) {
            ### Save @contents back to the file
            print "Saving\n" if (defined $opt_d);

            unless (open (FILE, ">$file")) {
                print STDERR "$file: $!\n";
                next FILELOOP;
            }

            foreach $line (@contents) {
                print FILE $line;
            }

            close (FILE);
        }
    }
}

