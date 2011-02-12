/* $Id: bench.c 81998 2008-06-17 09:31:15Z sma $ */
/**************************************************************************
 *
 *     Copyright (C) 1995 Silicon Graphics, Inc.
 *
 *  These coded instructions, statements, and computer programs were
 *  developed by SGI for public use.  If any changes are made to this code
 *  please try to get the changes back to the author.  Feel free to make
 *  modifications and changes to the code and release it.
 *
 **************************************************************************/

/* FUZZ: disable check_for_math_include */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <netdb.h>
#else
#include <windows.h>
#include <winsock.h>
#endif /* WIN32 */
#include "sysdep.h"
#include "bench.h"


/* allocate memory and exit if out of memory */
void *mymalloc(size_t size) {
void *ptr;

    ptr = malloc(size);
    if (ptr == 0)
      errexit("Call to malloc() failed\n");
    return ptr;
}

/*
 * Receive n bytes from a socket
 */
int
recvdata(SOCKET sock, char *ptr, int nbytes) {

  int nleft, nread;

  nleft = nbytes;
  while (nleft > 0)
    {
      D_PRINTF( "In recvdata(%d, %d)\n", sock, nleft );
      nread = NETREAD(sock, ptr, nleft);
      D_PRINTF( "NETREAD() returned %d\n", nread );
      if (BADSOCKET(nread) || nread == 0)
        {
          /* return error value NETWRITE */
          D_PRINTF( "Error in recvdata(): %s\n",neterrstr() );
          return(nread);
        }

      D_PRINTF( "NETREAD() data: \"%.*s\"\n", nread, ptr);
      nleft -= nread;
      ptr   += nread;
    } /* end while */

  /* return >= 0 */
  return(nbytes - nleft);

} /* end recvdata */


/*
 * Send n bytes to a socket
 */
int
senddata(SOCKET sock, char *ptr, int nbytes) {
  int nleft, nwritten;

  D_PRINTF( "In senddata(%d, \"%.*s\", %d)\n", sock, nbytes, ptr, nbytes );
  nleft = nbytes;
  while (nleft > 0)
    {
      nwritten = NETWRITE(sock, ptr, nleft);
      D_PRINTF( "senddata() returned %d\n", nwritten );
      if (BADSOCKET(nwritten))
        {
          /* return error value from NETWRITE */
          D_PRINTF( "Error in senddata(): %s\n", neterrstr() );
          return(nwritten);
        }
      nleft -= nwritten;
      ptr += nwritten;
    }
  return(nbytes - nleft);

} /* end senddata */

/* GENERAL NOTE: the conversion routines that follow pass their results
 * back in a static arrays.  A second call to the same routine overwrites
 * the previous buffer value for that routine.  If you want to save the
 * value in the buffer copy it to another variable.
 */

char *
timeval_to_text(const struct timeval *the_timeval) {
  /*
   * given a timeval (seconds and microseconds), put the text
   * "seconds.microseconds" into timeval_as_text
   */
  THREAD static char timeval_as_text[SIZEOF_TIMEVALTEXT+1];
  int seconds, microseconds;
  int returnval = 0;

  seconds = the_timeval->tv_sec;
  microseconds = the_timeval->tv_usec;
  returnval = sprintf(timeval_as_text,
                      "%10d.%6.6d\t", seconds, microseconds);
  return timeval_as_text;
}


char *
double_to_text(const double the_double) {
  /*
   * given a double, return text
   */
  THREAD static char double_as_text[SIZEOF_DOUBLETEXT + 1];
  int returnval = 0;

  returnval = sprintf(double_as_text, "%17.01f\t", the_double);
  return(double_as_text);
}

struct timeval
text_to_timeval(char *timeval_as_text) {
  int returnval = 0;
  long int seconds, microseconds;
  struct timeval the_timeval;

  D_PRINTF("T/%d %s\n", (int)timeval_as_text, timeval_as_text);
  returnval = sscanf(timeval_as_text, "%ld.%ld",
                     &seconds, &microseconds);
  the_timeval.tv_sec = seconds;
  the_timeval.tv_usec = microseconds;
  return the_timeval;
}

double
text_to_double(char *double_as_text) {
  double the_double = 0;
  int returnval = 0;

  D_PRINTF("D/%d %s\n", (int)double_as_text, double_as_text);
  returnval = sscanf(double_as_text, "%lf", &the_double);
  return(the_double);
}


rqst_stats_t *
text_to_rqst_stats(char *rqst_stats_as_text) {
  THREAD static rqst_stats_t rqst_stats;
  rqst_stats_t *the_rqst_stats = &rqst_stats;

  the_rqst_stats->totalresponsetime =
    text_to_timeval(strtok(rqst_stats_as_text, "\t"));

  the_rqst_stats->totalresponsetimesq =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->minresponsetime =
    text_to_timeval(strtok((char *)0, "\t"));

  the_rqst_stats->maxresponsetime =
    text_to_timeval(strtok((char *)0, "\t"));

  the_rqst_stats->totalconnecttime =
    text_to_timeval(strtok((char *)0, "\t"));

  the_rqst_stats->totalconnecttimesq =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->minconnecttime =
    text_to_timeval(strtok((char *)0, "\t"));

  the_rqst_stats->maxconnecttime =
    text_to_timeval(strtok((char *)0, "\t"));

  the_rqst_stats->totalconnects = (unsigned long)
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->totalerrs = (unsigned long)
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->totalerrortime =
    text_to_timeval(strtok((char *)0, "\t"));

  the_rqst_stats->totalbytes =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->totalbytessq =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->minbytes =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->maxbytes =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->totalbody =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->totalbodysq =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->minbody =
    text_to_double(strtok((char *)0, "\t"));

  the_rqst_stats->maxbody =
    text_to_double(strtok((char *)0, "\t"));

  return(the_rqst_stats);
} /* end text_to_rqst_stats */


char *
rqst_stats_to_text(rqst_stats_t *the_rqst_stats) {
  THREAD static char rqst_stats_as_text[SIZEOF_RQSTSTATSTEXT];
  char *tmpbuf;

  *rqst_stats_as_text = 0;

  tmpbuf = timeval_to_text(&(the_rqst_stats->totalresponsetime));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->totalresponsetimesq));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = timeval_to_text(&(the_rqst_stats->minresponsetime));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = timeval_to_text(&(the_rqst_stats->maxresponsetime));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = timeval_to_text(&(the_rqst_stats->totalconnecttime));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->totalconnecttimesq));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = timeval_to_text(&(the_rqst_stats->minconnecttime));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = timeval_to_text(&(the_rqst_stats->maxconnecttime));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->totalconnects));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->totalerrs));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = timeval_to_text(&(the_rqst_stats->totalerrortime));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->totalbytes));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->totalbytessq));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->minbytes));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->maxbytes));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->totalbody));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->totalbodysq));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->minbody));
  strcat(rqst_stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_rqst_stats->maxbody));
  strcat(rqst_stats_as_text, tmpbuf);

  D_PRINTF("rqst_stats_to_text returning %d: %s\n",
           strlen(rqst_stats_as_text),
           rqst_stats_as_text );

  return(rqst_stats_as_text);
}


stats_t *
text_to_stats(char *stats_as_text) {
  int i;
  rqst_stats_t *the_rqst_stats;
  THREAD static stats_t stats;
  stats_t *the_stats = &stats;

  D_PRINTF( "Parsing stats: %s\n", stats_as_text );
  /* grab stats.rs */
  the_rqst_stats = text_to_rqst_stats(stats_as_text);
  the_stats->rs = *the_rqst_stats;

  /* grab main structure */
  the_stats->starttime = text_to_timeval(strtok((char *)0, "\t"));
  the_stats->endtime = text_to_timeval(strtok((char *)0, "\t"));
  the_stats->datatime = text_to_timeval(strtok((char *)0, "\t"));
  the_stats->totalpages = (unsigned long) text_to_double(strtok((char *)0, "\t"));
  the_stats->total_num_of_files = (unsigned int) text_to_double(strtok((char *)0, "\t"));
  for (i = 0; i < number_of_pages; i++)
    {
      the_stats->page_numbers[i] = (unsigned int) text_to_double(strtok((char *)0, "\t"));
    }
  /* return bytes read */
  D_PRINTF( "Returning stats\n");
  return(the_stats);
} /* end text_to_stats */



char *
stats_to_text(const stats_t *the_stats) {
  int    i;
  THREAD static char  stats_as_text[SIZEOF_STATSTEXT];
  char  *tmpbuf;
  rqst_stats_t the_rqst_stats;

  *stats_as_text = 0;

  /* stats.rs */
  the_rqst_stats = the_stats->rs;
  tmpbuf = rqst_stats_to_text(&the_rqst_stats);
  strcat(stats_as_text, tmpbuf);

  /* main structure */

  tmpbuf = timeval_to_text(&(the_stats->starttime));
  strcat(stats_as_text, tmpbuf);

  tmpbuf = timeval_to_text(&(the_stats->endtime));
  strcat(stats_as_text, tmpbuf);

  tmpbuf = timeval_to_text(&(the_stats->datatime));
  strcat(stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_stats->totalpages));
  strcat(stats_as_text, tmpbuf);

  tmpbuf = double_to_text((the_stats->total_num_of_files));
  strcat(stats_as_text, tmpbuf);

  for (i = 0; i < number_of_pages; i++)
    {
      tmpbuf = double_to_text((the_stats->page_numbers[i]));
      strcat(stats_as_text, tmpbuf);
    }

  strcat(stats_as_text, "\n");

  return(stats_as_text);
} /* end stats_to_text */



page_stats_t *
text_to_page_stats(char *page_stats_as_text) {
  rqst_stats_t *the_rqst_stats;
  THREAD static page_stats_t pagestat;
  page_stats_t *pagestats = &pagestat;

  /* grab stats.rs */
  the_rqst_stats = text_to_rqst_stats(page_stats_as_text);

  /* grab main structure */
  pagestats->totalpages = (unsigned long) text_to_double(strtok((char *)0, "\t"));

  pagestats->page_size = (unsigned int) text_to_double(strtok((char *)0, "\t"));

  pagestats->page_valid = (int) text_to_double(strtok((char *)0, "\t"));

  pagestats->rs = *the_rqst_stats;
  /* return bytes read */

  return(pagestats);
} /* end text_to_page_stats */



char *
page_stats_to_text(const page_stats_t *pagestats) {
  THREAD static char page_stats_as_text[SIZEOF_PAGESTATSTEXT];
  char  *tmpbuf;
  rqst_stats_t the_rqst_stats;

  *page_stats_as_text = 0;

  /* stats.rs */
  the_rqst_stats = pagestats->rs;
  tmpbuf = rqst_stats_to_text(&the_rqst_stats);
  strcat(page_stats_as_text, tmpbuf);

  /* main structure */
  tmpbuf = double_to_text(pagestats->totalpages);
  strcat(page_stats_as_text, tmpbuf);

  tmpbuf = double_to_text(pagestats->page_size);
  strcat(page_stats_as_text, tmpbuf);

  tmpbuf = double_to_text(pagestats->page_valid);
  strcat(page_stats_as_text, tmpbuf);

  strcat(page_stats_as_text, "\n");

  return(page_stats_as_text);
} /* end page_stats_to_text */

void
rqtimer_init(rqst_timer_t *p) {
    memset(p, 0, sizeof(*p));
}

void
rqstat_init(rqst_stats_t *p) {
    memset(p, 0, sizeof(*p));

    p->minbytes = DBL_MAX;
    p->minbody  = DBL_MAX;
    p->minconnecttime.tv_sec = LONG_MAX;
    p->minconnecttime.tv_usec = LONG_MAX;
    p->minresponsetime.tv_sec = LONG_MAX;
    p->minresponsetime.tv_usec = LONG_MAX;
}

void
stats_init(stats_t *p) {

    memset(p, 0, sizeof(*p));

    p->rs.minbytes = DBL_MAX;
    p->rs.minbody  = DBL_MAX;
    p->rs.minconnecttime.tv_sec = LONG_MAX;
    p->rs.minconnecttime.tv_usec = LONG_MAX;
    p->rs.minresponsetime.tv_sec = LONG_MAX;
    p->rs.minresponsetime.tv_usec = LONG_MAX;
}

void
page_stats_init(page_stats_t *p) {

    memset(p, 0, sizeof(*p));

    /* commented out so that unread pages result in
       page_stats_as_text buffer overflow
    p->rs.minbytes = DBL_MAX;
    p->rs.minbody  = DBL_MAX;
    p->rs.minconnecttime.tv_sec = LONG_MAX;
    p->rs.minconnecttime.tv_usec = LONG_MAX;
    p->rs.minresponsetime.tv_sec = LONG_MAX;
    p->rs.minresponsetime.tv_usec = LONG_MAX;

    */
}

void
rqstat_times(rqst_stats_t *rs, rqst_timer_t *rt)
{
    double    t;

    compdifftime(&(rt->exittime), &(rt->entertime),
                 &(rs->totalresponsetime));
    t = timevaldouble(&(rs->totalresponsetime));
    rs->totalresponsetimesq = t * t;

    rs->minresponsetime = rs->totalresponsetime;
    rs->maxresponsetime = rs->totalresponsetime;

    compdifftime(&(rt->afterconnect), &(rt->beforeconnect),
                 &(rs->totalconnecttime));

    t = timevaldouble(&(rs->totalconnecttime));
    rs->totalconnecttimesq = t * t;

    rs->minconnecttime = rs->totalconnecttime;
    rs->maxconnecttime = rs->totalconnecttime;

    rs->totalbody =   rt->bodybytes;
    rs->totalbodysq = ((double)(rt->bodybytes)) * ((double)(rt->bodybytes));
    rs->minbody =     rt->bodybytes;
    rs->maxbody =     rt->bodybytes;

    rs->totalbytes =   rt->totalbytes;
    rs->totalbytessq = ((double)(rt->totalbytes)) * ((double)(rt->totalbytes));
    rs->minbytes =     rt->totalbytes;
    rs->maxbytes =     rt->totalbytes;

    rs->totalconnects = 1;
    rs->totalerrs = 0;
    rs->totalerrortime.tv_sec = 0;
    rs->totalerrortime.tv_usec = 0;
}

void
rqstat_sum(rqst_stats_t *sum, rqst_stats_t *incr)
{
    addtime( &(sum->totalresponsetime),   &(incr->totalresponsetime));
    mintime( &(sum->minresponsetime),     &(incr->minresponsetime));
    maxtime( &(sum->maxresponsetime),     &(incr->maxresponsetime));
    sum->totalresponsetimesq += incr->totalresponsetimesq;

    addtime( &(sum->totalconnecttime),    &(incr->totalconnecttime));
    mintime( &(sum->minconnecttime),      &(incr->minconnecttime));
    maxtime( &(sum->maxconnecttime),      &(incr->maxconnecttime));
    sum->totalconnecttimesq += incr->totalconnecttimesq;

    sum->totalconnects += incr->totalconnects;
    sum->totalerrs     += incr->totalerrs;
    addtime( &(sum->totalerrortime), &(incr->totalerrortime));

    sum->totalbytes    += incr->totalbytes;

    sum->totalbytessq  += incr->totalbytessq;
    sum->minbytes      =  min(sum->minbytes, incr->minbytes);
    sum->maxbytes      =  max(sum->maxbytes, incr->maxbytes);

    sum->totalbody     += incr->totalbody;

    sum->totalbodysq   += incr->totalbodysq;
    sum->minbody       =  min(sum->minbody, incr->minbody);
    sum->maxbody       =  max(sum->maxbody, incr->maxbody);

}


void
rqstat_print(rqst_stats_t *stats)
{
    rqstat_fprint(stdout, stats);
}


void
rqstat_fprint(FILE *f, rqst_stats_t *stats)
{
    struct timeval meantime, /*vartime,*/ stdtime;

    fprintf(f, "%d connection(s) to server, %d errors\n",
            stats->totalconnects, stats->totalerrs);

    if (stats->totalconnects == 0) {
      fprintf(f,"NO CONNECTIONS, THEREFORE NO STATISTICS\n"
              "IS YOUR WEBSERVER RUNNING?\n"
              "DO THE PAGES EXIST ON THE SERVER?\n");
      return;
    }

    /* title */
    fprintf(f, "\n\t\t\t   Average      Std Dev      Minimum      Maximum\n\n");

    /* first line (connect time) */
    avgtime(&(stats->totalconnecttime),
            stats->totalconnects, &meantime);

    /* variancetime(&(stats->totalconnecttime),
                    stats->totalconnecttimesq,
                    stats->totalconnects, &vartime); */

    stddevtime(&(stats->totalconnecttime),
               stats->totalconnecttimesq,
               stats->totalconnects, &stdtime);

    fprintf(f, "Connect time (sec) \t%3d.%6.6d   %3d.%6.6d   %3d.%6.6d   %3d.%6.6d\n",
            meantime.tv_sec,
            meantime.tv_usec,
            stdtime.tv_sec,
            stdtime.tv_usec,
            stats->minconnecttime.tv_sec,
            stats->minconnecttime.tv_usec,
            stats->maxconnecttime.tv_sec,
            stats->maxconnecttime.tv_usec);

    /* second line (response time) */
    avgtime(&(stats->totalresponsetime),
            stats->totalconnects, &meantime);

    /* variancetime(&(stats->totalresponsetime),
                    stats->totalresponsetimesq,
                    stats->totalconnects, &vartime); */

    stddevtime(&(stats->totalresponsetime),
               stats->totalresponsetimesq,
               stats->totalconnects, &stdtime);

    fprintf(f, "Response time (sec) \t%3d.%6.6d   %3d.%6.6d   %3d.%6.6d   %3d.%6.6d\n",
            meantime.tv_sec,
            meantime.tv_usec,
            stdtime.tv_sec,
            stdtime.tv_usec,
            stats->minresponsetime.tv_sec,
            stats->minresponsetime.tv_usec,
            stats->maxresponsetime.tv_sec,
            stats->maxresponsetime.tv_usec);

    /* 3rd-5th lines (response size, body size, # bytes moved */
    fprintf(f, "Response size (bytes) \t%10.0lf   %10.0lf   %10.0lf   %10.0lf\n",
            mean(stats->totalbytes, stats->totalconnects),
            stddev(stats->totalbytes, stats->totalbytessq, stats->totalconnects),
            stats->minbytes,
            stats->maxbytes);

    fprintf(f, "Body size (bytes) \t%10.0lf   %10.0lf   %10.0lf   %10.0lf\n\n",
            mean(stats->totalbody, stats->totalconnects),
            stddev(stats->totalbody, stats->totalbodysq, stats->totalconnects),
            stats->minbody,
            stats->maxbody);

    fprintf(f, "%.0lf body bytes moved + %.0lf header bytes moved = %.0lf total\n",
            stats->totalbody,
            stats->totalbytes - stats->totalbody,
            stats->totalbytes);
}
