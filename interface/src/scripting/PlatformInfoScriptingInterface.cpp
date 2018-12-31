//
//  Created by Nissim Hadar on 2018/12/28
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PlatformInfoScriptingInterface.h"
#include "Application.h"

#include <thread>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

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

QString PlatformInfoScriptingInterface::getCPUBrand() {
#ifdef Q_OS_WIN
    int CPUInfo[4] = { -1 };
    unsigned   nExIds, i = 0;
    char CPUBrandString[0x40];
    // Get the information associated with each extended ID.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];

    for (i = 0x80000000; i <= nExIds; ++i) {
       __cpuid(CPUInfo, i);
        // Interpret CPU brand string
        if (i == 0x80000002) {
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000003) {
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000004) {
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
        }
    }

    return CPUBrandString;
#else
    return "NOT IMPLEMENTED";
#endif
}

unsigned int PlatformInfoScriptingInterface::getNumLogicalCores() {

    return std::thread::hardware_concurrency();
}

int PlatformInfoScriptingInterface::getTotalSystemMemoryMB() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullTotalPhys / 1024 / 1024;
#else
    return -1;
#endif
}

QString PlatformInfoScriptingInterface::getGraphicsCardType() {
    return qApp->getGraphicsCardType();
}