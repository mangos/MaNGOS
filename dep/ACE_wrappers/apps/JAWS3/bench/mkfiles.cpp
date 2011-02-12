// $Id: mkfiles.cpp 91730 2010-09-13 09:31:11Z johnnyw $

#include "ace/Get_Opt.h"
//FUZZ: disable check_for_math_include/
#include <math.h>

static float gammln (float xx);
static float poidev (float xm);

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt options (argc, argv, ACE_TEXT("m:s:x:n:"));
  // m -- median file size in kB
  // x -- maximum file size in kB
  // n -- number of files

  long median = 8;
  long maximum = 1024;
  long number = 1000;

  int c;
  while ((c = options ()) != -1)
    {
      switch (c)
        {
        case 'm':
          median = ACE_OS::atoi (options.optarg);
          break;
        case 'x':
          maximum = ACE_OS::atoi (options.optarg);
          break;
        case 'n':
          number = ACE_OS::atoi (options.optarg);
          break;
        default:
          break;
        }
    }

  char filename[1024];
  const char *seventyfive_bytes = "\
01010101010101010101010101010101010101010101010101010101010101010101010101\n\
";

  int seen_max = 0;

  long i;
  for (i = 0; i < number; i++)
    {
      long size = 0;
      float p = ACE_OS::floor (::pow (2, poidev (::log (2 * median)/::log (2)) - 1));
      if (p > maximum)
        p = maximum;
      p *= 1024;
      if (p < 1.0)
        p = 1.0;
      size = (long) p;
      if (i == (number - 1))
        if (! seen_max)
          size = maximum * 1024;
      else
        seen_max = (size == (maximum * 1024));

      ACE_OS::sprintf (filename, "file%011ld.html", i);
      FILE *fp = ACE_OS::fopen (filename, "w+b");
      while (size > 75)
        {
          ACE_OS::fprintf (fp, "%s", seventyfive_bytes);
          size -= 75;
        }
      if (size > 15)
        {
          ACE_OS::fprintf (fp, "%0*.0f\n", (int) (size - 1), p);
        }
      else
        {
          ACE_OS::fprintf (fp, "%015.0f\n", p + 16 - size);
        }
        ACE_OS::fclose (fp);
    }

  return 0;
}

static float
gammln (float xx)
{
  double x, y, tmp, ser;
  static const double cof[6] = { 76.18009172947146,
                                 -86.50532032941677,
                                 24.01409824083091,
                                 -1.231739572450155,
                                 0.1208650973866179e-2,
                                 -0.5395239384953e-5 };
  int j;

  y = x = xx;
  tmp = x + 5.5;
  tmp -= (x+0.5) * ::log (tmp);

  ser = 1.000000000190015;
  for (j = 0; j < 6; j++)
    ser += cof[j]/++y;

  return -tmp + ::log (2.5066282746310005 * ser / x);
}

static float
poidev (float xm)
{
  static const double PI = 3.141592654;
  static float sq, alxm, g, oldm = -1.0;
  float em, t, y, fem;

  if (xm < 2.0)
    {
      if (xm != oldm)
        {
          oldm = xm;
          g = ::exp (-xm);
        }
      em = -1.0;
      t = 1.0;
      do
        {
          ++em;
          t *= (1.0 + ACE_OS::rand ())/RAND_MAX;
        }
      while (t > g);
    }
  else
    {
      if (xm != oldm)
        {
          oldm = xm;
          sq = ::sqrt (2.0 + xm);
          alxm = log (xm);
          g = xm * alxm - gammln (xm + 1.0);
        }
      do
        {
          do
            {
              y = ::tan (PI * (1.0 + ::rand ())/RAND_MAX);
              em = sq * y + xm;
            }
          while (em < 0.0);
          fem = ACE_OS::floor (em);
          t = 0.9 * (1.0 + y * y) * ::exp (fem * alxm - gammln (fem + 1.0) - g);
        }
      while ((1.0 + ACE_OS::rand ())/RAND_MAX > t);
    }

  return em;
}
