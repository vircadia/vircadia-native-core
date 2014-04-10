#ifndef __Systime__
#define __Systime__

#ifdef _WIN32

#include <winsock2.h>

struct timezone {
  int tz_minuteswest; /* minutes west of Greenwich */
  int tz_dsttime; /* type of dst correction */
};

int gettimeofday(struct timeval* p_tv, struct timezone* p_tz);

#endif _Win32

#endif __Systime__
