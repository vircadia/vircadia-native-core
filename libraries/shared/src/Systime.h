//
//  Systime.h
//  libraries/shared/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Systime_h
#define hifi_Systime_h

#ifdef WIN32

#include <winsock2.h>

int __cdecl gettimeofday(struct timeval *__restrict__, void *__restrict__);

#endif

#endif // hifi_Systime_h