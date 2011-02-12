// $Id: HTTP_Helpers.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// HTTP_Helpers.cpp -- Helper utilities for both server and client

#include "HTTP_Helpers.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/Guard_T.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_stdio.h"

// = Static initialization.
const char *const
HTTP_Helper::months_[12]=
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char const *HTTP_Helper::alphabet_ = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char * HTTP_Helper::date_string_ = 0;
ACE_SYNCH_MUTEX HTTP_Helper::mutex_;

ACE_SYNCH_MUTEX HTTP_Status_Code::lock_;
int HTTP_Status_Code::instance_ = 0;
const char *HTTP_Status_Code::Reason[HTTP_Status_Code::MAX_STATUS_CODE + 1];

time_t
HTTP_Helper::HTTP_mktime (const char *httpdate)
{
  char *buf;

  ACE_NEW_RETURN (buf, char[ACE_OS::strlen (httpdate) + 1], (time_t) -1);

  // Make spaces in the date be semi-colons so we can parse robustly
  // with sscanf.

  const char *ptr1 = httpdate;
  char *ptr2 = buf;

  do
    {
      if (*ptr1 == ' ')
        *ptr2++ = ';';
      else
        *ptr2++ = *ptr1;
    }
  while (*ptr1++ != '\0');

  // In HTTP/1.0, there are three versions of an HTTP_date.

  // rfc1123-date   = wkday "," SP dd month yyyy SP hh:mm:ss SP "GMT"
  // rfc850-date    = weekday "," SP dd-month-yy SP hh:mm:ss SP "GMT"
  // asctime-date   = wkday SP month dd SP hh:mm:ss SP yyyy

  // static const char rfc1123_date[] = "%3s,;%2d;%3s;%4d;%2d:%2d:%2d;GMT";
  // static const char rfc850_date[]  = "%s,;%2d-%3s-%2d;%2d:%2d:%2d;GMT";
  // static const char asctime_date[] = "%3s;%3s;%2d;%2d:%2d:%2d;%4d";

  // Should also support other versions (such as from NNTP and SMTP)
  // for robustness, but it should be clear how to extend this.

  struct tm tms;
  char month[4];
  char weekday[10];

  if (::sscanf(buf, "%3s,;%2d;%3s;%4d;%2d:%2d:%2d;GMT", // RFC-1123 date format
               weekday,
               &tms.tm_mday,
               month,
               &tms.tm_year,
               &tms.tm_hour,
               &tms.tm_min,
               &tms.tm_sec) == 7)
    ;
  else if (::sscanf(buf, "%s,;%2d-%3s-%2d;%2d:%2d:%2d;GMT", // RFC-850 date format
                    weekday,
                    &tms.tm_mday, month, &tms.tm_year,
                    &tms.tm_hour, &tms.tm_min, &tms.tm_sec) == 7)
    {
      weekday[3] = '\0';
    }
  else if (::sscanf(buf, "%3s;%3s;%2d;%2d:%2d:%2d;%4d", // ASCTIME date format.
                    weekday,
                    month, &tms.tm_mday,
                    &tms.tm_hour, &tms.tm_min, &tms.tm_sec,
                    &tms.tm_year) == 7)
    {
    }

  delete [] buf;

  tms.tm_year = HTTP_Helper::fixyear (tms.tm_year);
  tms.tm_mon = HTTP_Helper::HTTP_month (month);

  if (tms.tm_mon == -1)
    return (time_t) -1;

  // mktime is a Standard C function.
  {

#if !defined (ACE_HAS_REENTRANT_LIBC)
    ACE_MT (ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, g, HTTP_Helper::mutex_, -1));
#endif /* NOT ACE_HAS_REENTRANT_LIBC */

    return ACE_OS::mktime (&tms);
  }
}

const char *
HTTP_Helper::HTTP_date (void)
{
  if (HTTP_Helper::date_string_ == 0)
    {
      ACE_MT (ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, m, HTTP_Helper::mutex_, 0));

      if (HTTP_Helper::date_string_ == 0)
        {
          // 40 bytes is all I need.
          ACE_NEW_RETURN (HTTP_Helper::date_string_, char[40], 0);

          if (!HTTP_Helper::HTTP_date (HTTP_Helper::date_string_))
            {
              delete [] HTTP_Helper::date_string_;
              HTTP_Helper::date_string_ = 0;
            }
        }
    }

  return HTTP_Helper::date_string_;
}

const char *
HTTP_Helper::HTTP_date (char *s)
{
  // Return the date-string formatted per HTTP standards.  Time must
  // be in UTC, so using the 'strftime' call (which obeys the locale)
  // isn't correct.
  static const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                 "Jul","Aug","Sep","Oct","Nov","Dec"};
  static const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

  time_t tloc;
  struct tm tms;
  char * date_string = s;

  if (ACE_OS::time (&tloc) != (time_t) -1
      && ACE_OS::gmtime_r (&tloc, &tms) != 0)
  {
    ACE_OS::sprintf (date_string,
                     "%s, %2.2d %s %4.4d %2.2d:%2.2d:%2.2d GMT",
                     days[tms.tm_wday], tms.tm_mday, months[tms.tm_mon],
                     tms.tm_year + 1900, tms.tm_hour, tms.tm_min, tms.tm_sec);
  }
  else
    date_string = 0;

  return date_string;
}

int
HTTP_Helper::HTTP_month (const char *month)
{
  for (size_t i = 0; i < 12; i++)
    if (ACE_OS::strcmp(month, HTTP_Helper::months_[i]) == 0)
      return i;

  return -1;
}

const char *
HTTP_Helper::HTTP_month (int month)
{
  if (month < 0 || month >= 12)
    return 0;

  return HTTP_Helper::months_[month];
}

// Fix the path if it needs fixing/is fixable.

char *
HTTP_Helper::HTTP_decode_string (char *path)
{
  // replace the percentcodes with the actual character
  int i, j;
  char percentcode[3];

  for (i = j = 0; path[i] != '\0'; i++, j++)
    {
      if (path[i] == '%')
        {
          percentcode[0] = path[++i];
          percentcode[1] = path[++i];
          percentcode[2] = '\0';
          path[j] = (char) ACE_OS::strtol (percentcode, (char **) 0, 16);
        }
      else
        path[j] = path[i];
    }

  path[j] = path[i];

  return path;
}

char *
HTTP_Helper::HTTP_decode_base64 (char *data)
{
  char inalphabet[256], decoder[256];

  ACE_OS::memset (inalphabet, 0, sizeof (inalphabet));
  ACE_OS::memset (decoder, 0, sizeof (decoder));

  for (int i = ACE_OS::strlen (HTTP_Helper::alphabet_) - 1;
       i >= 0;
       i--)
    {
      inalphabet[(unsigned int) HTTP_Helper::alphabet_[i]] = 1;
      decoder[(unsigned int) HTTP_Helper::alphabet_[i]] = i;
    }

  char *indata = data;
  char *outdata = data;

  int bits = 0;
  int c;
  int char_count = 0;
  int errors = 0;

  while ((c = *indata++) != '\0')
    {
      if (c == '=')
        break;
      if (c > 255 || ! inalphabet[c])
        continue;
      bits += decoder[c];
      char_count++;
      if (char_count == 4)
        {
          *outdata++ = (bits >> 16);
          *outdata++ = ((bits >> 8) & 0xff);
          *outdata++ = (bits & 0xff);
          bits = 0;
          char_count = 0;
        }
      else
        bits <<= 6;
    }

  if (c == '\0')
    {
      if (char_count)
        {
          ACE_DEBUG ((LM_DEBUG,
                     "base64 encoding incomplete: at least %d bits truncated\n",
                     ((4 - char_count) * 6)));
            errors++;
        }
    }
  else
    {
      // c == '='
      switch (char_count)
        {
        case 1:
          ACE_DEBUG ((LM_DEBUG,
                      "base64 encoding incomplete: at least 2 bits missing\n"));
          errors++;
          break;
        case 2:
          *outdata++ = (bits >> 10);
          break;
        case 3:
          *outdata++ = (bits >> 16);
          *outdata++ = ((bits >> 8) & 0xff);
          break;
        }
    }
  *outdata = '\0';
  return errors ? 0 : data;
}

char *
HTTP_Helper::HTTP_encode_base64 (char *data)
{
  char buf[BUFSIZ];
  int c;
  int error;
  int char_count = 0;
  int bits = 0;
  error = 0;
  char *indata = data;
  char *outdata = buf;
  const unsigned char ASCII_MAX = ~0;

  while ((c = *indata++) != '\0')
    {
      if (c > (int)ASCII_MAX)
        {
          ACE_DEBUG ((LM_DEBUG, "encountered char > 255 (decimal %d)\n", c));
          error++;
          break;
        }
      bits += c;
      char_count++;

      if (char_count == 3)
        {
          *outdata++ = HTTP_Helper::alphabet_[bits >> 18];
          *outdata++ = HTTP_Helper::alphabet_[(bits >> 12) & 0x3f];
          *outdata++ = HTTP_Helper::alphabet_[(bits >> 6) & 0x3f];
          *outdata++ = HTTP_Helper::alphabet_[bits & 0x3f];
          bits = 0;
          char_count = 0;
        }
      else
          bits <<= 8;
    }

  if (!error)
    {
      if (char_count != 0)
        {
          bits <<= 16 - (8 * char_count);
          *outdata++ = HTTP_Helper::alphabet_[bits >> 18];
          *outdata++ = HTTP_Helper::alphabet_[(bits >> 12) & 0x3f];

          if (char_count == 1)
            {
              *outdata++ = '=';
              *outdata++ = '=';
            }
          else
            {
              *outdata++ = HTTP_Helper::alphabet_[(bits >> 6) & 0x3f];
              *outdata++ = '=';
            }
        }
      *outdata = '\0';
      ACE_OS::strcpy (data, buf);
    }

  return (error ? 0 : data);
}

int
HTTP_Helper::fixyear (int year)
{
  // Fix the year 2000 problem

  if (year > 1000)
    year -= 1900;
  else if (year < 100)
    {
      struct tm tms;
      time_t tloc;

      if (ACE_OS::time (&tloc) != (time_t) -1)
        {
          ACE_OS::gmtime_r (&tloc, &tms);

          if (tms.tm_year % 100 == year)
            year = tms.tm_year;

          // The last two cases check boundary conditions, in case the
          // year just changed at the moment we checked to see if we
          // need to fix it.
          if ((year+1) % 100 == tms.tm_year % 100)
            year = tms.tm_year - 1;

          if (year == (tms.tm_year + 1) % 100)
            year = tms.tm_year + 1;

          // What to do if none of the above?
        }
    }

  return year;
}

const char **
HTTP_Status_Code::instance (void)
{
  if (HTTP_Status_Code::instance_ == 0)
    {
      ACE_MT (ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, g, lock_, 0));

      if (HTTP_Status_Code::instance_ == 0)
        {
          for (size_t i = 0;
               i < HTTP_Status_Code::MAX_STATUS_CODE + 1;
               i++)
            {
              switch (i)
                {
                case STATUS_OK:
                  HTTP_Status_Code::Reason[i] = "OK"; break;
                case STATUS_CREATED:
                  HTTP_Status_Code::Reason[i] = "Created"; break;
                case STATUS_ACCEPTED:
                  HTTP_Status_Code::Reason[i] = "Accepted"; break;
                case STATUS_NO_CONTENT:
                  HTTP_Status_Code::Reason[i] = "No Content"; break;
                case STATUS_MOVED_PERMANENTLY:
                  HTTP_Status_Code::Reason[i] = "Moved Permanently"; break;
                case STATUS_MOVED_TEMPORARILY:
                  HTTP_Status_Code::Reason[i] = "Moved Temporarily"; break;
                case STATUS_NOT_MODIFIED:
                  HTTP_Status_Code::Reason[i] = "Not Modified"; break;
                case STATUS_BAD_REQUEST:
                  HTTP_Status_Code::Reason[i] = "Bad Request"; break;
                case STATUS_UNAUTHORIZED:
                  HTTP_Status_Code::Reason[i] = "Unauthorized"; break;
                case STATUS_FORBIDDEN:
                  HTTP_Status_Code::Reason[i] = "Forbidden"; break;
                case STATUS_NOT_FOUND:
                  HTTP_Status_Code::Reason[i] = "Not Found"; break;
                case STATUS_INTERNAL_SERVER_ERROR:
                  HTTP_Status_Code::Reason[i] = "Internal Server Error"; break;
                case STATUS_NOT_IMPLEMENTED:
                  HTTP_Status_Code::Reason[i] = "Not Implemented"; break;
                case STATUS_BAD_GATEWAY:
                  HTTP_Status_Code::Reason[i] = "Bad Gateway"; break;
                case STATUS_SERVICE_UNAVAILABLE:
                  HTTP_Status_Code::Reason[i] = "Service Unavailable"; break;
                default:
                  HTTP_Status_Code::Reason[i] = "Unknown";
                }
            }

          HTTP_Status_Code::instance_ = 1;
        }

      // GUARD released
    }

  return HTTP_Status_Code::Reason;
}
