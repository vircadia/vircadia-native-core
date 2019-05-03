//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "platform.h"
#include <QtGlobal>

#ifdef Q_OS_WIN
#include "WINPlatform.h"
#endif

#ifdef Q_OS_MACOS
#include "MACPlatform.h"
#endif

#ifdef Q_OS_LINUX
#endif

using namespace platform;

void platform::create() {

    #ifdef Q_OS_WIN
        _instance =new WINInstance();
    #elif Q_OS_MAC
    #endif
}

std::string platform::getProcessor(int index) {
    return _instance->getProcessor(index);
}

bool platform::enumerateProcessors() {
    return _instance->enumerateProcessors();
}

int platform::getTotalSystemRamMb() {
    return _instance->getTotalSystemRamMb();
}

int platform::getProcessorCount() {
    return _instance->getProcessorCount();
}