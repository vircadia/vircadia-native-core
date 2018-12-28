//
//  Created by Nissim Hadar on 2018/12/28
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PlatformInfoScriptingInterface.h"

PlatformInfoScriptingInterface* PlatformInfoScriptingInterface::getInstance() {
    static PlatformInfoScriptingInterface sharedInstance;
    return &sharedInstance;
}

QString PlatformInfoScriptingInterface::getOperatingSystemType() {
#ifdef Q_OS_WIN
    return "WINDOWS";
#elif defined Q_OS_MAC
    return "MACOS";
#else
    return "UNKNOWN";
#endif
}
