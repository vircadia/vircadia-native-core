#ifndef __Systime__
#define __Systime__

#ifdef _WIN32
#ifdef _WINSOCK2API_
#define _timeval_
#endif
#ifndef _timeval_
#define _timeval_
/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

#endif _timeval_

struct timezone {
  int tz_minuteswest; /* minutes west of Greenwich */
  int tz_dsttime; /* type of dst correction */
};

int gettimeofday( struct timeval* p_tv, struct timezone* p_tz );

#endif _Win32

#endif __Systime__
