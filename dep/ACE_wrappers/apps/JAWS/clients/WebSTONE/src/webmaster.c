/* $Id: webmaster.c 91813 2010-09-17 07:52:52Z johnnyw $ */
/**************************************************************************
 *                    *
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

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WIN32
#include <unistd.h>
#endif /* WIN32 */

#include <math.h>

#ifndef WIN32
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#define FD_SETSIZE  1024 /* max size for select() - keep before <winsock.h>
        * and same size as MAXCLIENTS */
#include <windows.h>
#include <winsock.h>
#include <io.h>
#include <process.h>
#endif /* WIN32 */

#include "sysdep.h"
#include "bench.h"

/* command line options/data */
int     savefile = 0;
int     debug = 0;
int     norexec = 0;
int     haveproxyserver = 0;
char      proxyserver[MAXHOSTNAMELEN];
char      network_mask_str[30] = "255.255.255.0";
unsigned    network_mask = 0;
int     servaddrin_config = 0;
int     dumpall = 0;
int     testtime = 0;
int     havewebserver = 0;
int     numloops = 0;
NETPORT     portnum = 0;
int     redirect = 0;
int     record_all_transactions = 0;
int     uil_filelist_f = 0;
int     verbose = 0;
char      webserver[MAXHOSTNAMELEN];
char      configfile[MAXPATHLEN];
char      uil_filelist[NCCARGS];

char      filelist[256][MAXPATHLEN];
fd_set      zerofdset;

/* other key data */
long int    number_of_pages = 0;
int     totalnumclients = 0;
int     num_rexecs = 0;
SOCKET      socknum[MAXCLIENTS];
SOCKET      sockIO[MAXTOTALPROCS];
SOCKET      sockErr[MAXTOTALPROCS];
THREAD FILE *debugfile = stderr;
struct hostent  *master_phe;   /* IP addresses for webmaster */
struct timeval sumedh_start, sumedh_end;

void HostEntCpy(struct hostent *dest, struct hostent *src);

static void
usage(const char *progname)
{

  fprintf(stderr, "Usage: %s [-a] [-d] -f config_file [-l numloops]\n",
    progname);
  fprintf(stderr, "          [-p port_num] [-r] [-s] [-t run_time] \n");
  fprintf(stderr, "\n");
        fprintf(stderr, "-w webserver URL [URL ...]\n\n");
        fprintf(stderr, "-a print timing information for all clients\n");
        fprintf(stderr, "-d turn on debug statements\n");
        fprintf(stderr, "-f config_file\tfile specifying clients\n");
        fprintf(stderr, "-l number of iterations to retrieve uils\n");
        fprintf(stderr, "-p port number of web server if not 80\n");
        fprintf(stderr, "-r redirect stdout of clients to /tmp/webstone.xxx\n");
        fprintf(stderr, "-s save client gets to /tmp/webstone.data.*\n");
        fprintf(stderr, "-t run_time\tduration of test in minutes\n");
        fprintf(stderr, "-w webserver\tname of webserver host to contact\n");
  fprintf(stderr, "-u URL file\tfilelist of URLs\n");
  fprintf(stderr, "-v verbose mode\n");
  fprintf(stderr, "-P servername\tuse proxy server for transactions\n");
  fprintf(stderr, "-W webserver addresses are in the config file\n");
  fprintf(stderr, "-R record all transactions\n");
  errexit("\n");
}

static SOCKET
passivesock(const NETPORT portnum, const char *protocol, const int qlen)
{
  struct protoent    *ppe; /* pointer to protocol info entry */
  struct sockaddr_in sin;  /* Internet endpoint address */
  SOCKET    s;             /* socket descriptor */
  int    type;             /* socket type */

  D_PRINTF( "Beginning passivesock with errno %d\n",errno );

  D_PRINTF( "Zeroing address structure\n" );
  memset((char *)&sin, 0, sizeof(sin));

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;

  /* NOT USED: Map service name to portnumber */
  D_PRINTF( "Mapping portnum errno %d\n",errno );
  sin.sin_port = htons(portnum);

  /* Map protocol name to number */
  D_PRINTF( "Mapping protocol name errno %d\n",errno );
  if ((ppe = getprotobyname(protocol)) == 0)
    {
    errexit("Can't get \"%s\" protocol entry\n", protocol);
    }
  errno = 0;

  /* use protocol to choose socket type */
  D_PRINTF( "Changing socket type, errno %d\n",errno );
  if (strcmp(protocol, "udp") == 0)
    {
      type = SOCK_DGRAM;
  D_PRINTF( "Choosing SOCK_DGRAM\n" );
    }
  else
    {
      type = SOCK_STREAM;
  D_PRINTF( "Choosing SOCK_STREAM, errno %d\n",errno );
    }

  /* allocate a socket */
  s = socket(PF_INET, type, ppe->p_proto);
  if (BADSOCKET(s))
    {
      D_PRINTF( "Socket PF_INET %d %d returned %d with %s\n",
  type, ppe->p_proto, s, neterrstr() );
      errexit("Can't create socket: %s\n", neterrstr());
    }
  D_PRINTF( "Socket %d created with errno %d\n",s,errno );

  /* Bind the socket */
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    errexit("Can't bind to port %d: %s\n", portnum, neterrstr());
  }
  D_PRINTF( "Bind succeeded\n" );

  /* If it's a stream, listen for connections */
  /* NOTE: ON NT, the listen() backlog parm is silently limited to 5 conns */
  if ((type == SOCK_STREAM) && BADSOCKET(listen(s, qlen)))
  {
      errexit("Can't listen on port %s: %s\n", portnum, neterrstr());
  }
  D_PRINTF( "Listen succeeded\n" );

  /* all done, return socket descriptor */
  return(s);
}


/* abort clients -- called by SIGINT handler */
static void abort_clients(void)
{
    /* Not supposed to have fprintf in a signal handler, but... */
    fprintf(stdout, "Webmaster received SIGINT.  Terminating.\n");
    /* exit will close all open connections */
    exit(2);
}

/* signal handler for SIGINT */
static void sig_int(int sig) {

    abort_clients();
}

#ifdef WIN32

/* echo stdout/stderr from clients */
void echo_client(void *stream)
{
    SOCKET *sockarr;
    FILE *outfile;
    int which_stream = (int) stream;
    char buf[BUFSIZ];
    int i, len, rv;
    fd_set readfds;

    /* This code which handles the timeout may need
       to be ifdef'ed for WIN32 */
    struct timeval timeout;

    timeout.tv_sec = (long)5;
    timeout.tv_usec = (long)0;

    if (which_stream) {
  sockarr = sockIO;
  outfile = stdout;
    } else {
  sockarr = sockErr;
  outfile = stderr;
    }

    D_PRINTF( "echo_client running\n" );
    signal( SIGINT, SIG_DFL); /* restore default behavior
         for SIGINT */

    while (1) {
  FD_ZERO(&readfds);
  for (i = 0; i < num_rexecs; i++)
      if (sockarr[i] != BADSOCKET_VALUE)
    FD_SET(sockarr[i], &readfds);
  rv = select(num_rexecs, &readfds, 0, 0, &timeout);
  if ( rv == 0)
    continue;
  if (rv < 0 && WSAGetLastError() == WSANOTINITIALISED)
      return;
  if (rv < 0)
      errexit("Error in echo_client(): select() returns %d: %s\n", rv, neterrstr());

  /* loop over the sockets that are ready with data */
  for (i = 0; i < num_rexecs; i++) {
      if (sockarr[i] != BADSOCKET_VALUE && FD_ISSET(sockarr[i], &readfds)) {
    len = NETREAD(sockarr[i], buf, sizeof(buf));
    if (len <= 0) {
        /* mark connection closed */
        sockarr[i] = BADSOCKET_VALUE;
        if (len < 0 && WSAGetLastError() == WSANOTINITIALISED)
      return;
        if (len < 0)
      fprintf(stderr, "Error in echo_client() after NETREAD(): %s\n", neterrstr());
        continue;
    }

    /* copy to stdout or stderr */
    fwrite(buf, sizeof(char), len, outfile);
      }
  }
    }
    D_PRINTF( "Exiting echo_client\n" );
}

#else
static int
echo_client(char *hostname, const int fd)
{
  /*
   * WRITE TEXT FROM FILE DESCRIPTOR INTO STDOUT
   */
  char buf[BUFSIZ];
  int  cc;
  D_PRINTF( "echo_client running\n" );

  while (getppid() != 1)
    {
      cc = NETREAD(fd, buf, sizeof(buf));
      if (cc > 0)
      {
    write(STDOUT_FILENO, buf, cc);
      }
  }
  D_PRINTF( "Exiting echo_client\n" );
  NETCLOSE(fd);
}
#endif /* WIN32 */

/* Picks the appropriate webmaster IP address based on the address of the client.
 * This is significant only for hosts with multiple interfaces
 *
 * return value is a string with the IP address or hostname (or NULL)
 */
char *pick_webmaster_IP_address(char *client_hostname, struct hostent *master_phe,
        unsigned netmask) {
static char buf[20];
unsigned char   addr[4];
int client_addr;
int i;

    if (client_hostname[0] >= '0' && client_hostname[0] <= '9') {
  /* we have an IP address */
  client_addr = inet_addr(client_hostname);
  if (client_addr == INADDR_NONE)
      return 0;
    } else {
  /* we have a hostname, use the webserver hostname */
  return master_phe->h_name;
    }

    for (i = 0; master_phe->h_addr_list[i] != 0; i++) {
  if ((*(int *)(master_phe->h_addr_list[i]) & netmask) ==
    (client_addr & netmask))
      goto gotit;
    }
    i = 0;  /* give up */

gotit:
    memcpy((char *)addr, master_phe->h_addr_list[i], sizeof(addr));  /* Internet specific */
    sprintf(buf, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    return buf;
}

/*
  Command line parsing
*/

void ParseCmdLine(int argc, char **argv )
{
char    getoptch;
int   currarg;
extern char *optarg;
extern int  optind;

    /*
     * PARSE THE COMMAND LINE OPTIONS
     */
    while((getoptch = getopt(argc,argv,"P:f:t:l:p:u:R:w:n:M:adrsvWX")) != (const char)EOF)
      {
        switch(getoptch)
    {
    case 'M':
      strcpy(network_mask_str, optarg);
      break;
    case 'P':
      haveproxyserver = 1;
      strcpy(proxyserver, optarg);
      break;
    case 'R':
      record_all_transactions = 1;
      break;
    case 'X':
      norexec = 1;
      break;
    case 'W':
      servaddrin_config = 1;
      break;
    case 'a':
      dumpall = 1;
      break;
    case 'd':
      debug = 1;
      break;
    case 'f':
      strcpy(configfile, optarg);
      break;
    case 'l':
      numloops = atoi(optarg);
      break;
    case 'p':
      portnum = atoi(optarg);
      break;
    case 'r':
      redirect = 1;
      break;
    case 's':
      savefile = 1;
      break;
    case 't':
      testtime = atoi(optarg);
      break;
    case 'u':
      uil_filelist_f = 1;
      strcpy(uil_filelist, optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    case 'w':
      havewebserver = 1;
      strcpy(webserver, optarg);
      break;
    default:
      usage(argv[0]);
    } /* end switch */
      } /* end while */

    if (numloops && testtime)
  errexit("Can't have both -l and -t\n");

    if(!havewebserver && !servaddrin_config)
    {
        /*
         * THE SERVERS NAME MUST BE SPECIFIED
         */

        fprintf(stderr,"No WWW Server specified\n");
        usage(argv[0]);
    }

    if (havewebserver && servaddrin_config)
    {
  /*
   * CAN'T HAVE BOTH -w and -W
   */
  fprintf(stderr, "Can't have both -w and -W options\n");
  usage(argv[0]);
    }

    network_mask = inet_addr(network_mask_str);
    if (network_mask == INADDR_NONE) {
  fprintf(stderr, "Invalid network mask (-M %s)\n", network_mask_str);
  usage(argv[0]);
    }

    if(strlen(configfile) == 0)
    {
        /*
         * THE MASTER MUST HAVE A CONFIGURATION FILE TO READ.
         */
        fprintf(stderr,"No Configuration file specified\n");
        usage(argv[0]);
    }
    /* IF WE DO NOT HAVE A FILE LIST THEN THERE ARE UIL'S AT THE END OF THE
     * COMMAND LINE SO GRAB THEM.
     */
    if (uil_filelist_f == 0)
    {
  currarg = optind;
  number_of_pages = 0;
  while(currarg != argc)
  {
     /*
      * GET THE UILS TO RETRIEVE.
      */

      sscanf(argv[currarg],"%s",filelist[number_of_pages]);
      number_of_pages++;
      currarg++;
  }
    }
    else
    {
  /* have filelist; take a stab at the number of valid URLs */
  D_PRINTF( "About to parse filelist %s\n", uil_filelist );
  number_of_pages = count_file_list(uil_filelist);
    }
    if (number_of_pages == 0)
    {
       /*
  * AT LEAST ONE FILE MUST BE SPECIFIED
  */
  fprintf(stderr,"No URL resources specified\n");
  usage(argv[0]);
    }
}

/*
  This function sets up the socket we will use to synchronize with the
  clients.
  Returns the socket number if successful, doesn't return if it fails
*/

SOCKET SetupSyncSocket( serveraddr )
struct sockaddr_in *serveraddr;
{
  int sock,len;

    /*
     * SET UP THE SOCKET WE ARE GOING TO USE TO SYNCHRONIZE WITH THE CLIENTS.
     */
    D_PRINTF( "About to call sock %d %d\n", portnum, MAXCLIENTS );

    sock = passivesock(0, "tcp", MAXCLIENTS);

    if (BADSOCKET(sock))
    {
        errexit("Couldn't open socket %d: %s\n", sock, neterrstr());
    }
  D_PRINTF( "The passivesock call succeeded\n" );

    D_PRINTF( "calling getsockname\n" );

    len = sizeof(struct sockaddr);
    if(getsockname(sock, (struct sockaddr *)serveraddr, &len) < 0)
    {
  errexit("Could not get socket informaton\n");
    }

  return( sock );
}

/*
  Function which generates a commandline for the webclients
*/
void MakeCmdLine( commandline)
char *commandline;
{
    char tmpcommandline[NCCARGS];
    char hostname[MAXHOSTNAMELEN];
    char *webclient_path, *temp;
    int  cnt;
    struct hostent  *master_phe_tmp; /* temp. variable added by - Rajesh Shah*/

    /*
     * BUILD THE PORTIONS OF THE cmdline FOR EACH CLIENT THAT WE CAN BUILD NOW.
     * WE WILL FILL IN THE NUMBER OF CLIENTS LATER WITH AN sprintf.
     */
    D_PRINTF( "Calling gethostname\n" );

    if(gethostname(hostname,MAXHOSTNAMELEN) != 0)
    {
  errexit("Could not retrieve local host name");
    } else {
  /* convert hostname to address (to avoid DNS problems for webclients) */
  /* The following lines are add to copy the system
     buffer (output of gethostbyname()) into user area.
           This is because, there are very good chances that later
     on system buffer might be overwritten by some calls and
     still if your pointer is pointing to same addr. nothing
     but only trouble and trouble! Infact this is what
           happening when I tried to run webstone benchmark for more
           then one clients. It used to over write the webmaster name
     with the first client name and so remaining on client(s)
     the webclient process(es) were invoked with wrong webmaster
     name! This behaviour is observed Solaris 2.4 this bug
     can be hit in any OS.    - Rajesh Shah 5/18/96 */

  /* master_phe = gethostbyname(hostname); */
  master_phe_tmp = gethostbyname(hostname);
  master_phe = (struct hostent *)malloc(sizeof(struct hostent));
  HostEntCpy(master_phe, master_phe_tmp);
    }

    /* set up executable pathname */
#ifndef WIN32
    temp = getenv("TMPDIR");

    if ( temp && *temp ) {
      webclient_path = (char *)mymalloc( strlen(temp) + strlen("/webclient")
              + 1);
      strcpy(webclient_path, temp);
      strcat(webclient_path, "/webclient");

    } else
#else
    temp = temp;
#endif /* WIN32 */
      webclient_path = PROGPATH;


    D_PRINTF( "Path to webclient is: %s\n", webclient_path );

    sprintf(commandline,"%s", webclient_path);

    if(haveproxyserver)
    {
        sprintf(tmpcommandline, " -P %s", proxyserver);
        strcat(commandline, tmpcommandline);
    }
    if (debug)
    {
       strcat(commandline," -d");
    }
    if (numloops != 0)
    {
        sprintf(tmpcommandline," -l %d", numloops);
        strcat(commandline,tmpcommandline);
    }
    if (portnum)
    {
        sprintf(tmpcommandline," -p %d", portnum);
        strcat(commandline,tmpcommandline);
    }
    if (redirect)
    {
        strcat(commandline," -r");
    }
    if (savefile)
    {
        strcat(commandline," -s");
    }
    if (uil_filelist_f)
    {
        strcat(commandline," -u ");
  strcat(commandline,uil_filelist);
    }
    if (record_all_transactions)
    {
  strcat(commandline," -R");
    }
    if (testtime != 0)
    {
        sprintf(tmpcommandline," -t %d", testtime);
        strcat(commandline,tmpcommandline);
    }

    /*
     * SET UP A SPACE FOR THE NUMBER OF CLIENTS ON THE commandline.
     */
    sprintf(tmpcommandline,"%s -n %%d -w %%s -c %%s:%%d", commandline);
    strcpy(commandline,tmpcommandline);

    if (uil_filelist_f == 0)
    {
  cnt = 0;
  while(cnt < number_of_pages)
  {
     /*
      * PUT THE FILES AT THE END OF THE LIST.
      */
      strcat(commandline," ");
      strcat(commandline,filelist[cnt]);
      cnt++;
  }
    }
    puts(commandline);
}

/*
  rexec to the client hosts and start the webclients
*/
int RexecClients( commandline, clienthostname, serveraddr)
char *commandline;
char  clienthostname[MAXCLIENTS][MAXHOSTNAMELEN];
struct sockaddr_in  *serveraddr;

{
  int   tmpfd;
  int   numclients = 0;
  char    tmpcommandline[NCCARGS];
  struct servent *inetport;
  int   cnt;
  char    buffer[NCCARGS];
  char    login[MAXUSERNAME];
  char    password[MAXPASSWD];
  FILE    *fp;
  int   returnval;
  char    *tmphostname;

  memset(buffer, 0, sizeof(buffer));

    /*
     * OPEN UP THE CONFIG FILE. FOR EACH LINE IN THE CONFIG FILE, CHECK
     * ITS VALIDITY AND THEN rexec A COMMAND ON THE CLIENT.
     */

    if ((fp = fopen(configfile,"r")) == 0)
    {
        errexit("Could not open config file %s\n", configfile);
    }

    if ((inetport = getservbyname("exec","tcp")) == 0)
    {
        errexit("Could not get service name for exec/tcp\n");
    }
    D_PRINTF( "getservbyname returned %d\n", ntohs(inetport->s_port) );

    cnt = 0;

    D_PRINTF( "rexec loop\n" );
    while(1)
    {
  char webserver2[MAXHOSTNAMELEN];
  char linebuf[150];
  int num;
  char *primename;

  if (0 == fgets(linebuf, sizeof(linebuf), fp))
      break;
  num = sscanf(linebuf,"%s %s %s %d %s",clienthostname[cnt],login,password,
              &numclients, webserver2);
  if (num < 4)
      break;
  if (servaddrin_config) {
      if (num == 4) {
    errexit("No webserver specified in config file for %s\n", clienthostname[cnt]);
      }
      strcpy(webserver, webserver2);
  }

  if (numclients <= 0)
      errexit("Number of clients must be >= 0\n");
  if (numclients > MAXPROCSPERNODE)
  {
      errexit("Number of clients per node can't exceed %d\n", MAXPROCSPERNODE);
  }
        totalnumclients += numclients;

  primename = pick_webmaster_IP_address(clienthostname[cnt], master_phe, network_mask);
  if (primename == 0) {
      errexit("Bad client address %s for Client %d\n", clienthostname[cnt], cnt);
  }

        fprintf(stdout,"Client %d: %s \t# Processes: %d\n    Webserver: %s\tWebmaster: %s:%d\n",
    cnt, clienthostname[cnt], numclients, webserver, primename,
    ntohs(serveraddr->sin_port));
  fflush(stdout);
        sprintf(tmpcommandline, commandline, numclients, webserver, primename,
      ntohs(serveraddr->sin_port));

        fprintf(stderr, "tmpcommandline: %s\n", tmpcommandline);

  D_PRINTF( "%s rexec %s\n",&clienthostname[cnt],tmpcommandline );
  if (norexec) {
      sleep(30);  /* gives some time to start clients for debugging */
  } else {

      tmphostname = &(clienthostname[cnt][0]);
      tmpfd = rexec(&tmphostname, inetport->s_port, login, password,
            tmpcommandline, &sockErr[cnt]);
      if((sockIO[cnt] = tmpfd) < 0)
      {
    errexit("Could not rexec: rexec to client %s, cmdline %s failed\n",
      clienthostname[cnt],tmpcommandline);
      }
  }


  returnval = NETREAD(tmpfd, buffer, OKSTRLEN);
  D_PRINTF( "read returns %d, %s\n", returnval, buffer );

  if (returnval <= 0 || memcmp(buffer, OKSTR, OKSTRLEN) != 0)
  {
          errexit("rexec to client %s, cmdline %s received error %s\n",
      clienthostname[cnt],tmpcommandline, buffer);
  }


  cnt++;
  if (cnt > MAXCLIENTS || cnt > FD_SETSIZE)
  {
      errexit("Number of Clients can't exceed %d\n", MAXCLIENTS);
  }
    }

    num_rexecs = cnt;
    if (totalnumclients > MAXTOTALPROCS)
    {
        errexit("Total number of processes can't exceed  %d\n",
    MAXTOTALPROCS);
    }

#ifndef WIN32
      /* NOW WE NEED TO HANDLE THE OUTPUT FROM THE REXEC.
       * TO DO THIS, WE FORK, THEN HAVE ONE PROCESS READ FROM TMPFD.
       * THE OTHER PROCESS CONTINUES WITH THE PROGRAM
       */
      D_PRINTF( "Forking webclient stderr/stdout processes\n" );
      switch (fork())
  {
  case -1:   /* ERROR */
    errexit("fork: %s\n", strerror(errno));
  case 0:    /* CHILD */
    exit(echo_client(clienthostname[cnt], tmpfd));
  default:   /* PARENT */
    break;
  }
#else
    /* start threads to echo stdout/stderr from clients */
    _beginthread(echo_client, 0, (void *)0);
    _beginthread(echo_client, 0, (void *)1);
#endif /* WIN32 */

    fprintf(stdout,"\n");
    fprintf(stdout,"\n");
    fclose(fp);

    return totalnumclients;
}

void GetReady( fdset, totalnumclients, sock )
fd_set *fdset;
int totalnumclients;
int sock;
{
  int cnt,len;
  fd_set tmpfdset, leftfdset;
  char  buffer[NCCARGS];

    /*
     * NOW WE NEED TO ACCEPT ALL THE CONNECTIONS FROM THE CLIENTS,
     * ACCEPT ALL THE READY MESSAGES
     */

    D_PRINTF( "Beginning accept loop\n" );
    for (cnt = 0; cnt < totalnumclients; cnt++)
    {
  D_PRINTF( "Client %d:\t", cnt );

  {
      fd_set readfds;
      struct timeval timeout;
      int rv;

      timeout.tv_sec = MAX_ACCEPT_SECS;
      timeout.tv_usec = 0;
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);

      /* if we're hung, quit */
      D_PRINTF("Before select() on listen() socket\n");
      if (!(rv = select(FD_SETSIZE, &readfds, 0, 0, &timeout)))  {
    fprintf(stdout,
        "Listen timeout after %d seconds (%d clients so far)\n",
        MAX_ACCEPT_SECS, cnt);
    D_PRINTF("select() timed out after %d seconds\n", MAX_ACCEPT_SECS);
    errexit("Webmaster terminating\n");
      }
  }

        if(BADSOCKET(socknum[cnt] = accept(sock, 0, 0)))
        {
            /*
             * ERROR accepting FROM THE CLIENTS. WE NEED TO ISSUE AN
             * ABORT TO ALL.
             */
            abort_clients();
            errexit("Error accepting from one of the clients: %s", neterrstr());
        } else
        {
            /*
             * SET THE FD IN THE MASK
             */
            FD_SET(socknum[cnt],fdset);
        }
  D_PRINTF( "on socket %d\n",socknum[cnt] );
    }
    D_PRINTF( "\n" );

    /*
     * WAIT FOR A READY.
     */
    sleep(1);
    fprintf(stdout,"Waiting for READY from %d clients\n",totalnumclients);
    fflush(stdout);
    leftfdset = *fdset;
#ifndef WIN32
    while(memcmp(&leftfdset,&zerofdset,sizeof(fd_set)))
    {
        tmpfdset = leftfdset;

        if(select(FD_SETSIZE,&tmpfdset,NULL,NULL,NULL) < 0)
        {
            /*
             * ERROR SELECTING. ABORT ALL.
             */
            abort_clients();
            errexit("Error accepting from one of the clients: %s\n",
      neterrstr());
            break;
        }
#else
    /* I don't see why a select is needed at all--all clients must respond
     * and there is no synchronization/timing issue.
     */
    tmpfdset = leftfdset;
    {
#endif /* WIN32 */

        for (cnt = 0; cnt < totalnumclients; cnt++)
        {
            /*
             * SEE WHICH SOCKETS HAVE A INPUT ON THEM PENDING
             * AND RECEIVE IT.
             */
            if(!BADSOCKET(socknum[cnt]) && (FD_ISSET(socknum[cnt],&tmpfdset)))
            {
                /*
                 * GET THE READY FROM THIS GUY.
                 * DON'T FORGET TO CLEAR HIS BIT IN THE tmpfdset
                 */
    len = NETREAD(socknum[cnt],buffer,READYSTRLEN);
                if(len != READYSTRLEN)
                {
                     abort_clients();
                     errexit("Error reading from client #%d\n", cnt);
                }
                if(memcmp(buffer, READYSTR, READYSTRLEN))
          {
                     abort_clients();
                     fprintf(stdout,"Received bad READY string: len %d, value %s\n",
            len,buffer);
                }
                FD_CLR(socknum[cnt],&leftfdset);
            }
        }
    }
    sleep(1);
    fprintf(stdout,"All READYs received\n");
    fflush(stdout);
}

/*
  Start all the clients by sending them a GO signal
  totalnumclients is the total number of clients
  socknum is an int array with the filedescriptors for all the
     client connections
*/
void SendGo( totalnumclients, socknum)
int totalnumclients;
int *socknum;
{
  int cnt;
  fprintf(stdout,"Sending GO to all clients\n");
  for(cnt = 0; cnt < totalnumclients; cnt++)
    {
      if(socknum[cnt] > 0)
        {
    /*
     * SEND A GO
     */
    if(NETWRITE(socknum[cnt], GOSTR, GOSTRLEN) != GOSTRLEN)
            {
        abort_clients();
        errexit("Error sending GO to client %d: %s\n", cnt, neterrstr());
            }
        }
    }
}

/*
  This function gathers statistics from all the clients
*/

void GetResults(fdset, page_stats, endtime, timestr, totalnumclients,
    statarray)
fd_set *fdset;
page_stats_t **page_stats;
time_t *endtime;
char *timestr;
int totalnumclients;
stats_t statarray[MAXCLIENTS];
{
  fd_set leftfdset,tmpfdset;
  char *stats_as_text;
  char *page_stats_as_text;
  int returnval;
  int cnt,i;


  /* DOESN'T ACTUALLY PRINT UNTIL THE FIRST CLIENT REPORTS */
  fprintf(stdout,"Reading results ");

  /*
   * COPY THE FILE DESCRIPTORS TO A TMP LIST,
   * ALLOCATE MEMORY FOR STATS, PAGESTATS IN TEXT FORM
   */
  leftfdset = *fdset;
  stats_as_text = (char *)mymalloc(SIZEOF_STATSTEXT+1);
  page_stats_as_text = (char *)mymalloc(SIZEOF_PAGESTATSTEXT+1);

    /*
    * COPY THE FILE DESCRIPTORS TO A TMP LIST,
    * PLUS A LIST OF REMAINING FDs
    */
    leftfdset = *fdset;
    /*
     * LOOP UNTIL ALL CLIENTS HAVE REPORTED
     * AND tmpfdset IS EMPTY
     */
#ifndef WIN32
    while(memcmp(&leftfdset,&zerofdset,sizeof(fd_set)))
    {
  tmpfdset = leftfdset;
  sleep(1);
  returnval = select(FD_SETSIZE,&tmpfdset,NULL,NULL,NULL);
  D_PRINTF( "Call to select returned %d, errno %d\n",
         returnval, errno );
        if(returnval < 0)
        {
            /*
             * ERROR SELECTING. ABORT ALL.
             */
    D_PRINTF( "select() error %s\n", neterrstr() );
            abort_clients();
            errexit("Error selecting from one of the clients\n");
        }
#else
    /* I don't see why a select is needed at all */
    tmpfdset = leftfdset;
    {
#endif /* WIN32 */
  for(cnt = 0; cnt < totalnumclients; cnt++)
  {
      /*
       * SEE WHICH SOCKETS HAVE A INPUT ON THEM PENDING AND
       * RECEIVE IT.
       */

      /* IS THIS A VALID SOCKET? IS IT READY TO READ? */
      if(!BADSOCKET(socknum[cnt]) && (FD_ISSET(socknum[cnt],&tmpfdset)))
      {
    int len;

    /*
     * GET THE TIMING DATA FROM THIS GUY
     * THEN REMOVE HIM FROM THE tmpfdset
     */
      /*
       * READ TIME STATS
       * DOES READ() RETURN THE CORRECT LENGTH?
       */
      D_PRINTF( "About to read timestats, count %d, errno %d\n",
      cnt, errno );
      len = SIZEOF_STATSTEXTBASE + number_of_pages*SIZEOF_DOUBLETEXT;
      returnval = recvdata(socknum[cnt], stats_as_text,
            len);
      D_PRINTF( "Read time stats %d\n", returnval );
      if (returnval != len) /* <= 0) */
        {
          D_PRINTF( "Error reading timing stats: %s\n",
          neterrstr() );
          fprintf(stderr, "Error reading timing stats: %s\nSocket number %d\n",
            neterrstr(),socknum[cnt]);
          abort_clients();
          errexit("");
        } /* end if */

      /* convert text to stats */
      stats_as_text[returnval] = 0; /* add an end marker */
      statarray[cnt] = *text_to_stats(stats_as_text);

    fputc('.', stdout); /* PROGRESS MARKER */
    fflush(stdout);

    if(uil_filelist_f) /* READ PAGE STATS */
    {
      for (i = 0; i < number_of_pages; i++)
        {
          D_PRINTF( "On page_stats[%d][%d]\n", cnt, i );
          returnval = recvdata(socknum[cnt], page_stats_as_text,
          SIZEOF_PAGESTATSTEXT);
          D_PRINTF( "Read page stats %d\n", returnval );

          if (returnval != SIZEOF_PAGESTATSTEXT) /* <= 0) */
      {
        D_PRINTF( "Error reading page_stats[%d][%d]: %s\n",
          cnt, i, neterrstr() );
        fprintf(stderr, "Error reading page_stats[%d][%d]: %s\n",
          cnt, i, neterrstr());
        abort_clients();
        errexit("");
      }
          D_PRINTF( "Page stats: read %d bytes\n",
        returnval );

          page_stats_as_text[returnval] = 0;    /* add an end marker */
          D_PRINTF("strlen(page_stats_as_text) = %d\n",
          strlen(page_stats_as_text));
          page_stats[cnt][i] =
      *text_to_page_stats(page_stats_as_text);

        } /* end for */
    } /* end if filelist */

    FD_CLR(socknum[cnt],&leftfdset);
    NETCLOSE(socknum[cnt]);
    socknum[cnt] = BADSOCKET_VALUE;
      } /* end if socknum */
  } /* end for cnt */
    } /* end while memcmp fd */

    /*
     * DONE READING RESULTS FROM CLIENTS
     */

    *endtime = time(0);
    timestr = asctime(localtime(endtime));
    fprintf(stdout,"\nAll clients ended at %s\n",timestr);
    fflush(stdout);

    /* FREE MEMORY ALLOCATED FOR CLIENT STATS, PAGESTATS AS TEXT */
    free(stats_as_text);
    free(page_stats_as_text);

}

/*
  Prints out all the results
*/
void PrintResults( page_stats, endtime, timestr, totalnumclients, statarray,
      page_stats_total)
page_stats_t **page_stats;
time_t endtime;
char *timestr;
int totalnumclients;
stats_t statarray[MAXCLIENTS];
page_stats_t *page_stats_total;
{
  stats_t masterstat;
  int cnt,i,j;
  double  thruput;
  struct timeval  dtime;

    /*
     * PRINT EVERYTHING OUT
     */
    stats_init(&masterstat);
    for(cnt = 0; cnt < totalnumclients; cnt++)
    {
        if((statarray[cnt].rs.totalconnects > 0) && (dumpall))
        {
            fprintf(stdout,"----------------------------------\n");
            /* fprintf(stdout,"Test for host: %s\n",statarray[cnt].hostname); */
            fprintf(stdout,"Total number of pages retrieved from server: %u\n",
        statarray[cnt].totalpages);

      rqstat_fprint(stdout, &(statarray[cnt].rs));

          thruput = thruputpersec((double)(statarray[cnt].rs.totalbytes),
      &(statarray[cnt].rs.totalresponsetime));

          fprintf(stdout, "Thruput average per connection: %.0f bytes/sec\n",
      thruput);
        }
        if(statarray[cnt].rs.totalconnects > 0)
        {
      D_PRINTF( "Summing stats for %d, with %ld total connections\n",
    cnt, statarray[cnt].rs.totalconnects );
            rqstat_sum(&masterstat.rs, &(statarray[cnt].rs));
        }
  else
  {
      masterstat.rs.totalerrs += statarray[cnt].rs.totalerrs;
  }
    }

    for (i=0; i < totalnumclients; i++)
    {
  for (j=0; j < number_of_pages; j++)
  {
      D_PRINTF( "Summing page stats for %d, page %d, with %d connects\n",
        i, j, statarray[i].page_numbers[j] );

      if (statarray[i].page_numbers[j] != 0)
      {
    rqst_stats_t *pst_rs;
    rqst_stats_t *ps_rs;

    pst_rs = &(page_stats_total[j].rs);
    ps_rs =  &(page_stats[i][j].rs);

    rqstat_sum(pst_rs, ps_rs);

    page_stats_total[j].totalpages += page_stats[i][j].totalpages;
    masterstat.totalpages += page_stats[i][j].totalpages;

    /* yes, this is assignment, not sum */
    page_stats_total[j].page_size = page_stats[i][j].page_size;

    page_stats_total[j].page_valid = 1;
      }
  }
    }

    /* print page statistics */
    if (verbose) {
  for (i = 0; i < number_of_pages; i++)
  {
      if (page_stats_total[i].page_valid == 1)
      {
    page_stats_t *pst;

    pst = &(page_stats_total[i]);

    printf ("===============================================================================\n");
    printf ("Page # %d\n\n", i);
    printf ("Total number of times page was hit %u\n",
          pst->totalpages);

    rqstat_print(&(pst->rs));

    printf ("Page size %u \n", pst->page_size);
    printf ("===============================================================================\n\n");
      }
  }
    }

    fprintf(stdout,"===============================================================================\n");

    /*
     * Validate run.
     */
    masterstat.total_num_of_files = statarray[0].total_num_of_files;
    for (i=1; i < totalnumclients; i++)
    {
        if ((statarray[i].rs.totalconnects > 0) &&
      (statarray[i].total_num_of_files != masterstat.total_num_of_files))
  {
      fprintf(stdout,"**********************************************************************\n");
      fprintf(stdout,"**** ERROR: number of files in each test configuration is not the same\n");
      fprintf(stdout,"**** ERROR: Check configuration file %s on each client\n", configfile);
      fprintf(stdout,"**********************************************************************\n");
      break;
  }
    }


    /*
     * Print summary statistics
     */
    fprintf(stdout, "WEBSTONE 2.0 results:\n");

    fprintf(stdout, "Total number of clients: \t%d\n", totalnumclients);
    testtime = sumedh_end.tv_sec - sumedh_start.tv_sec;
    fprintf(stdout,"Test time: \t\t\t%d seconds\n", testtime);

    fprintf(stdout, "Server connection rate: \t%3.2f connections/sec\n",
            (double)(masterstat.rs.totalconnects)/(testtime));

    fprintf(stdout, "Server error rate: \t\t%4.4f err/sec\n",
            (double)(masterstat.rs.totalerrs)/(testtime));

    fprintf(stdout, "Server thruput: \t\t%2.2f Mbit/sec\n",
            (double)(8*masterstat.rs.totalbytes)/(testtime*1024*1024));

    fprintf(stdout, "Little's Load Factor: \t\t%3.2f \n",
            (double)(masterstat.rs.totalresponsetime.tv_sec)
            /(testtime));
    avgtime(&masterstat.rs.totalresponsetime,
          masterstat.rs.totalconnects, &dtime);

    fprintf(stdout, "Average response time: \t\t%4.4f millisec\n",
  (double)1000*(dtime.tv_sec + (double)dtime.tv_usec / 1000000));

    fprintf(stdout, "Error Level:\t\t\t%4.4f %%\n",
      (double)(100 * masterstat.rs.totalerrs)/(masterstat.rs.totalconnects));

    /* so much for the key metrics */

    thruput = 8 * thruputpersec((double)(masterstat.rs.totalbytes),
      &(masterstat.rs.totalresponsetime));

    fprintf(stdout, "Average client thruput: \t%4.4f Mbit/sec\n",
      thruput/(1024*1024));

    fprintf(stdout,"Sum of client response times:\t%u.%u sec\n",
      masterstat.rs.totalresponsetime.tv_sec,
      masterstat.rs.totalresponsetime.tv_usec);

    fprintf(stdout,"Total number of pages read:\t%u\n\n",
        masterstat.totalpages);

    /* Remaining stats are the same as usual */

    rqstat_fprint(stdout, &(masterstat.rs));
    fflush(stdout);

}

#ifdef WIN32
/* close socket library */
void sock_cleanup(void) {

    WSACleanup();
}
#endif /* WIN32 */

void
main(const int argc, char *argv[])
{

    int   sync_sock;
    int   i;
    int   j;
    char  buffer[NCCARGS];
    char  commandline[NCCARGS];
    char  *timestr;
    time_t  starttime;
    time_t  endtime;
    fd_set  fdset;
    /* make the big arrays static to avoid stack overflow */
    static char   clienthostname[MAXCLIENTS][MAXHOSTNAMELEN];
    static stats_t  statarray[MAXCLIENTS];
    page_stats_t **page_stats;
    page_stats_t *page_stats_total;
    struct sockaddr_in  serveraddr;


#ifdef WIN32
    WSADATA WSAData;
    COORD dwSize;

    if ((WSAStartup(MAKEWORD(1,1), &WSAData)) != 0) {
  errexit("Error in WSAStartup()\n");
    }
    atexit(sock_cleanup);

    /* increase size of output window */
    dwSize.X = 80;
    dwSize.Y = 500;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), dwSize);
#endif /* WIN32 */


    /* Initalization of variables. */
    debugfile = stdout;
    memset(buffer, 0, NCCARGS);
    memset(webserver, 0, MAXHOSTNAMELEN);
    memset(configfile, 0, MAXPATHLEN);
    FD_ZERO(&zerofdset);
    FD_ZERO(&fdset);

    for(i = 0; i < MAXCLIENTS; i++)
    {
        socknum[i] = BADSOCKET_VALUE;
        statarray[i].rs.totalconnects = 0;
    }

    signal(SIGINT, sig_int);

    ParseCmdLine( argc, argv);

    sync_sock = SetupSyncSocket( &serveraddr );

    MakeCmdLine( commandline);

    totalnumclients = RexecClients( commandline, clienthostname, &serveraddr);

    /* Initalization of variables. */
    page_stats =
      (page_stats_t **)
      mymalloc(totalnumclients*sizeof(page_stats_t *));
    for (i=0; i < totalnumclients; i++)
      {
  page_stats[i] = (page_stats_t *)
    mymalloc(number_of_pages*sizeof(page_stats_t));
      }
    page_stats_total =
      (page_stats_t *)mymalloc(number_of_pages*sizeof(page_stats_t));

    for (i=0; i < totalnumclients; i++) {
        stats_init(&(statarray[i]));
    }
    for (i=0; i < totalnumclients; i++) {
        for (j=0; j < number_of_pages; j++) {
            page_stats_init(&(page_stats[i][j]));
  }
    }
    for (i=0; i < number_of_pages; i++) {
        page_stats_init(&(page_stats_total[i]));
    }

    for(i = 0; i < totalnumclients; i++)
    {
        socknum[i] = BADSOCKET_VALUE;
        statarray[i].rs.totalconnects = 0;
    }

    GetReady( &fdset, totalnumclients, sync_sock );
    NETCLOSE(sync_sock);

    /*
     * START ALL OF THE CLIENTS BY SENDING THEM A GO SIGNAL.
     */


    gettimeofday (&sumedh_start, 0);
    SendGo( totalnumclients, socknum);

    /*
     * WAIT FOR ALL OF THE CLIENTS TO COMPLETE.  WE SHOULD GET A REPLY
     * FOR EACH SOCKET WE HAVE OPEN.  THE REPLY WILL BE THE TIMING
     * INFORMATION WE USE.
     */

    starttime = time(0);
    timestr = asctime(localtime(&starttime));
    fprintf(stdout,"All clients started at %s\n",timestr);
    fprintf(stdout,"Waiting for clients completion\n");
    fflush(stdout);

    /* IF THIS IS A TIMED TEST, WE MIGHT AS WELL SNOOZE */
    if (testtime) {
      sleep(testtime * 60);
    }

    GetResults( &fdset, page_stats, &endtime, timestr, totalnumclients,
         statarray);

    gettimeofday (&sumedh_end, 0);
    PrintResults( page_stats, endtime, timestr, totalnumclients, statarray,
     page_stats_total);

    /* free memory */
    for (i = 0; i < totalnumclients; i++)
      {
  free(page_stats[i]);
      }
    free(page_stats);
    free(page_stats_total);

    exit(0);
}

/* Added by Rajesh Shah 5/18/96 */
void
HostEntCpy(struct hostent *dest, struct hostent *src)
{

  dest->h_name = (char *)malloc(strlen(src->h_name)+1);
  strcpy(dest->h_name, src->h_name);
  printf("WebMaster name = %s\n", dest->h_name);
  dest->h_aliases = src->h_aliases;
  dest->h_addrtype = src->h_addrtype;
  dest->h_length = src->h_length;
  dest->h_addr_list = src->h_addr_list;
}
