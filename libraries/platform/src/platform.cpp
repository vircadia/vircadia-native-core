//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "platform.h"

#include <qglobal.h>

#if defined(Q_OS_WIN)
#include "WINPlatform.h"
#elif defined(Q_OS_MAC)
#include "MACOSPlatform.h"
#elif defined(Q_OS_ANDROID)
#include "AndroidPlatform.h"
#elif defined(Q_OS_LINUX)
#include "LinuxPlatform.h"
#endif

using namespace platform;

Instance *_instance;

void platform::create() {
#if defined(Q_OS_WIN)
    _instance =new WINInstance();
#elif defined(Q_OS_MAC)
    _instance = new MACOSInstance();
#elif defined(Q_OS_ANDROID)
    _instance= new AndroidInstance();
#elif defined(Q_OS_LINUX)
    _instance= new LinuxInstance();
#endif
}

void platform::destroy() {
    if(_instance)
        delete _instance;
}

bool platform::enumeratePlatform() {
    return _instance->enumeratePlatform();
}

int platform::getNumCPU() {
    return _instance->getNumCPU();
}

json platform::getCPU(int index) {
    return _instance->getCPU(index);
}

int platform::getNumGPU() {
    return _instance->getNumGPU();
}

json platform::getGPU(int index) {
    return _instance->getGPU(index);
}

int platform::getNumDisplay() {
    return _instance->getNumDisplay();
}

json platform::getDisplay(int index) {
    return _instance->getDisplay(index);
}

int platform::getNumMemory() {
    return _instance->getNumMemory();
}

json platform::getMemory(int index) {
    return _instance->getMemory(index);
}

int platform::getNumComputer(){
    return _instance->getNumComputer();
}

json platform::getComputer(int index){
    return _instance->getComputer(index);
}
