//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "../Platform.h"
#include "../PlatformKeys.h"

namespace platform { namespace keys {
    const char*  UNKNOWN = "UNKNOWN";

    namespace cpu {
        const char*  vendor = "vendor";
        const char*  vendor_Intel = "Intel";
        const char*  vendor_AMD = "AMD";

        const char*  model = "model";
        const char*  clockSpeed = "clockSpeed";
        const char*  numCores = "numCores";
    }
    namespace gpu {
        const char*  vendor = "vendor";
        const char*  vendor_NVIDIA = "NVIDIA";
        const char*  vendor_AMD = "AMD";
        const char*  vendor_Intel = "Intel";

        const char*  model = "model";
        const char*  videoMemory = "videoMemory";
        const char*  driver = "driver";
        const char*  displays = "displays";
    }
    namespace nic {
        const char* mac = "mac";
        const char* name = "name";
    }
    namespace display {
        const char*  description = "description";
        const char*  name = "deviceName";
        const char*  boundsLeft = "boundsLeft";
        const char*  boundsRight = "boundsRight";
        const char*  boundsTop = "boundsTop";
        const char*  boundsBottom = "boundsBottom";
        const char*  gpu = "gpu";
        const char*  ppi = "ppi";
        const char*  ppiDesktop = "ppiDesktop";
        const char*  physicalWidth = "physicalWidth";
        const char*  physicalHeight = "physicalHeight";
        const char*  modeRefreshrate = "modeRefreshrate";
        const char*  modeWidth = "modeWidth";
        const char*  modeHeight = "modeHeight";
        const char*  isMaster = "isMaster";
    }
    namespace memory {
        const char*  memTotal = "memTotal";
    }
    namespace computer {
        const char*  OS = "OS";
        const char*  OS_WINDOWS = "WINDOWS";
        const char*  OS_MACOS = "MACOS";
        const char*  OS_LINUX = "LINUX";
        const char*  OS_ANDROID = "ANDROID";

        const char*  OSVersion = "OSVersion";

        const char*  vendor = "vendor";
        const char*  vendor_Apple = "Apple";

        const char*  model = "model";

        const char*  profileTier = "profileTier";
    }

    const char*  CPUS = "cpus";
    const char*  GPUS = "gpus";
    const char*  DISPLAYS = "displays";
    const char*  NICS = "nics";
    const char*  MEMORY = "memory";
    const char*  COMPUTER = "computer";
}}

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

int platform::getNumCPUs() {
    return _instance->getNumCPUs();
}

json platform::getCPU(int index) {
    return _instance->getCPU(index);
}

int platform::getNumGPUs() {
    return _instance->getNumGPUs();
}

json platform::getGPU(int index) {
    return _instance->getGPU(index);
}

int platform::getNumDisplays() {
    return _instance->getNumDisplays();
}

json platform::getDisplay(int index) {
    return _instance->getDisplay(index);
}

json platform::getMemory() {
    return _instance->getMemory();
}

json platform::getComputer() {
    return _instance->getComputer();
}

json platform::getAll() {
    return _instance->getAll();
}
