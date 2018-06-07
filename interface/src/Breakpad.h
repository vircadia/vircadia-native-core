//
//  Breakpad.h
//  interface/src
//
//  Created by Gabriel Calero & Cristian Duarte on 06/06/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Breakpad_h
#define hifi_Breakpad_h

#include <string>

bool startCrashHandler();
void setCrashAnnotation(std::string name, std::string value);

#endif // hifi_Crashpad_h
