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
using namespace nlohmann;

Instance* _instance;

void platform::create() {

    #ifdef Q_OS_WIN
        _instance =new WINInstance();
    #elif Q_OS_MAC
    #endif
}

json Instance::getProcessor(int index) {
    assert(index < _processor.size());

    json result;
    if (index >= _processors.size())
        return result;

    return _processors.at(index);
}


//These are ripe for template.. will work on that next
json Instance::getSystemRam(int index) { 
    
    assert(index < _memory.size());

    json result;
    if(index>= _memory.size())
        return result;

    return _memory.at(index);
}

json Instance::getGPU(int index) {
    assert(index < _gpu.size());

    json result;
    if (index >= _gpu.size())
        return result;

    return _gpu.at(index);
}


bool platform::enumerateProcessors() {
    return _instance->enumerateProcessors();
}

json platform::getProcessor(int index) {
    return _instance->getProcessor(index);
}

json platform::getSystemRam(int index) {
    return _instance->getSystemRam(index);
}

int platform::getNumMemory() {
    return _instance->getNumMemory();
}

int platform::getNumProcessor() {
    return _instance->getNumProcessor();
}