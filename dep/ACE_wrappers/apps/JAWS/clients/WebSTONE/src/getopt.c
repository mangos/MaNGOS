/* $Id: getopt.c 81993 2008-06-16 20:26:16Z sowayaa $ */
/* this is a public domain version of getopt */
/* FTP Site: ftp.uu.net/pub/OS/unix/bsd-sources/lib/librpc/etc/getopt.c */

#include <stdio.h>
#include <string.h>

#define MYNULL 0
#define ERR(s, c) if(opterr){\
    extern size_t strlen();\
    extern int write();\
    char errbuf[2];\
    errbuf[0] = c; errbuf[1] = '\n';\
    (void) write(2, argv[0], strlen(argv[0]));\
    (void) write(2, s, strlen(s));\
    (void) write(2, errbuf, 2);}

int opterr = 1;
int optind = 1;
int optopt;
char *optarg;

int
getopt(argc, argv, opts)
int argc;
char **argv, *opts;
{
    static int sp = 1;
    register int c;
    register char *cp;

    if(sp == 1)
        if(optind >= argc ||
        argv[optind][0] != '-' || argv[optind][1] == '\0')
            return(EOF);
        else if(strcmp(argv[optind], "--") == MYNULL) {
            optind++;
            return(EOF);
        }
    optopt = c = argv[optind][sp];
    if(c == ':' || (cp=strchr(opts, c)) == 0) {
        ERR(": unknown option, -", c);
        if(argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return('?');
    }
    if(*++cp == ':') {
        if(argv[optind][sp+1] != '\0')
            optarg = &argv[optind++][sp+1];
        else if(++optind >= argc) {
            ERR(": argument missing for -", c);
            sp = 1;
            return('?');
        } else
            optarg = argv[optind++];
        sp = 1;
    } else {
        if(argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = 0;
    }
    return(c);
}
