/* $Id: parse_file_list.c 91813 2010-09-17 07:52:52Z johnnyw $ */
/**************************************************************************
 *
 *  Copyright (C) 1995 Silicon Graphics, Inc.
 *
 *  These coded instructions, statements, and computer programs were
 *  developed by SGI for public use.  If any changes are made to this code
 *  please try to get the changes back to the author.  Feel free to make
 *  modifications and changes to the code and release it.
 *
 **************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <netdb.h>
#include <sys/param.h>
#endif /* WIN32 */

#include <sys/types.h>

#ifndef WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#else
#include <windows.h>
#include <process.h>
#include <time.h>
#include <winsock.h>
#endif /* WIN32 */

#include <ctype.h>

#include "sysdep.h"
#include "bench.h"


/*
 * count_file_list()
 * given a filename, return a guess at the number of pages
 */
int
count_file_list(const char *url_list_file)
{
  FILE *fp;
  long int num_of_pages;
  char a_line[BUFSIZ];
  char *textvalue;
  int returnval;

  fp = fopen(url_list_file, "r");

  if (fp == 0)
    {
      D_PRINTF("Error %d opening filelist %s: %s\n",
               errno, url_list_file, strerror(errno) );;
      return(returnerr("Error %d opening filelist %s: %s\n",
             errno, url_list_file, strerror(errno)));
    }
  else
    {
      num_of_pages = 0;
      D_PRINTF( "Opened file, about to count\n" );
      /*
       * parse through the file line-by-line
       * strip out comments, but don't check for URL consistency
       */
      while (fgets(a_line, BUFSIZ, fp) != 0)
        {
          textvalue = a_line;
          /* check for comments */
          if (strchr(textvalue, '#') != 0)
            {
              /* throw out anything after any '#' */
              D_PRINTF( "Stripping comment from line: %s\n", textvalue );
              returnval = strcspn(textvalue, "#");
              D_PRINTF( "Found first # at %d\n", returnval );
              if (returnval == 0)
                {
                  textvalue = 0;
                }
            }
          /* is there more? */
          if (textvalue != 0)
            {
              num_of_pages++;
              D_PRINTF( "Found %ld pages\n", num_of_pages );
            }
        }
      return(num_of_pages);
    }
  fclose(fp);
} /* end count_file_list() */

/*
 * parse_file_list()
 * given a filename, a pointer to a page list, and pointers to integers
 * for the number of pages and the number of files, parse its contents.
 */
void
parse_file_list (const char *url_list_file, page_list_t *page_list,
                 long int *num_of_pages, long int *num_of_files)
{
  FILE        *fp;
  int         filenum;
  int         returnval;
  int         loadnum;
  char        a_line[BUFSIZ];
  char        tempbuf[BUFSIZ];
  char        *textvalue;
  int         numfiles = 1, numpages = 0;
  int         maxpages = *num_of_pages;
  page_list_t *pp;

  extern int haveproxyserver;

  fp = fopen(url_list_file, "r");

  if (fp == 0)
    {
      errexit("Error %d opening filelist: %s\n", errno, strerror(errno));
    }
  else
    {
      /*
       * GRAB A LINE. FORMAT IS: URL WEIGHTINGFACTOR
       * EXAMPLE: http://www/file.html 1
       */
      D_PRINTF( "File is open.\n" );
      while(fgets(a_line, BUFSIZ, fp) != 0)
        {
          textvalue = a_line;
          /* check for comments */
          if (strchr(textvalue, '#') != 0)
            {
              /* throw out anything after a '#' */
              D_PRINTF( "Stripping comment from line: %s\n", textvalue );
              returnval = strcspn(textvalue, "#");
              D_PRINTF( "Found first # at %d\n", returnval );
              if (returnval == 0)
                continue;
            }

          if (numpages >= *num_of_pages)
          errexit("Out of space in parse_file_list()\n");

          pp = &page_list[numpages];

          D_PRINTF( "Processing page %ld\n", numpages );
          loadnum = 0;

          if (textvalue != 0)
            {
              /* is there more? */
              /* check for weighting factor */
              D_PRINTF( "Setting page values from: %s\n", textvalue );
              returnval = sscanf(textvalue, "%s%d", tempbuf, &loadnum);
              D_PRINTF("Scan for weighting returns %d, %d\n",
                       returnval, loadnum);
              if (returnval == EOF || loadnum <= 0)
                {
                  pp->load_num = 1;
                }
              else
                {
                  pp->load_num = loadnum;
                }
              D_PRINTF("Setting load=%d for line: %s\n",
                       pp->load_num, textvalue);

              /* placeholder for grouping multiple files onto one page */
              pp->num_of_files = 1;
              filenum = 0;

              textvalue = tempbuf;
              D_PRINTF( "Line is now: %s\n", textvalue );

              /*
              * if we've got a proxy server, we'll assume that the
              * remaining text is a valid URL, and stuff it into
              * page_list[numpages].filename[filenum]
              * Otherwise, we'll have to parse it out.
              */

              if (haveproxyserver)
                {
                  pp->servername[filenum] = 0;
                  pp->port_number[filenum] = 0;
                  strcpy(pp->filename[filenum], textvalue);
                }
              else /* no proxy server, so we have to parse it out... */
                {
                  /* try http://server(:port)/file */
                  D_PRINTF( "Trying http://server(:port)/filename\n" );
                  returnval = sscanf(textvalue, "http://%[^/]%s",
                                     tempbuf,
                                     a_line);
                  /* check server string for :port */
                  if (returnval != 0 && returnval != EOF)
                    {
                      D_PRINTF( "Setting filename %s\n", a_line );
                      strcpy(pp->filename[filenum], a_line);

                      D_PRINTF( "Checking %s for :portnumber\n", tempbuf );
                      returnval = sscanf(tempbuf, "%[^:]:%d", a_line,
                      &pp->port_number[filenum]);

                      if (returnval < 2)
                        {
                          pp->port_number[filenum] = 80;
                        }
                      if (returnval == EOF)
                        {
                          pp->servername[filenum] = 0;
                        }
                      else
                        {
                          D_PRINTF("Port number %d, setting server %s\n",
                                   pp->port_number[filenum], a_line);

                          strcpy(pp->servername[filenum], a_line);
                        }

                      D_PRINTF("Server %s, port number %d\n",
                               pp->servername[filenum],
                               pp->port_number[filenum]);
                    }
                  else /* no good - try straight filename */
                    {
                      pp->port_number[filenum] = 80;
                      D_PRINTF( "Trying filename, returnval=%d\n", returnval );
                      pp->servername[filenum] = 0;
                      D_PRINTF("Server %s, port number %d\n",
                               pp->servername[filenum], pp->port_number[filenum]);
                      returnval = sscanf(textvalue, "%s", a_line);
                      D_PRINTF( "Scan returned filename %s\n", a_line );

                      strcpy(pp->filename[filenum], a_line);
                    } /* end of parsing */
                } /* end if haveproxyserver */

              D_PRINTF("Done parsing line\n");
              D_PRINTF("Got server %s, port %d, file %s, returnval %d\n",
                       pp->servername[filenum],
                       pp->port_number[filenum],
                       pp->filename[filenum],
                       returnval);
            } /* end if textvalue not NULL */

          numpages++;
        } /* end while not EOF */
      if (numpages < 1)
        {
          returnerr("No files are specified by filelist\n");
        }
    } /* end if file ok */
    fclose(fp);
    D_PRINTF("Returning %ld pages and %ld files\n",
             numpages, numfiles );

    *num_of_pages = numpages;
    *num_of_files = numfiles;
}
/* end parse_file_list */

long int
load_percent(page_list_t *page_list, long int number_of_pages)
{
  int i;
  long int index_number = 0;

  for (i = 0; i < number_of_pages; i++)
    {
      index_number += page_list[i].load_num;
    }

  D_PRINTF( "load_percent returning %d\n", (index_number) );
  return(index_number);
}

