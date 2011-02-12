#! /usr/bin/perl
eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}'
    & eval 'exec perl -w -S $0 $argv:q'
    if 0;

# $Id: split-cpp.pl 85436 2009-05-25 21:57:28Z coryan $
#
# Splits C++ source files into one file per function or data item.
#
# Author: David L. Levine, with much help and encouragment from
#         Umar Syyid and Gonzalo A. Diethelm.
#         Completed by Andrew Gilpin, July 2000
# Date:   10 November 1998
#
# For each C++ source file:
# 1) Extracts the "intro" code, i.e., #includes and declarations.
# 2) Identifies function definitions, relying on {, and } at the
#    beginning of a line, to delineate the function begin and
#    end.
#
# Assumptions: (applies only to the files being split, i.e. .cpp files)
# * Function definition bodies are terminated with } appearing at
#   the beginning of a line.
# * Free-standing (outside of functions) macro invocations must be
#   followed by a blank line, or terminated with a semicolon.
# * A function must not have a blank line between its header
#   (signature) and its body.
# * There aren't multiple C-style comments on one line, with code
#   between them.
# * typedefs are on a single line
# * A #endif doesn't have a multi-line C comment starting on that line.

# The first three lines above let this script run without specifying the
# full path to perl, as long as it is in the user's PATH.
# Taken from perlrun man page.

# Changes made by Andrew Gilpin (June - July 2000)
# * Added option -c to use .c extension instead of .cpp extension
# * Prints message when no filenames are specified on the command line
# * Changed -? option to -h so that it works properly in most shells
# * Added option -s to skip certain files, but copy them to $split_dir,
#   renaming them. (filename.cpp -> $split_dir/filename_S1.cpp). This is
#   here so that ACE can selectively not split certain files (namely those
#   that this script doesn't work with :)
# * Added support for classes declared in the .cpp file.

$usage="usage: $0 [-h] [-d] [-v] [-c] [-s filename] filenames\n";

#### Configuration parameters.
$verbose = 0;
$debug = 0;
$split_dir = 'SPLIT';
$extension = 'cpp';
@files_to_skip = ();

#### Constants.
$DIR_SEPARATOR = $^O eq "MSWin32"  ?  '\\'  :  '/';


####
#### Process command line args.
####
while ( $#ARGV >= $[  &&  $ARGV[0] =~ /^-/ ) {
  if ( $ARGV[0] eq '-d' ) {
    $debug = 1;
  } elsif ( $ARGV[0] eq '-v' ) {
    $verbose = 1;
  } elsif ( $ARGV[0] eq '-c' ) {
    $extension = 'c';
  } elsif ( $ARGV[0] eq '-s' ) {
    push @files_to_skip, $ARGV[1];
    shift;
  } elsif ( $ARGV[0] eq '-h' ) {
    print "$usage";
    exit;
  } else {
    print STDERR "$0:  unknown option $ARGV[0]\n";
    die $usage;
  }
  shift;
}


&main ();


####
#### Reset state, to process a new file starting with a clean slate.
####
sub reset {
    #### Working data buffers.
  @intro = ();
  @current_comments = ();
  @current_code = ();
  @if = ();
  @save_if = ();
  @endif = ();
  @unknown = ();
  ####@unknown_s = ();

  #### State variables.
  $current_file_number = 0;
  $top_of_file = 1;
  $in_braces = 0;
  $in_nonfunction_code = 0;
  $in_C_comment = 0;
  $intro_length = 0;
  $preprocessor_continuation = 0;
  $preserved_ifs = 0;
}


sub main {
  #### Print error message if no files are specified.
  #### We need to do this before we modify anything on disk.
  die "No files specified!\n$usage" if (@ARGV == 0);

  #### Remove the destination subdirectory, if it exists.
  #### Attempts to clean it out using unlink may fail because
  #### it can have many files.
  if (-d "$split_dir") {
    system ("/bin/rm -r $split_dir") << 256  &&
      die "$0: unable to rm \"$split_dir\"\n";
  }

  #### Create the destination subdirectory.
  mkdir "$split_dir", 0755  ||
    die "$0: unable to create $split_dir directory: $!\n";

  MAIN_LOOP: foreach $file (@ARGV) {
    #### Strip off filename extension.
    ($basename = $file) =~ s/\.[^\.]+$//;

    foreach $skip_file (@files_to_skip) {
      if ($skip_file eq $file) {
        system ("/bin/cp $file $split_dir/" . $basename. "_S1\.$extension");
	next MAIN_LOOP;
      }
    }

    &reset ();

    print "FILE: $file\n" if $verbose;
    open INPUT, "$file"  ||  die "$0: unable to open \"$file\"\n";

    while (<INPUT>) {
      #### Strip comments from $line and use that for processing.
      #### But, use $_ for output, so that comments will be preserved.
      my $line = $_;

      #### If we're in the midst of a multiline C comment, see
      #### if it's finished on this line.
      if ($in_C_comment) {
        if ($line =~ s%^.*\*/%%) {
          #### End C-style comment.
          $in_C_comment = 0;

          if ($line =~ /^\s*$/  &&  ! $in_braces) {
            #### No code on the line.
            #&save_comment ($_);
            next;
          }
        } else {
          unless ($in_braces) {
            #&save_comment ($_);
            next;
          }
        }
      }

      #### Strip C++-style comments.
      if ($line =~ s%\s*//.*$%%) {
        if ($line =~ /^\s*$/  &&  ! $in_braces) {
          #### C++-style comment, without any code on the line.
          #&save_comment ($_);
          next;
        }
      }

      #### And C-style comments.
      if ($line =~ m%/\*%) {
        #### Begin C-style comment.  Strip any complete comment(s),
        #### then see what's left.

        $line =~ s%\s*/\*.*\*/\s*%%g;

        #### check to see if a preprocessor is on this line
        if (! $in_braces) {
          if ($line eq '') {
          #### The line just had comment(s).  Save it.
            #&save_comment ($_);
            next;
          } else {
            #### There's other text on the line.  See if it's just the
            #### start of a comment.
            if ($line =~ m%/\*%  &&  $line !~ m%\*/%) {
              #### The C-style comment isn't terminated on this line.
              $in_C_comment = 1;
              #&save_comment ($_);
              next;
            }
          }
        }
      }

      #### For now, skip ACE_RCSID's.  Eventually, we might want to
      #### consider putting them in _every_ file, if they're enabled.
      next if $line =~ /^ACE_RCSID/;

      if ($in_braces) {
        push @unknown, $_;
        if ($line =~ /{/) {
          ++$in_braces;
        } elsif ($line =~ /^};/) {
          #### }; at beginning of line could signify end of class
          --$in_braces;
          if ($in_braces == 0) {
            push @intro, @unknown;
            @unknown = ();
          }
        } elsif ($line =~ /^}/) {
          #### } at beginning of line signifies end of function.
          --$in_braces;
          push @current_code, @unknown;
          @unknown = ();
          &finish_current ($basename, ++$current_file_number);
        } elsif ($line =~ /};/) {
          #### end of multi-line data delcaration
          --$in_braces;
          if ($in_braces == 0) {
            push @current_code, @unknown;
            @unknown = ();
            &finish_current ($basename, ++$current_file_number);
          }
        }
      } else {
        #### Not in braces.
        if (($line =~ m%[^/]*{%) && (! $preprocessor_continuation)) {
          #### { signifies beginning of braces (obviously :).
          if ($line =~ /};/) {
            #### braces end on this line
            push @unknown, $_;
            push @current_code, @unknown;
            @unknown = ();
            &finish_current ($basename, ++$current_file_number);
          } else {
            push @unknown, $_;
            $in_braces = 1;
            $in_nonfunction_code = $top_of_file = 0;
          }
        } elsif ($line =~ /^}/) {
          warn "$0: skipping unexpected } on line $. of \"$file\"\n";
          next;
        } elsif ($line =~ /^typedef/) {
          push @intro, $_;
        } elsif ($line =~ /^\s*#/  ||  $preprocessor_continuation) {
          #### Preprocessor directive.
          if ($in_nonfunction_code) {
            push @unknown, $_;
          } else {
            push @intro, $_;
          }
          $top_of_file = 0;
          $preprocessor_continuation = /\\$/  ?  1  :  0;

          if ($line =~ m%^\s*#\s*if\s*(.*)(/.*)*$%) {
            push @save_if, $_;
            unshift @endif, "#endif /* $1 [Added by split-cpp.] */\n";

          } elsif ($line =~ /^\s*#\s*endif/) {
            #### End an #if/#else block.
            unless (defined pop @save_if) {
              pop @if;
              if ($preserved_ifs > 0) {
                --$preserved_ifs;
              }
            }
            shift @endif;

          #### } elsif ($line =~ /^\s*#/) {
            #### Any other preprocessor directive.
          }

        } elsif ($line =~ /^\s*$/) {
          #### Whitespace only, or empty line..
          push @current_code, "\n";
          if ($in_nonfunction_code) {
            #### In the midst of non-function code, we reached a
            #### blank line.  Assume that we're done with it.
            &finish_current ($basename, ++$current_file_number);
          } else {
            #### Not in a function, so add to intro.  Just in case data or
            #### a function follow it, flush now.
            $preserved_ifs += $#save_if + 1;
            &flush_current (\@intro);
          }

        } elsif ($line =~ /;/) {
          #### Data definition or semicolon-terminated macro invocation.
          push @unknown, $_;
          $top_of_file = 0;

          #### Is it file-static?  Squash newlines out of @current_code.
          my $statement = join (' ', @current_code);
          if ($statement =~ /([^=[(]+)[=[(](.*)/) {
            if ($1 =~ /static/) {
              #### Move code to the intro.
              push @intro, @current_comments;
              @current_comments = ();
              &flush_current (\@intro);

              #### Not separate code.
              $in_nonfunction_code = 0;

              #### ???? Extract name from the left side and save for
              #### later matching.
            } else {
              if ($statement =~ /^USEUNIT\s*\(/) {
                #### Special-case those Borland USEUNIT things.
                &flush_current (\@intro);
              } else {
                #### Non-static entity, with semicolon.  Wrap it up.
                push @current_code, @unknown;
                @unknown = ();
                &finish_current ($basename, ++$current_file_number);
              }
            }
          } else {
            #### Dunno.  Wrap it up, anyways.
            push @current_code, @unknown;
            @unknown = ();
            &finish_current ($basename, ++$current_file_number);
          }
        } else {
          #### Beginning of data definition or function or class.
          push @unknown, $_;
          $in_nonfunction_code = 1;
          $top_of_file = 0;
        }
      }

      if (eof) {
        close (ARGV);  #### To reset line number counter.
        if ($#intro > $intro_length) {
          #### Leftover prepreprocessor statement(s), such as #pragma
          #### instantiate.
          &finish_current ($basename, ++$current_file_number);
        }
      }
    }

    close INPUT;
  }
};


####
#### Save a comment in the appropriate array.
####
#sub save_comment {
#  my ($comment) = @_;
#
#  if ($top_of_file) {
#    push @intro, $comment;
#  } else {
#    push @current_comments, $comment;
#  }
#}


####
#### Flush the contents of the @current_code array to the destination
#### argument array.  It is passed by reference.
####
sub flush_current {
  my ($destination) = @_;

  push @$destination, @current_code;
  @current_code = ();
}


####
#### Flush what we've got now to an output (split) file.
####
sub finish_current {
  my ($basename, $current_file_number) = @_;

  my $current_file_name =
    sprintf "$split_dir$DIR_SEPARATOR${basename}_S%d.$extension",
      $current_file_number++;

  if ($verbose) {
    print "CURRENT OUTPUT FILE: $current_file_name\n";
    print "INTRO:\n";
    print @intro;
    print @if;
    print @current_comments;
    print "CURRENT CODE:\n";
    print @current_code;
    print @endif;
  }

  open OUTPUT, "> $current_file_name"  ||
    die "unable to open $current_file_name\n";

  print OUTPUT "// Automatically generated by ACE's split-cpp.\n" .
               "// DO NOT EDIT!\n\n";
  if ($debug) {
      print OUTPUT "INTRO:\n", @intro, "IF:\n", @if,
                   "COMMENTS:\n", @current_comments,
                   "CURRENT:\n", @current_code, "ENDIF:\n", @endif;
  } else {
      print OUTPUT @intro, @if, @current_comments, @current_code, @endif;
  }

  close OUTPUT;

  #### For detection of leftover preprocessor statements and
  #### comments at end of file.
  $intro_length = $#intro;

  @current_comments = @current_code = @save_if = ();
  $in_braces = $in_nonfunction_code = 0;
}
