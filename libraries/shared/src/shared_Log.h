//
// shared_Log.h
// hifi
//
// Created by Tobias Schwinger on 4/17/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__shared_Log__
#define __hifi__shared_Log__

namespace shared_lib { 

    // variable that can be set from outside to redirect the log output
    // of this library
    extern int (* printLog)(char const*, ...);
}

#endif /* defined(__hifi__shared_Log__) */

