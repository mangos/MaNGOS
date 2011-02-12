// $Id: average.cpp 91730 2010-09-13 09:31:11Z johnnyw $

int
ACE_TMAIN(int, ACE_TCHAR *[])
{
  double sum = 0;
  double count = 0;
  int input;
  char buf[BUFSIZ];

  while (ACE_OS::fgets (buf, sizeof (buf), stdin) != 0)
    {
      input = ACE_OS::atoi (buf);
      sum += input;
      count++;
    }

  ACE_OS::printf ("sum of input is: %f\n", sum);
  ACE_OS::printf ("number of inputs is: %f\n", count);
  ACE_OS::printf ("average of input is: %f\n", sum / count);

  return 0;
}
