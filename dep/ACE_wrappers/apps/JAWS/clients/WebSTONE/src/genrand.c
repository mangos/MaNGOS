/* $Id: genrand.c 91813 2010-09-17 07:52:52Z johnnyw $ */
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
/* FUZZ: disable check_for_improper_main_declaration */

#include <stdio.h>
#include <fcntl.h>
#include <math.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sysdep.h"

void
main(const int argc, char* argv[])
{
  FILE* file;
  int i;
  int my_random;
  int size;
  char *cp;

  if (argc != 3)
    {
      printf("usage: %s file_size_in_bytes[K|M] name\n", argv[0]);
      exit(2);
    }

  if ((file = fopen(argv[2], "w")) == 0)
    {
      perror("fopen");
      exit(1);
    }

  size = atoi(argv[1]);

  for (cp = argv[1]; *cp; cp++)
  {
    switch(*cp)
      {
        case 'k':
        case 'K':
          size *= 1024;
          break;
        case 'm':
        case 'M':
          size *= 1024*1024;
          break;
      }
  }

  for (i = 0; i < size; i++)
    {
      my_random = ((RANDOM() % 94) + 33);
      fputc((char)my_random, file);
    }

  fclose(file);
}
