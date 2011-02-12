/* $Id: timefunc.c 81998 2008-06-17 09:31:15Z sma $ */
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
#ifndef WIN32
#include <netdb.h>
#include <sys/time.h>
#include <sys/param.h>
#else
#include <windows.h>
#include <winsock.h>
#include <time.h>
#endif /* WIN32 */
#include <math.h>
#include <stdio.h>
#include "sysdep.h"
#include "bench.h"

double
timevaldouble(struct timeval *tin)
{
    return ((double)tin->tv_sec + ((double)tin->tv_usec / USECINSEC));
}


void
doubletimeval(const double tin, struct timeval *tout)
{
    tout->tv_sec = (long)floor(tin);
    tout->tv_usec = (long)((tin - tout->tv_sec) * USECINSEC );
}


void
addtime(struct timeval *OrigTime, struct timeval *NewTime)
{
    OrigTime->tv_usec += NewTime->tv_usec;
    if(OrigTime->tv_usec >= USECINSEC)
    {
        /*
         * NEED TO CARRY 1.
         */
        OrigTime->tv_sec++;
        OrigTime->tv_usec -= USECINSEC;
    }
    OrigTime->tv_sec += NewTime->tv_sec;
}


void
compdifftime(struct timeval *EndTime, struct timeval *StartTime, struct timeval *DiffTime)
{
    struct timeval endtime = *EndTime;

      if((endtime.tv_usec - StartTime->tv_usec) < 0)
      {
         /*
          * NEED TO BORROW.
          */
          endtime.tv_usec += USECINSEC;
          endtime.tv_sec--;
      }
      DiffTime->tv_usec = endtime.tv_usec - StartTime->tv_usec;
      DiffTime->tv_sec =  endtime.tv_sec  - StartTime->tv_sec;
}


void
mintime(struct timeval *CurrMinTime, struct timeval *CheckMinTime)
{
    if(CheckMinTime->tv_sec < CurrMinTime->tv_sec)
    {
       *CurrMinTime = *CheckMinTime;
       return;
    }
    if(CheckMinTime->tv_sec == CurrMinTime->tv_sec)
    {
        if(CheckMinTime->tv_usec < CurrMinTime->tv_usec)
        {
           *CurrMinTime = *CheckMinTime;
           return;
        }
    }
}


void
maxtime(struct timeval *CurrMaxTime, struct timeval *CheckMaxTime)
{
    if(CheckMaxTime->tv_sec > CurrMaxTime->tv_sec)
    {
       *CurrMaxTime = *CheckMaxTime;
       return;
    }
    if(CheckMaxTime->tv_sec == CurrMaxTime->tv_sec)
    {
        if(CheckMaxTime->tv_usec > CurrMaxTime->tv_usec)
        {
           *CurrMaxTime = *CheckMaxTime;
           return;
        }
    }
}

void
avgtime(struct timeval *TotalTime, int NumTimes, struct timeval *AvgTime)
{
    double meantime;

    meantime = mean(timevaldouble(TotalTime), NumTimes);

    doubletimeval(meantime, AvgTime);
}


void
sqtime(struct timeval *Time, struct timeval *SqTime)
{
    double sec;

    sec = timevaldouble(Time);
    sec *= sec;  /* square */

    doubletimeval(sec, SqTime);
}


void
variancetime(struct timeval *SumTime, double SumSquareTime, int NumTimes, struct timeval *VarianceTime)
{
    double result;

    result = variance(timevaldouble(SumTime), SumSquareTime, NumTimes);

    doubletimeval(result, VarianceTime);
}


void
stddevtime(struct timeval *SumTime, double SumSquareTime,
    int NumTimes, struct timeval *StdDevTime)
{
    double result;

    result = stddev(timevaldouble(SumTime), SumSquareTime, NumTimes);

    doubletimeval(result, StdDevTime);
}

double
thruputpersec(const double n, struct timeval *t)
{
    double tv;

    tv = timevaldouble(t);
    if (tv != 0)
        return n / timevaldouble(t);
    else
        return 0;
}
