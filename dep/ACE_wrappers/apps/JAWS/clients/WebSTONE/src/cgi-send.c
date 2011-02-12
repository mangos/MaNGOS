/* $Id: cgi-send.c 81993 2008-06-16 20:26:16Z sowayaa $ */
/*
 * Send 10K file; send random bits.
 *
 */

//FUZZ: disable check_for_improper_main_declaration

#include <stdio.h>
#include <stdlib.h>

#define FILE_SIZE       10240
#define MALLOC_FAILURE  "Out of memory"

int
main()
{
  int filesize;
  char *str_filesize;
  char *buffer;
  int index;

  printf("Content-type: text/plain\r\n\r\n");

  if ( !(str_filesize = getenv("QUERY_STRING")) )
    filesize = FILE_SIZE;
  else {
    if ( !strncmp(str_filesize, "size=", 5) )
      filesize = atoi(&(str_filesize[5]));
    else
      filesize = FILE_SIZE;
  }

  if ( !(buffer = (char *)malloc(filesize)) ) {
    fwrite(MALLOC_FAILURE, strlen(MALLOC_FAILURE), 1, stdout);
    return -1;
  }

  for (index=0; index< filesize; index++)
    /* generate random characters from A-Z */
    buffer[index] = rand() %26 + 63;

  fwrite(buffer, filesize, 1, stdout);

  free(buffer);

  return 0;
}
