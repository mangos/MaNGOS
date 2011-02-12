eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id: fuzz.pl 92287 2010-10-20 18:24:39Z johnnyw $
#   Fuzz is a script whose purpose is to check through ACE/TAO/CIAO files for
#   easy to spot (by a perl script, at least) problems.

use lib "$ENV{ACE_ROOT}/bin";
if (defined $ENV{srcdir}) {
  use lib "$ENV{srcdir}/bin";
}

use Cwd;
use File::Find;
use File::Basename;
use Getopt::Std;
use PerlACE::Run_Test;

###### TODO
#
# Add tests for these:
#
# - Guards in .h files
# - no global functions
# - other commit_check checks
#
# And others in ACE_Guidelines and Design Rules
#
# Also add a -g flag to ignore tao_idl generated files
#
###### END TODO

# Lists of all the files
@files_cdp = ();
@files_cpp = ();
@files_inl = ();
@files_h = ();
@files_html = ();
@files_dsp = ();
@files_dsw = ();
@files_gnu = ();
@files_idl = ();
@files_pl = ();
@files_changelog = ();
@files_makefile = ();
@files_mpc = ();
@files_bor = ();
@files_noncvs = ();
@files_sln = ();
@files_vcproj = ();
@files_run_pl = ();
@files_generic = ();
@files_doxygen = ();

# To keep track of errors and warnings
$errors = 0;
$warnings = 0;

##############################################################################

# Find_Modified_Files will use 'cvs -nq' to get a list of locally modified
# files to look through
sub find_mod_files ()
{
    unless (open (CVS, "cvs -nq up |")) {
        print STDERR "Error: Could not run cvs\n";
        exit (1);
    }

    while (<CVS>) {
        if (/^[M|A] (.*)/) {
            store_file ($1);
        }
    }
    close (CVS);
}



# Find_Files will search for files with certain extensions in the
# directory tree
sub find_files ()
{
    # wanted is only used for the File::Find
    sub wanted
    {
        store_file ($File::Find::name);
    }

    find (\&wanted, '.');
}

#
sub store_file ($)
{
    my $name = shift;
    if ($name =~ /\.(c|cc|cpp|cxx|tpp)$/i) {
        push @files_cpp, ($name);
    }
    elsif ($name =~ /\.(inl|i)$/i) {
        push @files_inl, ($name);
    }
    elsif ($name =~ /\.(h|hh|hpp|hxx)$/i) {
        push @files_h, ($name);
    }
    elsif ($name =~ /\.(htm|html)$/i) {
        push @files_html, ($name);
    }
    elsif ($name =~ /\.(bor)$/i) {
        push @files_bor, ($name);
    }
    elsif ($name =~ /\.(GNU)$/i) {
        push @files_gnu, ($name);
    }
    elsif ($name =~ /\.(dsp|vcp)$/i) {
        push @files_dsp, ($name);
    }
    elsif ($name =~ /\.(dsw|vcp)$/i) {
        push @files_dsw, ($name);
    }
    elsif ($name =~ /\.(pidl|idl|idl3|idl3p)$/i) {
        push @files_idl, ($name);
    }
    elsif ($name =~ /\.pl$/i) {
        push @files_pl, ($name);
        if ($name =~ /^run.*\.pl$/i) {
            push @files_run_pl, ($name);
        }
    }
    elsif ($name =~ /\.vcproj$/i) {
        push @files_vcproj, ($name);
    }
    elsif ($name =~ /\.sln$/i) {
        push @files_sln, ($name);
    }
    elsif ($name =~ /ChangeLog/i && -f $name) {
        push @files_changelog, ($name);
    }
    elsif ($name =~ /\/GNUmakefile.*.[^~]$/) {
        push @files_makefile, ($name);
    }
    elsif ($name =~ /\.(mpc|mwc|mpb|mpt)$/i) {
        push @files_mpc, ($name);
    }
    elsif ($name =~ /\.(icc|ncb|zip)$/i) {
        push @files_noncvs, ($name);
    }
    elsif ($name =~ /\.(cdp)$/i) {
        push @files_cdp, ($name);
    }
    elsif ($name =~ /\.(doxygen)$/i) {
        push @files_doxygen, ($name);
    }
    elsif ($name =~ /\.(pm|cmd|java|sh|txt|xml)$/i) {
        push @files_generic, ($name);
    }
}

##############################################################################
## Just messages

sub print_error ($)
{
    my $msg = shift;
    print "Error: $msg\n";
    ++$errors;
}


sub print_warning ($)
{
    my $msg = shift;
    print "Warning: $msg\n";
    ++$warnings;
}


##############################################################################
## Tests

# The point of this test is to check for the existence of ACE_INLINE
# or ASYS_INLINE in a .cpp file.  This is most commonly caused by
# copy/pasted code from a .inl/.i file
sub check_for_inline_in_cpp ()
{
    print "Running ACE_INLINE/ASYS_INLINE check\n";
    foreach $file (@files_cpp) {
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/^ACE_INLINE/) {
                    print_error ("$file:$.: ACE_INLINE found");
                }
                if (/^ASYS_INLINE/) {
                    print_error ("$file:$.: ASYS_INLINE found");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks to make sure files have the $Id string in them.
# Commit_check should find these when checking in files, but this can
# be used locally or to check for files
sub check_for_id_string ()
{
    print "Running \$Id\$ string check\n";
    foreach $file (@files_cpp, @files_inl, @files_h, @files_mpc, @files_bor, @files_gnu,
                   @files_html, @files_idl, @files_pl, @makefile_files, @files_cdp) {
        my $found = 0;
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/\$Id\:/ or /\$Id\$/) {
                    $found = 1;
                }
                if (/\$id\$/) {
                    print_error ("$file:$.: Incorrect \$id\$ found (correct casing)");
                }
                if (/\$Id:\$/) {
                    print_error ("$file:$.: Incorrect \$Id:\$ found (remove colon)");
                }
                if (/\$Id\$/) {
                    print_error ("$file:$.: Seems to lack svn:keywords property");
                }
            }
            close (FILE);
            if ($found == 0) {
                print_error ("$file:1: No \$Id\$ string found.");
            }
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# check for _MSC_VER
sub check_for_msc_ver_string ()
{
    print "Running _MSC_VER check\n";
    foreach $file (@files_cpp, @files_inl, @files_h) {
        my $found = 0;
        if (open (FILE, $file)) {
            my $disable = 0;
            my $mscline = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_msc_ver/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_msc_ver/) {
                    $disable = 0;
                }
                if ($disable == 0 and /\_MSC_VER \<= 1200/) {
                    $found = 1;
                    $mscline = $.;
                }
                if ($disable == 0 and /\_MSC_VER \>= 1200/) {
                    $found = 1;
                    $mscline = $.;
                }
                if ($disable == 0 and /\_MSC_VER \> 1200/) {
                    $found = 1;
                    $mscline = $.;
                }
                if ($disable == 0 and /\_MSC_VER \< 1300/) {
                    $found = 1;
                    $mscline = $.;
                }
                if ($disable == 0 and /\_MSC_VER \<= 1300/) {
                    $found = 1;
                    $mscline = $.;
                }
                if ($disable == 0 and /\_MSC_VER \>= 1300/) {
                    $found = 1;
                    $mscline = $.;
                }
                if ($disable == 0 and /\_MSC_VER \< 1310/) {
                    $found = 1;
                    $mscline = $.;
                }
                if ($disable == 0 and /\_MSC_VER \>= 1310/) {
                    $found = 1;
                    $mscline = $.;
                }
            }
            close (FILE);
            if ($found == 1) {
               print_error ("$file:$mscline: Incorrect _MSC_VER check found");
            }
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the newline at the end of a file
sub check_for_newline ()
{
    print "Running newline check\n";
    foreach $file (@files_cpp, @files_inl, @files_h,
                   @files_html, @files_idl, @files_pl) {
        if (open (FILE, $file)) {
            my $line;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                $line = $_
            }
            close (FILE);
            if ($line !~ /\n$/) {
                print_error ("$file:$.: No ending newline found in $file");
            }
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}


# This test checks for files that are not allowed to be in svn
sub check_for_noncvs_files ()
{
    print "Running non svn files check\n";
    foreach $file (@files_noncvs, @files_dsp, @files_dsw, @files_makefile, @files_bor) {
        print_error ("File $file should not be in svn!");
    }
}

# This test checks for the use of ACE_SYNCH_MUTEX in TAO/CIAO,
# TAO_SYNCH_MUTEX should used instead.

sub check_for_ACE_SYNCH_MUTEX ()
{
    print "Running ACE_SYNCH_MUTEX check\n";
    ITERATION: foreach $file (@files_cpp, @files_inl, @files_h) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_ACE_SYNCH_MUTEX/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_ACE_SYNCH_MUTEX/) {
                    $disable = 0;
                    next;
                }
                if ($disable == 0 and /ACE_SYNCH_MUTEX/) {
                    # It is okay to use ACE_SYNCH_MUTEX in ACE
                    # so don't check the ACE directory. The below
                    # will check it for TAO and CIAO.
                    if (($file !~ /.*TAO.*/)) {
                      next ITERATION;
                    }

                    # Disable the check in the ESF directory for the
                    # time being until we fix the issues there.
                    if(($file =~ /.*TAO\/orbsvcs\/orbsvcs\/ESF.*/)) {
                      next ITERATION;
                    }

                    print_error ("$file:$.: found ACE_SYNCH_MUTEX, use TAO_SYNCH_MUTEX instead");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for not having export files in CIAO, all have to be ---
# generated using TAO_IDL. If you have a file that must be in the repository
# remove the generated automatically by line
sub check_for_export_file ()
{
    print "Running export file check\n";
    ITERATION: foreach $file (@files_h) {
        if (($file =~ /.*CIAO.*export.h/) || ($file =~ /.*DAnCE.*export.h/)) {
          if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
              if (/generated automatically by/) {
                print_error ("$file:$.: found should be generated by TAO_IDL, check -Gxh** option");
              }
            }
            close (FILE);
          }
          else {
            print STDERR "Error: Could not open $file\n";
          }
        }
    }
}


# This test checks for the use of ACE_Thread_Mutex in TAO/CIAO,
# TAO_SYNCH_MUTEX should used instead to make the code build
# in single-threaded builds.
sub check_for_ACE_Thread_Mutex ()
{
    print "Running ACE_Thread_Mutex check\n";
    ITERATION: foreach $file (@files_cpp, @files_inl, @files_h) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_ACE_Thread_Mutex/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_ACE_Thread_Mutex/) {
                    $disable = 0;
                }
                if ($disable == 0 and /ACE_Thread_Mutex/) {
                    # It is okay to use ACE_Thread_Mutex in ACE
                    # so don't check the ACE directory. The below
                    # will check it for TAO and CIAO.
                    if (($file !~ /.*TAO.*/)) {
                      next ITERATION;
                    }

                    print_error ("$file:$.: found ACE_Thread_Mutex, use TAO_SYNCH_MUTEX instead to allow the code to work in single-threaded builds");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the use of ACE_Guard
# ACE_GUARD should used because it checks if we really got a lock
# in single-threaded builds.
sub check_for_ACE_Guard ()
{
    print "Running ACE_Guard check\n";
    ITERATION: foreach $file (@files_cpp, @files_inl, @files_h) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_ACE_Guard/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_ACE_Guard/) {
                    $disable = 0;
                    next;
                }
                if ($disable == 0 and /ACE_Guard/) {
                    print_error ("$file:$.: found ACE_Guard, use ACE_GUARD");
                }
                if ($disable == 0 and /ACE_Read_Guard/) {
                    print_error ("$file:$.: found ACE_Read_Guard, use ACE_READ_GUARD");
                }
                if ($disable == 0 and /ACE_Write_Guard/) {
                    print_error ("$file:$.: found ACE_Write_Guard, use ACE_WRITE_GUARD");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the use of tabs, spaces should be used instead of tabs
sub check_for_tab ()
{
    print "Running tabs check\n";
    ITERATION: foreach $file (@files_cpp, @files_inl, @files_h, @files_idl, @files_cdp, @files_doxygen) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_tab/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_tab/) {
                    $disable = 0;
                }
                if ($disable == 0 and /.*\t.*/) {
                    print_error ("$file:$.: found tab");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

sub check_for_trailing_whitespace ()
{
    print "Running trailing_whitespaces check\n";
    ITERATION: foreach $file (@files_cpp, @files_inl, @files_h, @files_idl,
                              @files_cdp, @files_pl, @files_generic) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_trailing_whitespace/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_trailing_whitespace/) {
                    $disable = 0;
                }
                if ($disable == 0 and /\s\n$/) {
                    print_error ("$file:$.: found trailing whitespace");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the lack of ACE_OS
sub check_for_lack_ACE_OS ()
{
    $OS_NS_arpa_inet_symbols = "inet_addr|inet_aton|inet_ntoa|inet_ntop|inet_pton";

    $OS_NS_ctype_symbols = "isalnum|isalpha|iscntrl|isdigit|isgraph|islower|isprint|ispunct|isspace|isupper|isxdigit|tolower|toupper|isblank|isascii|isctype|iswctype";

    $OS_NS_dirent_symbols = "closedir|opendir|readdir|readdir_r|rewinddir|scandir|alphasort|seekdir|telldir|opendir_emulation|scandir_emulation|closedir_emulation|readdir_emulation";

    $OS_NS_dlfcn_symbols = "dlclose|dlerror|dlopen|dlsym";

    $OS_NS_errno_symbols = "last_error|set_errno_to_last_error|set_errno_to_wsa_last_error";

    $OS_NS_fcntl_symbols = "fcntl|open";

    $OS_NS_math_symbols = "floor|ceil|log2";

    $OS_NS_netdb_symbols = "gethostbyaddr|gethostbyaddr_r|gethostbyname|gethostbyname_r|getipnodebyaddr|getipnodebyname|getmacaddress|getprotobyname|getprotobyname_r|getprotobynumber|getprotobynumber_r|getservbyname|getservbyname_r|netdb_acquire|netdb_release";

    $OS_NS_poll_symbols = "poll";

    $OS_NS_pwd_symbols = "endpwent|getpwent|getpwnam|getpwnam_r|setpwent";

    $OS_NS_regex_symbols = "compile|step";

    $OS_NS_signal_symbols = "kill|pthread_sigmask|sigaction|sigaddset|sigdelset|sigemptyset|sigfillset|sigismember|signal|sigprocmask|sigsuspend|raise";

    $OS_NS_stdio_symbols = "fileno|checkUnicodeFormat|clearerr|cuserid|fclose|fdopen|fflush|fgetc|getc|fgetpos|fgets|flock_adjust_params|flock_init|flock_destroy|flock_rdlock|flock_tryrdlock|flock_trywrlock|flock_unlock|flock_wrlock|fopen|default_win32_security_attributes|default_win32_security_attributes_r|get_win32_versioninfo|get_win32_resource_module|set_win32_resource_module|fprintf|ungetc|fputc|putc|fputs|fread|freopen|fseek|fsetpos|ftell|fwrite|perror|printf|puts|rename|rewind|snprintf|sprintf|tempnam|vsprintf|vsnprintf|asprintf|aswprintf|vasprintf|vaswprintf";

    $OS_NS_stdlib_symbols = "_exit|abort|atexit|atof|atol|atoi|atop|bsearch|calloc|exit|free|getenv|getenvstrings|itoa|itoa_emulation|itow_emulation|malloc|mkstemp|mkstemp_emulation|mktemp|setenv|unsetenv|putenv|qsort|rand|rand_r|realloc|realpath|set_exit_hook|srand|strenvdup|strtod|strtol|strtol_emulation|strtoul|strtoul_emulation|strtoll|strtoll_emulation|strtoull|strtoull_emulation|system|getprogname|setprogname";

    $OS_NS_string_symbols = "memchr|memchr_emulation|memcmp|memcpy|fast_memcpy|memmove|memset|strcat|strchr|strcmp|strcpy|strcspn|strdup|strdup_emulation|strecpy|strerror|strerror_emulation|strsignal|strlen|strncat|strnchr|strncmp|strncpy|strnlen|strnstr|strpbrk|strrchr|strrchr_emulation|strsncpy|strspn|strstr|strtok|strtok_r|strtok_r_emulation";

    $OS_NS_strings_symbols = "strcasecmp|strncasecmp|strcasecmp_emulation";

    $OS_NS_stropts_symbols = "getmsg|getpmsg|fattach|fdetach|ioctl|isastream|putmsg|putpmsg";

    $OS_NS_sys_mman_symbols = "madvise|mmap|mprotect|msync|munmap|shm_open|shm_unlink";

    $OS_NS_sys_msg_symbols = "msgctl|msgget|msgrcv|msgsnd";

    $OS_NS_sys_resource_symbols = "getrlimit|getrusage|setrlimit";

    $OS_NS_sys_select_symbols = "select";

    $OS_NS_sys_sendfile_symbols = "sendfile|sendfile_emulation";

    $OS_NS_sys_shm_symbols = "shmat|shmctl|shmdt|shmget";

    $OS_NS_sys_socket_symbols = "accept|bind|closesocket|connect|enum_protocols|getpeername|getsockname|getsockopt|join_leaf|listen|recv|recvfrom|recvmsg|recvv|send|sendmsg|sendto|sendv|setsockopt|shutdown|if_nametoindex|if_indextoname|if_nameindex|socket_init|socket_fini|socket|socketpair";

    $OS_NS_sys_stat_symbols = "creat|filesize|fstat|lstat|mkdir|mkfifo|stat|umask";

    $OS_NS_sys_time_symbols = "gettimeofday";

    $OS_NS_sys_uio_symbols = "readv|readv_emulation|writev|writev_emulation";

    $OS_NS_sys_utsname_symbols = "uname";

    $OS_NS_sys_wait_symbols = "wait|waitpid";

    $OS_NS_Thread_symbols = "cleanup_tss|condattr_init|condattr_destroy|cond_broadcast|cond_destroy|cond_init|cond_signal|cond_timedwait|cond_wait|event_destroy|event_init|event_pulse|event_reset|event_signal|event_timedwait|event_wait|lwp_getparams|lwp_setparams|mutex_destroy|mutex_init|mutex_lock|mutex_lock_cleanup|mutex_trylock|mutex_unlock|priority_control|recursive_mutex_cond_unlock|recursive_mutex_cond_relock|recursive_mutex_destroy|recursive_mutex_init|recursive_mutex_lock|recursive_mutex_trylock|recursive_mutex_unlock|rw_rdlock|rw_tryrdlock|rw_trywrlock|rw_trywrlock_upgrade|rw_unlock|rw_wrlock|rwlock_destroy|rwlock_init|sched_params|scheduling_class|sema_destroy|sema_init|sema_post|sema_trywait|sema_wait|semctl|semget|semop|set_scheduling_params|sigtimedwait|sigwait|sigwaitinfo|thr_cancel|thr_cmp|thr_continue|thr_create|thr_equal|thr_exit|thr_getconcurrency|thr_getprio|thr_getspecific_native|thr_getspecific|thr_join|thr_get_affinity|thr_set_affinity|thr_key_detach|thr_key_used|thr_keycreate_native|thr_keycreate|thr_keyfree|thr_kill|thr_min_stack|thr_self|thr_setcancelstate|thr_setcanceltype|thr_setconcurrency|thr_setprio|thr_setspecific_native|thr_setspecific|thr_sigsetmask|thr_suspend|thr_testcancel|thr_yield|thread_mutex_destroy|thread_mutex_init|thread_mutex_lock|thread_mutex_trylock|thread_mutex_unlock|unique_name";

    $OS_NS_time_symbols = "asctime|asctime_r|clock_gettime|clock_settime|ctime|ctime_r|difftime|gmtime|gmtime_r|localtime|localtime_r|mktime|nanosleep|readPPCTimeBase|strftime|strptime|strptime_emulation|strptime_getnum|time|timezone|tzset";

    $OS_NS_unistd_symbols = "access|alarm|allocation_granularity|argv_to_string|chdir|rmdir|close|dup|dup2|execl|execle|execlp|execv|execve|execvp|fork|fork_exec|fsync|ftruncate|getcwd|getgid|getegid|getopt|getpagesize|getpgid|getpid|getppid|getuid|geteuid|hostname|isatty|lseek|llseek|num_processors|num_processors_online|pipe|pread|pwrite|read|read_n|readlink|sbrk|setgid|setegid|setpgid|setregid|setreuid|setsid|setuid|seteuid|sleep|string_to_argv|swab|sysconf|sysinfo|truncate|ualarm|unlink|write|write_n";

    $OS_NS_wchar_symbols = "fgetwc|wcscat_emulation|wcschr_emulation|wcscmp_emulation|wcscpy_emulation|wcscspn_emulation|wcsicmp_emulation|wcslen_emulation|wcsncat_emulation|wcsncmp_emulation|wcsncpy_emulation|wcsnicmp_emulation|wcspbrk_emulation|wcsrchr_emulation|wcsrchr_emulation|wcsspn_emulation|wcsstr_emulation|wslen|wscpy|wscmp|wsncmp|ungetwc";

    print "Running ACE_OS check\n";
    foreach $file (@files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_lack_ACE_OS/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_lack_ACE_OS/) {
                    $disable = 0;
                }
                if ($disable == 0) {
                    if($file !~ /.c$/ && $file !~ /S.cpp$/ && $file !~ /S.inl$/ && $file !~ /C.cpp$/ && $file !~ /C.inl$/) {
                        if($file !~ /OS_NS_arpa_inet/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_arpa_inet_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_arpa_inet.h");
                            }
                        }
                        if($file !~ /OS_NS_ctype/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_ctype_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_ctype.h");
                            }
                        }
                        if($file !~ /OS_NS_dirent/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_dirent_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_dirent.h");
                            }
                        }
                        if($file !~ /OS_NS_dlfcn/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_dlfcn_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_dlfcn.h");
                            }
                        }
                        if($file !~ /OS_NS_errno/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_errno_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_errno.h");
                            }
                        }
                        if($file !~ /OS_NS_fcntl/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_fcntl_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_fcntl.h");
                            }
                        }
                        if($file !~ /OS_NS_math/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_math_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_math.");
                            }
                        }
                        if($file !~ /OS_NS_netdb/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_netdb_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_netdb.h");
                            }
                        }
                        if($file !~ /OS_NS_poll/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_netdb_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_poll.h");
                            }
                        }
                        if($file !~ /OS_NS_pwd/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_pwd_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_pwd.h");
                            }
                        }
                        if($file !~ /OS_NS_regex/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_regex_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_regex.h");
                            }
                        }
                        if($file !~ /OS_NS_signal/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_signal_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_signal.h");
                            }
                        }
                        if($file !~ /OS_NS_stdlib/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_stdlib_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_stdlib.h");
                            }
                        }
                        if($file !~ /OS_NS_stdio/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_stdio_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_stdio.h");
                            }
                        }
                        if($file !~ /OS_NS_string/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_string_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_string.h");
                            }
                        }
                        if($file !~ /OS_NS_strings/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_strings_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_strings.h");
                            }
                        }
                        if($file !~ /OS_NS_stropts/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_stropts_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_stropts.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_mman/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_mman_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_mman.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_msg/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_msg_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_msg.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_resource/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_resource_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_resource.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_select/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_select_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_select.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_sendfile/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_sendfile_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_sendfile.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_shm/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_shm_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_shm.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_socket/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_socket_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_socket.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_stat/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_stat_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_stat.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_time/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_time_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_time.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_uio/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_uio_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_uio.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_utsname/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_utsname_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_utsname.h");
                            }
                        }
                        if($file !~ /OS_NS_sys_wait/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_sys_wait_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_sys_wait.h");
                            }
                        }
                        if($file !~ /OS_NS_Thread/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_Thread_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_Thread.h");
                            }
                        }
                        if($file !~ /OS_NS_time/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_time_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_time.h");
                            }
                        }
                        if($file !~ /OS_NS_unistd/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_unistd_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_unistd.h");
                            }
                        }
                        if($file !~ /OS_NS_wchar/) {
                            if(/(\s+:{0,2}|\(:{0,2}|\s*!:{0,2}|^|\):{0,2})($OS_NS_wchar_symbols)\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                                print_error ("$file:$.: missing ACE_OS use ace/OS_NS_wchar.h");
                            }
                        }
                    }
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the use of exception specification,
# exception specification has fallen out of favor, and generally
# should not be used.
sub check_for_exception_spec ()
{
    print "Running exception specification check\n";

    foreach $file (@files_cpp, @files_inl, @files_h) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_exception_sepc/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_exception_sepc/) {
                    $disable = 0;
                }
                if ($disable == 0) {
                    if(/throw\s*\(\s*\)/) {
                        #next;
                    }
                    elsif(/(^|\s+)throw\s*\(/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                        print_error ("$file:$.: exception specification found");
                    }
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the use of NULL,
# NULL shouldn't be used, use 0 instead
sub check_for_NULL ()
{
    print "Running NULL usage check\n";

    foreach $file (@files_cpp, @files_inl, @files_h) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_NULL/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_NULL/) {
                    $disable = 0;
                }
                if ($disable == 0) {
                    if(/(\(|\)|\s+|=)NULL(\)|\s+|\;|\,)/ and $` !~ /\/\// and $` !~ /\/\*/ and $` !~ /\*\*+/ and $` !~ /\s+\*+\s+/) {
                        print_error ("$file:$.: NULL found");
                    }
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for improper main declaration,
# the proper form should look like:
# int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
sub check_for_improper_main_declaration ()
{
    print "Running Improper main() declaration check\n";

    foreach $file (@files_cpp) {
        if (open (FILE, $file)) {
            my $disable = 0;
            my $type_of_main;
            my $multi_line;
            my $not_found_end_line_count= 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (!defined $multi_line) {
                    if (/FUZZ\: disable check_for_improper_main_declaration/) {
                        $disable = 1;
                        next;
                    }
                    elsif (/FUZZ\: enable check_for_improper_main_declaration/) {
                        $disable = 0;
                        next;
                    }
                    elsif ($disable == 0) {
                        s/^\s+//;           ## Remove leading space
                        s/\s*(\/\/.*)?$//;  ## Remove trailing space and line comments
                        if (s/^(?:.*\s)?(main|ACE_TMAIN)\s*//) {
                            $type_of_main = $1; ## main or ACE_TMAIN
                            $multi_line   = $_; ## Rest of the line
                        }
                        else {
                            next;
                        }
                    }
                }
                else {
                    $_ =~ s/^\s+//;           ## Remove leading space
                    $_ =~ s/\s*(\/\/.*)?$//;  ## Remove trailling space and line comments
                    if ($multi_line eq "") {  ## Append this line to existing statement.
                        $multi_line = $_;
                    }
                    else {
                        $multi_line .= ' ' . $_;
                    }
                }
                $multi_line =~ s!^(/+\*.*?\*/\s*)*!!;  ## Remove leading /* ... */ comments
                next if ($multi_line eq "");  ## Must have something after main
                if ($multi_line !~ m/^\(/) {
                    ## Not a function opening bracket, we will ignore this one
                    ## it is not a main function.
                    undef $multi_line;
                    $not_found_end_line_count = 0;
                }
                elsif ($multi_line =~ s/^\(\s*([^\)]*?)\s*\)[^;\{]*?\{//) {
                    $multi_line = $1;                             ## What was between the main's ( and )
                    $multi_line =~ s/\s{2,}/ /g;                  ## Compress white space
                    my $was = $multi_line;
                    $multi_line =~ s!([^/])\*\s([^/])!$1\*$2!g;   ## Remove space after * (except around comment)
                    $multi_line =~ s!([^/])\s\[!$1\[!g;           ## Remove space before [ (except following comment)
                    $multi_line =~ s!\s?\*/\s?/\*\s?! !g;         ## Connect seporate adjacent /* ... */ comments
                    if ($multi_line =~ s!^([^,]*?)\s?,\s?(/+\*.*?\*/\s?)*!!) { # Fails if only 1 parameter (ignore this main)
                        my $arg1 = $1;
                        if ($multi_line =~ s/^(\w[\w\d]*)\s?//) { # Fails if no type for 2nd parameter (ignore this main)
                            my $arg2_type = $1;
                            $multi_line =~ s!^(?:/+\*.*?\*/\s?)?(\**)(\w[\w\d]*|\s?/\*.*?\*/\s?)?!!;
                            my $prefix = $1; ## should be * or **
                            my $name   = $2; ## is now arg2's variable name
                            $multi_line =~ s!\s?\*/\s?/\*\s?! !g;  ## Connect seporate adjacent /* ... */ comments

                            ## remove any comment after postfix
                            if ($multi_line =~ s!\s?(/+\*.*?\*/)$!! && $name eq '') {
                                $name = "$1 ";  ## Some name argv in comment after []
                            }
                            ## multi_line now postfix, should be []

                            if ($type_of_main ne 'ACE_TMAIN'       ||
                                $arg2_type ne 'ACE_TCHAR'          ||
                                !(($prefix eq '*' && $multi_line eq '[]') ||
                                  ($prefix eq '**' && $multi_line eq '' ))  ) {
                                print_error ("$file:$.:  $type_of_main ($was)  should be  ACE_TMAIN ($arg1, ACE_TCHAR \*$name\[])");
                           }
                        }
                    }

                    undef $multi_line;
                    $not_found_end_line_count = 0;
                }
                elsif ($not_found_end_line_count < 10) { # Limit the search for ( ... ) following main to ten lines
                    ++$not_found_end_line_count;
                }
                else {
                    undef $multi_line;
                    $not_found_end_line_count = 0;
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the use of "inline" instead of ACE_INLINE
sub check_for_inline ()
{
    print "Running inline check\n";
    foreach $file (@files_inl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_inline/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_inline/) {
                    $disable = 0;
                }
                if ($disable == 0 and m/^\s*inline/) {
                    print_error ("$file:$.: 'inline' keyword found");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}


# This test checks for the inclusion of math.h.  math.h should be avoided
# since on some platforms, "exceptions" is defined as a struct, which will
# cause problems with exception handling
sub check_for_math_include ()
{
    print "Running math.h test\n";
    foreach $file (@files_h, @files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_math_include/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_math_include/) {
                    $disable = 0;
                }
                if ($disable == 0
                    and /^\s*#\s*include\s*(\/\*\*\/){0,1}\s*\<math\.h\>/) {
                    print_error ("$file:$.: <math.h> included");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the inclusion of streams.h.
# // FUZZ: disable check_for_streams_include
sub check_for_streams_include ()
{
    print "Running ace/streams.h test\n";
    foreach $file (@files_h, @files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_streams_include/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_streams_include/) {
                    $disable = 0;
                }
                if ($disable == 0
                    and /^\s*#\s*include\s*\"ace\/streams\.h\"/) {
                    print_error ("$file:$.: expensive ace/streams.h included; consider ace/iosfwd.h");
                    print " ace/streams.h is very expensive in both ";
                    print "compile-time and footprint. \n";
                    print " Please consider including ace/iosfwd.h instead.\n\n";
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the inclusion of Synch*.h.
sub check_for_synch_include ()
{
    print "Running ace/Synch*.h test\n";
    foreach $file (@files_h, @files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_synch_include/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_synch_include/) {
                    $disable = 0;
                }
                if ($disable == 0
                    and (/^\s*#\s*include\s*\"(ace\/Synch\.h)\"/
                         or /^\s*#\s*include\s*\"(ace\/Synch_T\.h)\"/)) {
                    my $synch = $1;
                    print_error ("$file:$.: expensive $synch included;  consider individual synch file");
                    print " $synch is very expensive in both ";
                    print "compile-time and footprint. \n";
                    print " Please consider including one of the ";
                    print "individual synch files instead.\n\n";
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# For general readability, lines should not contain more than 80 characters
sub check_for_line_length ()
{
    print "Running line length test\n";
    foreach $file (@files_h, @files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {

                # Make sure to ignore ACE_RCSID lines, since they
                # are difficult to get under 80 chars.
                if (/.{80,}/ and !/^ACE_RCSID/) {
                    print_error ("$file:$.: line longer than 80 chars");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}


# For preprocessor directives, only the old C style comments (/* */)
# should be used, not the newer // style.
sub check_for_preprocessor_comments ()
{
    print "Running preprocessor comment test\n";
    foreach $file (@files_h, @files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/^\#.*\/\//) {
                    print_error ("$file:$.: C++ comment in directive");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# We should not have empty files in the repo
sub check_for_empty_files ()
{
    print "Running empty file test\n";
    foreach $file (@files_inl, @files_cpp) {
        my $found_non_empty_line = 0;
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
              next if /^[:blank:]*$/; # skip empty lines
              next if /^[:blank:]*\/\//; # skip C++ comments
              next if /^[:blank:]*\/\*/; # skip C++ comments
              $found_non_empty_line = 1;
              last;
            }
            close (FILE);
            if ($found_non_empty_line == 0) {
             print_error ("$file:1: empty file should not be in the repository");
            }
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}


# This test checks for the use of the Win32 Unicode string defines
# or outdated ASYS_* macros
# We should only be using the ACE_TCHAR, ACE_TEXT macros instead.
sub check_for_tchar
{
    print "Running TCHAR test\n";
    foreach $file (@files_h, @files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_tchar/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_tchar/) {
                    $disable = 0;
                }
                if ($disable == 0) {
                    if (/LPTSTR/) {
                        print_error ("$file:$.: LPTSTR found");
                    }

                    if (/LPCTSTR/) {
                        print_error ("$file:$.: LPCTSTR found");
                    }

                    if (/ASYS_TCHAR/) {
                        print_error ("$file:$.: ASYS_TCHAR found");
                    }
                    elsif (/TCHAR/ and !/ACE_TCHAR/) {
                        ### Do a double check, since some macros do have TCHAR
                        ### (like DEFAULTCHARS)
                        if (/^TCHAR[^\w_]/ or /[^\w_]TCHAR[^\w_]/) {
                            print_error ("$file:$.: TCHAR found");
                        }
                    }

                    if (/ASYS_TEXT/) {
                        print_error ("$file:$.: ASYS_TEXT found");
                    }
                    elsif (/TEXT/ and !/ACE_TEXT/) {
                        ### Do a double check, since there are several macros
                        ### that end with TEXT
                        if (/^TEXT\s*\(/ or /[^\w_]TEXT\s*\(/) {
                            print_error ("$file:$.: TEXT found");
                        }
                    }
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This checks to see if Makefiles define a DEPENDENCY_FILE, and if they do
# whether or not it's in the cvs repo.
sub check_for_dependency_file ()
{
    print "Running DEPENDENCY_FILE test\n";
    foreach $file (@files_makefile) {
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/^DEPENDENCY_FILE\s* =\s*(.*)/) {
                    my $depend = $1;
                    my $path = $file;
                    $path =~ s/\/GNUmakefile.*/\//;
                    $depend = $path . $depend;
                    unless (open (DFILE, $depend)) {
                        print_error ("DEPENDENCY_FILE \"$depend\" not found");
                        print " Either add \"$depend\" to svn ";
                        print "or remove DEPENDENCY_FILE variable\n";
                        print " from $file\n\n";
                    }
                    close (DFILE);
                }
            }
            close (FILE);
        }
        else {
            print_error ("cannot open $file");
        }
    }
}

# This checks to see if GNUmakefiles define a MAKEFILE, and if it matches the
# name of the GNUmakefile
sub check_for_makefile_variable ()
{
    print "Running MAKEFILE variable test\n";
    foreach $file (@files_makefile) {
        if (!(substr($file,-4) eq ".bor")
            and !(substr($file,-3) eq ".am")
            and !(substr($file,-4) eq ".vac")
            and !(substr($file,-4) eq ".alt")) {
            if (open (FILE, $file)) {
                print "Looking at file $file\n" if $opt_d;
                my $makevarfound = 0;
                my $filename = basename($file,"");
                while (<FILE>) {
                    if (/^MAKEFILE\s*=\s*(.*)/) {
                        $makevarfound = 1;
                        $makevar = $1;
                        if (!($makevar eq $filename)) {
                            print_error ("$file:$.: MAKEFILE variable $makevar != $filename");
                            print " Change MAKEFILE = $filename in $file.\n\n";
                        }
                    }
                }
                if ($makevarfound == 0 and !($filename eq "GNUmakefile")) {
                    print_error ("$file:$.: MAKEFILE variable missing in $file");
                    print " Add MAKEFILE = $filename to the top of $file.\n\n";
                }
                close (FILE);
            }
            else {
                print_error ("cannot open $file");
            }
        }
    }
}


# This checks to make sure files include ace/post.h if ace/pre.h is included
# and vice versa.
sub check_for_pre_and_post ()
{
    print "Running pre.h/post.h test\n";
    foreach $file (@files_h) {
        my $pre = 0;
        my $post = 0;
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_pre_and_post/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_pre_and_post/) {
                    $disable = 0;
                }
                if ($disable == 0) {
                    if (/^\s*#\s*include\s*\"ace\/pre\.h\"/) {
                        print_error ("$file:$.: pre.h  missing \"/**/\"");
                        ++$pre;
                    }
                    if (/^\s*#\s*include\s*\s*\"ace\/post\.h\"/) {
                        print_error ("$file:$.: post.h missing \"/**/\"");
                        ++$post;
                    }
                    if (/^\s*#\s*include\s*\/\*\*\/\s*\"ace\/pre\.h\"/) {
                        ++$pre;
                    }
                    if (/^\s*#\s*include\s*\/\*\*\/\s*\"ace\/post\.h\"/) {
                        ++$post;
                    }
                }
            }
            close (FILE);

            if ($disable == 0 && $pre != $post) {
                print_error ("$file:1: pre.h/post.h mismatch");
            }
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test verifies that the same number of "#pragma warning(push)" and
# "#pragma warning(pop)" pragmas are used in a given header.
sub check_for_push_and_pop ()
{
    print "Running #pragma (push)/(pop) test\n";
    foreach $file (@files_h) {
        my $push_count = 0;
        my $pop_count = 0;
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_push_and_pop/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_push_and_pop/) {
                    $disable = 0;
                }
                if ($disable == 0) {
                    if (/^\s*#\s*pragma\s*warning\s*\(\s*push[,1-4]*\s*\)/) {
                        ++$push_count;
                    }
                    if (/^\s*#\s*pragma\s*warning\s*\(\s*pop\s*\)/) {
                        ++$pop_count;
                    }
                }
            }
            close (FILE);

            if ($disable == 0 && $push_count != $pop_count) {
                print_error ("$file: #pragma warning(push)/(pop) mismatch");
            }
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test verifies that the same number of
# "ACE_VERSIONED_NAMESPACE_BEGIN_DECL" and
# "ACE_END_VERSIONED_NAMESPACE_DECL" macros are used in a given
# source file.
sub check_for_versioned_namespace_begin_end ()
{
  print "Running versioned namespace begin/end test\n";
  foreach $file (@files_cpp, @files_inl, @files_h) {
    my $begin_count = 0;
    my $end_count = 0;
    if (open (FILE, $file)) {
      print "Looking at file $file\n" if $opt_d;
      while (<FILE>) {
        if (/^\s*\w+_BEGIN_VERSIONED_NAMESPACE_DECL/) {
          ++$begin_count;
        }
        if (/^\s*\w+_END_VERSIONED_NAMESPACE_DECL/) {
          ++$end_count;
        }
        if ($begin_count > $end_count and
            /^\s*#\s*include(\s*\/\*\*\/)?\s*"/) {
          print_error ("$file:$.: #include directive within Versioned namespace block");
        }
      }

      close (FILE);

      if ($begin_count != $end_count) {
        print_error ("$file: Versioned namespace begin($begin_count)/end($end_count) mismatch");
      }
    }
    else {
      print STDERR "Error: Could not open $file\n";
    }
  }
}


# Check doxygen @file comments
sub check_for_mismatched_filename ()
{
    print "Running doxygen \@file test\n";
    foreach $file (@files_h, @files_cpp, @files_inl, @files_idl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (m/\@file\s*([^\s]+)/){
                    # $file includes complete path, $1 is the name after
                    # @file. We must strip the complete path from $file.
                    # we do that using the basename function from
                    # File::BaseName
                    $filename = basename($file,"");
                    if (!($filename eq $1)){
                        print_error ("$file:$.: \@file mismatch in $file, found $1");
                    }
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# check for bad run_test
sub check_for_bad_run_test ()
{
    print "Running run_test.pl test\n";
    foreach $file (@files_run_pl) {
        if (open (FILE, $file)) {
            my $is_run_test = 0;
            my $sub = 0;

            if (($file =~ /.*TAO\/examples\/Advanced.*/)) {
              next ITERATION;
            }
            if (($file =~ /.*TAO\/orbsvcs\/examples\/Security\/Send_File.*/)) {
              next ITERATION;
            }

            print "Looking at file $file\n" if $opt_d;

            while (<FILE>) {
                if (m/PerlACE/ || m/ACEutils/) {
                    $is_run_test = 1;
                }

                if ($is_run_test == 1) {
                    if (m/ACEutils/) {
                        print_error ("$file:$.: ACEutils.pm still in use");
                    }

                    if (m/unshift \@INC/) {
                        print_error ("$file:$.: unshifting \@INC; use \"use lib\"");
                    }

                    if (m/\$EXEPREFIX/) {
                        print_error ("$file:$.: using \$EXEPREFIX");
                    }

                    if (m/\$EXE_EXT/) {
                        print_error ("$file:$.: using \$EXE_EXT");
                    }

                    if (m/Sys::Hostname/) {
                        print_error ("$file:$.: using Sys::Hostname");
                    }

                    if (m/PerlACE::wait_interval_for_process_creation/) {
                        print_error ("$file:$.: using PerlACE::wait_interval_for_process_creation");
                    }

                    if (m/PerlACE::waitforfile_timed/) {
                        print_error ("$file:$.: using PerlACE::waitforfile_timed");
                    }

                    if (m/PerlACE::is_vxworks_test/) {
                        print_error ("$file:$.: using PerlACE::is_vxworks_test");
                    }

                    if (m/PerlACE::add_lib_path/) {
                        print_error ("$file:$.: using PerlACE::add_lib_path, use AddLibPath on the target");
                    }

                    if (m/PerlACE::Run_Test/) {
                        print_error ("$file:$.: using PerlACE::Run_Test, use PerlACE::TestTarget");
                    }

                    if (m/PerlACE::random_port/) {
                        print_error ("$file:$.: using PerlACE::random_port, use TestTarget::random_port");
                    }

                    if (m/PerlACE::Process/) {
                        print_error ("$file:$.: using PerlACE::Process");
                    }

                    if (m/PerlACE::TestConfig/) {
                        print_error ("$file:$.: using PerlACE::TestConfig");
                    }

                    if (m/ACE_RUN_VX_TGTHOST/) {
                        print_error ("$file:$.: using ACE_RUN_VX_TGTHOST, use TestTarget::HostName");
                    }

                    if (m/Spawn(Wait(Kill)?)?\s*\(.+\->ProcessStop.*\)/) {
                        print_error ("$file:$.: uses Stop together with Spawn");
                    }

                    if (m/Spawn(Wait(Kill)?)?\s*\(\d+\)/) {
                        print_error ("$file:$.: uses hardcoded timeout for Spawn");
                    }

                    if (m/Kill\s*\(\d+\)/) {
                        print_error ("$file:$.: uses hardcoded timeout for Kill");
                    }

                    if (m/unlink/) {
                        print_error ("$file:$.: using unlink");
                    }

                    if (m/PerlACE::LocalFile/) {
                        print_error ("$file:$.: using PerlACE::LocalFile");
                    }

                    if (m/\$DIR_SEPARATOR/) {
                        print_error ("$file:$.: using \$DIR_SEPARATOR");
                    }
                    if (m/ACE\:\:/ && !m/PerlACE\:\:/) {
                        print_error ("$file:$.: using ACE::*");
                    }

                    if (m/Process\:\:/ && !m/PerlACE\:\:Process\:\:/) {
                        print_error ("$file:$.: using Process::*");
                    }

                    if (m/Process\:\:Create/) {
                        print_error ("$file:$.: using Process::Create");
                    }

                    if (m/^  [^ ]/) {
                        print_warning ("$file:$.: using two-space indentation");
                    }

                    if (m/^\s*\t/) {
                        print_error ("$file:$.: Indenting using tabs");
                    }

                    if (m/^\s*\{/ && $sub != 1) {
                        print_warning ("$file:$.: Using Curly Brace alone");
                    }

                    if (m/timedout/i && !m/\#/) {
                        print_error ("$file:$.: timedout message found");
                    }

                    if (m/^\s*sub/) {
                        $sub = 1;
                    }
                    else {
                        $sub = 0;
                    }
                }
            }

            close (FILE);

            if ($is_run_test) {
                my @output = `perl -wc $file 2>&1`;

                foreach $output (@output) {
                    chomp $output;
                    if ($output =~ m/error/i) {
                        print_error ($output);
                    }
                    elsif ($output !~ m/syntax OK/) {
                        print_warning ($output);
                    }
                }
            }
        }
    }
}


# Check for links to ~schmidt/ACE_wrappers/, which should not be in the
# documentation
sub check_for_absolute_ace_wrappers()
{
    print "Running absolute ACE_wrappers test\n";
    foreach $file (@files_html) {
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (m/\~schmidt\/ACE_wrappers\//) {
                    chomp;
                    print_error ("$file:$.: ~schmidt/ACE_wrappers found");
                    print_error ($_);
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# Check for generated headers in the code documentation
sub check_for_generated_headers()
{
    print "Running generated headers test\n";
    foreach $file (@files_cpp, @files_inl, @files_h) {
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (m/Code generated by the The ACE ORB \(TAO\) IDL Compiler/) {
                    chomp;
                    print_error ("$file:$.: header found");
                    print_error ($_);
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# Make sure ACE_[OS_]TRACE matches the function/method
sub check_for_bad_ace_trace()
{
    print "Running TRACE test\n";
    foreach $file (@files_inl, @files_cpp) {
        if (open (FILE, $file)) {
            my $class;
            my $function;

            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {

                # look for methods or functions
                if (m/(^[^\s][^\(]*)\:\:([^\:^\(]*[^\s^\(])\s*/) {
                    $class = $1;
                    $function = $2;
                }
                elsif (m/^([^\s^\(^\#]*) \(/i) {
                    $class = "";
                    $function = $1;
                }
                elsif (m/^(operator.*) \(/i) {
                    $class = "";
                    $function = $1;
                }

                # Look for TRACE statements
                if (m/ACE_OS_TRACE\s*\(\s*\"(.*)\"/
                    || m/ACE_TRACE\s*\(\s*\"(.*)\"/
                    || m/CIAO_TRACE\s*\(\s*\"(.*)\"/
                    || m/DDS4CCM_TRACE\s*\(\s*\"(.*)\"/) {
                    my $trace = $1;

                    # reduce the classname
                    if ($class =~ m/([^\s][^\<^\s]*)\s*\</) {
                        $class = $1;
                    }

                    if ($class =~ m/([^\s^\&^\*]*)\s*$/) {
                        $class = $1;
                    }

                    if ($trace !~ m/\Q$function\E/
                        || ($trace =~ m/\:\:/ && !($trace =~ m/\Q$class\E/ && $trace =~ m/\Q$function\E/))) {
                        print_error ("$file:$.: Mismatched TRACE");
                        print_error ("$file:$.:   I see \"$trace\" but I think I'm in \""
                                     . $class . "::" . $function . "\"");
                    }
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}


# This test checks for broken ChangeLog entries.
sub check_for_changelog_errors ()
{
    print "Running ChangeLog check\n";
    foreach $file (@files_changelog) {
        if (open (FILE, $file)) {
            my $found_backslash = 0;
            my $found_cvs_conflict = 0;

            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {

                next if m/^\s*\/\//;
                next if m/^\s*$/;

                # Check for backslashes in paths.
                if (m/\*.*\\[^ ]*:/) {
                    print_error ("$file:$.: Backslashes in file path");
                }

                # Check for CVS conflict tags
                if (m/^<<<<</ || m/^=====/ || m/^>>>>>/) {
                    print_error ("$file:$.: svn conflict markers");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

sub check_for_deprecated_macros ()
{
    ## Take the current working directory and remove everything up to
    ## ACE_wrappers (or ACE for the peer-style checkout).  This will be
    ## used to determine when the use of ACE_THROW_SPEC is an error.
    my($cwd) = getcwd();
    if ($cwd =~ s/.*(ACE_wrappers)/$1/) {
    }
    elsif ($cwd =~ s/.*(ACE)/$1/) {
    }

    print "Running deprecated macros check\n";
    foreach $file (@files_cpp, @files_inl, @files_h) {
        if (open (FILE, $file)) {

            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                # Check for ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION usage.
                if (m/ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION\)/) {
                    print_error ("$file:$.: ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION found.");
                }
                elsif (/ACE_THROW_SPEC/) {
                    ## Do not use ACE_THROW_SPEC in TAO or CIAO.
                    if ($file =~ /TAO|CIAO/i || $cwd =~ /TAO|CIAO/i) {
                        print_error ("$file:$.: ACE_THROW_SPEC found.");
                    }
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}
# This test checks for ptr_arith_t usage in source code.  ptr_arith_t
# is non-portable.  Use ptrdiff_t instead.
sub check_for_ptr_arith_t ()
{
    print "Running ptr_arith_t check\n";
    foreach $file (@files_cpp, @files_inl, @files_h) {
        if (open (FILE, $file)) {

            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {

                next if m/^\s*\/\//;  # Ignore C++ comments.
                next if m/^\s*$/;     # Skip lines only containing
                                      # whitespace.

                # Check for ptr_arith_t usage.  This test should
                # ignore typedefs, and should only catch variable
                # declarations and parameter types.
                if (m/ptr_arith_t / || m/ptr_arith_t,/) {
                    print_error ("$file:$.: ptr_arith_t; use ptrdiff_t instead.");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the #include <ace/...>
# This check is suggested by Don Hinton to force user to use
# " " instead of <> to avoid confict with Doxygen.
sub check_for_include ()
{
    print "Running the include check\n";
    foreach $file (@files_h, @files_cpp, @files_inl, @files_idl) {
        my $bad_occurance = 0;
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_include/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_include/) {
                    $disable = 0;
                }
                if ($disable == 0) {
                    if (/^\s*#\s*include\s*<[(ace)|(TAO)|(CIAO)]\/.*>/) {
                        print_error ("$file:$.: include <ace\/..> used");
                        ++$bad_occurance;
                    }
                    if (/^\s*#\s*include\s*<tao\/.*>/) {
                        print_error ("$file:$.: include <tao\/..> used");
                        ++$bad_occurance;
                    }
                    if (/^\s*#\s*include\s*<ciao\/.*>/) {
                        print_error ("$file:$.: include <ciao\/..> used");
                        ++$bad_occurance;
                    }
                }
            }
            close (FILE);

            if ($disable == 0 && $bad_occurance > 0 ) {
                print_error ("$file:1: found $bad_occurance usage(s) of #include <> of ace\/tao\/ciao.");
            }
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test verifies that all equality, relational and logical
# operators return bool, as is the norm for modern C++.
#
# NOTE:  This test isn't fool proof yet.
sub check_for_non_bool_operators ()
{
    print "Running non-bool equality, relational and logical operator check\n";
    foreach $file (@files_h, @files_inl, @files_cpp) {
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            my $found_bool = 0;
            my $found_return_type = 0;
            while (<FILE>) {

                if ($found_bool == 0
                    && (/[^\w]bool\s*$/
                        || /^bool\s*$/
                        || /\sbool\s+\w/
                        || /^bool\s+\w/
                        || /[^\w]return\s*$/))
                  {
                    $found_bool = 1;
                    $found_return_type = 0;
                    next;
                  }

                if ($found_bool == 0 && $found_return_type == 0
                    && /^(?:\w+|\s+\w+)\s*$/
                    && !/[^\w]return\s*$/)
                  {
                    $found_return_type = 1;
                    $found_bool = 0;
                    next;
                  }

                if ($found_bool == 0
                    && /(?<![^\w]bool)(\s+|\w+::|>\s*::)operator\s*(?:!|<|<=|>|>=|==|!=|&&|\|\|)\s*\(/
                    && !/\(.*operator\s*(?:!|<|<=|>|>=|==|!=|&&|\|\|)\s*\(/
                    && !/^\s*return\s+/) {
                    print_error ("$file:$.: non-bool return type for operator");
                }

                $found_return_type = 0;
                $found_bool = 0;
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test verifies that all filenames are short enough

sub check_for_long_file_names ()
{
    my $max_filename = 50;
    my $max_mpc_projectname = $max_filename - 12; ## GNUmakefile.[project_name]
    print "Running file names check\n";

    foreach $file (@files_cpp, @files_inl, @files_h, @files_html,
                   @files_dsp, @files_dsw, @files_gnu, @files_idl,
                   @files_pl, @files_changelog, @files_makefile,
                   @files_bor, @files_mpc, @files_generic) {
        if ( length( basename($file) ) >= $max_filename )
        {
            print_error ("File name $file meets or exceeds $max_filename chars.");
        }
    }
    foreach $file (grep(/\.mpc$/, @files_mpc)) {
      if (open(FH, $file)) {
        my($blen) = length(basename($file)) - 4; ## .mpc
        while(<FH>) {
          if (/project\s*(:.*)\s*{/) {
            if ($blen >= $max_mpc_projectname) {
              print_warning ("File name $file meets or exceeds $max_mpc_projectname chars.");
            }
          }
          elsif (/project\s*\(([^\)]+)\)/) {
            my($name) = $1;
            if ($name =~ /\*/) {
              my($length) = length($name) + (($name =~ tr/*//) * $blen);
              if ($length >= $max_mpc_projectname) {
                print_warning ("Project name ($name) from $file will meet or exceed $max_mpc_projectname chars when expanded by MPC.");
              }
            }
            else {
              if (length($name) >= $max_mpc_projectname) {
                print_warning ("Project name ($name) from $file meets or exceeds $max_mpc_projectname chars.");
              }
            }
          }
        }
        close(FH);
      }
    }
}

sub check_for_refcountservantbase ()
{
    print "Running PortableServer::RefCountServantBase derivation check\n";

    foreach $file (@files_h, @files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {

                if (/PortableServer::RefCountServantBase/) {
                  print_error ("$file:$.: reference to deprecated PortableServer::RefCountServantBase");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

sub check_for_TAO_Local_RefCounted_Object ()
{
    print "Running TAO_Local_RefCounted_Object check\n";

    ITERATION: foreach $file (@files_h, @files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (/FUZZ\: disable check_for_TAO_Local_RefCounted_Object/) {
                    $disable = 1;
                }
                if (/FUZZ\: enable check_for_TAO_Local_RefCounted_Object/) {
                    $disable = 0;
                }

                if ($disable == 0 and /TAO_Local_RefCounted_Object/) {
                  print_error ("$file:$.: TAO_Local_RefCounted_Object is deprecated, use CORBA::LocalObject instead");
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

# This test checks for the correct use of ORB_init() so as
# to be compatiable with wide character builds.
sub check_for_ORB_init ()
{
    print "Running the ORB_init() wide character incompatability check\n";
    foreach $file (@files_cpp, @files_inl) {
        if (open (FILE, $file)) {
            my $disable = 0;
            my $multi_line;
            my $not_found_end_line_count= 0;
            print "Looking at file $file\n" if $opt_d;
            while (<FILE>) {
                if (!defined $multi_line) {
                    if (/FUZZ\: disable check_for_ORB_init/) {
                        $disable = 1;
                        next;
                    }
                    elsif (/FUZZ\: enable check_for_ORB_init/) {
                        $disable = 0;
                        next;
                    }
                    elsif ($disable == 0) {
                        s/^\s+//;           ## Remove leading space
                        s/\s*(\/\/.*)?$//;  ## Remove trailling space and line comments
                        if (s/^([^=]*=)?\s*(CORBA\s*::\s*)?ORB_init\s*//) {
                            $multi_line = $_; ## Rest of the line
                        }
                        else {
                            next;
                        }
                    }
                }
                else {
                    $_ =~ s/^\s+//;           ## Remove leading space
                    $_ =~ s/\s*(\/\/.*)?$//;  ## Remove trailling space and line comments
                    if ($multi_line eq "") {  ## Append this line to existing statement.
                        $multi_line = $_;
                    }
                    else {
                        $multi_line .= ' ' . $_;
                    }
                }
                my $testing = $multi_line;
                if ($testing =~ s/^\(([^\"\/\)]*(\"([^\"\\]*(\\.)*)\")?(\/+\*.*?\*\/\s*)*)*\)//) {
                    # $testing has thrown away what we actually want, i.e.
                    # we want to ignore what's left in $testing.

                    $multi_line = substr ($multi_line, 0, -length ($testing));
                    $multi_line =~ s!/\*.*?\*/! !g;  ## Remove any internal /* ... */ comments
                    $multi_line =~ s!\s{2,}! !g;     ## collapse multi spaces
                    $multi_line =~ s/^\(\s*//;       ## Trim leading ( and space
                    $multi_line =~ s/\s*\)$//;       ## Trim trailing space and )

                    if ($multi_line =~ s/^[^,]*,\s*//) { # If this fails there is only 1 parameter (which we will ignore)
                        # 1st parameter has been removed by the above, split up remaining 2 & 3
                        $multi_line =~ s/^([^,]*),?\s*//;
                        my $param2 = $1;
                        $param2 =~ s/\s+$//; # Trim trailing spaces

                        print_error ("$file:$.: ORB_init() 2nd parameter requires static_cast<ACE_TCHAR **>(0)") if ($param2 eq '0');
                        print_error ("$file:$.: ORB_init() 3rd parameter is redundant (default orbID or give as string)") if ($multi_line eq '0');
                        print_error ("$file:$.: ORB_init() 3rd parameter is redundant (default orbID already \"\")") if ($multi_line eq '""');
                    }

                    undef $multi_line;
                    $not_found_end_line_count = 0;
                }
                elsif ($not_found_end_line_count < 10) { # Limit the search for ( ... ) following ORB_init to ten lines
                    ++$not_found_end_line_count;
                }
                else {
                    undef $multi_line;
                    $not_found_end_line_count = 0;
                }
            }
            close (FILE);
        }
        else {
            print STDERR "Error: Could not open $file\n";
        }
    }
}

##############################################################################

use vars qw/$opt_c $opt_d $opt_h $opt_l $opt_t $opt_m/;

if (!getopts ('cdhl:t:mv') || $opt_h) {
    print "fuzz.pl [-cdhm] [-l level] [-t test_names] [file1, file2, ...]\n";
    print "\n";
    print "    -c             only look at the files passed in\n";
    print "    -d             turn on debugging\n";
    print "    -h             display this help\n";
    print "    -l level       set detection level (default = 5)\n";
    print "    -t test_names  specify comma-separated list of tests to run\n".
          "                       this will disable the run level setting\n";
    print "    -m             only check locally modified files (uses cvs)\n";
    print "======================================================\n";
    print "list of the tests that could be run:\n";
    print "\t   check_for_noncvs_files
           check_for_generated_headers
           check_for_synch_include
           check_for_streams_include
           check_for_dependency_file
           check_for_makefile_variable
           check_for_inline_in_cpp
           check_for_id_string
           check_for_newline
           check_for_ACE_SYNCH_MUTEX
           check_for_ACE_Thread_Mutex
           check_for_tab
           check_for_exception_spec
           check_for_NULL
           check_for_improper_main_declaration
           check_for_lack_ACE_OS
           check_for_inline
           check_for_math_include
           check_for_line_length
           check_for_preprocessor_comments
           check_for_tchar
           check_for_pre_and_post
           check_for_push_and_pop
           check_for_versioned_namespace_begin_end
           check_for_mismatched_filename
           check_for_bad_run_test
           check_for_absolute_ace_wrappers
           check_for_bad_ace_trace
           check_for_changelog_errors
           check_for_ptr_arith_t
           check_for_include
           check_for_non_bool_operators
           check_for_long_file_names
           check_for_refcountservantbase
           check_for_TAO_Local_RefCounted_Object
           check_for_ORB_init
           check_for_trailing_whitespace\n";
    exit (1);
}

if (!$opt_l) {
    $opt_l = 5;
}

if ($opt_c) {
    foreach $file (@ARGV) {
        store_file ($file);
    }
}
elsif ($opt_m) {
    find_mod_files ();
}
else {
    find_files ();
}

if ($opt_t) {
    my @tests = split '\s*,\s*', $opt_t;
    for my $test (@tests) {
      &$test();
    }
    print "\nfuzz.pl - $errors error(s), $warnings warning(s)\n";
    exit ($errors > 0) ? 1 : 0;
}

print "--------------------Configuration: Fuzz - Level ",$opt_l,
      "--------------------\n";

check_for_export_file () if ($opt_l >= 4);
check_for_trailing_whitespace () if ($opt_l >= 4);
check_for_lack_ACE_OS () if ($opt_l >= 6);
check_for_ACE_Guard () if ($opt_l >= 1);
check_for_generated_headers () if ($opt_l >= 6);
check_for_bad_run_test () if ($opt_l >= 5);
check_for_deprecated_macros () if ($opt_l >= 1);
check_for_refcountservantbase () if ($opt_l >= 1);
check_for_msc_ver_string () if ($opt_l >= 3);
check_for_empty_files () if ($opt_l >= 1);
check_for_noncvs_files () if ($opt_l >= 1);
check_for_streams_include () if ($opt_l >= 6);
check_for_dependency_file () if ($opt_l >= 1);
check_for_makefile_variable () if ($opt_l >= 1);
check_for_inline_in_cpp () if ($opt_l >= 2);
check_for_id_string () if ($opt_l >= 1);
check_for_newline () if ($opt_l >= 1);
check_for_ACE_Thread_Mutex () if ($opt_l >= 1);
check_for_ACE_SYNCH_MUTEX () if ($opt_l >= 1);
check_for_tab () if ($opt_l >= 1);
check_for_exception_spec () if ($opt_l >= 1);
check_for_NULL () if ($opt_l >= 1);
check_for_inline () if ($opt_l >= 2);
check_for_math_include () if ($opt_l >= 3);
check_for_synch_include () if ($opt_l >= 6);
check_for_line_length () if ($opt_l >= 8);
check_for_preprocessor_comments () if ($opt_l >= 7);
check_for_tchar () if ($opt_l >= 4);
check_for_pre_and_post () if ($opt_l >= 4);
check_for_push_and_pop () if ($opt_l >= 4);
check_for_versioned_namespace_begin_end () if ($opt_l >= 4);
check_for_mismatched_filename () if ($opt_l >= 2);
check_for_absolute_ace_wrappers () if ($opt_l >= 3);
check_for_bad_ace_trace () if ($opt_l >= 4);
check_for_changelog_errors () if ($opt_l >= 4);
check_for_ptr_arith_t () if ($opt_l >= 4);
check_for_include () if ($opt_l >= 5);
check_for_non_bool_operators () if ($opt_l > 2);
check_for_long_file_names () if ($opt_l >= 1);
check_for_improper_main_declaration () if ($opt_l >= 1);
check_for_TAO_Local_RefCounted_Object () if ($opt_l >= 1);
check_for_ORB_init () if ($opt_l >= 1);

print "\nfuzz.pl - $errors error(s), $warnings warning(s)\n";

exit (1) if $errors > 0;
