//
// Log.h
// hifi
//
// Created by Tobias Schwinger on 4/17/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__shared_Log__
#define __hifi__shared_Log__

//
// Pointer to log function
//
// An application may reset this variable to receive the log messages
// issued using 'printLog'. It defaults to a pointer to 'printf'.
//
extern int (* printLog)(char const*, ...);

#endif /* defined(__hifi__shared_Log__) */

