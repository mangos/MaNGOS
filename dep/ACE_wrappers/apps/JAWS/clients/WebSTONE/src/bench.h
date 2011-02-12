/* $Id: bench.h 80826 2008-03-04 14:51:23Z wotte $ */
/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1995 Silicon Graphics, Inc.                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs were      *
 *  developed by SGI for public use.  If any changes are made to this code*
 *  please try to get the changes back to the author.  Feel free to make  *
 *  modifications and changes to the code and release it.                 *
 *                                                                        *
 **************************************************************************/
#ifndef __BENCH_H__
#define __BENCH_H__
#include <stdio.h>
#include <stdarg.h>
#ifndef WIN32
#include <sys/time.h>
#endif /* WIN32 */

#define USECINSEC           1000000
#define MSECINSEC           1000
#define MAX_ACCEPT_SECS     180         /* maximum time master will wait for listen() */

#define NCCARGS             4096
#define MAXCLIENTS          1024
#define MAXUSERNAME         25
#define MAXPASSWD           20
#define BUFSIZE             4096

#define MAXTOTALPROCS       MAXCLIENTS          /* overall max # of procs */
#define MAXPROCSPERNODE     MAXCLIENTS          /* max # of procs/node */


#define CONTENT_LENGTH_STRING   "CONTENT-LENGTH:"
#define OKSTR               "OK"
#define OKSTRLEN            ((int)strlen(OKSTR))
#define GOSTR               "GO"
#define GOSTRLEN            ((int)strlen(GOSTR))
#define READYSTR            "READY"
#define READYSTRLEN         ((int)strlen(READYSTR))
#define ABORTSTR            "ABORT"
#define ABORTSTRLEN         ((int)strlen(ABORTSTR))


#define MAXNUMOFFILES 1     /* max # of files per page */
#define URL_SIZE            1024
#define MAXNUMOFPAGES       100
#define SIZEOF_TIMEVALTEXT  18
#define SIZEOF_DOUBLETEXT   18
#define SIZEOF_RQSTSTATSTEXT ((7 *  SIZEOF_TIMEVALTEXT) + \
                              (12 * SIZEOF_DOUBLETEXT) + 1)
#define SIZEOF_STATSTEXTBASE    (SIZEOF_RQSTSTATSTEXT + \
                             (3 * SIZEOF_TIMEVALTEXT) + \
                             (2 * SIZEOF_DOUBLETEXT) + 1)
#define SIZEOF_STATSTEXT   (SIZEOF_STATSTEXTBASE + MAXNUMOFPAGES * SIZEOF_DOUBLETEXT)
#define SIZEOF_PAGESTATSTEXT (SIZEOF_RQSTSTATSTEXT + \
                              (0 * SIZEOF_TIMEVALTEXT) + \
                              (3 * SIZEOF_DOUBLETEXT) + 1)

#define D_PRINTF  debug && d_printf

#ifdef USE_TIMEZONE
typedef struct rqst_timer {
    struct timeval      entertime;
    struct timezone     entertimezone;
    struct timeval      beforeconnect;
    struct timezone     beforeconnectzone;
    struct timeval      afterconnect;
    struct timezone     afterconnectzone;
    struct timeval      beforeheader;
    struct timezone     beforeheaderzone;
    struct timeval      afterheader;
    struct timezone     afterheaderzone;
    struct timeval      afterbody;
    struct timezone     afterbodyzone;
    struct timeval      exittime;
    struct timezone     exittimezone;
    long unsigned int   totalbytes;
    long unsigned int   bodybytes;
    int                 valid;
    long unsigned int   page_number;
} rqst_timer_t;
#else
typedef struct rqst_timer {
    struct timeval      entertime;
    struct timeval      beforeconnect;
    struct timeval      afterconnect;
    struct timeval      beforeheader;
    struct timeval      afterheader;
    struct timeval      afterbody;
    struct timeval      exittime;
    long unsigned int   totalbytes;
    long unsigned int   bodybytes;
    int                 valid;
    long unsigned int   page_number;
} rqst_timer_t;
#endif /* USE_TIMEZONE */

extern void rqtimer_init(rqst_timer_t *);

#ifdef USE_TIMEZONE
typedef struct rqst_stats {
    struct timeval      totalresponsetime;
    struct timezone      totalresponsetimezone;
    double              totalresponsetimesq;
    struct timeval      minresponsetime;
    struct timezone      minresponsetimezone;
    struct timeval      maxresponsetime;
    struct timezone      maxresponsetimezone;
    struct timeval      totalconnecttime;
    struct timezone      totalconnecttimezone;
    double              totalconnecttimesq;
    struct timeval      minconnecttime;
    struct timezone      minconnecttimezone;
    struct timeval      maxconnecttime;
    struct timezone      maxconnecttimezone;
    long unsigned int   totalconnects;
    long unsigned int   totalerrs;
    struct timeval      totalerrortime;
    struct timezone     totalerrortimezone;
    double              totalbytes;
    double              totalbytessq;
    double              minbytes;
    double              maxbytes;
    double              totalbody;
    double              totalbodysq;
    double              minbody;
    double              maxbody;
} rqst_stats_t;
#else
typedef struct rqst_stats {
    struct timeval      totalresponsetime;
    double              totalresponsetimesq;
    struct timeval      minresponsetime;
    struct timeval      maxresponsetime;
    struct timeval      totalconnecttime;
    double              totalconnecttimesq;
    struct timeval      minconnecttime;
    struct timeval      maxconnecttime;
    long unsigned int   totalconnects;
    long unsigned int   totalerrs;
    struct timeval      totalerrortime;
    double              totalbytes;
    double              totalbytessq;
    double              minbytes;
    double              maxbytes;
    double              totalbody;
    double              totalbodysq;
    double              minbody;
    double              maxbody;
} rqst_stats_t;
#endif /* USE_TIMEZONE */

extern void rqstat_init(rqst_stats_t *);
extern void rqstat_sum(rqst_stats_t *, rqst_stats_t *);
extern void rqstat_print(rqst_stats_t *);
extern void rqstat_fprint(FILE *, rqst_stats_t *);
extern void rqstat_times(rqst_stats_t *, rqst_timer_t *);

#ifdef USE_TIMEZONE
typedef struct stats {
    /* char             hostname[MAXHOSTNAMELEN]; */
    rqst_stats_t        rs;
    struct timeval      starttime;
    struct timezone      starttimezone;
    struct timeval      endtime;
    struct timezone      endtimezone;
    struct timeval      datatime;
    struct timezone     datatimezone;
    long unsigned int   totalpages;
    unsigned int        total_num_of_files;
    unsigned int        page_numbers[MAXNUMOFPAGES];
} stats_t;
#else
typedef struct stats {
    /* char             hostname[MAXHOSTNAMELEN]; */
    rqst_stats_t        rs;
    struct timeval      starttime;
    struct timeval      endtime;
    struct timeval      datatime;
    long unsigned int   totalpages;
    unsigned int        total_num_of_files;
    unsigned int        page_numbers[MAXNUMOFPAGES];
} stats_t;
#endif /* USE_TIMEZONE */

extern void stats_init(stats_t *);
extern stats_t * text_to_stats(char *);
extern char * stats_to_text(const stats_t *);

typedef struct page_stats {
    rqst_stats_t        rs;
    long unsigned int   totalpages;
    unsigned int        page_size;
    int                 page_valid;
} page_stats_t;

extern void page_stats_init(page_stats_t *);
extern page_stats_t * text_to_page_stats(char *);
extern char * page_stats_to_text(const page_stats_t *);

/* THIS STRUCTURE DEFINES A PAGE. */
typedef struct page_list {
    int                 load_num;
    int                 num_of_files;
    char                *(filename[MAXNUMOFFILES]);
    char                *(servername[MAXNUMOFFILES]);
    int                 port_number[MAXNUMOFFILES];
}page_list_t;



/* shared variables */
extern THREAD FILE *debugfile;
extern int debug;

extern int      savefile;
extern int      timeexpired;
extern long int number_of_pages;

/* routines in bench.c */

extern void *mymalloc(size_t size);
extern int recvdata(SOCKET sock, char *ptr, int nbytes);
extern int senddata(SOCKET sock, char *ptr, int nbytes);
extern void rqstat_times(rqst_stats_t *rs, rqst_timer_t *rt);
/* note several others listed above */

/* routines in errexit.c */

void errexit(const char *, ...);
extern int returnerr(const char *, ...);
extern int d_printf(const char *, ...);
extern char *neterrstr(void);

/* routines in get.c */

extern int  get(char *loc, NETPORT port, char *url, rqst_timer_t *timer);

/* routines in parse_file_list.c */

extern int count_file_list(const char *url_list_file);
extern void parse_file_list (const char *url_list_file, page_list_t *page_list,
                 long int *num_of_pages, long int *num_of_files);
extern long int load_percent(page_list_t *page_list, long int number_of_pages);

/* routines in statistics.c (formerly statistics.h) */

extern double   mean(const double, const int);
extern double   variance(const double, const double, const int);
extern double   stddev(const double, const double, const int);

/* routines in timefunc.c (formerly timefunc.h) */

extern double   timevaldouble(struct timeval *);
extern void     doubletimeval(const double, struct timeval *);

extern void     addtime(struct timeval *, struct timeval *);
extern void     compdifftime(struct timeval *, struct timeval *, struct timeval *);
extern void     mintime(struct timeval *, struct timeval *);
extern void     maxtime(struct timeval *, struct timeval *);
extern void     avgtime(struct timeval *, int, struct timeval *);
extern void     variancetime(struct timeval *, double, int, struct timeval *);
extern void     stddevtime(struct timeval *, double, int, struct timeval *);

extern void     sqtime(struct timeval *, struct timeval *);

extern double   thruputpersec(const double, struct timeval *);

/* routines in webclient.c */

extern SOCKET connectsock(char *host, NETPORT portnum, char *protocol);

#endif /* !__BENCH_H__ */
