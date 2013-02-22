//
//  SharedUtil.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/22/13.
//
//

#include "SharedUtil.h"

double usecTimestamp(timeval *time) {
    return (time->tv_sec * 1000000.0);
}

double usecTimestampNow() {
    timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec * 1000000.0);
}