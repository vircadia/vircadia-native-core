//
//  SharedUtil.h
//  hifi
//
//  Created by Stephen Birarda on 2/22/13.
//
//

#ifndef __hifi__SharedUtil__
#define __hifi__SharedUtil__

#include <iostream>
#include <sys/time.h>

double usecTimestamp(timeval *time);
double usecTimestampNow();

#endif /* defined(__hifi__SharedUtil__) */
