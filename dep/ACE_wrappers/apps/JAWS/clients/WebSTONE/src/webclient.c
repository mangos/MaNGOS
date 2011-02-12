/* $Id: webclient.c 81998 2008-06-17 09:31:15Z sma $ */
/**************************************************************************
 *                                                                        *
 *     Copyright (C) 1995 Silicon Graphics, Inc.      *
 *                    *
 *  These coded instructions, statements, and computer programs were    *
 *  developed by SGI for public use.  If any changes are made to this code*
 *  please try to get the changes back to the author.  Feel free to make  *
 *  modifications and changes to the code and release it.     *
 *                    *
 **************************************************************************/

/* FUZZ: disable check_for_math_include */
/* FUZZ: disable check_for_improper_main_declaration */

#include <thread.h>

#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <time.h>
#include <process.h>
#include <io.h>
#endif /* WIN32 */

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <netdb.h>
#include <unistd.h>
#endif /* WIN32 */

#include <time.h>
#include <math.h>

#ifndef WIN32
#include <sys/param.h>
#endif /* WIN32 */

#include <sys/types.h>

#ifndef WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* WIN32 */

#include <ctype.h>

#include "sysdep.h"
#include "bench.h"

#define _BSD_SIGNALS
#define INFINITY 100000000
#define DEFAULTWWWPORT 80
#define LOG_FILE "logfile"
#ifdef WIN32
#define DEBUG_FILE  "c:/tmp/webstone-debug"
#else
#define DEBUG_FILE  "/tmp/webstone-debug"
#endif /* WIN32 */
#define NCCARGS 4096

/* global variables */

  THREAD FILE *debugfile = stderr;
  page_list_t *load_file_list; /* actually a dynamic array */

int amclient = 0;
int havewebserver = 0;
int haveproxyserver = 0;
int   savefile = 0;
NETPORT portnum = DEFAULTWWWPORT;
int   timeexpired = 0;
int     debug = 0;
long int  number_of_pages = 0;
char  webmaster[MAXHOSTNAMELEN];
char  webserver[MAXHOSTNAMELEN];
char  proxyserver[MAXHOSTNAMELEN];


#ifdef WIN32
HANDLE  hSemaphore;
int CounterSemaphore = 0;   /* counter semaphore for children */
#endif /* WIN32 */

static void ClientThread(void *);

/* used to bypass DNS/YP name resolution for every page */
struct hostent  webserv_phe, webmast_phe;
struct protoent webserv_ppe, webmast_ppe;
unsigned long webserv_addr, webmast_addr;
short     webserv_type, webmast_type;               /* socket type */

/* End of globals */


static void
usage(const char *progname)
{
    returnerr("Usage: %s [-d]  [-w webserver]  [-p port_num]\n",
  progname);
    returnerr("\t[-c masterhost:port]  [-t run_time | -l loops]\n");
    returnerr("\t[-n numclients]  [-R]\n");
    returnerr("\t[-f config_file]  [-u uilfile | url ...]\n");
    errexit("\n");
} /* END usage() */

static void
alarmhandler(void)
{
  /* RECEIVED AN ALARM SIGNAL */
  timeexpired = 1;
} /* END alarmhandler() */

#ifndef WIN32
static void
childhandler(void)
{
  int status;

  /* RECEIVED A SIGNAL THAT A CHILD PROCESS HAS DIED */
  D_PRINTF( "A child process has died\n" );
  while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0)
    {
      /* do nothing */
      ;
    }
} /* END childhandler() */
#endif /* WIN32 */


/* look up the host name and protocol
 * called once by main() since all threads
 * use the same protocol and address
 */

int resolve_addrs(char *host, char *protocol, struct hostent *host_phe, struct protoent *proto_ppe, unsigned long *addr,
  short *type) {
struct hostent *phe;
struct protoent *ppe;

    /* if IP address given, convert to internal form */
    if (host[0] >= '0' && host[0] <= '9') {
  *addr = inet_addr(host);
  if (*addr == INADDR_NONE)
      return(returnerr("Invalid IP address %s\n", host));

    } else {
  /* look up by name */
  phe = gethostbyname(host);
  if (phe == 0)
  {
      D_PRINTF( "Gethostbyname failed: %s", neterrstr() );
      return(returnerr("Can't get %s host entry\n", host));
  }
  memcpy(host_phe, phe, sizeof(struct hostent));
  memcpy((char *)addr, phe->h_addr, sizeof(*addr));
    }

    /* Map protocol name to protocol number */
    ppe = getprotobyname(protocol);

    if (ppe == 0)
    {
  D_PRINTF( "protobyname returned %d\n",  ppe );
  return(returnerr("Can't get %s protocol entry\n",protocol));
    }
    memcpy(proto_ppe, ppe, sizeof(struct protoent));

    D_PRINTF( "Protocol number %d\n", ppe->p_proto );

    /* Use protocol to choose a socket type */
    if (strcmp(protocol,"udp") == 0)
    {
  *type = SOCK_DGRAM;
    }
    else
    {
  *type = SOCK_STREAM;
  D_PRINTF( "Choosing SOCK_STREAM %d type %d %s\n",
      SOCK_STREAM, *type, neterrstr() );
    }

    return 0;
}

/* connect to a socket given the hostname and protocol */
SOCKET
connectsock(char *host, NETPORT portnum, char *protocol)
 {
  struct sockaddr_in sin;   /* an Internet endpoint address */
  SOCKET  s;              /* socket descriptor */
  int     type;           /* socket type */
  short   proto;
  int     returnval;  /* temporary return value */

  D_PRINTF( "Beginning connectsock; host=%s port=%d proto=%s\n", host,
  portnum, protocol );

  sin.sin_family = AF_INET;
  memset((char *)&sin, 0, sizeof(sin));
  D_PRINTF( "Zeroed address structure\n" );

  sin.sin_port = htons(portnum);
  D_PRINTF( "Set port number %d\n", portnum );

  /* get the contact information */
  if (strcmp(host, webserver) == 0) {
      sin.sin_addr.S_ADDR = webserv_addr;
      sin.sin_family = PF_INET;
      proto = webserv_ppe.p_proto;
      type = webserv_type;
  } else if (strcmp(host, webmaster) == 0) {
      sin.sin_addr.S_ADDR = webmast_addr;
      sin.sin_family = PF_INET;
      proto = webmast_ppe.p_proto;
      type = webmast_type;
  } else {
      struct hostent  host_phe;
      struct protoent host_ppe;
      unsigned long host_addr;
      short     host_type;       /* socket type */

      if (resolve_addrs(host, "tcp", &host_phe, &host_ppe, &host_addr, &host_type))
    return returnerr("Can't resolve hostname %s in get()\n", host);
      sin.sin_addr.S_ADDR = host_addr;
      sin.sin_family = PF_INET;
      proto = host_ppe.p_proto;
      type = host_type;
  }

  /* Allocate a socket */
  s = socket(PF_INET, type, proto);
  D_PRINTF( "Socket %d returned %d, %s\n",
  type, s, neterrstr() );

  if (BADSOCKET(s))
  {
    D_PRINTF( "Can't create socket: %s\n",neterrstr() );
    return BADSOCKET_VALUE;
  }

  /* Connect the socket */
  D_PRINTF( "Trying to connect %d with size %d, %s\n",
  s, sizeof(sin), neterrstr() );
  D_PRINTF( "Address is family %d, port %d, addr %s\n",
  sin.sin_family, ntohs(sin.sin_port),
  inet_ntoa(sin.sin_addr) );

  returnval = connect(s, (struct sockaddr *)&sin, sizeof(sin));
  D_PRINTF( "Connect returned %d, %s\n",
  returnval, neterrstr() );
  if (returnval < 0)
    {
      D_PRINTF( "Can't connect: %s\n", neterrstr() );
      NETCLOSE(s);
      return BADSOCKET_VALUE;
    }

  /* all done, returning socket descriptor */
  D_PRINTF( "Returning %d from connectsock call\n", s );
  return(s);

} /* END connectsock() */

SOCKET
connecttomaster(char *str)
{
    char    *tempch;
    SOCKET  sock;
    char  msg[100];
    char  ConnectStr[100];  /* Fix to handle multiple threads */
    int   tries;

    strcpy(ConnectStr, str);

    /*
     * BREAK UP THE connectstr INTO A HOSTNAME/HOST-IP AND A PORT NUMBER.
     */
    if((tempch = strpbrk(ConnectStr,":")) == 0)
    {
        /*
         * INCORRECT FORMAT OF ConnectStr. CORRECT FORMAT IS
         * HOSTNAME:PORT OR HOST-IP:PORT
         */
        D_PRINTF( "Incorrect format %s: use hostname:port or ip_addr:port\n",
    ConnectStr );
  return(returnerr("Incorrect format %s: use host:port or ip_addr:port\n",
    ConnectStr));
    }

    /*
     * ZERO OUT THE COLON SO WE HAVE TWO STRINGS, THE HOSTNAME AND THE PORT
     */
    *tempch = '\0';
    tempch++;

    /* loop here to connect to webmaster - TCP/IP allows no more than 5
     * connection requests outstanding at once and thus the webmaster may
     * reject a connection if there are a lot of client processes
     */
#define MAXTRIES 30
#define TRYDELAY_SEC  1
    for (tries = 0; tries < MAXTRIES; tries++) {

        sock = connectsock(ConnectStr,(NETPORT)atoi(tempch),"tcp");

  if (!BADSOCKET(sock))
      break;

  sleep(TRYDELAY_SEC);
    }

    if (BADSOCKET(sock))
    {
  /*  ERROR CONNECTING TO MASTER PROCESS */
  return(returnerr("Could not connect to master process\n"));
    }

    /*
     * SIGNAL THE MASTER THAT WE ARE READY TO PROCEED.  WHEN ALL
     * CHILD PROCESSES HAVE CONNECTED AND SENT THIS SIGNAL,
     * THE MASTER WILL ISSUE US A GO SIGNAL.
     */
    if(NETWRITE(sock,READYSTR,READYSTRLEN) != READYSTRLEN)
    {
  return(returnerr("Error sending READY message to master"));
    }

    memset(msg,0,GOSTRLEN+1);
    if(NETREAD(sock,msg,GOSTRLEN) != GOSTRLEN)
    {
  D_PRINTF( "Error receiving GO message from master: %s\n", neterrstr()
    );
  return(returnerr("Error receiving GO message from master\n"));
    }

    if(strncmp(GOSTR,msg,GOSTRLEN))
    {
       /*
        * WE RECEIVED A MESSAGE OTHER THAN GO. PRINT IT OUT AND RETURN ERROR
        */
  return(returnerr("Received non-GO message %s\n",msg));
    }

    return(sock);

} /* END connecttomaster() */


static void
accumstats(rqst_timer_t *rqsttimer, page_stats_t *pagestats, stats_t *timestat)
{
    rqst_stats_t  rqststats;

#define TFMT  "%10u:%10u"
    /*
     * DUMP THE TIMING INFORMATION HERE, OR COMPUTE WHAT YOU WANT TO
     * PRINT OUT LATER.
     */

    D_PRINTF( "Total bytes read: %d \t Body size read: %d\n",
  rqsttimer->totalbytes,
  rqsttimer->bodybytes );

    D_PRINTF( "Enter time:     " TFMT " \t Exit Time:     " TFMT "\n",
  rqsttimer->entertime.tv_sec,
  rqsttimer->entertime.tv_usec,
  rqsttimer->exittime.tv_sec,
  rqsttimer->exittime.tv_usec );
    D_PRINTF( "Before connect: " TFMT " \t After connect: " TFMT "\n",
  rqsttimer->beforeconnect.tv_sec,
  rqsttimer->beforeconnect.tv_usec,
  rqsttimer->afterconnect.tv_sec,
  rqsttimer->afterconnect.tv_usec );
    D_PRINTF( "Before header:  " TFMT " \t After header:  " TFMT "\n",
  rqsttimer->beforeheader.tv_sec,
  rqsttimer->beforeheader.tv_usec,
  rqsttimer->afterheader.tv_sec,
  rqsttimer->afterheader.tv_usec );
    D_PRINTF( "After body:     " TFMT "\n",
  rqsttimer->afterbody.tv_sec,
  rqsttimer->afterbody.tv_usec );

    rqstat_times(&(rqststats), rqsttimer);
    rqstat_sum(&(timestat->rs), &(rqststats));
    rqstat_sum(&(pagestats->rs), &(rqststats));

    if (rqsttimer->page_number != 999)
    {
  timestat->page_numbers[rqsttimer->page_number] += 1;
    }

#undef TFMT
} /* END accumstats */


/*
 * fetch the set of files that constitute a page
 *
 * maxcount = the number of files in the WWW page
 * pageval = the number of the WWW page (offset in load_file_list[])
 *  (if -1, use page # 0 - does this still work?)
 *
 * returns the number of files retrieved
 */
static int
makeload(int maxcount, int pageval, THREAD rqst_timer_t *timerarray, THREAD stats_t *timestat, THREAD SOCKET mastersock, THREAD page_stats_t *page_stats)
{
    int cnt;
    int returnval;
    page_stats_t page_stats_tmp;
    char server[MAXHOSTNAMELEN];

    NETPORT loc_portnum;

    D_PRINTF( "Starting makeload(maxcount %d, pageval %d)\n",
  maxcount, pageval );

    strcpy( server, webserver); /* Put in default value */

    page_stats_init(&page_stats_tmp);
    D_PRINTF( "Page stats initialized\n" );

    for (cnt = 0; cnt < maxcount; cnt++)
    {
  D_PRINTF( "Loop count %d in makeload()\n", cnt );
  if (pageval == -1)
  {
      pageval = cnt;
  }
  if (timeexpired)
  {
      break;
  }

  /* check for a filename */
  if (strlen(load_file_list[pageval].filename[cnt]) < 1)
  {
      D_PRINTF( "Bad filename at pageval %d, count %d\n",
    pageval, cnt );
      return(returnerr("Bad filename at pageval %d, count %d\n",
    pageval, cnt));
  }

        /*        if (load_file_list[pageval].port_number[cnt] != 0)
        {
            loc_portnum = load_file_list[pageval].port_number[cnt];
        }
        else
        {
            loc_portnum = portnum;
        } */
        loc_portnum = portnum;
        if ((load_file_list[pageval].servername[cnt] != 0)
        &&
      *load_file_list[pageval].servername[cnt])
  {
      D_PRINTF( "Copying URL server %s to server\n",
    load_file_list[pageval].servername[cnt] );
      strcpy(server, load_file_list[pageval].servername[cnt]);
  }

  if (haveproxyserver)
    {
      D_PRINTF( "Copying proxy %s to webserver\n", proxyserver );
      strcpy(server, proxyserver);
    }


  D_PRINTF( "Calling get(%s, %d, %s, &(timearray[%d]))\n",
      server, loc_portnum, load_file_list[pageval].filename[cnt],
      cnt );

  returnval = get(server, loc_portnum,
        load_file_list[pageval].filename[cnt],
        &(timerarray[cnt]));
  if (returnval < 0)
  {
      D_PRINTF( "***GET() RETURNED AN ERROR\n" );
  }

  /*
  * DID GET() RETURN A VALID TIME?
  */
  if ((returnval == 0) && (timerarray[cnt].valid == 2))
  {
      timerarray[cnt].page_number = pageval;

      accumstats(&timerarray[cnt], &page_stats_tmp, timestat);
  }
  else if (!timeexpired) /* INVALID, INCREMENT THE ERROR COUNTER */
  {
      D_PRINTF( "GET error counter incremented\n" );
      timestat->rs.totalerrs++;
  }

  if (amclient) {
      fd_set readfds;
      struct timeval timeout;
      int rv;

      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      FD_ZERO(&readfds);
      FD_SET(mastersock, &readfds);

      /* if the webmaster has aborted, quit */
      D_PRINTF("Before select() on webmaster socket\n");
      if (rv = select(FD_SETSIZE, &readfds, 0, 0, &timeout))  {
    D_PRINTF("select() returned %d\n", rv);
    D_PRINTF("Client terminating at request of webmaster\n");
    exit(2);
      }
  }

    } /* END for cnt */

    /*
     * DO WE HAVE A VALID RETURN FROM GET()?
     * WHY NOT USE returnval HERE?
     */
    if ((returnval == 0) &&
    (cnt == load_file_list[pageval].num_of_files) &&
    (timerarray[cnt-1].valid == 2))
    {
  rqst_stats_t *ps_rs;
  rqst_stats_t *pst_rs;

  ps_rs = &(page_stats[pageval].rs);
  pst_rs = &(page_stats_tmp.rs);

  rqstat_sum(ps_rs, pst_rs);

  page_stats[pageval].totalpages++;

  if (page_stats[pageval].page_size == 0)
  {
      page_stats[pageval].page_size = (unsigned)
    page_stats_tmp.rs.totalbody;
  }
    }

    D_PRINTF( "\nMakeload output page %d: %d errors, %d pages\n",
      pageval, timestat->rs.totalerrs, page_stats[pageval].totalpages );
    D_PRINTF( "Makeload returning %d\n", cnt );

    return(cnt);

} /* END makeload() */

#ifdef WIN32
/* close socket library at exit() time */
void sock_cleanup(void) {

    WSACleanup();
}
#endif /* WIN32 */

/* globalize variables that were in main() */
long int  numfiles = 0;
int testtime = 0;
int numloops = 0;
int numclients = 0;
int record_all_transactions = 0;
int uil_filelist_f = 0; /* filedescriptor of URLs to fetch? */
int verbose = 0;
int     total_weight;
char  uil_filelist[NCCARGS];
char  filelist[MAXNUMOFFILES][MAXPATHLEN];
char  configfile[MAXPATHLEN];
char  connectstr[MAXHOSTNAMELEN+10];

void
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
    int   file_count=0;
    int   getoptch;
    int   currarg;
    extern char *optarg;
    extern int  optind;
    int   i, j;
    char  *tempch;
    int   err;

#define SLEEP_USEC  100
#ifdef WIN32
    WSADATA   WSAData;
#else

    struct timeval sleeptime;

    /* set the amount of time that we'll pause before sending a "." to the
       webmaster */

    sleeptime.tv_sec = SLEEP_USEC/1000000;
    sleeptime.tv_usec = SLEEP_USEC % 1000000;
#endif /* WIN32 */

    debugfile = stderr;

#ifdef WIN32
    MessageBeep(~0U); /* announce our existence */
    MessageBeep(~0U);
    MessageBeep(~0U);

    err = WSAStartup(MAKEWORD(1,1), &WSAData);
    if (err != 0) {
  errexit("Error in WSAStartup()\n");
    }

    atexit(sock_cleanup);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    /* create semaphore in locked state */
    hSemaphore = CreateSemaphore(0, 0, 1, 0);
    if(hSemaphore == 0)
    {
  errexit("Create semaphore failed: %d", GetLastError());
    }
#endif /* WIN32 */

    memset(webserver, 0, sizeof(webserver));
    memset(webmaster, 0, sizeof(webmaster));
    memset(proxyserver, 0, sizeof(proxyserver));
    memset(connectstr, 0, sizeof(connectstr));

    /*
     * PARSE THE COMMAND LINE OPTIONS
     */

    while((getoptch = getopt(argc,argv,"P:f:t:l:p:u:R:w:c:n:sdv")) != EOF)
    {
        switch(getoptch)
        {
  case 'c':
            sprintf(connectstr, "%s", optarg);
      amclient = 1;
            printf("%s", OKSTR);     /* sent back to webmaster */
            fflush(stdout);
      break;
        case 'd':
            debug = 0; /* sumedh */
            break;
  case 'f':
      sprintf(configfile, "%s", optarg);
      break;
        case 'l':
            numloops = atoi(optarg);
            break;
        case 'n':
            numclients = atoi(optarg);
            break;
  case 'u':
      sprintf(uil_filelist, "%s", optarg);
      uil_filelist_f = 1;
      break;
  case 'p':
      portnum = atoi(optarg);
      break;
  case 's':
      savefile = 1;
      break;
        case 't':
            testtime = 60 * atoi(optarg);
            break;
  case 'v':
      verbose = 1;
      break;
        case 'w':
            havewebserver = 1;
            sprintf(webserver,"%s",optarg);
            break;
  case 'P':
      haveproxyserver = 1;
      sprintf(proxyserver, "%s", optarg);
      break;
  case 'R':
      record_all_transactions = 1;
      break;
        default:
            usage(argv[0]);
        }
    }

    returnerr("Client begins...\n");
    D_PRINTF( "Running in debug mode\n\n" );

    /* print the command line */
    for (i = 0; i < argc; i++)
  D_PRINTF( "%s ", argv[i] );
    D_PRINTF( "\n\n" );

    if(testtime && numloops)
    {
  /*
         * EITHER numloops OR testtime, BUT NOT BOTH.
         */
        usage(argv[0]);
    }

    if(havewebserver != 1)
    {
#ifdef WIN32
        /*
         * THE SERVER'S NAME MUST BE SPECIFIED
         */
        returnerr("No WWW Server specified\n");
        usage(argv[0]);
#else
  /* IF IT ISN'T, WE ASSUME LOCALHOST */
  sprintf(webserver, "%s", "localhost");
  havewebserver = 1;
#endif /* WIN32 */
    }

    currarg = optind;
    numfiles = 0;
    while(currarg != argc)
    {
       /*
        * GET THE URLS TO RETRIEVE.
        */
       if (numfiles == MAXNUMOFFILES) {
     returnerr("Maximum of %d files on the command line.\n");
     usage(argv[0]);
       }
       sscanf(argv[currarg],"%s",filelist[numfiles]);
       numfiles++;
       currarg++;
    }

    if ((numfiles != 0) && uil_filelist_f)
    {
  returnerr("Both a filelist and UIL specified.\n");
  usage(argv[0]);
    }

    if((numfiles == 0) && !(uil_filelist_f))
    {
        /*
         * AT LEAST ONE FILE MUST BE SPECIFIED
         */
  returnerr("No UIL resources or filelist specified \n");
        usage(argv[0]);
    }

    if((numloops == 0) && (testtime == 0))
    {
        /*
         * NO SPECIFIED NUMBER OF LOOPS, AND NO TEST TIME
         */
       usage(argv[0]);
    }
    if(numclients > MAXPROCSPERNODE || numclients < 1)
    {
  returnerr("Number of Clients must be between 1 and %d\n", MAXPROCSPERNODE);
  exit(1);
    }

    /* allow use of IP address */
    if(amclient) {
  if((tempch = strpbrk(connectstr,":")) == 0)
  {
      /*
       * INCORRECT FORMAT OF ConnectStr. CORRECT FORMAT IS
       * HOSTNAME:PORT OR HOST-IP:PORT
       */
      D_PRINTF( "Incorrect format %s: use hostname:port or ip_addr:port\n",
         connectstr );
      returnerr("Incorrect format %s: use host:port or ip_addr:port\n", connectstr);
      exit(1);
  } else {
      strncpy(webmaster, connectstr, tempch-connectstr);
  }
  if(resolve_addrs(webmaster, "tcp", &webmast_phe, &webmast_ppe, &webmast_addr, &webmast_type))
      exit(1);
    }

    if (haveproxyserver)
    {
  D_PRINTF( "Copying proxy %s to webserver\n", proxyserver );
  strcpy(webserver, proxyserver);
    }

    if (resolve_addrs(webserver, "tcp", &webserv_phe, &webserv_ppe, &webserv_addr, &webserv_type))
  exit(1);

    /*
     * INITIALIZE DATA
     */
    /* allocate space for dynamic arrays */
    load_file_list =
      (page_list_t *)mymalloc((MAXNUMOFPAGES)*sizeof(page_list_t));

    if (uil_filelist_f)
    {
      /* take a guess at the number of URLs in the file */
      D_PRINTF( "About to parse filelist %s\n", uil_filelist );
      number_of_pages = count_file_list(uil_filelist);
      numfiles = 1;

      /* IF WE HAVE A FILELIST, PARSE IT */
      /* allocate memory */
      D_PRINTF( "Allocating page list: %ld by %d\n",
  number_of_pages, numfiles );
      for (i=0; i<number_of_pages; i++)
  {
    for (j=0; j<MAXNUMOFFILES; j++)
      {
        load_file_list[i].servername[j] =
    (char *)mymalloc(URL_SIZE);
        load_file_list[i].filename[j] =
    (char *)mymalloc(URL_SIZE);
      }
  }

      D_PRINTF( "Parsing file list: %s\n", uil_filelist );
      parse_file_list(uil_filelist, load_file_list,
          &number_of_pages, &numfiles);
      /* free memory for pages that won't be used? */
      D_PRINTF( "Actual page list: %ld by %d\n",
  number_of_pages, MAXNUMOFFILES );

      D_PRINTF( "Setting up weighting for %ld pages\n",
  number_of_pages );
      total_weight = load_percent(load_file_list, number_of_pages);
      /*      total_weight = load_percent(load_file_list, number_of_pages, pages); */
    }
    else
    {
  /* no uil file */
  number_of_pages = numfiles;
    }

    if (number_of_pages < 1)
      {
  /* no pages - exit */
  D_PRINTF( "No valid URLs found\n" );
  errexit("No valid URLs found\n");
      }

#ifndef WIN32
    /*
     * IF WE ARE TO FORK ADDITIONAL CLIENTS ON THIS MACHINE,
     * WE MUST DO IT BEFORE WE CONNECT TO THE MASTER.
     *
     * FIRST, SET UP SIGNAL HANDLING
     */
    signal(SIGCHLD, childhandler);
    for(i = 0; i < numclients; i++)
    {
      thr_create (0, 0, ClientThread, 0, THR_BOUND, 0);

      /* switch(fork())
         {
             case 0:
                 numclients = 1;
                 ClientThread(0);
                 exit(0);
                 break;
             case -1:
                 errexit("Error forking child processes\n");
                 exit(1);
             default:
                break;
         } */
      select(0,(fd_set *)0,(fd_set *)0, (fd_set *)0, &sleeptime);
    }

    /*
     * Wait for all children to exit.
     */

    while (thr_join(0, 0, 0) == 0);

    /*     for(;;)
       {
      int pid = wait((int*)0);
  if ((pid == -1) && errno == ECHILD) break;
       } */
#else
    /* start threads on NT */
    for (i = 0; i < numclients; i++)
    {
  if (_beginthread(ClientThread, 0, 0) == -1)
  {
      errexit("_beginthread failed: %d", GetLastError());
  }
    }
#endif /* WIN32 */

#ifdef WIN32
    /* wait for children to get to sync point */
    while (CounterSemaphore < numclients)
        sleep(1);
    CounterSemaphore = 0;

    /* start all children simultaneously */
    ReleaseSemaphore(hSemaphore, 1, 0);

    if (testtime) {
  sleep(testtime);
  alarmhandler(); /* signal end of test to threads */
    }

    /*
     * Wait for all threads to exit.
     */
    while (CounterSemaphore < numclients)
        sleep(1);

    CloseHandle(hSemaphore);
#endif /* WIN32 */

    return;
} /* end main() */

void ClientThread(void *dummy)
{

  THREAD FILE *logfile;

  THREAD stats_t  timestat;

  THREAD rqst_timer_t timerarray[MAXNUMOFFILES];
  THREAD SOCKET mastersock = BADSOCKET_VALUE; /* connection to webmaster */


  THREAD page_stats_t *page_stats;    /* actually a dynamic array */

    int   loopcnt = 0;
    int   filecnt;
    int   loop;
    int   ran_number;
    int   page_index;
    int   page_number;
    int   file_count = 0;
    char  file_name[50];
    struct timeval  runningtime;
    time_t    junk;
    int   i;
    int   returnval;

    /*
     * INITIALIZE DATA
     */

    page_stats =
      (page_stats_t *)mymalloc((number_of_pages)*sizeof(page_stats_t));

    for (i=0; i < number_of_pages; i++) {
        page_stats_init(&(page_stats[i]));
    }

    if (debug)
      {
  /*
   *  OPEN A DEBUG FILE
   */
  fflush(stderr);
  sprintf(file_name, "%s.%d", DEBUG_FILE, (int)getpid());
  debugfile = fopen(file_name, "w+");
  if (debugfile == 0)
      errexit("Can't open debug file\n");
  D_PRINTF( "Running in debug mode, %d\n",amclient );
    }

    if (record_all_transactions)
    {
  /*
   *  OPEN A LOG FILE.
   */
  sprintf(file_name, "%s%d", LOG_FILE, (int)getpid());
  returnerr("Log file is %s\n", file_name);
  logfile = fopen(file_name, "w+");
    }

    /* Initialize random number generator */
    junk = getpid ();
    rand_r(&junk);
    D_PRINTF( "Random seed: %d\n", junk );

    for (i=0; i < MAXNUMOFFILES; i++)
    {
  rqtimer_init(&(timerarray[i]));
    }
    stats_init(&timestat);

    D_PRINTF( "Number of files %d\n", numfiles );

    timestat.total_num_of_files = numfiles;

    if (amclient)
    {
        /*
         * WE ARE A CLIENT PROCESS. (i.e. WE ARE NOT RUN BY A USER, BUT BY
         * THE MASTER WWWSTONE PROCESS. WE NEED TO CONNECT TO THE
         * MASTER WHO WILL SYNCHRONIZE ALL THE CLIENTS.
         */
  D_PRINTF( "Trying to connect with %s\n",connectstr );

  mastersock = connecttomaster(connectstr);

  D_PRINTF( "connecttomaster returns %d, %s\n",
          mastersock, neterrstr() );

  if(BADSOCKET(mastersock))
        {
            /*
             * ERROR CONNECTING TO THE MASTER. ABORT.
             */
            errexit("Error connecting to the master: %s\n", neterrstr());
        }
    } /* END IF CLIENT */

#ifdef WIN32
    /* Tell parent we're ready */
    InterlockedIncrement(&CounterSemaphore);

    /* Wait for main() thread to release us */
    WaitForSingleObject(hSemaphore, INFINITE);
    ReleaseSemaphore(hSemaphore, 1, 0);
#endif /* WIN32 */
    if (testtime != 0)
    {
       /*
        * IF RUNNING A TIMED TEST, WE WILL LOOP
  * UNTIL THE ALARM GOES OFF.
        * WE'LL ALSO NEED TO SET THE SIGNAL HANDLER
        */
#ifndef WIN32
      /*signal(SIGALRM, alarmhandler);*/
       /*
        * SEND SIGALRM IN testtime SECONDS
        */
      /*alarm(testtime);*/
#endif /* WIN32 */
    }

    /*
     * AND THEY'RE OFF...
     */

    if (testtime)
  numloops = INFINITY;
    GETTIMEOFDAY(&(timestat.starttime), &(timestat.starttimezone));

    /* LOOP UNTIL WE HIT numloops, OR WE RUN OUT OF TIME */
    for(loopcnt = 0;  (loopcnt < numloops) && !timeexpired;  loopcnt++)
    {
  /*
   * THIS IS WHERE LOAD TESTING IS DONE.
   * GET A RANDOM NUMBER, THEN INDEX INTO THE
   * PAGE, AND THEN REQUEST THAT SET OF FILES.
   */
  if (uil_filelist_f) /* HAVE FILELIST */
  {
      D_PRINTF( "Have filelist\n" );
      /* if (testtime != 0) /* RUNNING IN TIMED MODE */
            if (1)
      {
    D_PRINTF( "Running in timed mode\n" );
    /* random number between 0 and totalweight-1 */
                junk = getpid ();
    ran_number = (rand_r(&junk) % total_weight);
    D_PRINTF( "random %ld\n", ran_number );

    /* loop through pages, find correct one
     * while ran_number is positive, decrement it
     * by the load_num of the current page
     * example: ran_number is 5, pages have weights of 10 and 10
     *          first iteration page_index = 0, ran_number = -5
     *          iteration halted, page_index = 0
     */
    page_index = -1;
    while (ran_number >= 0)
      {
        page_index++;
        D_PRINTF( "Current page index %d: %ld - %d\n",
          page_index, ran_number,
          load_file_list[page_index].load_num
          );
        ran_number -= load_file_list[page_index].load_num;
      }

    if (page_index >= number_of_pages) { page_index--; }

    D_PRINTF( "Final page index %d\n", page_index );
    filecnt = makeload(load_file_list[page_index].num_of_files,
               page_index, timerarray, &timestat, mastersock, page_stats);
                testtime = 1;
      }
      else /* NOT RUNNING IN TIMED MODE */
      {
    for (page_number = 0;
         page_number < number_of_pages;
         page_number++)
    {
        filecnt = makeload(load_file_list[page_number].num_of_files,
      page_number, timerarray, &timestat, mastersock, page_stats);

    } /* END for page_number */
      } /* END if/else TIMED MODE */
  }
  else /* NO FILELIST */
  {
      D_PRINTF( "No filelist\n" );
     /*
      * LOOP THROUGH UNTIL numfiles TIMES OR UNTIL TIMER EXPIRES
      * AND ALARM SETS filecnt TO INFINITY.
      */

      /* does this still work?? */
      /* filecnt = makeload(numfiles, -1, timerarray); */
  } /* END if HAVE FILELIST */

  if (filecnt > 0)
      file_count += filecnt;

    } /* END while loopcnt */

    GETTIMEOFDAY(&(timestat.endtime), &(timestat.endtimezone));
    D_PRINTF( "Test run complete\n" );
    signal(SIGALRM, 0);

    if (testtime == 0)
    {
  numfiles = loopcnt;

  if (uil_filelist_f)
  {
      numfiles = file_count;
  }
    }

    /* This option ( "-R" ) looks broken (e.g. l > 50) -- JEF 2/15/96 */
    if (record_all_transactions)
    {
  /*
   * DUMP THE LOG FILE INFORMATION.
   */
  for (loop=0; loop < (loopcnt * file_count); loop++)
  {
      fprintf(logfile, " entertime \t%d.%d\n"
           " beforeconnect \t%d.%d\n"
           " afterconnect \t%d.%d\n"
           " beforeheader \t%d.%d\n"
           " afterheader \t%d.%d\n"
           " afterbody \t%d.%d\n"
           " exittime \t%d.%d\n"
           " total bytes \t%d\n"
           " body bytes\t%d\n",
    timerarray[loop].entertime.tv_sec,
    timerarray[loop].entertime.tv_usec,
    timerarray[loop].beforeconnect.tv_sec,
    timerarray[loop].beforeconnect.tv_usec,
    timerarray[loop].afterconnect.tv_sec,
    timerarray[loop].afterconnect.tv_usec,
    timerarray[loop].beforeheader.tv_sec,
    timerarray[loop].beforeheader.tv_usec,
    timerarray[loop].afterheader.tv_sec,
    timerarray[loop].afterheader.tv_usec,
    timerarray[loop].afterbody.tv_sec,
    timerarray[loop].afterbody.tv_usec,
    timerarray[loop].exittime.tv_sec,
    timerarray[loop].exittime.tv_usec,
    timerarray[loop].totalbytes,
    timerarray[loop].bodybytes);
  } /* end for loop */
    } /* end if recording all transactions */

    D_PRINTF( "total errors: %d\n",timestat.rs.totalerrs );
    /* gethostname(timestat.hostname,MAXHOSTNAMELEN); */
    /* D_PRINTF( "Test for host: %s\n",timestat.hostname ); */
    D_PRINTF( "Server is: %s running at port number: %d\n",
  webserver,portnum );

    /* sprintf(timestat.hostname,"%s:%d",timestat.hostname,getpid()); */
    if (amclient) /* CLIENT TO A WEBMASTER */
    {
   char *stats_as_text;

        /*
         * SEND THE TIMING DATA TO THE MASTER
         */
  stats_as_text = stats_to_text(&timestat);
  D_PRINTF( "stats_to_text returned %s\n", stats_as_text );

  returnval = senddata(mastersock, stats_as_text,
    SIZEOF_STATSTEXTBASE + number_of_pages*SIZEOF_DOUBLETEXT);
  D_PRINTF( "Wrote time stats to master %d\n", returnval );

  if (returnval < 1)
    {
      D_PRINTF( "Error while writing time stats: %s\n",
        neterrstr() );
      errexit("Error while writing time stats: %s\n",
        neterrstr());
    }

  if (uil_filelist_f)
    /* write pagestats */
    {
      char *page_stats_as_text;
      for (i = 0; i < number_of_pages; i++)
        {
    D_PRINTF( "On page_stats[%d]\n", i );
    page_stats_as_text = page_stats_to_text(&page_stats[i]);
    returnval = strlen(page_stats_as_text);
    D_PRINTF( "page_stats_to_text[%d] returned %d\n",
      i, returnval );
    returnval = senddata(mastersock, page_stats_as_text,
             SIZEOF_PAGESTATSTEXT);
    if (returnval < 1)
      {
        D_PRINTF( "Error while writing page_stats[%d]: %s\n",
          i, neterrstr() );
        errexit("Error while writing page_stats[%d]: %s\n",
          i, neterrstr());
      } /* end if */
    D_PRINTF( "Wrote %d bytes of page_stats[%d] to master\n",
      returnval, i );
        } /* end for */
    } /* end if filelist */

  D_PRINTF( "About to close socket\n" );
        if (NETCLOSE(mastersock))
      D_PRINTF( "Close socket error: %s\n", neterrstr() );
    }
    else /* NOT A CLIENT TO A WEBMASTER */
    {
  if (testtime)
    {
      printf("Test ran for: %d minutes\n",(testtime/60));
  }
  else
  {
      printf("Test ran for: %d iterations.\n",numloops);
  }
  compdifftime(&(timestat.endtime), &(timestat.starttime),
    &(runningtime));
  printf("Total time of test (sec) %d.%d\n", runningtime.tv_sec,
    runningtime.tv_usec);
  printf("Files retrieved per iteration: %d\n",numfiles);  /* 'per iteration' */
  printf("----------------------------------\n");
  printf("Totals:\n\n");
  rqstat_print(&(timestat.rs));

  if (timestat.rs.totalconnects == 0)
      goto end;
  printf("Thruput = %5.2lf Kbytes/sec\n",
         thruputpersec(timestat.rs.totalbytes, &runningtime) / 1000);

  if (uil_filelist_f && numloops && verbose)
  {
      for (loop = 0; loop < number_of_pages; loop++)
      {
    if (timestat.page_numbers[loop] != 0)
    {
        printf ("===============================================================================\n");
        printf ("Page # %d\n\n", loop);
        printf ("Total number of times page was hit %d\n",
          page_stats[loop].totalpages);
        rqstat_print(&(page_stats[loop].rs));
        printf ("Page size %d \n", page_stats[loop].page_size);
        printf ("===============================================================================\n\n");
    } /* END if timestat */
      } /* END for loop */
  } /* END if filelist */
    } /* END if client */

end:
    if(record_all_transactions)
      fclose(logfile);
    if(debug)
    {
  D_PRINTF( "Client exiting.\n" );
  fclose(debugfile);
    }

#ifdef WIN32
    /* tell parent we're done */
    InterlockedIncrement(&CounterSemaphore);
#endif /* WIN32 */

} /* END ClientThread() */
