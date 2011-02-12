/* $Id: statistics.c 81994 2008-06-16 21:23:17Z sowayaa $ */
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

/* FUZZ: disable check_for_math_include */
#include <math.h>
#include <stdlib.h>
#include "sysdep.h"
#include "bench.h"


double
mean(const double sum, const int n)
{
  if (n)
    {
      return(sum / n);
    }
  else
    {
      return(0);
    }
}


double
variance(const double sum, const double sumofsquares, const int n)
{
    double meanofsum;

    meanofsum = mean(sum, n);

    return (mean(sumofsquares,n) - (meanofsum * meanofsum));
}


double
stddev(const double sum, const double sumofsquares, const int n)
{
    return(sqrt(fabs(variance(sum, sumofsquares, n))));
}
