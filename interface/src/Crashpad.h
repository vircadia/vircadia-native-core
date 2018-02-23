//
//  Crashpad.h
//  interface/src
//
//  Created by Clement Brisset on 01/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Crashpad_h
#define hifi_Crashpad_h

#include <string>

bool startCrashHandler();
void setCrashAnnotation(std::string name, std::string value);

#endif // hifi_Crashpad_h
