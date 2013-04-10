#ifdef _WIN32
#include <windows.h>
#define _timeval_
#include "Systime.h"

    int gettimeofday( timeval* p_tv, timezone* p_tz )
    {
      int tt = timeGetTime();

      p_tv->tv_sec = tt / 1000;
      p_tv->tv_usec = tt % 1000 * 1000;
      return 0;
    }
#endif
