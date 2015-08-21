//
//  windowshacks.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 1/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  hacks to get windows to compile
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_windowshacks_h
#define hifi_windowshacks_h

#ifdef WIN32

#include <cmath>
inline double roundf(double value) {
	return (value > 0.0) ? floor(value + 0.5) : ceil(value - 0.5);
}
#define round roundf

#ifdef _MSC_VER
#ifndef SNPRINTF_FIX
#define SNPRINTF_FIX
#include <stdio.h>
#include <stdarg.h>

#define snprintf c99_snprintf

inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    int count = -1;
    if (size != 0) {
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
	}
    if (count == -1) {
        count = _vscprintf(format, ap);
	}
    return count;
}

inline int c99_snprintf(char* str, size_t size, const char* format, ...) {
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}
#endif // SNPRINTF_FIX
#endif // _MSC_VER

#include <winsock2.h>
#include <WS2tcpip.h>

#else // WIN32

#include <netinet/in.h>

#endif // WIN32

#endif // hifi_windowshacks_h
