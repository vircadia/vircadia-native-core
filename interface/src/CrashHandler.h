//
//  CrashHandler.h
//  interface/src
//
//  Created by Clement Brisset on 01/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CrashHandler_h
#define hifi_CrashHandler_h

#include <string>

#if HAS_CRASHPAD

bool startCrashHandler();
void setCrashAnnotation(std::string name, std::string value);

#elif HAS_BREAKPAD

bool startCrashHandler();
void setCrashAnnotation(std::string name, std::string value);

#else

bool startCrashHandler();
void setCrashAnnotation(std::string name, std::string value);

#endif // hifi_Crashpad_h
